"""
Vector-based knowledge base for translation retrieval.
Implements RAG (Retrieval-Augmented Generation) pattern using ChromaDB.
"""
from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Protocol

import structlog
from langchain_chroma import Chroma
from langchain_community.embeddings import HuggingFaceEmbeddings
from langchain_core.documents import Document

from backend.config.settings import get_settings
from backend.knowledge.data_pipeline import TranslationDataPipeline, TranslationRecord

logger = structlog.get_logger(__name__)


@dataclass
class KBSearchResult:
    """A single knowledge base search result with metadata."""
    source: str
    target: str
    score: float
    source_file: str
    record_id: str

    @property
    def is_high_confidence(self) -> bool:
        return self.score >= get_settings().kb_score_threshold


class EmbeddingProvider(Protocol):
    """Protocol for embedding providers - enables easy backend swapping."""

    def embed_documents(self, texts: list[str]) -> list[list[float]]: ...
    def embed_query(self, text: str) -> list[float]: ...


class TranslationKnowledgeBase:
    """
    Manages translation knowledge retrieval using vector similarity search.

    Architecture:
    - ChromaDB as persistent vector store
    - BGE-M3 multilingual embeddings (supports 100+ languages)
    - Similarity search with configurable threshold
    """

    def __init__(self, settings=None) -> None:
        self._settings = settings or get_settings()
        self._embedding_fn = self._build_embeddings()
        self._store: Chroma | None = None
        self._pipeline = TranslationDataPipeline()

    def _build_embeddings(self) -> EmbeddingProvider:
        """Build multilingual embedding model."""
        return HuggingFaceEmbeddings(
            model_name=self._settings.embedding_model,
            model_kwargs={"device": "cpu"},
            encode_kwargs={"normalize_embeddings": True},
        )

    def initialize(self, force_rebuild: bool = False) -> None:
        """Initialize or load the vector store."""
        persist_dir = str(self._settings.vector_db_dir)
        collection = self._settings.collection_name

        if not force_rebuild and self._collection_exists():
            logger.info("loading_existing_kb", collection=collection)
            self._store = Chroma(
                collection_name=collection,
                embedding_function=self._embedding_fn,
                persist_directory=persist_dir,
            )
        else:
            logger.info("building_kb_from_scratch")
            self._build_from_excel()

    def _collection_exists(self) -> bool:
        """Check if a populated collection already exists."""
        try:
            import chromadb
            client = chromadb.PersistentClient(path=str(self._settings.vector_db_dir))
            col = client.get_collection(self._settings.collection_name)
            return col.count() > 0
        except Exception:
            return False

    def _build_from_excel(self) -> None:
        """Build vector store from processed Excel files."""
        records, stats = self._pipeline.process_directory(
            self._settings.knowledge_base_dir
        )

        if not records:
            logger.warning("no_records_found_creating_empty_store")
            self._store = Chroma(
                collection_name=self._settings.collection_name,
                embedding_function=self._embedding_fn,
                persist_directory=str(self._settings.vector_db_dir),
            )
            return

        documents = [self._record_to_document(r) for r in records]
        logger.info("indexing_documents", count=len(documents))

        self._store = Chroma.from_documents(
            documents=documents,
            embedding=self._embedding_fn,
            collection_name=self._settings.collection_name,
            persist_directory=str(self._settings.vector_db_dir),
        )
        logger.info("kb_built", total_records=len(documents), stats=str(stats))

    @staticmethod
    def _record_to_document(record: TranslationRecord) -> Document:
        """Convert pipeline record to LangChain document."""
        return Document(
            page_content=record.source,
            metadata={
                "target": record.target,
                "source_lang": record.source_lang,
                "target_lang": record.target_lang,
                "source_file": record.source_file,
                "record_id": record.record_id,
                "quality_score": record.quality_score,
            },
        )

    def search(
        self, query: str, top_k: int | None = None, threshold: float | None = None
    ) -> list[KBSearchResult]:
        """
        Search knowledge base for relevant translations.
        Returns results above the similarity threshold.
        """
        if self._store is None:
            raise RuntimeError("Knowledge base not initialized. Call initialize() first.")

        k = top_k or self._settings.kb_top_k
        min_score = threshold or self._settings.kb_score_threshold

        try:
            results_with_scores = self._store.similarity_search_with_relevance_scores(
                query=query, k=k
            )
        except Exception as e:
            logger.error("kb_search_failed", query=query[:50], error=str(e))
            return []

        kb_results = []
        for doc, score in results_with_scores:
            if score >= min_score:
                kb_results.append(KBSearchResult(
                    source=doc.page_content,
                    target=doc.metadata.get("target", ""),
                    score=score,
                    source_file=doc.metadata.get("source_file", ""),
                    record_id=doc.metadata.get("record_id", ""),
                ))

        logger.debug("kb_search", query=query[:50], results=len(kb_results))
        return kb_results

    def add_records(self, records: list[TranslationRecord]) -> None:
        """Incrementally add new records to the knowledge base."""
        if self._store is None:
            raise RuntimeError("Knowledge base not initialized.")

        documents = [self._record_to_document(r) for r in records]
        self._store.add_documents(documents)
        logger.info("kb_records_added", count=len(documents))

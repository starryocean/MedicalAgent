"""FAISS-backed terminology retrieval tool."""
from __future__ import annotations
import pickle
from pathlib import Path
import numpy as np
import faiss
from sentence_transformers import SentenceTransformer
from config.config import get_settings


class VectorSearchTool:
    """Loads FAISS index + metadata. Thread-safe for read."""

    def __init__(self) -> None:
        s = get_settings()
        # self._model = SentenceTransformer(s.embedding_model_path)
        self._model = None  # Lazy load
        self._index_path = s.faiss_index_dir / "index.faiss"
        self._meta_path = s.faiss_index_dir / "meta.pkl"
        self._index: faiss.Index | None = None
        self._meta: list[dict] = []
        self._threshold = s.similarity_threshold
        self._load()

    def _load(self) -> None:
        if not self._index_path.exists():
            # Lazy: empty index, system still usable (no terminology)
            self._index = faiss.IndexFlatIP(get_settings().embedding_dim)
            return
        self._index = faiss.read_index(str(self._index_path))
        with open(self._meta_path, "rb") as f:
            self._meta = pickle.load(f)

    def search(self, query: str, top_k: int, target_lang: str) -> list[dict]:
        if self._model is None:
            s = get_settings()
            self._model = SentenceTransformer(s.embedding_model_path)
        if self._index is None or self._index.ntotal == 0:
            return []
        vec = self._model.encode([query], normalize_embeddings=True)
        scores, idxs = self._index.search(np.asarray(vec, dtype="float32"), top_k)

        results = []
        for score, idx in zip(scores[0], idxs[0]):
            if idx < 0 or score < self._threshold:
                continue
            m = self._meta[idx]
            target = m.get("translations", {}).get(target_lang)
            if not target:
                continue
            results.append({
                "term_id": m["term_id"],
                "source": m["source"],
                "target": target,
                "score": float(score),
            })
        return results

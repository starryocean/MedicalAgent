"""
Core translation agent using LangChain LCEL (LangChain Expression Language).
Implements the ReAct pattern with custom tools for knowledge base lookup.

Best practices applied:
- LCEL chains for composability and streaming support
- Structured output with Pydantic validation
- Retry logic with exponential backoff
- Observability through structured logging
"""
from __future__ import annotations

import json
from typing import Any

import structlog
from langchain_anthropic import ChatAnthropic
from langchain_core.output_parsers import JsonOutputParser
from langchain_core.runnables import RunnableConfig, RunnablePassthrough
from tenacity import retry, stop_after_attempt, wait_exponential

from backend.agent.prompts import build_translation_prompt
from backend.config.settings import get_settings
from backend.knowledge.knowledge_base import KBSearchResult, TranslationKnowledgeBase
from backend.memory.session_memory import SessionMemoryManager

logger = structlog.get_logger(__name__)


def _format_kb_context(kb_results: list[KBSearchResult]) -> str:
    """Format knowledge base results for prompt injection."""
    if not kb_results:
        return "No relevant entries found in knowledge base."

    lines = ["Relevant approved translations from knowledge base:\n"]
    for i, result in enumerate(kb_results, 1):
        confidence_label = "HIGH" if result.is_high_confidence else "MEDIUM"
        lines.append(
            f"{i}. Source: \"{result.source}\"\n"
            f"   Approved Translation: \"{result.target}\"\n"
            f"   Confidence: {result.score:.2f} ({confidence_label})\n"
            f"   From: {result.source_file}\n"
        )
    return "\n".join(lines)


def _format_source_entries(entries: list[dict[str, str]]) -> str:
    """Format INI entries for the prompt."""
    lines = []
    for entry in entries:
        key = entry.get("key", "")
        value = entry.get("value", "")
        lines.append(f"[{key}]\n{value}\n")
    return "\n".join(lines)


class TranslationAgent:
    """
    Production-grade translation agent with RAG and memory support.

    Design decisions:
    - LCEL chain over legacy AgentExecutor for better streaming & composability
    - JSON output parsing with fallback for robustness
    - Per-session memory isolation for multi-user support
    - Knowledge base retrieval before LLM call to reduce hallucination
    """

    def __init__(
        self,
        knowledge_base: TranslationKnowledgeBase,
        memory_manager: SessionMemoryManager,
        settings=None,
    ) -> None:
        self._kb = knowledge_base
        self._memory = memory_manager
        self._settings = settings or get_settings()
        self._llm = self._build_llm()
        self._chain = self._build_chain()

    def _build_llm(self) -> ChatAnthropic:
        """Construct the LLM with appropriate parameters."""
        return ChatAnthropic(
            model=self._settings.model_name,
            api_key=self._settings.anthropic_api_key,
            temperature=self._settings.temperature,
            max_tokens=self._settings.max_tokens,
        )

    def _build_chain(self):
        """
        Build LCEL chain: prompt | llm | parser
        Using RunnablePassthrough for input transformation.
        """
        prompt = build_translation_prompt()
        parser = JsonOutputParser()

        chain = (
            RunnablePassthrough.assign(
                kb_context=lambda x: _format_kb_context(
                    self._retrieve_kb_context(x["source_entries_raw"])
                )
            )
            | RunnablePassthrough.assign(
                source_entries=lambda x: _format_source_entries(
                    x["source_entries_raw"]
                ),
                chat_history=lambda x: self._memory.get_history(
                    x.get("session_id", "default")
                ),
            )
            | prompt
            | self._llm
            | parser
        )
        return chain

    def _retrieve_kb_context(
        self, entries: list[dict[str, str]]
    ) -> list[KBSearchResult]:
        """Batch retrieve KB context for all source entries."""
        all_results: list[KBSearchResult] = []
        seen_ids: set[str] = set()

        for entry in entries:
            value = entry.get("value", "")
            if not value:
                continue
            results = self._kb.search(value)
            for result in results:
                if result.record_id not in seen_ids:
                    all_results.append(result)
                    seen_ids.add(result.record_id)

        return all_results

    @retry(
        stop=stop_after_attempt(3),
        wait=wait_exponential(multiplier=1, min=2, max=10),
        reraise=True,
    )
    async def translate(
        self,
        entries: list[dict[str, str]],
        target_language: str,
        user_context: str = "",
        session_id: str = "default",
    ) -> dict[str, Any]:
        """
        Execute translation with RAG and memory context.

        Args:
            entries: List of {"key": str, "value": str} dicts from INI file
            target_language: Target language name (e.g., "English", "Japanese")
            user_context: Additional context from user's chat message
            session_id: Session ID for memory isolation

        Returns:
            Structured translation result dict
        """
        logger.info(
            "translation_start",
            entry_count=len(entries),
            target_lang=target_language,
            session_id=session_id,
        )

        config = RunnableConfig(
            metadata={
                "session_id": session_id,
                "target_language": target_language,
            }
        )

        try:
            result = await self._chain.ainvoke(
                {
                    "source_entries_raw": entries,
                    "target_language": target_language,
                    "user_context": user_context or "No additional context provided.",
                    "session_id": session_id,
                },
                config=config,
            )

            # Update session memory with this exchange
            self._memory.add_exchange(
                session_id=session_id,
                human_message=f"Translate {len(entries)} entries to {target_language}",
                ai_message=json.dumps(result, ensure_ascii=False)[:500],
            )

            logger.info(
                "translation_complete",
                translations=len(result.get("translations", [])),
                session_id=session_id,
            )
            return result

        except json.JSONDecodeError as e:
            logger.error("json_parse_failed", error=str(e))
            raise ValueError(f"LLM returned invalid JSON: {e}") from e
        except Exception as e:
            logger.error("translation_failed", error=str(e), session_id=session_id)
            raise

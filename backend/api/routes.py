"""
FastAPI routes for the translation agent API.
"""
from __future__ import annotations

import asyncio

import structlog
from fastapi import APIRouter, BackgroundTasks, Depends, HTTPException

from backend.agent.translation_agent import TranslationAgent
from backend.api.schemas import (
    ApplyTranslationRequest,
    KBRebuildRequest,
    TranslationRequest,
    TranslationResponse,
)
from backend.knowledge.knowledge_base import TranslationKnowledgeBase
from backend.memory.session_memory import SessionMemoryManager
from backend.utils.diff_utils import IniDiffEngine
from backend.utils.ini_parser import IniFileParser

logger = structlog.get_logger(__name__)
router = APIRouter(prefix="/api/v1")

# ── Dependency Injection ───────────────────────────────────────────────────────

_kb: TranslationKnowledgeBase | None = None
_memory: SessionMemoryManager | None = None
_agent: TranslationAgent | None = None


def get_knowledge_base() -> TranslationKnowledgeBase:
    if _kb is None:
        raise HTTPException(status_code=503, detail="Knowledge base not initialized")
    return _kb


def get_agent() -> TranslationAgent:
    if _agent is None:
        raise HTTPException(status_code=503, detail="Agent not initialized")
    return _agent


def setup_dependencies(kb: TranslationKnowledgeBase, memory: SessionMemoryManager) -> None:
    """Called at application startup to inject dependencies."""
    global _kb, _memory, _agent
    _kb = kb
    _memory = memory
    _agent = TranslationAgent(knowledge_base=kb, memory_manager=memory)


# ── Routes ─────────────────────────────────────────────────────────────────────

@router.post("/translate", response_model=TranslationResponse)
async def translate(
    request: TranslationRequest,
    agent: TranslationAgent = Depends(get_agent),
) -> TranslationResponse:
    """
    Main translation endpoint.
    Accepts INI entries, returns structured translations with diff.
    """
    logger.info("translate_request", entries=len(request.entries), lang=request.target_language)

    try:
        entries_dicts = [
            {"key": e.key, "value": e.value, "section": e.section}
            for e in request.entries
        ]

        result = await agent.translate(
            entries=entries_dicts,
            target_language=request.target_language,
            user_context=request.user_context,
            session_id=request.session_id,
        )

        translations = result.get("translations", [])

        return TranslationResponse(
            success=True,
            translations=translations,
            summary=result.get("summary", ""),
            warnings=result.get("warnings", []),
            session_id=request.session_id,
        )

    except Exception as e:
        logger.error("translate_failed", error=str(e))
        raise HTTPException(status_code=500, detail=str(e)) from e


@router.post("/apply-translation")
async def apply_translation(request: ApplyTranslationRequest) -> dict:
    """Apply confirmed translations to the target INI file."""
    from pathlib import Path

    parser = IniFileParser()
    diff_engine = IniDiffEngine()

    translations_map = {item.key: item.target for item in request.translations}

    try:
        source_path = Path(request.source_file_path)
        target_path = Path(request.target_file_path)

        parser.write_translations(source_path, target_path, translations_map)
        diff = diff_engine.compare_files(source_path, target_path)

        return {
            "success": True,
            "applied_count": len(translations_map),
            "diff": diff.unified_diff,
            "changed_keys": diff.changed_keys,
        }
    except Exception as e:
        logger.error("apply_translation_failed", error=str(e))
        raise HTTPException(status_code=500, detail=str(e)) from e


@router.post("/kb/rebuild")
async def rebuild_knowledge_base(
    request: KBRebuildRequest,
    background_tasks: BackgroundTasks,
    kb: TranslationKnowledgeBase = Depends(get_knowledge_base),
) -> dict:
    """Trigger knowledge base rebuild from Excel files."""
    background_tasks.add_task(kb.initialize, force_rebuild=request.force)
    return {"message": "Knowledge base rebuild started in background"}


@router.get("/health")
async def health_check() -> dict:
    return {"status": "healthy", "kb_ready": _kb is not None}


@router.delete("/session/{session_id}")
async def clear_session(session_id: str) -> dict:
    if _memory:
        _memory.clear_session(session_id)
    return {"message": f"Session {session_id} cleared"}

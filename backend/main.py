"""
Application entry point with lifespan management.
"""
from __future__ import annotations

import structlog
import uvicorn
from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from backend.api.routes import router, setup_dependencies
from backend.config.settings import get_settings
from backend.knowledge.knowledge_base import TranslationKnowledgeBase
from backend.memory.session_memory import SessionMemoryManager

logger = structlog.get_logger(__name__)


def create_app() -> FastAPI:
    settings = get_settings()

    app = FastAPI(
        title="Translation Agent API",
        version="1.0.0",
        description="AI-powered multi-language translation agent",
    )

    app.add_middleware(
        CORSMiddleware,
        allow_origins=["*"],
        allow_methods=["*"],
        allow_headers=["*"],
    )

    @app.on_event("startup")
    async def startup() -> None:
        logger.info("server_starting", host=settings.host, port=settings.port)
        kb = TranslationKnowledgeBase()
        kb.initialize()
        memory = SessionMemoryManager()
        setup_dependencies(kb, memory)
        logger.info("server_ready")

    app.include_router(router)
    return app


if __name__ == "__main__":
    settings = get_settings()
    uvicorn.run(
        "backend.main:create_app",
        factory=True,
        host=settings.host,
        port=settings.port,
        reload=settings.debug,
        log_level=settings.log_level.lower(),
    )

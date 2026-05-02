"""
Application configuration using Pydantic BaseSettings.
Follows the 12-factor app methodology for configuration management.
"""
from __future__ import annotations

from functools import lru_cache
from pathlib import Path
from typing import Literal

from pydantic import Field, field_validator
from pydantic_settings import BaseSettings


class Settings(BaseSettings):
    """Centralized application settings with validation."""

    # Server
    host: str = "0.0.0.0"
    port: int = 8765
    debug: bool = False

    # LLM Provider
    llm_provider: Literal["anthropic", "openai"] = "anthropic"
    anthropic_api_key: str = Field(default="", alias="ANTHROPIC_API_KEY")
    openai_api_key: str = Field(default="", alias="OPENAI_API_KEY")
    model_name: str = "claude-sonnet-4-5"
    temperature: float = 0.1
    max_tokens: int = 8192

    # Knowledge Base
    knowledge_base_dir: Path = Path("data/knowledge")
    vector_db_dir: Path = Path("data/vector_db")
    embedding_model: str = "BAAI/bge-m3"
    collection_name: str = "translation_kb"
    kb_top_k: int = 5
    kb_score_threshold: float = 0.75

    # Memory
    max_memory_tokens: int = 4096
    session_ttl_seconds: int = 3600

    # Logging
    log_level: str = "INFO"
    log_format: Literal["json", "console"] = "console"

    @field_validator("knowledge_base_dir", "vector_db_dir", mode="before")
    @classmethod
    def ensure_dir_exists(cls, v: str | Path) -> Path:
        path = Path(v)
        path.mkdir(parents=True, exist_ok=True)
        return path

    model_config = {"env_file": ".env", "env_file_encoding": "utf-8", "populate_by_name": True}


@lru_cache(maxsize=1)
def get_settings() -> Settings:
    """Singleton settings instance - cached after first call."""
    return Settings()

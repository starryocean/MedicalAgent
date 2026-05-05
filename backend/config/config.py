"""Centralized configuration using pydantic-settings."""
from functools import lru_cache
from pathlib import Path
from pydantic_settings import BaseSettings, SettingsConfigDict


class Settings(BaseSettings):
    model_config = SettingsConfigDict(env_file=".env", env_prefix="MEDTRANS_")

    # --- Service ---
    host: str = "0.0.0.0"
    port: int = 8765

    # --- LLM ---
    llm_provider: str = "anthropic"  # anthropic | openai | azure
    llm_model: str = "claude-sonnet-4-5"
    llm_api_key: str = ""
    llm_base_url: str | None = None
    llm_temperature: float = 0.1

    # --- Embedding ---
    embedding_model_path: str = "BAAI/bge-m3"
    embedding_dim: int = 1024

    # --- Vector Store ---
    faiss_index_dir: Path = Path("./data/faiss")
    terminology_excel: Path = Path("./data/medical_terms.xlsx")

    # --- Memory ---
    sqlite_path: Path = Path("./data/memory.db")

    # --- Retrieval ---
    top_k: int = 5
    similarity_threshold: float = 0.6


@lru_cache
def get_settings() -> Settings:
    return Settings()

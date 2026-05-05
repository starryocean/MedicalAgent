"""API Pydantic schemas — the contract between UI and Agent."""
from typing import Literal
from pydantic import BaseModel, Field


class TranslationRequest(BaseModel):
    source_texts: list[str] = Field(..., description="待翻译的中文词条列表")
    target_language: Literal["en", "ja", "ko", "de", "fr", "es"] = "en"
    user_instruction: str = Field("", description="用户自由文本指令")
    session_id: str = Field(..., description="会话ID，用于术语一致性")


class TermReference(BaseModel):
    term_id: str
    source: str
    target: str
    score: float


class TranslationItem(BaseModel):
    source: str
    target: str
    term_refs: list[TermReference] = []
    confidence: float = 1.0


class TranslationMeta(BaseModel):
    warnings: list[str] = []
    sources: list[str] = []
    tokens_used: int = 0


class TranslationResponse(BaseModel):
    translations: list[TranslationItem]
    meta: TranslationMeta

"""
Pydantic v2 schemas for API request/response validation.
"""
from __future__ import annotations

from pydantic import BaseModel, Field


class IniEntry(BaseModel):
    key: str
    value: str
    section: str = ""
    line_number: int = 0


class TranslationRequest(BaseModel):
    entries: list[IniEntry] = Field(..., min_length=1, max_length=500)
    target_language: str = Field(..., min_length=2, max_length=50)
    user_context: str = Field(default="", max_length=2000)
    session_id: str = Field(default="default", max_length=128)
    source_file_path: str = Field(default="")
    target_file_path: str = Field(default="")


class TranslationItem(BaseModel):
    key: str
    source: str
    target: str
    confidence: float = Field(ge=0.0, le=1.0)
    source_type: str = "llm_generated"
    notes: str = ""


class TranslationResponse(BaseModel):
    success: bool
    translations: list[TranslationItem]
    summary: str = ""
    warnings: list[str] = Field(default_factory=list)
    session_id: str
    diff_result: dict | None = None


class ApplyTranslationRequest(BaseModel):
    source_file_path: str
    target_file_path: str
    translations: list[TranslationItem]


class KBRebuildRequest(BaseModel):
    force: bool = False

"""Agent state — the single source of truth across graph nodes."""
from typing import TypedDict, Annotated
from operator import add


class AgentState(TypedDict):
    # Input
    session_id: str
    source_texts: list[str]
    target_language: str
    user_instruction: str

    # Intermediate
    retrieved_terms: list[dict]          # [{source, target, score, term_id}]
    draft_translations: list[dict]       # LLM 初译
    validation_issues: Annotated[list[str], add]

    # Control
    reflection_count: int
    max_reflections: int

    # Output
    final_translations: list[dict]
    warnings: list[str]
    tokens_used: int

"""Skill: Retrieve medical terminology from FAISS."""
from tools.vector_search import VectorSearchTool
from config.config import get_settings

_settings = get_settings()
_tool = VectorSearchTool()


def retrieve_node(state: dict) -> dict:
    """For each source text, retrieve top-k similar terms."""
    retrieved = []
    for text in state["source_texts"]:
        hits = _tool.search(
            query=text,
            top_k=_settings.top_k,
            target_lang=state["target_language"],
        )
        retrieved.append({"source": text, "candidates": hits})
    return {"retrieved_terms": retrieved}

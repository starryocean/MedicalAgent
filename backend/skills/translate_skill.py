"""Skill: Translate via LLM with retrieved context."""
import json
from tools.llm_client import LLMClient
from agent.prompts import render_prompt

_llm = LLMClient()


def translate_node(state: dict) -> dict:
    prompt = render_prompt(
        "translate.j2",
        source_texts=state["source_texts"],
        target_language=state["target_language"],
        user_instruction=state["user_instruction"],
        retrieved_terms=state["retrieved_terms"],
    )

    response = _llm.chat_json(
        system=render_prompt("system.j2"),
        user=prompt,
    )

    translations = response.get("translations", [])
    return {
        "draft_translations": translations,
        "tokens_used": state.get("tokens_used", 0) + response.get("_tokens", 0),
    }

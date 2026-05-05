"""
Prompt engineering following Claude's best practices.
Prompts are externalized for easy iteration without code changes.
Reference: Anthropic prompt engineering guide + LangChain prompt templates.
"""
from __future__ import annotations

import os
from pathlib import Path
from jinja2 import Template

from langchain_core.prompts import ChatPromptTemplate, MessagesPlaceholder

# ── System Prompt ─────────────────────────────────────────────────────────────
# Claude prompt engineering best practices:
# 1. Clear role definition at the top
# 2. Explicit output format specification
# 3. Chain-of-thought encouragement with <thinking> tags
# 4. Grounding instructions for knowledge base usage
TRANSLATION_SYSTEM_PROMPT = """\
You are an expert software localization engineer specializing in UI string translation.
Your translations must be:
- **Accurate**: Preserve the original meaning precisely
- **Consistent**: Use terminology from the approved knowledge base
- **Natural**: Sound native in the target language
- **Contextual**: Consider UI context (button labels, menu items, error messages, tooltips)

<knowledge_base_instructions>
When relevant translations are found in the knowledge base:
1. Prefer knowledge base translations for exact or near-exact matches
2. Adapt knowledge base entries for similar-but-different strings
3. Clearly indicate when a translation comes from the knowledge base
4. Flag any inconsistencies you notice in the knowledge base
</knowledge_base_instructions>

<output_format>
Return your response as valid JSON with this exact structure:
{{
  "translations": [
    {{
      "key": "original.ini.key",
      "source": "original text",
      "target": "translated text",
      "confidence": 0.0-1.0,
      "source_type": "knowledge_base|llm_generated|hybrid",
      "notes": "optional translation notes"
    }}
  ],
  "summary": "Brief summary of translation decisions",
  "warnings": ["any issues or inconsistencies found"]
}}
</output_format>

<thinking_instructions>
Before translating, use <thinking> tags to:
1. Analyze the UI context of each string
2. Check if knowledge base matches are appropriate
3. Consider cultural adaptation needs
4. Plan consistent terminology usage across all strings
</thinking_instructions>
"""

# ── Human Message Template ─────────────────────────────────────────────────────
TRANSLATION_HUMAN_TEMPLATE = """\
<task>
Translate the following INI file entries to **{target_language}**.
</task>

<source_entries>
{source_entries}
</source_entries>

<knowledge_base_matches>
{kb_context}
</knowledge_base_matches>

<additional_context>
{user_context}
</additional_context>
"""

# ── Data Cleaning Prompt ───────────────────────────────────────────────────────
DATA_CLEANING_SYSTEM_PROMPT = """\
You are a data quality engineer. Analyze translation pairs and identify issues.
Return JSON with fields: is_valid (bool), cleaned_source (str), cleaned_target (str),
issue_type (null | "encoding" | "incomplete" | "formatting" | "duplicate"), notes (str).
"""


def build_translation_prompt() -> ChatPromptTemplate:
    """Build the main translation chat prompt with memory support."""
    return ChatPromptTemplate.from_messages([
        ("system", TRANSLATION_SYSTEM_PROMPT),
        MessagesPlaceholder(variable_name="chat_history", optional=True),
        ("human", TRANSLATION_HUMAN_TEMPLATE),
    ])


def build_data_cleaning_prompt() -> ChatPromptTemplate:
    """Build prompt for knowledge base data cleaning."""
    return ChatPromptTemplate.from_messages([
        ("system", DATA_CLEANING_SYSTEM_PROMPT),
        ("human", "Source: {source}\nTarget: {target}\nLanguage pair: {lang_pair}"),
    ])


def render_prompt(template_name: str, **kwargs) -> str:
    """Render a Jinja2 template from the prompts directory."""
    template_path = Path(__file__).parent.parent / "prompts" / template_name
    with open(template_path, "r", encoding="utf-8") as f:
        template = Template(f.read())
    return template.render(**kwargs)

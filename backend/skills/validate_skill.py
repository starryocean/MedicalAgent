"""Skill: Validate consistency & coverage."""
from collections import defaultdict


def validate_node(state: dict) -> dict:
    issues: list[str] = []
    drafts = state["draft_translations"]

    # Rule 1: Completeness
    if len(drafts) != len(state["source_texts"]):
        issues.append(
            f"翻译条目数({len(drafts)}) 与源条目数({len(state['source_texts'])}) 不匹配"
        )

    # Rule 2: Terminology consistency — same source should map to same target
    src_to_tgt: dict[str, set[str]] = defaultdict(set)
    for item in drafts:
        src_to_tgt[item["source"]].add(item["target"])
    for src, tgts in src_to_tgt.items():
        if len(tgts) > 1:
            issues.append(f"术语 '{src}' 存在多个译法: {tgts}")

    # Rule 3: Non-empty
    for item in drafts:
        if not item.get("target", "").strip():
            issues.append(f"'{item.get('source')}' 翻译为空")

    return {"validation_issues": issues}

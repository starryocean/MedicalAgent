"""LangGraph StateGraph — mirrors Claude Code agent loop."""
from langgraph.graph import StateGraph, END
from graph.state import AgentState
from skills.retrieve_skill import retrieve_node
from skills.translate_skill import translate_node
from skills.validate_skill import validate_node


def _should_reflect(state: AgentState) -> str:
    """Conditional edge: reflect only when validation fails and budget left."""
    if not state["validation_issues"]:
        return "finalize"
    if state["reflection_count"] >= state["max_reflections"]:
        return "finalize"
    return "reflect"


def _reflect_node(state: AgentState) -> dict:
    """Self-correction: feed issues back into translation."""
    return {
        "reflection_count": state["reflection_count"] + 1,
        "user_instruction": (
            state["user_instruction"]
            + "\n[修正要求] 上一轮存在以下问题，请修正：\n- "
            + "\n- ".join(state["validation_issues"])
        ),
        "validation_issues": [],  # reset
    }


def _finalize_node(state: AgentState) -> dict:
    return {"final_translations": state["draft_translations"]}


def build_graph():
    g = StateGraph(AgentState)

    g.add_node("retrieve", retrieve_node)
    g.add_node("translate", translate_node)
    g.add_node("validate", validate_node)
    g.add_node("reflect", _reflect_node)
    g.add_node("finalize", _finalize_node)

    g.set_entry_point("retrieve")
    g.add_edge("retrieve", "translate")
    g.add_edge("translate", "validate")
    g.add_conditional_edges(
        "validate",
        _should_reflect,
        {"reflect": "translate", "finalize": "finalize"},
    )
    g.add_edge("reflect", "translate")
    g.add_edge("finalize", END)

    return g.compile()

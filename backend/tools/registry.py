"""ToolRegistry — unified tool discovery and invocation.

Mirrors Claude Code's Tools.md pattern: each tool is a callable with
a name, description, and input_schema for structured invocation.
"""
from __future__ import annotations
from typing import Callable, Any


class Tool:
    def __init__(
        self,
        name: str,
        description: str,
        fn: Callable,
        input_schema: dict[str, Any] | None = None,
    ):
        self.name = name
        self.description = description
        self.fn = fn
        self.input_schema = input_schema or {}

    def invoke(self, **kwargs) -> Any:
        return self.fn(**kwargs)


class ToolRegistry:
    """Global tool registry — tools register themselves on import."""

    def __init__(self) -> None:
        self._tools: dict[str, Tool] = {}

    def register(self, tool: Tool) -> None:
        self._tools[tool.name] = tool

    def get(self, name: str) -> Tool | None:
        return self._tools.get(name)

    def list_tools(self) -> list[Tool]:
        return list(self._tools.values())


# Singleton
_registry = ToolRegistry()


def get_registry() -> ToolRegistry:
    return _registry

"""Conversation memory — short-term, in-process.

Mirrors Claude Code's Memory.md concept:
short-term = current session context (conversation history)
long-term  = persisted to SQLite for cross-session recall.

This module provides the short-term side: a simple sliding-window
conversation buffer.
"""
from collections import deque
from dataclasses import dataclass, field
from typing import Any


@dataclass
class Turn:
    role: str       # "user" | "assistant" | "system"
    content: str
    metadata: dict[str, Any] = field(default_factory=dict)


class ConversationMemory:
    """Sliding-window conversation memory."""

    def __init__(self, max_turns: int = 50):
        self._turns: deque[Turn] = deque(maxlen=max_turns)

    def add_turn(self, role: str, content: str, **meta) -> None:
        self._turns.append(Turn(role=role, content=content, metadata=meta))

    def recent(self, n: int = 10) -> list[Turn]:
        return list(self._turns)[-n:]

    def clear(self) -> None:
        self._turns.clear()

    def all_turns(self) -> list[Turn]:
        return list(self._turns)

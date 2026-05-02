"""
Session-based conversation memory management.
Implements sliding window memory to stay within token limits.
"""
from __future__ import annotations

import time
from collections import defaultdict
from dataclasses import dataclass, field

import structlog
from langchain_core.messages import AIMessage, BaseMessage, HumanMessage

from backend.config.settings import get_settings

logger = structlog.get_logger(__name__)


@dataclass
class SessionState:
    """State for a single translation session."""
    session_id: str
    messages: list[BaseMessage] = field(default_factory=list)
    created_at: float = field(default_factory=time.time)
    last_accessed: float = field(default_factory=time.time)

    def touch(self) -> None:
        self.last_accessed = time.time()

    @property
    def is_expired(self) -> bool:
        ttl = get_settings().session_ttl_seconds
        return (time.time() - self.last_accessed) > ttl


class SessionMemoryManager:
    """
    Manages per-session conversation history with TTL-based expiration.

    Uses in-memory storage suitable for single-instance deployment.
    For distributed deployment, replace with Redis-backed implementation
    following the same interface.
    """

    def __init__(self, settings=None) -> None:
        self._settings = settings or get_settings()
        self._sessions: dict[str, SessionState] = defaultdict(
            lambda: SessionState(session_id="")
        )

    def _get_or_create(self, session_id: str) -> SessionState:
        """Get existing session or create new one."""
        if session_id not in self._sessions or self._sessions[session_id].is_expired:
            self._sessions[session_id] = SessionState(session_id=session_id)
            logger.debug("session_created", session_id=session_id)

        session = self._sessions[session_id]
        session.touch()
        return session

    def get_history(self, session_id: str) -> list[BaseMessage]:
        """Retrieve conversation history for a session."""
        session = self._get_or_create(session_id)
        return self._trim_to_token_limit(session.messages)

    def add_exchange(
        self, session_id: str, human_message: str, ai_message: str
    ) -> None:
        """Add a human-AI exchange to session history."""
        session = self._get_or_create(session_id)
        session.messages.extend([
            HumanMessage(content=human_message),
            AIMessage(content=ai_message),
        ])

    def clear_session(self, session_id: str) -> None:
        """Clear all history for a session."""
        if session_id in self._sessions:
            del self._sessions[session_id]
            logger.info("session_cleared", session_id=session_id)

    def cleanup_expired(self) -> int:
        """Remove all expired sessions. Returns count of removed sessions."""
        expired = [sid for sid, s in self._sessions.items() if s.is_expired]
        for sid in expired:
            del self._sessions[sid]
        if expired:
            logger.info("sessions_expired", count=len(expired))
        return len(expired)

    def _trim_to_token_limit(self, messages: list[BaseMessage]) -> list[BaseMessage]:
        """
        Trim message history to stay within token limits.
        Uses simple character-based approximation (4 chars ≈ 1 token).
        """
        max_chars = self._settings.max_memory_tokens * 4
        total_chars = sum(len(m.content) for m in messages)

        if total_chars <= max_chars:
            return messages

        # Remove oldest messages until within limit (keep most recent context)
        trimmed = list(messages)
        while trimmed and sum(len(m.content) for m in trimmed) > max_chars:
            trimmed = trimmed[2:]  # Remove in human-AI pairs

        return trimmed

"""Long-term memory — persisted to SQLite for cross-session recall.

Stores:
- Translation history (source → target per session)
- User preference signals (e.g., preferred terms for specific sources)
- Audit log for compliance
"""
import json
import sqlite3
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from agent.config import get_settings


class LongTermMemory:
    """Thread-safe requires external lock if shared across requests."""

    def __init__(self, db_path: Path | None = None) -> None:
        self._path = db_path or get_settings().sqlite_path
        self._conn = sqlite3.connect(str(self._path), check_same_thread=False)
        self._init_tables()

    def _init_tables(self) -> None:
        self._conn.executescript("""
            CREATE TABLE IF NOT EXISTS translation_history (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                session_id  TEXT NOT NULL,
                source      TEXT NOT NULL,
                target      TEXT NOT NULL,
                target_lang TEXT NOT NULL,
                term_refs   TEXT,       -- JSON array
                created_at  TEXT NOT NULL DEFAULT (datetime('now'))
            );
            CREATE TABLE IF NOT EXISTS user_preferences (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                source      TEXT NOT NULL UNIQUE,
                preferred_target TEXT NOT NULL,
                updated_at  TEXT NOT NULL DEFAULT (datetime('now'))
            );
            CREATE INDEX IF NOT EXISTS idx_history_session
                ON translation_history(session_id);
        """)
        self._conn.commit()

    def record_translation(
        self,
        session_id: str,
        source: str,
        target: str,
        target_lang: str,
        term_refs: list[dict] | None = None,
    ) -> None:
        self._conn.execute(
            "INSERT INTO translation_history (session_id, source, target, target_lang, term_refs) "
            "VALUES (?, ?, ?, ?, ?)",
            (session_id, source, target, target_lang,
             json.dumps(term_refs, ensure_ascii=False) if term_refs else "[]"),
        )
        self._conn.commit()

    def get_session_history(self, session_id: str) -> list[dict]:
        rows = self._conn.execute(
            "SELECT source, target, target_lang, term_refs, created_at "
            "FROM translation_history WHERE session_id = ? ORDER BY id",
            (session_id,),
        ).fetchall()
        return [
            {"source": r[0], "target": r[1], "target_lang": r[2],
             "term_refs": json.loads(r[3]), "created_at": r[4]}
            for r in rows
        ]

    def close(self) -> None:
        self._conn.close()

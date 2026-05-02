"""
Diff utilities for comparing INI file translations.
"""
from __future__ import annotations

import difflib
from dataclasses import dataclass
from pathlib import Path


@dataclass
class DiffResult:
    """Result of comparing two INI files."""
    unified_diff: str
    changed_keys: list[str]
    added_keys: list[str]
    removed_keys: list[str]
    change_count: int


class IniDiffEngine:
    """Compute meaningful diffs between INI files."""

    def compare_files(self, original: Path, translated: Path) -> DiffResult:
        """Generate a unified diff between two INI files."""
        original_lines = self._read_lines(original)
        translated_lines = self._read_lines(translated)

        diff_lines = list(difflib.unified_diff(
            original_lines,
            translated_lines,
            fromfile=str(original),
            tofile=str(translated),
            lineterm='',
            n=2,
        ))

        original_dict = self._parse_to_dict(original)
        translated_dict = self._parse_to_dict(translated)

        orig_keys = set(original_dict.keys())
        trans_keys = set(translated_dict.keys())

        changed_keys = [
            k for k in orig_keys & trans_keys
            if original_dict[k] != translated_dict[k]
        ]

        return DiffResult(
            unified_diff='\n'.join(diff_lines),
            changed_keys=changed_keys,
            added_keys=list(trans_keys - orig_keys),
            removed_keys=list(orig_keys - trans_keys),
            change_count=len(changed_keys),
        )

    @staticmethod
    def _read_lines(path: Path) -> list[str]:
        if not path.exists():
            return []
        with open(path, encoding="utf-8-sig") as f:
            return f.readlines()

    @staticmethod
    def _parse_to_dict(path: Path) -> dict[str, str]:
        result = {}
        if not path.exists():
            return result
        with open(path, encoding="utf-8-sig") as f:
            for line in f:
                line = line.strip()
                if '=' in line and not line.startswith((';', '#')):
                    key, _, value = line.partition('=')
                    result[key.strip()] = value.strip()
        return result

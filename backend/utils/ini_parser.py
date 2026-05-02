"""
INI file parser that preserves formatting and handles Chinese/Unicode content.
"""
from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path


@dataclass
class IniEntry:
    """Represents a single INI key-value pair with line metadata."""
    key: str
    value: str
    line_number: int
    raw_line: str
    section: str = ""


class IniFileParser:
    """
    Robust INI parser that:
    - Preserves comments and whitespace
    - Handles multi-byte (Chinese, Japanese, etc.) characters
    - Tracks line numbers for in-place updates
    - Supports selection ranges for partial file translation
    """

    _SECTION_RE = re.compile(r'^\[(.+)\]$')
    _ENTRY_RE = re.compile(r'^([^=]+)=(.*)$')
    _COMMENT_RE = re.compile(r'^[;#]')

    def parse_file(self, file_path: Path) -> list[IniEntry]:
        """Parse an INI file and return ordered list of entries."""
        entries: list[IniEntry] = []
        current_section = ""

        with open(file_path, encoding="utf-8-sig", errors="replace") as f:
            lines = f.readlines()

        for line_num, raw_line in enumerate(lines, start=1):
            line = raw_line.rstrip('\n')

            if self._COMMENT_RE.match(line.strip()) or not line.strip():
                continue

            section_match = self._SECTION_RE.match(line.strip())
            if section_match:
                current_section = section_match.group(1)
                continue

            entry_match = self._ENTRY_RE.match(line)
            if entry_match:
                key = entry_match.group(1).strip()
                value = entry_match.group(2).strip()
                entries.append(IniEntry(
                    key=key,
                    value=value,
                    line_number=line_num,
                    raw_line=raw_line,
                    section=current_section,
                ))

        return entries

    def parse_selection(
        self, file_path: Path, start_line: int, end_line: int
    ) -> list[IniEntry]:
        """Parse only selected lines from an INI file (for partial translation)."""
        all_entries = self.parse_file(file_path)
        return [e for e in all_entries if start_line <= e.line_number <= end_line]

    def write_translations(
        self,
        source_file: Path,
        target_file: Path,
        translations: dict[str, str],
    ) -> None:
        """
        Write translations to target file, preserving structure of source.
        translations: {key: translated_value} mapping
        """
        with open(source_file, encoding="utf-8-sig", errors="replace") as f:
            lines = f.readlines()

        output_lines = []
        for raw_line in lines:
            line = raw_line.rstrip('\n')
            match = self._ENTRY_RE.match(line)
            if match:
                key = match.group(1).strip()
                if key in translations:
                    # Preserve original formatting, replace only value
                    prefix = raw_line[:raw_line.index('=') + 1]
                    suffix = '\n'
                    output_lines.append(f"{prefix}{translations[key]}{suffix}")
                    continue
            output_lines.append(raw_line if raw_line.endswith('\n') else raw_line + '\n')

        target_file.parent.mkdir(parents=True, exist_ok=True)
        with open(target_file, 'w', encoding="utf-8") as f:
            f.writelines(output_lines)

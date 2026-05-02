"""
Data pipeline for processing Excel translation files into the knowledge base.
Implements: Extract → Clean → Normalize → Deduplicate → Load pattern.
"""
from __future__ import annotations

import hashlib
import re
import unicodedata
from dataclasses import dataclass, field
from pathlib import Path
from typing import Iterator

import pandas as pd
import structlog

logger = structlog.get_logger(__name__)


@dataclass
class TranslationRecord:
    """Normalized translation record with metadata."""
    source: str
    target: str
    source_lang: str
    target_lang: str
    source_file: str
    record_id: str = field(init=False)
    quality_score: float = 1.0
    tags: list[str] = field(default_factory=list)

    def __post_init__(self) -> None:
        self.record_id = hashlib.sha256(
            f"{self.source}:{self.source_lang}:{self.target_lang}".encode()
        ).hexdigest()[:16]


@dataclass
class PipelineStats:
    """Track data pipeline execution statistics."""
    total_read: int = 0
    valid: int = 0
    cleaned: int = 0
    duplicates_removed: int = 0
    errors: int = 0

    @property
    def success_rate(self) -> float:
        return self.valid / self.total_read if self.total_read > 0 else 0.0


class TextNormalizer:
    """
    Handles text normalization for translation records.
    Single Responsibility: Only text normalization logic.
    """

    # Common OCR/encoding artifacts in translated Excel files
    _ARTIFACT_PATTERNS = [
        (r'\u00a0', ' '),          # Non-breaking space → regular space
        (r'\r\n|\r', '\n'),        # Normalize line endings
        (r'[ \t]+', ' '),          # Collapse horizontal whitespace
        (r'^\s+|\s+$', ''),        # Strip leading/trailing whitespace
        (r'&amp;', '&'),           # HTML entities
        (r'&lt;', '<'),
        (r'&gt;', '>'),
    ]

    def __init__(self) -> None:
        self._compiled = [
            (re.compile(p), r) for p, r in self._ARTIFACT_PATTERNS
        ]

    def normalize(self, text: str) -> str:
        """Apply all normalization rules to text."""
        if not isinstance(text, str):
            return ""

        # Unicode NFC normalization first
        text = unicodedata.normalize("NFC", text)

        for pattern, replacement in self._compiled:
            text = pattern.sub(replacement, text)

        return text

    def is_valid(self, text: str) -> bool:
        """Check if text is non-empty after normalization."""
        normalized = self.normalize(text)
        return bool(normalized) and len(normalized) >= 1


class ExcelExtractor:
    """
    Extracts raw translation data from Excel files.
    Handles multiple sheet layouts and column name variations.
    Single Responsibility: Only Excel reading logic.
    """

    # Flexible column name matching - real-world Excel files are inconsistent
    _SOURCE_ALIASES = {
        "chinese", "source", "原文", "中文", "src", "cn",
        "simplified chinese", "zh", "zh-cn", "chinese (simplified)"
    }
    _TARGET_ALIASES = {
        "english", "target", "译文", "translation", "en",
        "translated", "result", "output"
    }

    def extract(self, file_path: Path) -> Iterator[tuple[str, str, str]]:
        """
        Yield (source, target, sheet_name) tuples from Excel file.
        Tries all sheets and handles malformed structures gracefully.
        """
        logger.info("extracting_excel", file=str(file_path))
        try:
            xl = pd.ExcelFile(file_path, engine="openpyxl")
        except Exception as e:
            logger.error("excel_open_failed", file=str(file_path), error=str(e))
            return

        for sheet_name in xl.sheet_names:
            try:
                df = xl.parse(sheet_name, dtype=str)
                yield from self._extract_from_dataframe(df, sheet_name)
            except Exception as e:
                logger.warning("sheet_parse_failed", sheet=sheet_name, error=str(e))

    def _extract_from_dataframe(
        self, df: pd.DataFrame, sheet_name: str
    ) -> Iterator[tuple[str, str, str]]:
        """Extract source-target pairs from a DataFrame."""
        df.columns = [str(c).strip().lower() for c in df.columns]

        source_col = self._find_column(df, self._SOURCE_ALIASES)
        target_col = self._find_column(df, self._TARGET_ALIASES)

        if source_col is None or target_col is None:
            # Fallback: try first two columns if header matching fails
            if len(df.columns) >= 2:
                source_col, target_col = df.columns[0], df.columns[1]
                logger.warning(
                    "using_fallback_columns",
                    sheet=sheet_name,
                    source=source_col,
                    target=target_col,
                )
            else:
                return

        for _, row in df.iterrows():
            source = str(row.get(source_col, "")).strip()
            target = str(row.get(target_col, "")).strip()
            if source and target and source != "nan" and target != "nan":
                yield source, target, sheet_name

    @staticmethod
    def _find_column(df: pd.DataFrame, aliases: set[str]) -> str | None:
        """Find the first column matching any alias."""
        for col in df.columns:
            if any(alias in col for alias in aliases):
                return col
        return None


class DeduplicationEngine:
    """
    Removes duplicate translation records using content hashing.
    Keeps the highest quality record when duplicates are found.
    Single Responsibility: Only deduplication logic.
    """

    def deduplicate(
        self, records: list[TranslationRecord]
    ) -> tuple[list[TranslationRecord], int]:
        """
        Returns (deduplicated_records, duplicates_removed_count).
        When duplicates exist, keeps record with highest quality_score.
        """
        seen: dict[str, TranslationRecord] = {}

        for record in records:
            existing = seen.get(record.record_id)
            if existing is None:
                seen[record.record_id] = record
            elif record.quality_score > existing.quality_score:
                seen[record.record_id] = record

        duplicates = len(records) - len(seen)
        return list(seen.values()), duplicates


class TranslationDataPipeline:
    """
    Orchestrates the full ETL pipeline for translation knowledge base construction.
    Coordinates: Extract → Normalize → Validate → Deduplicate.
    """

    def __init__(
        self,
        source_lang: str = "zh",
        target_lang: str = "en",
    ) -> None:
        self.source_lang = source_lang
        self.target_lang = target_lang
        self._extractor = ExcelExtractor()
        self._normalizer = TextNormalizer()
        self._deduplicator = DeduplicationEngine()

    def process_directory(self, directory: Path) -> tuple[list[TranslationRecord], PipelineStats]:
        """Process all Excel files in directory and return clean records."""
        stats = PipelineStats()
        raw_records: list[TranslationRecord] = []

        excel_files = list(directory.glob("**/*.xls*"))
        logger.info("pipeline_start", file_count=len(excel_files), directory=str(directory))

        for excel_file in excel_files:
            for source, target, sheet in self._extractor.extract(excel_file):
                stats.total_read += 1

                norm_source = self._normalizer.normalize(source)
                norm_target = self._normalizer.normalize(target)

                if not self._normalizer.is_valid(norm_source) or \
                   not self._normalizer.is_valid(norm_target):
                    stats.errors += 1
                    continue

                if norm_source != source or norm_target != target:
                    stats.cleaned += 1

                record = TranslationRecord(
                    source=norm_source,
                    target=norm_target,
                    source_lang=self.source_lang,
                    target_lang=self.target_lang,
                    source_file=excel_file.name,
                )
                raw_records.append(record)
                stats.valid += 1

        final_records, dup_count = self._deduplicator.deduplicate(raw_records)
        stats.duplicates_removed = dup_count

        logger.info(
            "pipeline_complete",
            **{k: v for k, v in stats.__dict__.items()},
            final_count=len(final_records),
        )
        return final_records, stats

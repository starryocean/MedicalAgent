# SPEC: Translation Capability

## Intent
Provide medical-domain-aware translation from zh → {en,ja,ko,de,fr,es}
that preserves terminology consistency with an internal terminology library.

## Inputs
- source_texts: List[str], len ≥ 1
- target_language: enum
- user_instruction: str (optional, free-form)
- session_id: str (uuid)

## Outputs
- translations: List[{source, target, term_refs[], confidence}]
- meta: {warnings[], sources[], tokens_used}

## Acceptance Criteria
1. 100% of source_texts must have a corresponding translation in the same order.
2. If a source exactly matches a terminology library entry (score ≥ 0.95),
   the target MUST equal the library's translation.
3. Placeholders ({0}, %s, %d, {name}) MUST appear in target unchanged.
4. For low-confidence (score < 0.6) items, `meta.warnings` MUST include
   "unverified: <source>".
5. Response time p95 ≤ 8 s for batch ≤ 20 items.

## Non-Goals
- Patient-data de-identification (handled by a separate preprocessor).
- Translation of non-medical text (best-effort only).

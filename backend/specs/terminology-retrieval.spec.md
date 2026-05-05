# SPEC: Terminology Retrieval Capability

## Intent
Retrieve medical terminology translations from an internal vector database (FAISS)
to guide the LLM translator toward consistent, accurate translations.

## Inputs
- query: str — the Chinese source text to look up
- top_k: int (default 5)
- target_lang: str — language code (en, ja, ko, de, fr, es)

## Outputs
- candidates: List[{term_id, source, target, score}]
  - score ∈ [0, 1], cosine similarity of embedding
  - threshold: 0.6 (below this, omit from results)

## Acceptance Criteria
1. Exact-match queries (identical to a library entry) return score ≥ 0.99.
2. Semantic variants return relevant results (e.g., "糖尿病" → "diabetes").
3. Empty/irrelevant queries return empty list.
4. p99 latency ≤ 10 ms for index < 100K entries.

## Non-Goals
- Query expansion or synonym injection (handled by embedding model).
- Multi-language retrieval (query is always zh).

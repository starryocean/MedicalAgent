"""One-shot script: Excel → FAISS index.

Excel schema expected:
| term_id | zh | en | ja | ko | category | ... |
"""
from __future__ import annotations
import pickle
import argparse
from pathlib import Path
import numpy as np
import pandas as pd
import faiss
from sentence_transformers import SentenceTransformer
from agent.config import get_settings


LANG_COLS = ["en", "ja", "ko", "de", "fr", "es"]


def build_index(excel_path: Path, out_dir: Path) -> None:
    s = get_settings()
    df = pd.read_excel(excel_path).fillna("")
    if "zh" not in df.columns:
        raise ValueError("Excel must contain a 'zh' column")

    print(f"Loaded {len(df)} rows from {excel_path}")
    model = SentenceTransformer(s.embedding_model_path)

    sources = df["zh"].astype(str).tolist()
    vecs = model.encode(sources, normalize_embeddings=True,
                        show_progress_bar=True, batch_size=64)

    index = faiss.IndexFlatIP(vecs.shape[1])
    index.add(np.asarray(vecs, dtype="float32"))

    meta = []
    for i, row in df.iterrows():
        translations = {lang: str(row.get(lang, "")).strip()
                        for lang in LANG_COLS if str(row.get(lang, "")).strip()}
        meta.append({
            "term_id": str(row.get("term_id", f"T{i:06d}")),
            "source": str(row["zh"]),
            "translations": translations,
            "category": str(row.get("category", "")),
        })

    out_dir.mkdir(parents=True, exist_ok=True)
    faiss.write_index(index, str(out_dir / "index.faiss"))
    with open(out_dir / "meta.pkl", "wb") as f:
        pickle.dump(meta, f)
    print(f"Index saved to {out_dir}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--excel", required=True, type=Path)
    parser.add_argument("--out", type=Path, default=Path("./data/faiss"))
    args = parser.parse_args()
    build_index(args.excel, args.out)

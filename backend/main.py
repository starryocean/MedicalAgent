"""FastAPI server exposing the translation agent."""
import logging
import uuid
from contextlib import asynccontextmanager
from fastapi import FastAPI, HTTPException
from fastapi.middleware.cors import CORSMiddleware

from config.config import get_settings
from schemas.schemas import TranslationRequest, TranslationResponse, TranslationItem, TranslationMeta
from graph.workflow import build_graph

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
)
log = logging.getLogger("medtrans")

_graph = None


@asynccontextmanager
async def lifespan(app: FastAPI):
    global _graph
    log.info("Building agent graph...")
    _graph = build_graph()
    log.info("Agent ready.")
    yield


app = FastAPI(title="MedTrans Agent", version="1.0.0", lifespan=lifespan)
app.add_middleware(
    CORSMiddleware, allow_origins=["*"],
    allow_methods=["*"], allow_headers=["*"],
)


@app.get("/health")
async def health():
    return {"status": "ok"}


@app.post("/v1/translate", response_model=TranslationResponse)
async def translate(req: TranslationRequest) -> TranslationResponse:
    if not req.source_texts:
        raise HTTPException(400, "source_texts must not be empty")

    initial_state = {
        "session_id": req.session_id or str(uuid.uuid4()),
        "source_texts": req.source_texts,
        "target_language": req.target_language,
        "user_instruction": req.user_instruction,
        "retrieved_terms": [],
        "draft_translations": [],
        "validation_issues": [],
        "reflection_count": 0,
        "max_reflections": 2,
        "final_translations": [],
        "warnings": [],
        "tokens_used": 0,
    }

    try:
        final = await _graph.ainvoke(initial_state)
    except Exception as e:
        log.exception("Agent failure")
        raise HTTPException(500, f"Agent failure: {e}")

    return TranslationResponse(
        translations=[
            TranslationItem(
                source=t["source"],
                target=t["target"],
                term_refs=t.get("term_refs", []),
                confidence=t.get("confidence", 1.0),
            ) for t in final["final_translations"]
        ],
        meta=TranslationMeta(
            warnings=final.get("warnings", []),
            tokens_used=final.get("tokens_used", 0),
        ),
    )


if __name__ == "__main__":
    import uvicorn
    s = get_settings()
    uvicorn.run("main:app", host=s.host, port=s.port, reload=False)

from fastapi import FastAPI
from pydantic import BaseModel
from inference import InferQuest,infer_text
from prometheus_client import Counter, Histogram
from prometheus_client import CONTENT_TYPE_LATEST, generate_latest
from starlette.responses import Response


app=FastAPI(title="ai infra learning api",
            description="由Docker Compose管理的FastAPI服务",
            version="3.0.0",
            )

@app.get("/")
def test():
    return {"message":"api is running on docker",
    "status":"ok"
    }
@app.post("/infer")
def run_inference(request:InferQuest):

    return infer_text(request)
@app.get("/health")
def health():
    return {"status":"ok"}
@app.get("/metrics")
def metrics():
    """
    以Prometheus文本格式返回指标。
    """
    return Response(
        content=generate_latest(),
        media_type=CONTENT_TYPE_LATEST,
    )
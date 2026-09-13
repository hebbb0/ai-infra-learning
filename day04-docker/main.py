from fastapi import FastAPI
from pydantic import BaseModel
from inference import infer_text
app=FastAPI(title="ai infra learning api")
class InferQuest(BaseModel):
    text:str
@app.get("/")
def test():
    return {"message":"api is running on docker",
    "status":"ok"
    }
@app.post("/infer")
def run_inference(request:InferQuest):
    return infer_text(request.text)
@app.get("/health")
def health():
    return {"status":"ok"}
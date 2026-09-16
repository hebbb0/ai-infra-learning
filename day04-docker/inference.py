
import time
from fastapi import FastAPI
from pydantic import BaseModel
from prometheus_client import Counter, Histogram
from prometheus_client import CONTENT_TYPE_LATEST, generate_latest
from starlette.responses import Response


# 统计/infer接口被调用的总次数
INFER_REQUESTS = Counter(
    "infer_requests_total",
    "Total number of inference requests",
)


# 统计/infer接口的处理耗时
INFER_DURATION = Histogram(
    "infer_request_duration_seconds",
    "Inference request processing time in seconds",
)
class InferQuest(BaseModel):
    text:str
def infer_text(request:InferQuest):
    start_time=time.perf_counter()
    # 每收到一次请求，计数器加1
    INFER_REQUESTS.inc()
    text = request.text.strip()
    result = {"input" : text,
            "length" : len(text),
            "result" : f"已处理: {text}",
            }
    elapsed_seconds = time.perf_counter() - start_time
    # 记录本次请求的处理耗时
    INFER_DURATION.observe(elapsed_seconds)

    return result
    # if not text:
    #     return {
    #         "label": "invalid",
    #         "score": 0.0,
    #         "message": "输入不能为空"
    #     }

    # contains_ai = "ai" in text.lower()

    # return {
    #     "label": "ai_related" if contains_ai else "general",
    #     "score": 0.9 if contains_ai else 0.6,
    #     "input_length": len(text)
    # }


if __name__ == "__main__":
    result = infer_text("I am learning AI Infra")
    print(result)
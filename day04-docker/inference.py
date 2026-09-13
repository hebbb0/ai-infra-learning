
def infer_text(text: str):
    text = text.strip()

    if not text:
        return {
            "label": "invalid",
            "score": 0.0,
            "message": "输入不能为空"
        }

    contains_ai = "ai" in text.lower()

    return {
        "label": "ai_related" if contains_ai else "general",
        "score": 0.9 if contains_ai else 0.6,
        "input_length": len(text)
    }


if __name__ == "__main__":
    result = infer_text("I am learning AI Infra")
    print(result)
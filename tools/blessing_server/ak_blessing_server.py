#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
Adventure-King 赐福后端（LangChain）

说明（展示阶段）：
- 游戏客户端把 apiKey 通过 HTTP Authorization 传入（Bearer Token）
- 服务端使用 LangChain 工具调用强约束赐福输出结构，避免“看起来像 JSON 但解析失败”
- 默认 OpenAI 兼容 Base URL 读取自 AK_OPENAI_BASE_URL（未配置则用公共网关）

注意：
- 不会在日志中输出 apiKey
- 若 AI 未按要求调用工具/输出越界，服务端会兜底生成合法赐福
"""

from __future__ import annotations

import argparse
import os
import random
import re
import uuid
from dataclasses import dataclass
from typing import Any, Dict, List, Optional, Tuple

from fastapi import FastAPI, Header
from pydantic import BaseModel, Field

from langchain_openai import ChatOpenAI
from langchain_core.messages import HumanMessage, SystemMessage
from langchain_core.tools import tool


DEFAULT_OPENAI_BASE_URL = "https://elysiver.h-e.top/v1"
DEFAULT_MODEL = "gemini-3-flash-preview"

# 与 C++ 侧 GameConfig::AI::Blessing 保持一致（展示阶段写死）
PICK_COUNT = 2
RANGES: Dict[str, Tuple[float, float]] = {
    "strength": (2.0, 10.0),
    "defense": (1.0, 6.0),
    "criticalRate": (0.02, 0.10),
    "moveSpeed": (10.0, 60.0),
    "maxHp": (30.0, 150.0),
    "maxMp": (10.0, 80.0),
}


def _get_openai_base_url() -> str:
    return os.environ.get("AK_OPENAI_BASE_URL", DEFAULT_OPENAI_BASE_URL).rstrip("/")


def _extract_bearer_token(authorization: Optional[str]) -> str:
    if not authorization:
        return ""
    m = re.match(r"(?i)^\s*bearer\s+(.+?)\s*$", authorization)
    return m.group(1) if m else ""


@dataclass
class BlessingResult:
    npc_text: str
    buff: List[Dict[str, Any]]


def _validate_and_normalize_buff(buff: List[Dict[str, Any]]) -> Tuple[bool, str, List[Dict[str, Any]]]:
    if not isinstance(buff, list):
        return False, "buff 不是数组", []
    if len(buff) != PICK_COUNT:
        return False, f"buff 条目数必须为 {PICK_COUNT}", []

    normalized: List[Dict[str, Any]] = []
    used_keys: set[str] = set()
    for item in buff:
        if not isinstance(item, dict):
            return False, "buff 条目不是对象", []
        key = item.get("key")
        value = item.get("value")
        if not isinstance(key, str) or key not in RANGES:
            return False, "key 非法", []
        if key in used_keys:
            return False, "key 重复", []
        used_keys.add(key)
        try:
            val_f = float(value)
        except Exception:
            return False, "value 不是数字", []

        mn, mx = RANGES[key]
        if val_f < mn or val_f > mx:
            return False, f"value 越界：{key}={val_f} 不在 [{mn},{mx}]", []

        # 输出保持原 key 与数值（数值不强行取整；由客户端决定如何处理）
        normalized.append({"key": key, "value": val_f})

    return True, "", normalized


def _fallback_blessing(npc_text: str = "愿你的脚步不再犹豫。") -> BlessingResult:
    keys = list(RANGES.keys())
    random.shuffle(keys)
    picked = keys[:PICK_COUNT]
    buff: List[Dict[str, Any]] = []
    for k in picked:
        mn, mx = RANGES[k]
        val = mn + (mx - mn) * random.random()
        buff.append({"key": k, "value": round(val, 4)})
    return BlessingResult(npc_text=npc_text, buff=buff)


@tool
def apply_blessing(
    npc_text: str,
    strength: float = 0,
    defense: float = 0,
    max_hp: float = 0,
    max_mp: float = 0,
    critical_rate: float = 0,
    move_speed: float = 0,
) -> dict:
    """给玩家应用赐福属性加成（服务端会再次校验范围与数量）。"""
    buff = []
    if strength:
        buff.append({"key": "strength", "value": strength})
    if defense:
        buff.append({"key": "defense", "value": defense})
    if max_hp:
        buff.append({"key": "maxHp", "value": max_hp})
    if max_mp:
        buff.append({"key": "maxMp", "value": max_mp})
    if critical_rate:
        buff.append({"key": "criticalRate", "value": critical_rate})
    if move_speed:
        buff.append({"key": "moveSpeed", "value": move_speed})
    return {"npcText": npc_text, "buff": buff}


class QuestionRequest(BaseModel):
    model: str = Field(default=DEFAULT_MODEL)
    user_prompt: str = Field(default="")


class AnswerRequest(BaseModel):
    model: str = Field(default=DEFAULT_MODEL)
    npc_questions: str = Field(default="")
    player_answer: str = Field(default="")


app = FastAPI()


@app.get("/")
async def root():
    return {"ok": True, "message": "Adventure-King Blessing Server (LangChain)"}


def _build_llm(model: str, api_key: str) -> ChatOpenAI:
    base_url = _get_openai_base_url()
    # 通过环境变量也兼容 openai 库的行为（不把 apiKey 写入 env）
    os.environ.setdefault("OPENAI_BASE_URL", base_url)
    return ChatOpenAI(model=model or DEFAULT_MODEL, api_key=api_key, temperature=0.6)


@app.post("/api/blessing/question")
async def blessing_question(req: QuestionRequest, authorization: Optional[str] = Header(default=None)):
    api_key = _extract_bearer_token(authorization)
    if not api_key:
        return {"ok": False, "error": "缺少 Authorization: Bearer <apiKey>"}

    llm = _build_llm(req.model, api_key)

    system_prompt = (
        "你是游戏里的“赐福NPC”。你需要先考验玩家的冒险决心。\n"
        "请提出 2-3 个问题，总字数不超过 80 字。\n"
        "要求：中文、语气神秘、不要输出 Markdown。\n"
    )
    user_prompt = req.user_prompt.strip() or "请开始考验。"

    try:
        resp = llm.invoke([SystemMessage(content=system_prompt), HumanMessage(content=user_prompt)])
        text = (resp.content or "").strip()
        if not text:
            # 兜底
            text = "你为何踏入此地？你愿为此失去什么？你最惧怕的失败是什么？"
        return {"ok": True, "data": {"npcQuestions": text}}
    except Exception:
        # 不泄露 key，不返回异常细节
        return {"ok": False, "error": "生成问题失败（请检查网络/模型/Key）"}


@app.post("/api/blessing/answer")
async def blessing_answer(req: AnswerRequest, authorization: Optional[str] = Header(default=None)):
    api_key = _extract_bearer_token(authorization)
    if not api_key:
        return {"ok": False, "error": "缺少 Authorization: Bearer <apiKey>"}

    llm = _build_llm(req.model, api_key).bind_tools([apply_blessing])

    # system prompt：强约束工具调用与范围
    ranges_text = "\n".join([f"- {k}: [{mn},{mx}]" for k, (mn, mx) in RANGES.items()])
    system_prompt = (
        "你是游戏里的“赐福NPC”。你必须使用 apply_blessing 工具给玩家赐福。\n"
        f"赐福规则：必须选择且仅选择 {PICK_COUNT} 个属性。\n"
        "属性只能来自候选列表，数值必须严格落在范围内。\n"
        "npc_text 用一句中文台词（不超过 30 字）。\n"
        "候选列表：\n"
        f"{ranges_text}\n"
        "禁止输出其它内容；必须调用工具。\n"
    )

    npc_q = req.npc_questions.strip()
    answer = req.player_answer.strip()
    if not npc_q or not answer:
        return {"ok": False, "error": "npc_questions / player_answer 不能为空"}

    user_prompt = f"考验问题：{npc_q}\n玩家回答：{answer}\n请赐福。"

    try:
        resp = llm.invoke([SystemMessage(content=system_prompt), HumanMessage(content=user_prompt)])
        tool_calls = getattr(resp, "tool_calls", None) or []
        if not tool_calls:
            fallback = _fallback_blessing("你的回答尚可……收下这份赐福。")
            return {"ok": True, "data": {"npcText": fallback.npc_text, "buff": fallback.buff, "fallback": True}}

        # 只取第一个工具调用
        args = tool_calls[0].get("args", {}) if isinstance(tool_calls[0], dict) else {}
        tool_ret = apply_blessing.invoke(args)
        npc_text = str(tool_ret.get("npcText") or "").strip() or "愿你无惧前路。"
        buff_raw = tool_ret.get("buff") or []
        ok, err, normalized = _validate_and_normalize_buff(buff_raw)
        if not ok:
            fallback = _fallback_blessing("你的誓言未尽……仍可受赐。")
            return {"ok": True, "data": {"npcText": fallback.npc_text, "buff": fallback.buff, "fallback": True, "hint": err}}

        return {"ok": True, "data": {"npcText": npc_text, "buff": normalized}}
    except Exception:
        fallback = _fallback_blessing("风起云涌……赐福仍将降临。")
        return {"ok": True, "data": {"npcText": fallback.npc_text, "buff": fallback.buff, "fallback": True}}


def main():
    parser = argparse.ArgumentParser(description="Adventure-King Blessing Server (LangChain)")
    sub = parser.add_subparsers(dest="cmd", required=True)

    serve = sub.add_parser("serve", help="启动服务")
    serve.add_argument("--host", default="0.0.0.0")
    serve.add_argument("--port", type=int, default=5181)

    args = parser.parse_args()
    if args.cmd == "serve":
        import uvicorn

        uvicorn.run(app, host=args.host, port=args.port, log_level="info")


if __name__ == "__main__":
    main()


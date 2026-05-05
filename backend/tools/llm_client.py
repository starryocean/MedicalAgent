"""Thin LLM abstraction. Defaults to Anthropic, pluggable."""
import json
import re
from config.config import get_settings


class LLMClient:
    def __init__(self) -> None:
        self._s = get_settings()
        self._client = self._build_client()

    def _build_client(self):
        if self._s.llm_provider == "anthropic":
            from anthropic import Anthropic
            return Anthropic(api_key=self._s.llm_api_key,
                             base_url=self._s.llm_base_url)
        elif self._s.llm_provider == "openai":
            from openai import OpenAI
            return OpenAI(api_key=self._s.llm_api_key,
                          base_url=self._s.llm_base_url)
        raise ValueError(f"Unsupported provider: {self._s.llm_provider}")

    def chat_json(self, system: str, user: str) -> dict:
        """Returns parsed JSON. Strips markdown fences defensively."""
        if self._s.llm_provider == "anthropic":
            msg = self._client.messages.create(
                model=self._s.llm_model,
                max_tokens=4096,
                temperature=self._s.llm_temperature,
                system=system,
                messages=[{"role": "user", "content": user}],
            )
            text = msg.content[0].text
            tokens = msg.usage.input_tokens + msg.usage.output_tokens
        else:
            resp = self._client.chat.completions.create(
                model=self._s.llm_model,
                temperature=self._s.llm_temperature,
                messages=[
                    {"role": "system", "content": system},
                    {"role": "user", "content": user},
                ],
                response_format={"type": "json_object"},
            )
            text = resp.choices[0].message.content
            tokens = resp.usage.total_tokens

        data = self._extract_json(text)
        data["_tokens"] = tokens
        return data

    @staticmethod
    def _extract_json(text: str) -> dict:
        # Strip ```json fences
        m = re.search(r"```(?:json)?\s*(\{.*?\})\s*```", text, re.DOTALL)
        if m:
            text = m.group(1)
        return json.loads(text)

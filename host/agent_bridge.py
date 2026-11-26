#!/usr/bin/env python3
"""Project Genesis - host side of the AI agent link.

The laptop runs the kernel and the terminal; this script runs on the machine
that has the model and the CPU to run it. They talk over a null-modem serial
cable, so no network stack, no TLS and no driver work is needed on the laptop.

Protocol (see include/kernel/agent.h in the kernel tree):

    laptop -> host    "AI <question>\n"
    host -> laptop    "<answer>" followed by one 0x04 byte (EOT)

Two backends are supported:

  * Ollama (default):   http://127.0.0.1:11434, model from --model
  * OpenAI-compatible:  --api-url https://.../v1, key from --api-key or
                        the GENESIS_API_KEY environment variable

Usage:
    pip install pyserial
    python3 host/agent_bridge.py --port /dev/ttyUSB0 --model llama3.2

On Windows the port is something like COM3.
"""

import argparse
import json
import os
import sys
import urllib.error
import urllib.request

try:
    import serial  # pyserial
except ImportError:
    sys.exit("error: pyserial is missing (pip install pyserial)")

EOT = b"\x04"

SYSTEM_PROMPT = (
    "You are the assistant built into Project Genesis, a small 32-bit operating "
    "system running on a Dell Latitude D600. The user sees your answer on an "
    "80x25 text screen, so answer in the user's language, plainly and briefly: "
    "at most 10 short lines, no Markdown tables, no code fences longer than "
    "needed."
)


def ask_ollama(base_url, model, question):
    payload = json.dumps(
        {
            "model": model,
            "prompt": f"{SYSTEM_PROMPT}\n\nUser: {question}\nAssistant:",
            "stream": False,
        }
    ).encode()

    request = urllib.request.Request(
        base_url.rstrip("/") + "/api/generate",
        data=payload,
        headers={"Content-Type": "application/json"},
    )

    with urllib.request.urlopen(request, timeout=300) as response:
        body = json.loads(response.read().decode())
    return body.get("response", "").strip()


def ask_openai(base_url, api_key, model, question):
    payload = json.dumps(
        {
            "model": model,
            "messages": [
                {"role": "system", "content": SYSTEM_PROMPT},
                {"role": "user", "content": question},
            ],
        }
    ).encode()

    headers = {"Content-Type": "application/json"}
    if api_key:
        headers["Authorization"] = f"Bearer {api_key}"

    request = urllib.request.Request(
        base_url.rstrip("/") + "/chat/completions", data=payload, headers=headers
    )

    with urllib.request.urlopen(request, timeout=300) as response:
        body = json.loads(response.read().decode())
    return body["choices"][0]["message"]["content"].strip()


def sanitise(answer):
    """The link is line based and ends with EOT; keep those bytes out."""
    cleaned = answer.replace("\x04", "").replace("\r\n", "\n").replace("\r", "\n")
    if not cleaned.endswith("\n"):
        cleaned += "\n"
    return cleaned


def main():
    parser = argparse.ArgumentParser(description="Project Genesis agent bridge")
    parser.add_argument("--port", required=True, help="serial port, e.g. /dev/ttyUSB0 or COM3")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--model", default=os.environ.get("GENESIS_MODEL", "llama3.2"))
    parser.add_argument("--backend", choices=["ollama", "openai"], default="ollama")
    parser.add_argument(
        "--ollama-url", default=os.environ.get("GENESIS_OLLAMA_URL", "http://127.0.0.1:11434")
    )
    parser.add_argument(
        "--api-url", default=os.environ.get("GENESIS_API_URL", "https://api.openai.com/v1")
    )
    parser.add_argument("--api-key", default=os.environ.get("GENESIS_API_KEY", ""))
    args = parser.parse_args()

    link = serial.Serial(args.port, args.baud, timeout=1)
    print(f"listening on {args.port} at {args.baud} baud, model {args.model}", file=sys.stderr)

    buffer = b""

    while True:
        chunk = link.read(4096)
        if chunk:
            buffer += chunk
            while b"\n" in buffer:
                line, buffer = buffer.split(b"\n", 1)
                text = line.decode("utf-8", "replace").strip()
                if not text.startswith("AI "):
                    continue

                question = text[3:].strip()
                print(f"laptop: {question}", file=sys.stderr)

                try:
                    if args.backend == "openai":
                        answer = ask_openai(args.api_url, args.api_key, args.model, question)
                    else:
                        answer = ask_ollama(args.ollama_url, args.model, question)
                except (urllib.error.URLError, OSError, KeyError, ValueError) as error:
                    answer = f"error: could not reach the model ({error})"

                print(f"model: {answer}", file=sys.stderr)
                link.write(sanitise(answer).encode("utf-8") + EOT + b"\r\n")
                link.flush()
        elif buffer and len(buffer) > 4096:
            # A long garbage line with no newline: drop it instead of growing.
            buffer = b""


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass

# Host side of the agent link (optional)

> **Opcional e desligado por omissão.** O agente é trabalho futuro do professor;
> este programa só faz sentido com um kernel construído com `make AGENT=1`.

O portátil não corre o modelo — não caberia em 512 MB. Este programa corre no
computador que tem o modelo e o CPU para o mover, e liga-se ao Genesis por um
cabo série.

```text
Dell Latitude D600                      PC com o modelo
┌──────────────────────┐  cabo série   ┌──────────────────────────┐
│ Genesis              │  (null modem) │ agent_bridge.py          │
│  ai <pergunta>  ─────┼──────────────▶│  Ollama / API            │
│  resposta       ◀────┼───────────────│  modelo                  │
└──────────────────────┘               └──────────────────────────┘
```

## O que precisas

- Um cabo **null modem** DB9 (fêmea-fêmea, cruzado) entre o portátil e o PC, ou
  um adaptador USB-série no PC e um cabo DB9 normal.
- Python 3 e `pyserial` no PC.
- Um modelo local com Ollama, ou uma chave de API.

```sh
pip install pyserial
```

## Modelo local (recomendado)

Numa máquina com 8 GB de RAM, um modelo pequeno responde bem:

```sh
ollama pull llama3.2        # ~2 GB, boa qualidade para o tamanho
# ou, mais leve:
ollama pull qwen2.5:1.5b
```

Depois, no PC:

```sh
python3 host/agent_bridge.py --port /dev/ttyUSB0 --model llama3.2
```

No Windows a porta é `COM3` (ou outra):

```sh
python3 host\agent_bridge.py --port COM3 --model llama3.2
```

## Modelo na nuvem

Se preferires um modelo grande, sem instalar nada localmente:

```sh
export GENESIS_API_KEY=...
python3 host/agent_bridge.py --port /dev/ttyUSB0 \
    --backend openai --api-url https://api.openai.com/v1 --model gpt-4o-mini
```

A chave fica só no PC; o portátil nunca a vê.

## No portátil

Arranca o Genesis com o cabo ligado e escreve:

```text
genesis> ai explica-me o que e um kernel
```

A resposta aparece no ecrã do portátil, linha a linha, à medida que chega.

## Problemas comuns

| Sintoma | Causa provável |
|---|---|
| `agent: no serial link` | a porta série está desativada na BIOS, ou o kernel arrancou sem UART |
| `agent: the host did not answer` | o bridge não está a correr, a porta errada, ou o modelo ainda a carregar |
| texto com caracteres estranhos | velocidade errada: usa 115200 dos dois lados |
| resposta cortada a meio | o modelo respondeu com mais de 1024 bytes; o kernel trunca |

## Limites atuais

- Cada pergunta é independente: não há memória da conversa anterior.
- A resposta é texto simples, sem streaming token a token (chega por linhas).
- O link é local e não passa pela rede, por isso não há SSH nem TLS nesta
  etapa; ver `docs/NETWORK.md` para o caminho até lá.

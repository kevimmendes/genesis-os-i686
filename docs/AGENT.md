# O agente de IA no Genesis (opção futura)

> **Estado: desligado por omissão.** O agente é trabalho do professor e fica
> reservado como opção futura. O código do kernel, o protocolo e o programa do
> lado do PC existem e estão prontos a ligar, mas não fazem parte do sistema
> normal: só entram no build com `make AGENT=1`. Este documento descreve a
> tomada que fica à espera, para o agente do professor encaixar sem mexer no
> resto do sistema.

O objetivo é ter um agente de IA utilizável a partir do terminal do Genesis, no
Dell Latitude D600. Este documento explica o que foi construído, o que ele faz
e — igualmente importante — o que ele não faz.

## A divisão de trabalho

```text
┌────────────────────────────┐        ┌─────────────────────────────┐
│ D600: Genesis              │  COM1  │ PC com CPU e RAM suficientes │
│  terminal, teclado, ecrã   │◀──────▶│  agent_bridge.py + modelo    │
│  comando `ai`              │ 115200 │  (Ollama ou API)             │
└────────────────────────────┘        └─────────────────────────────┘
```

O portátil **não** corre o modelo. Um modelo de linguagem minimamente útil
precisa de vários GB de RAM; o D600 tem 512 MB e um núcleo de 1,4 GHz. Tentar
correr um modelo local ali daria respostas de poucas palavras, muito lentas, e
não caberia na memória. O sistema faz o que é honesto: é o terminal, e o modelo
corre na máquina que consegue movê-lo.

Isto não é uma limitação do Genesis: é a mesma arquitetura de um Raspberry Pi
como servidor com portáteis antigos como clientes.

## Porque a porta série, e não a rede

A rede seria o caminho "moderno", mas exige driver da Broadcom 570x, pilha
ARP/IP/TCP, TLS e um cliente HTTP — meses de trabalho, e a maior parte só se
pode testar no portátil. A porta série existe no D600 (16550, 9 pinos), o driver
já existia, e um cabo null modem dá uma ligação fiável de 115200 baud. Para um
agente de texto, chega perfeitamente.

Quando a rede existir (ver [NETWORK.md](NETWORK.md)), o mesmo protocolo pode
passar por TCP sem mudar o comando `ai`.

## O protocolo

Uma linha em cada direção, para poder ser depurado com qualquer terminal série:

```text
kernel → host   "AI <pergunta>\n"
host → kernel   "<resposta>" seguida de um único byte 0x04 (EOT)
```

Regras do lado do kernel (`kernel/agent.c`):

- o comando `ai` só corre se `COM1` tiver respondido ao teste do kernel;
- a resposta é impressa à medida que chega, com `\r` removido;
- a resposta é limitada a 1024 bytes;
- a espera é limitada a 60 s pelo temporizador, com um limite de polls para
  ambientes onde o timer não entrega interrupções (o teste Unicorn);
- uma falha imprime uma explicação e devolve o prompt à shell — nunca bloqueia.

Regras do lado do host (`host/agent_bridge.py`):

- lê linhas, ignora tudo o que não comece por `AI `;
- pergunta ao Ollama local ou a uma API compatível com OpenAI;
- limpa a resposta, acrescenta `\n` e envia-a com EOT;
- nunca envia a chave da API para o portátil.

## O que este agente é, e o que não é

**É** um assistente conversacional: perguntas e respostas de texto, com o modelo
a correr noutra máquina, visível no ecrã de 80x25 do portátil.

**Não é** ainda um agente autónomo que lê ficheiros, edita código e corre
comandos. Para isso seriam precisos três coisas que o sistema não tem: um
sistema de ficheiros, ferramentas expostas ao modelo e um ciclo de aprovação. O
`agent_bridge.py` pode ganhar ferramentas que atuam **no host** (por exemplo,
ler um ficheiro do projeto), mas isso é uma decisão de segurança separada e
deve ser documentada como tal.

## Testar

Sem o portátil à frente, o teste Unicorn simula a UART:

- o registo *scratch* responde, por isso o kernel deteta a porta;
- o estado da linha diz "há dados" e entrega uma resposta preparada;
- o teste escreve `ai hello` e confere que a resposta aparece no ecrã.

No hardware real:

1. cabo null modem entre o D600 e o PC;
2. `python3 host/agent_bridge.py --port /dev/ttyUSB0 --model llama3.2` no PC;
3. no Genesis, `ai olá` (sem acentos, o teclado do kernel ainda é ASCII).

## Próximos passos naturais

1. **Memória da conversa:** o bridge guarda as últimas trocas e envia-as como
   contexto.
2. **Streaming:** enviar a resposta à medida que o modelo a produz.
3. **Ferramentas:** o bridge lê ficheiros do projeto do host e injeta o
   conteúdo na pergunta (`ai resume o ficheiro README.md`).
4. **Rede:** substituir o cabo série por TCP quando a pilha de rede existir; o
   comando `ai` não muda.

# Genesis OS (i686)

**Um sistema operativo de 32 bits, leve, para PCs fracos — com instalador, shell e uma tomada pronta para o agente de IA do professor.**

Escrito do zero em C e Assembly para o Dell Latitude D600 (Pentium M, 512 MB de RAM),
mas corre em qualquer PC x86 de 32 bits. Não usa bibliotecas do sistema, não usa
paging e cabe em menos de 100 KB de código.

## O que já funciona

- Arranque Multiboot1 (GRUB ou `qemu -kernel`) com GDT plana e pilha própria.
- Ecrã de arranque com arte ASCII e consola VGA 80x25 com scroll.
- Consola série COM1 (16550) como segunda saída.
- IDT com 32 exceções e 16 IRQs; PIC remapeado; relatório de exceção em vez de reset silencioso.
- Temporizador a 100 Hz e teclado PS/2 com Shift, Backspace e layouts US/PT/ES.
- Gestão de memória: alocador de frames de 4 KiB por bitmap e heap do kernel.
- Shell: `help`, `clear`, `info`, `mem`, `uptime`, `calc`, `hexdump`, `splash`, `reboot`, `halt`.
- **Assistente de instalação**: idioma (PT/EN/ES), teclado (US/PT/ES), espaço em disco (1–20 GB),
  resumo e confirmação.

## Opção futura: agente de IA

O agente é trabalho do professor e está **desligado por omissão**. O kernel já tem
a tomada pronta: um protocolo de uma linha por COM1, com espera e resposta
limitadas, e um programa do lado do PC (`host/agent_bridge.py`) que fala com
Ollama ou com uma API compatível com OpenAI. Para o experimentar:

```sh
make clean && make AGENT=1
```

O modelo nunca corre no portátil — 512 MB não chegam. O portátil é o terminal; o
modelo corre no PC ligado pelo cabo série.

## Estado honesto

- A instalação **ainda não escreve no disco**: falta o driver ATA, a tabela de
  partições, um sistema de ficheiros e a instalação do GRUB. O assistente recolhe
  as escolhas e di-lo no ecrã em vez de fingir que instalou.
- Ainda não há rede, sistema de ficheiros, multitarefa nem modo utilizador.

## Construir

Precisas de GCC com `-m32`, binutils e GNU Make:

```sh
make check     # ferramentas e modo 32 bits
make           # build/kernel.elf
make emulate   # corre no Unicorn: sessão normal + assistente de instalação
make AGENT=1   # inclui o comando opcional `ai` (trabalho futuro)
make iso       # genesis.iso (precisa de grub-mkrescue e xorriso)
make run       # QEMU com ecrã e série
```

O menu do ISO tem duas entradas: **live session** e **Install Project Genesis**
(esta passa `setup` ao kernel e abre o assistente).

## Código completo

O código, os testes, os documentos e a ferramenta do agente estão no arquivo
`project-genesis-i686.zip` desta conversa. Para os colocares aqui com o teu SSH:

```sh
unzip project-genesis-i686.zip    # cria a pasta genesis-os/
cd genesis-os
git init -b main
git add -A && git commit -m "Genesis OS i686: kernel, shell, instalador e agente opcional"
git remote add origin git@github.com:kevimmendes/genesis-os-i686.git
git push -u origin main
```

## Licença

Ver `LICENSE` no arquivo do projeto.

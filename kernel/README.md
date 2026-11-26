# Código-fonte do kernel

O ponto de entrada fica o menor possível. Um subsistema ganha seu próprio diretório assim que cresce além de algumas linhas, então este arquivo é o mapa de onde cada coisa mora.

```text
kernel/
├── main.c              ponto de entrada (kmain): liga os subsistemas na ordem certa e entra na shell
├── shell.c             editor de linha e comandos (help, mem, uptime, ...)
├── panic.c             pânico e relatório de exceções da CPU
├── report.c            texto do sistema sobre si mesmo (banner, boot, máquina)
├── arch/
│   └── i686/           código específico de CPU e plataforma (32 bits)
├── drivers/            drivers de dispositivo (teclado PS/2)
├── mm/                 memória física (bitmap de frames) e heap do kernel
├── sched/              threads de kernel e escalonamento (vazio até M7)
├── fs/                 sistemas de arquivos (vazio até M8)
└── lib/                utilitários freestanding compartilhados (console, strings)
```

Os headers espelham esta estrutura em `include/`, então o caminho de um header sempre diz a qual implementação ele pertence: `include/arch/i686/pit.h` combina com `kernel/arch/i686/pit.c`.

## Conteúdo atual

| Caminho | O que tem |
|---|---|
| `main.c` | `kmain`: splash, console, memória, interrupções e depois a shell |
| `agent.c` | **opcional** (`make AGENT=1`): ligação ao agente de IA por COM1, pergunta em linha e resposta até ao byte 0x04 |
| `setup.c` | assistente de instalação: idioma, teclado, espaço em disco e confirmação |
| `shell.c` | linha de comandos em ring 0, com eco e Backspace, e as ferramentas `calc` e `hexdump` |
| `panic.c` | `panic()` e `exception_report()`: imprimem o motivo e param a CPU |
| `report.c` | banner, informação de boot, CPU, console, temporizador e frames |
| `arch/i686/idt.c` | IDT de 256 portas; despacha exceções (0–31) e IRQs (32–47) |
| `arch/i686/interrupts.s` | 48 stubs de interrupção, tabela de endereços e retorno com `iret` |
| `arch/i686/pic.c` | 8259 remapeado para 32–47, máscara por linha e EOI |
| `arch/i686/pit.c` | canal 0 do 8254 a 100 Hz; contador de ticks, segundos e frequência |
| `drivers/keyboard.c` | teclado PS/2 (scan code set 1), buffer circular, Shift e Backspace |
| `mm/pmm.c` | alocador de frames de 4 KiB por bitmap, 512 MiB no máximo |
| `mm/heap.c` | `kmalloc` por bump, pegando frames inteiros do alocador físico |
| `lib/console.c` | console único que escreve no ecrã VGA e, se existir, na porta série |
| `lib/string.c` | `memset`, `memcpy`, `strlen`, `strcmp`, `strncmp`, `strcpy` |
| `arch/i686/vga.c` | modo texto 80x25 em 0xB8000: scroll, cursor de hardware, `\n` `\r` `\t` |
| `arch/i686/splash.c` | ecrã de arranque: pinta a arte no buffer VGA e espera ~2,5 s com o 8254 (canal 2, sem interrupções) |
| `arch/i686/splash_art.c` | arte do arranque, gerada por `tools/gen-splash.py` a partir de `tools/splash-ascii.txt` |
| `arch/i686/serial.c` | UART 16550 em COM1 (115200 8N1), com deteção de hardware e escrita com limite de espera |
| `arch/i686/boot_info.c` | decodificação do Multiboot1: nome do bootloader e tamanhos de memória |
| `arch/i686/cpu.c` | CPUID: deteta se existe e lê o nome do fabricante (ex.: `GenuineIntel`) |
| `sched/`, `fs/` | vazios, mantidos com um `.gitkeep` até chegar o primeiro arquivo real |

## Regras

1. **`main.c` fica pequeno.** Ele inicializa subsistemas e imprime diagnósticos. Qualquer lógica que tenha nome próprio pertence a um subsistema.
2. **Código específico de arquitetura vai em `arch/i686/`.** Port I/O, MSRs, frames de interrupção, formatos de tabela de página e protocolos de boot não são portáveis; não os deixe vazar para `main.c` ou `lib/`.
3. **Interrupções fazem o mínimo.** Um handler conta um tick, enfileira um byte ou reconhece o PIC. Nunca imprime, bloqueia ou aloca.
4. **`lib/` tem que continuar freestanding.** Nenhuma chamada de biblioteca hospedeira, nenhuma alocação antes de o alocador existir.
5. **Um subsistema por mudança de diretório.** Um diretório novo aparece na mesma mudança que traz seu primeiro arquivo-fonte de verdade, e o `.gitkeep` sai nesse momento.
6. **O stub do bootloader não fica aqui.** O header Multiboot, a entrada de 32 bits, a GDT e o linker script vivem em `boot/`, porque rodam antes de existir C.
7. **32 bits é uma decisão, não um acidente.** O hardware alvo (Pentium M) não executa x86_64; não introduza código de 64 bits.
8. **Nenhuma saída pode bloquear o kernel.** O portátil não tem porta série: tudo o que fala com hardware tem de desistir depois de um número limitado de tentativas.

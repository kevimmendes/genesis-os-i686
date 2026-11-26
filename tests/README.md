# Testes

Duas formas de verificar o sistema, uma delas sem QEMU.

## 1. QEMU (`make test`)

Arranca `build/kernel.elf` com `-kernel` (o mesmo caminho Multiboot1 que o GRUB
usa), sem ecrã, e guarda a saída série em `build/serial.log`. Falha se faltar
alguma das linhas esperadas.

Requer `qemu-system-i386`. Não precisa de GRUB nem de ISO.

```sh
make test
```

## 2. Unicorn (`make emulate`)

Corre o kernel dentro do processo Python (Unicorn), carrega os segmentos ELF nos
endereços de carga, finge a entrega do Multiboot1 e executa o `_start` real.
No fim lê o buffer de texto VGA em 0xB8000 e procura as linhas esperadas.

```sh
pip install unicorn
make emulate
```

O `make emulate` corre o kernel duas vezes: uma sessão normal e o assistente de
instalação (`python3 tests/emulate_boot.py build/kernel.elf setup`), que responde
`1`, `2`, `10` e `s` — português, teclado PT, 10 GB, confirmar.

O harness vai além do arranque:

- guarda uma cópia do ecrã no momento em que a arte do arranque acaba de ser
  pintada, para conferir que o splash aparece;
- guarda outra cópia quando o primeiro `genesis>` aparece, antes de a saída da
  shell empurrar o relatório para fora do ecrã;
- serve a porta 0x64 e a porta 0x60 com uma fila de scan codes, por isso o
  teclado funciona sem 8042 e sem PIC;
- escreve `help`, `mem`, `calc 81 / 9`, `hexdump` e `halt` e confere as respostas;
- emula uma UART 16550 em COM1 (registo *scratch*, estado da linha e dados), de modo que o comando `ai` funciona sem cabo nem hardware.

O que isto verifica: caminho de entrada, GDT plana, pilha, limpeza de `.bss`,
splash, console, driver VGA, decodificação da informação de boot, alocador de
frames, heap, driver de teclado, shell e comando `halt`.

O que **não** verifica: firmware, GRUB, entrega real de IRQs (o Unicorn não
emula PIC nem PIT), comportamento em hardware real e temporização. Para isso,
`make test` e depois o ISO no portátil.

## Plano de testes

| # | Teste | Como | Estado |
|---|---|---|---|
| 1 | O kernel liga e entra em `_start` | `make test`, `make emulate` | passa |
| 2 | O protocolo é reconhecido (Multiboot1) | linha `protocol : multiboot1` | passa |
| 3 | O ecrã VGA mostra o relatório | `make emulate` (lê 0xB8000) | passa |
| 4 | A memória é reportada | linha `memory :` | passa |
| 5 | O CPU é identificado | linha `cpu vendor :` | passa |
| 6 | Sem UART o kernel não bloqueia | `console : ... (no serial UART detected)` | passa |
| 7 | O ecrã de arranque mostra a arte completa | `make emulate` (cópia do ecrã durante o boot) | passa |
| 8 | As interrupções ficam ativas | linha `interrupts : PIC at vectors 32-47` | passa (emulador) |
| 9 | O alocador de frames arranca | linha `frames : N free of M` | passa |
| 10 | O heap entrega memória | linha `heap : heap allocator online` | passa |
| 11 | O teclado alimenta a shell | `help` escrito pelo harness | passa |
| 12 | Os comandos respondem | `help`, `mem`, `halt` | passa |
| 16 | A calculadora inteira funciona | `calc 81 / 9` devolve `81 / 9 = 9` | passa |
| 17 | O hexdump lê memória | `hexdump` mostra o header Multiboot em `0x00100000` | passa |
| 18 | O protocolo do agente envia a pergunta | só com `make AGENT=1` | opcional |
| 19 | A resposta do agente é mostrada | só com `make AGENT=1` | opcional |
| 20 | O agente funciona no portátil com cabo | trabalho futuro do professor | **não testado** |
| 21 | O assistente mostra idioma/teclado/disco | `make emulate` modo setup | passa |
| 22 | O assistente valida e confirma | resumo + `nada foi escrito no disco` | passa |
| 23 | A instalação real no disco | driver ATA + FS + GRUB (M8) | **não existe ainda** |
| 13 | O ISO arranca no hardware real | `make iso` + CD-R no D600 | **não testado** |
| 14 | A saída série funciona num UART real | QEMU com `-serial stdio` | **não testado** |
| 15 | IRQs são entregues a sério (timer/teclado) | QEMU / portátil | **não testado aqui** |

Os testes 13 a 15 exigem material que não existe no ambiente de
desenvolvimento: uma máquina com `grub-mkrescue` e QEMU, e o próprio portátil.

## Verificar o ISO (numa máquina host)

```sh
make iso
qemu-system-i386 -cdrom genesis.iso -m 128M -serial stdio
```

Se o GRUB arrancar, o relatório aparecer e a shell responder ao teclado, o ISO
está bom para gravar em CD-R.

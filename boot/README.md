# Boot

Código que corre antes de existir C.

```text
boot/
├── boot.s       header Multiboot1, entrada de 32 bits, GDT plana, pilha, call kmain
├── linker.ld    liga em 1 MiB, exporta __bss_start/__bss_end, descarta .eh_frame
└── grub.cfg     menuentry do GRUB: `multiboot /boot/kernel.elf`
```

## Porquê Multiboot1 e não Multiboot2

O alvo é uma máquina de 32 bits. O Multiboot1 funciona com o GRUB
(`multiboot /boot/kernel.elf`) e também com o `-kernel` do QEMU, o que permite
testar sem construir um ISO. O Multiboot2 obrigaria a rejeitar o `-kernel` do
QEMU para imagens de 64 bits e não traz nada de que este kernel precise.

O header tem de estar nos primeiros 8 KiB do ficheiro e alinhado a 4 bytes; o
linker script coloca a secção `.multiboot` em primeiro lugar, em 0x100000.

## O que `_start` faz, por ordem

1. `cli` / `cld`.
2. Guarda o *magic* (EAX) em ESI, porque `rep stosb` destrói EAX.
3. Aponta ESP para a pilha do kernel no fim de `.bss` (16 KiB).
4. Limpa `.bss` entre `__bss_start` e `__bss_end` — nenhum carregador garante
   que venha limpo.
5. Carrega a GDT do kernel (`lgdt`) e recarrega CS com `ljmp $0x08`.
6. Recarrega os segmentos de dados a partir de **CX**, nunca de AX, e só então
   restaura EAX a partir de ESI.
7. `call kmain(magic, info)`.

## Regras

- Nada aqui pode depender da GDT, da pilha ou das interrupções do carregador.
- O código de 32 bits fica em `.text.boot` e é a primeira coisa executada.
- Se `kmain` retornar, a CPU pára em `cli; hlt` — nunca há `ret` para o vazio.

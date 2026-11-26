# Instalação

O ISO do Genesis tem duas entradas no menu do GRUB:

- **Project Genesis - live session**: arranca directo para a shell, sem perguntas.
- **Install Project Genesis**: passa `setup` na linha de comandos do kernel e
  mostra o assistente de instalação antes da shell.

## O assistente

Quatro ecrãs, todos conduzidos pelo teclado, em português, inglês ou espanhol:

1. **Idioma** — Português, English ou Español.
2. **Teclado** — US, PT ou ES. A escolha aplica-se imediatamente: as teclas de
   pontuação que diferem são remapeadas. Como o ecrã é ASCII, letras acentuadas
   aparecem como a letra base (`ç` → `c`, `ñ` → `n`).
3. **Espaço em disco** — um número entre 1 e 20 GB, validado.
4. **Resumo e confirmação** — idioma, teclado e espaço escolhidos, com `s/n`
   (`y/n` em inglês).

Se responder `n`, o assistente recomeça do primeiro ecrã. Se confirmar, as
preferências ficam guardadas para a sessão e o sistema continua para a shell.

## O que ainda não acontece (e porquê)

**Nada é escrito no disco.** O kernel desta versão não tem driver ATA, não lê
nem escreve tabelas de partições, não tem sistema de ficheiros e não instala o
GRUB no disco. Sem essas quatro peças, "instalar" seria fingir — e no portátil
real isso significa arriscar o disco. O assistente diz isto no ecrã final, na
língua escolhida, em vez de dar a instalação por concluída.

A ordem para tornar a instalação real está no [ROADMAP.md](ROADMAP.md):

1. driver ATA em modo PIO (M8);
2. leitura e escrita da tabela de partições;
3. um sistema de ficheiros simples onde caibam o kernel e o `grub.cfg`;
4. cópia do kernel e instalação do GRUB no disco.

## Construir o ISO

O ISO precisa de `grub-mkrescue` e `xorriso` numa máquina Linux (ou WSL):

```sh
make            # build/kernel.elf
make iso        # genesis.iso com as duas entradas do menu
```

Depois, no D600: gravar `genesis.iso` num CD-R como imagem, arrancar com F12 e
escolher a unidade óptica. Ver [HARDWARE.md](HARDWARE.md).

## Testar sem o portátil

```sh
pip install unicorn
make emulate
```

O `make emulate` corre o kernel duas vezes: uma sessão normal (comandos e agente)
e o assistente de instalação com as escolhas `1`, `2`, `10`, `s` — português,
teclado PT, 10 GB, confirmar. As duas têm de terminar em
`result: emulate test passed`.

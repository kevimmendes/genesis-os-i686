# Rede e SSH — o que falta, por etapas

Este documento existe para ser honesto sobre uma coisa: **o sistema ainda não
tem rede e não pode ter SSH por enquanto**. Um servidor SSH não é um comando que
se acrescenta à shell; é a última camada de uma pilha inteira, e cada camada
abaixo tem de existir e funcionar antes.

## Porque é que SSH é grande

Um servidor SSH precisa, por esta ordem:

1. **Um driver para a placa de rede.** No Dell Latitude D600 é uma Broadcom
   570x (BCM5705M, PCI `14E4:165D`), com anéis de descritores DMA para
   transmissão e receção.
2. **Uma pilha de rede:** Ethernet, ARP, IPv4, ICMP e depois TCP (com janelas,
   retransmissões e temporizadores). DHCP é opcional se o IP for fixo.
3. **Um gerador de números aleatórios** com entropia real, e **criptografia**:
   SHA-256, ChaCha20-Poly1305 (ou AES-GCM) e Curve25519 para a troca de chaves.
4. **O protocolo SSH:** troca de versões, negociação de algoritmos, derivação de
   chaves, autenticação (chave pública ou password), canais, sessão e terminal.
5. **Um sistema de ficheiros** para as chaves do servidor e para os utilitários
   que se querem executar do outro lado.

Não há atalho: cada uma destas peças precisa de testes próprios, e a maior parte
só se pode validar em hardware real, porque o ambiente de desenvolvimento não
tem QEMU.

## Etapas propostas

| Etapa | Entrega | Verificável sem hardware? |
|---|---|---|
| N0 | Enumerar o bus PCI (portas 0xCF8/0xCFC) e um comando `lspci` na shell | Parcial: o emulador devolve "sem dispositivos", o QEMU mostra a placa |
| N1 | Driver da BCM5705M: reset, anéis DMA, envio e receção de um frame | Não: precisa da máquina ou de QEMU com modelo de placa |
| N2 | Ethernet + ARP + IPv4 + ICMP (responder a `ping`) | QEMU com rede em modo user |
| N3 | UDP e um cliente DHCP simples; IP fixo como alternativa | QEMU |
| N4 | TCP: handshake, envio, receção, retransmissão, fecho | QEMU + host |
| N5 | RNG (entropia do teclado e do timer) + SHA-256 + ChaCha20-Poly1305 + Curve25519 | Sim: vetores de teste conhecidos |
| N6 | Servidor SSH: versão, KEX, autenticação, canal `session`, `pty-req` | QEMU + cliente `ssh` do host |
| N7 | Utilitários do lado do sistema: `ls`, `cat`, `mkdir` e um sistema de ficheiros simples | Parcial |

A etapa N5 é a única grande parte que pode ser escrita e testada por inteiro
aqui, porque criptografia tem vetores de teste oficiais e não precisa de placa
de rede. As etapas N1–N4 e N6 precisam de uma máquina com QEMU ou do próprio
portátil.

## O que já existe e ajuda

- Temporizador a 100 Hz: base para retransmissões de TCP e para a entropia.
- Interrupções com PIC: a placa de rede usa IRQ (tipicamente IRQ11) e o driver
  encaixa no mesmo `irq_install()` que o teclado.
- Alocador de frames: buffers DMA precisam de memória física contígua; com
  paginação desligada, um frame é diretamente utilizável pela placa.
- Sem paginação, endereços virtuais são físicos — o que simplifica DMA, mas
  obriga a reservar buffers abaixo do limite que a placa aceita.

## Ordem recomendada

1. **N0** — enumerar o PCI e ver a placa. É pequeno, encaixa na shell e é o
   primeiro contacto real com o hardware de rede.
2. **N5** — criptografia com vetores de teste. É trabalho de biblioteca, seguro
   de fazer sem hardware.
3. **N1–N4** — só com QEMU ou o portátil à frente.
4. **N6–N7** — SSH e utilitários, quando a rede já transportar bytes de forma
   fiável.

Enquanto isso, a shell continua a ser o interface do sistema, e as ferramentas
básicas (`calc`, `hexdump`, `mem`, `info`, `uptime`) já estão disponíveis.

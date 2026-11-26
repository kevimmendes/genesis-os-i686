# Hardware de referência - Dell Latitude D600

O Project Genesis é escrito para correr nesta máquina. Tudo o que o kernel assume
sobre o computador está resumido aqui.

## Especificações

| Componente | Detalhe |
|---|---|
| Modelo | Dell Latitude D600 Series |
| Service Tag | 49CPX0J |
| Processador | Intel Pentium M 1.40 GHz - **32 bits apenas** |
| Cache L2 | 1 MB |
| Memória | 512 MB @ 266 MHz |
| Gráfica | ATI Radeon 9000, 32 MB |
| Ecrã | 14" XGA, 1024x768 |
| Disco | 20 GB IDE/PATA |
| Unidade ótica | DVD-ROM (Modular Bay) |
| Rede | Broadcom 570x Gigabit (BCM5705M), PCI 14E4:165D |
| Áudio | Sigmatel 9750 |
| Modem | Conexant D480 |
| Porta série | 9 pinos RS-232, 16550, COM1 (o cabo do agente de IA) |

## O que isto significa para o kernel

1. **32 bits, sem exceção.** O Pentium M não executa código x86_64. Por isso o
   kernel é i686 e fica em modo protegido de 32 bits, com paginação desligada.
2. **O ecrã é a saída principal, e a porta série é o canal de dados.** O driver
   VGA de texto (80x25 em 0xB8000) mostra tudo; o 16550 em COM1 serve de segunda
   consola e é por onde o portátil fala com o PC que corre o modelo de IA
   (ver [AGENT.md](AGENT.md)).
3. **512 MB de RAM são suficientes e limitam o estilo.** Nada de alocações
   grandes, nada de estruturas de dados gordas. O kernel atual usa menos de 64 KB.
4. **O disco é IDE/PATA.** Quando chegar o Milestone 8, o driver ATA tem de ser o
   modo PIO, não AHCI.
5. **A rede é PCI.** A placa Broadcom 570x aparece no bus PCI como
   `14E4:165D`; o driver exige enumeração PCI, anéis de descritores DMA e IRQ.
   Ver [NETWORK.md](NETWORK.md).
6. **A porta série é 16550 e está na BIOS.** Se o comando `ai` disser que não há
   ligação, confirmar em F2 que a Serial Port está ativa e em COM1.

## Avisos do POST (e como resolvê-los)

- *"Invalid configuration information - please run SETUP program"* e
  *"Time-of-day not set"*: a pilha CMOS (CR2032) descarregou. Entrar na BIOS com
  **F2**, acertar data e hora, gravar e sair. Se a hora voltar a perder-se,
  substituir a pilha.
- *"The AC Power Adapter type cannot be determined"*: o portátil não reconhece o
  carregador. Isto limita a CPU a 600 MHz. Experimentar outro carregador Dell
  original; **F3** desativa os avisos futuros e **F1** continua o arranque.

## Como arrancar o kernel na máquina

1. Num computador Linux (ou WSL), construir a imagem:

   ```sh
   make
   make iso        # produz genesis.iso, precisa de grub-mkrescue e xorriso
   ```

2. Gravar `genesis.iso` num CD-R **como imagem** (não copiar o ficheiro para o
   disco). Em Linux: `xorriso -as cdrecord -v dev=/dev/sr0 -eject genesis.iso`.
   Em Windows: "Gravar imagem de disco" do Explorador, com um CD-R vazio.

3. No D600, ligar, carregar em **F12** para o menu de arranque e escolher a
   unidade de CD/DVD. Se o menu não aparecer, entrar na BIOS com **F2** e pôr o
   CD-ROM primeiro na ordem de arranque.

4. O ecrã deve mostrar o relatório do kernel e o prompt `genesis>`. Escrever
   `help` e Enter para ver os comandos disponíveis; `halt` desliga o sistema de
   forma controlada.

## Notas de segurança

- Arrancar a partir do CD não altera nada no disco: o kernel não tem driver de
  disco nem escrita. Se algo correr mal, desligar e arrancar o sistema normal.
- O kernel tem teclado, interrupções, gestão de memória e shell, mas **não**
  tem sistema de ficheiros, disco nem rede: não guarda nem altera nada. É o
  ponto de partida para os Milestones 7-12.
- Testar primeiro em QEMU (`make run`). O ISO é o último passo.

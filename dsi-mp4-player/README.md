# DSi MP4 Player — prototipo iniziale

Homebrew per Nintendo DSi in C con devkitPro/libnds.

## Stato
Questa prima versione inizializza la SD e mostra i file `.mp4` trovati in `/video`. È uno scheletro di interfaccia: **non decodifica né converte ancora video**.

MP4 è un contenitore e può contenere codec diversi. Il DSi ha CPU e RAM limitate: una conversione generica di MP4 moderni (H.264/AAC) direttamente sulla console non è realistica come prima implementazione. La strada affidabile è preparare su PC un flusso a bassa risoluzione e integrare un decoder leggero sul DSi.

## Convertitore di test (PC)
È stato aggiunto `tools/convert_video.py`, che richiede FFmpeg sul computer e prepara un file MPEG-1 256x144 a 15 fps con audio MP2 mono:

```sh
python3 tools/convert_video.py filmato.mp4
```

Questo è soltanto un preset per testare un futuro decoder: **la ROM attuale non riproduce ancora i file convertiti**. Dettagli e prossimi traguardi in [docs/VIDEO_PIPELINE.md](docs/VIDEO_PIPELINE.md).

## Compilazione
Con devkitPro installato e il gruppo `nds-dev`:

```sh
cd dsi-mp4-player
make
```

Output: `dsi-mp4-player.nds`.

## Uso previsto
Crea `/video` sulla scheda SD e copia lì i video. In questa build vengono soltanto elencati.

## Comandi
- Su/Giù: seleziona
- A: dettagli del file
- B: aggiorna elenco / torna indietro

## Prossimi passi
1. Verificare browser SD e compilazione su hardware/toolchain.
2. Integrare un decoder e misurare prestazioni/audio su DSi.
3. Collegare il player al browser.
4. Valutare il flusso automatico di preparazione video (PC o formato ristretto on-device).

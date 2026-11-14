# DSi Video Player — prototipo touch

Homebrew per Nintendo DSi in C con devkitPro/libnds.

## Interfaccia
- Schermata principale con i comandi touch **PLAY** e **PAUSA**.
- Il tasto **START** apre/chiude il menu per scegliere un file dalla SD.
- I video vengono cercati nella cartella `/video`.
- Nel menu si tocca il nome del file per selezionarlo; **R** aggiorna l'elenco.

## Stato reale del progetto
Questa build implementa l'interfaccia touch e la selezione dei file MP4. **Non contiene ancora un decoder né riproduce effettivamente il video**: PLAY/PAUSA aggiornano per ora lo stato mostrato nell'interfaccia. Non considerare quindi questa ROM un lettore video completo.

MP4 è un contenitore e può contenere codec diversi. Il DSi ha CPU e RAM limitate: la strada pratica è convertire il video su PC in un flusso a bassa risoluzione e integrare un decoder leggero sulla console.

## Preparazione video di test (PC)
Lo script `tools/convert_video.py` richiede FFmpeg sul computer e genera un flusso MPEG-1 256x144 a 15 fps con audio MP2 mono:

```sh
python3 tools/convert_video.py filmato.mp4
```

Il file convertito è soltanto un formato candidato per il futuro decoder: la ROM attuale non lo riproduce ancora. Vedi [docs/VIDEO_PIPELINE.md](docs/VIDEO_PIPELINE.md).

## Compilazione
Con devkitPro e il gruppo `nds-dev` installati:

```sh
cd dsi-mp4-player
make
```

Output previsto: `dsi-mp4-player.nds`.

## Installazione SD
Crea la cartella `/video` nella scheda SD e copia lì i file MP4. Avvia la homebrew: START apre il selettore, tocca un nome per tornare al lettore.

## Prossimi passi
1. Verificare la compilazione e l'interfaccia su DSi reale/emulatore.
2. Integrare un decoder adatto al DSi.
3. Collegare PLAY/PAUSA al motore di riproduzione.
4. Testare audio, sincronizzazione e prestazioni.

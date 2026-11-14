# DSi Video Player — prototipo touch

Homebrew per Nintendo DSi in C con devkitPro/libnds.

## Interfaccia personalizzata (prototipo)
- Schermata principale con i comandi touch **PLAY** e **PAUSA**.
- Il tasto **START** apre/chiude il menu per scegliere un file dalla SD.
- Il browser cerca attualmente file `.mp4` nella cartella `/video`.
- Nel menu si tocca il nome del file per selezionarlo; **R** aggiorna l'elenco.

**Importante:** questa interfaccia non contiene ancora un decoder video. PLAY/PAUSA aggiornano solo lo stato visualizzato: non avviano o fermano la riproduzione. La ROM del prototipo non è ancora un lettore video completo.

## Percorso concreto per riprodurre video su DSi
Per la riproduzione reale, il progetto usa come riferimento **Rocket Video Player**, un lettore DS/DSi esistente con controlli touch Play/Pause e browser file. Richiede video convertiti nel formato proprietario `.rvid` tramite Vid2RVID; non legge MP4 direttamente.

- [Rocket Video Player (codice e istruzioni)](https://github.com/RocketRobz/RocketVideoPlayer)
- [Vid2RVID (convertitore)](https://github.com/RocketRobz/Vid2RVID)
- [Release Vid2RVID](https://github.com/RocketRobz/Vid2RVID/releases)
- [Dettagli del backend e procedura](docs/PLAYER_BACKEND.md)

La conversione si fa su PC. Copia poi il file `.rvid` sulla SD e aprilo con Rocket Video Player. Il collegamento del motore Rocket Video alla nostra interfaccia START personalizzata è ancora un lavoro da completare.

## Vecchio convertitore MPEG-1
`tools/convert_video.py` genera un file MPEG-1 di test per esperimenti futuri. Non è il formato del player Rocket Video e il prototipo attuale non riproduce nemmeno quel file.

## Compilazione del prototipo UI
Con devkitPro e il gruppo `nds-dev` installati:

```sh
cd dsi-mp4-player
make
```

Output previsto: `dsi-mp4-player.nds`. La compilazione e il funzionamento su hardware reale devono ancora essere verificati.

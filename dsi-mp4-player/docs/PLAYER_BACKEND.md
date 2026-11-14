# Backend video DSi: Rocket Video Player

## Scelta tecnica

Non implementiamo un decoder MP4 moderno da zero nel piccolo prototipo libnds: MP4 è un contenitore, non un codec, e la CPU/RAM del DSi sono limitate. Per una riproduzione reale scegliamo come riferimento **Rocket Video Player** di RocketRobz, progetto open source MIT già funzionante su DS/DSi.

- Player: https://github.com/RocketRobz/RocketVideoPlayer
- Convertitore ufficiale: https://github.com/RocketRobz/Vid2RVID
- Release Vid2RVID: https://github.com/RocketRobz/Vid2RVID/releases

Rocket Video Player riproduce file `.rvid`, non MP4 direttamente. Il README upstream documenta il controllo Play/Pause tramite pulsante touch e il browser integrato per scegliere il file. Il formato va generato con Vid2RVID.

## Come usare subito un MP4 sul DSi

1. Scaricare Rocket Video Player dalla pagina del progetto/release e avviarlo tramite TWiLight Menu++ o un altro loader homebrew compatibile.
2. Scaricare Vid2RVID dalla pagina Releases.
3. Seguire la guida di conversione collegata dalla release di Vid2RVID per convertire il video sorgente in `.rvid`.
4. Copiare il file `.rvid` generato nella scheda SD.
5. Aprire Rocket Video Player, selezionare il file dal browser, quindi usare il pulsante touch Play/Pause.

Il convertitore è un programma per PC; non converte il video direttamente sul DSi. I file `.rvid` possono essere molto grandi. La compatibilità e la fluidità dipendono da risoluzione, frame rate, audio e velocità della scheda SD.

## Stato del nostro launcher personalizzato

La directory `dsi-mp4-player/source` contiene ancora un prototipo UI touch: START apre il selettore, toccando un nome si torna al lettore, e PLAY/PAUSA cambiano l'indicatore sullo schermo. **Questa UI non contiene il decoder Rocket Video e non riproduce ancora il video.** Non presentare il prototipo come player completo finché il motore non è effettivamente integrato e testato.

L'integrazione successiva dovrà basarsi sul codice Rocket Video Player (licenza MIT), rispettando i crediti e la licenza upstream, oppure distribuire il player upstream come componente separato. Il progetto upstream usa BlocksDS; non è intercambiabile automaticamente con il Makefile libnds attuale.

## Perché non usare il vecchio preset MPEG-1

`tools/convert_video.py` crea un MPEG-1 di test, ma non esiste un decoder MPEG-1 collegato al nostro prototipo. Quel file non è riproducibile dalla ROM attuale. Per un percorso concreto di playback, usare il formato `.rvid` e gli strumenti ufficiali indicati sopra.

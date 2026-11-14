# DSi MP4 Player — prototipo iniziale

Homebrew per Nintendo DSi in C con devkitPro/libnds.

## Stato
Questa prima versione inizializza la SD e mostra i file `.mp4` trovati in `/video`. È uno scheletro di interfaccia: **non decodifica né converte ancora video**.

MP4 è un contenitore e può contenere codec diversi. Il DSi ha CPU e RAM limitate: una conversione generica di MP4 moderni (H.264/AAC) direttamente sulla console non è realistica come prima implementazione. La strada affidabile per supportare i comuni MP4 sarà un convertitore su PC e un formato/decoder leggero sul DSi; la conversione interamente on-device richiederebbe un insieme molto ristretto di codec e parametri.

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
1. Verificare browser SD e compatibilità nomi file su hardware.
2. Scegliere un formato video decodificabile dal DSi.
3. Implementare decoder/riproduzione.
4. Aggiungere conversione compatibile (probabilmente su PC per gli MP4 comuni).

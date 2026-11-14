# DSi Craft

Gioco fan-made creativo per Nintendo DSi, realizzato in C con devkitPro/devkitARM e libnds.

## Stato attuale

Prima versione giocabile in stile sandbox 2D testuale:
- mondo a blocchi 16 x 16 con 8 livelli di altezza;
- fino a 4 mondi selezionabili dal menu;
- creazione di mondi vuoti con terreno iniziale;
- blocchi: erba, terra, pietra, legno e sabbia;
- cursore di costruzione, senza personaggio visibile o visuale in terza persona;
- rottura e piazzamento dei blocchi;
- selezione del blocco tramite touch screen.

I mondi sono attualmente conservati in memoria durante la sessione: non sono ancora salvati su SD e si perdono quando si spegne o riavvia la console.

## Modalita' di gioco

DSi Craft e' esclusivamente creativo: niente mob, meteo, pioggia o sopravvivenza.

## Comandi

### Menu mondi
- Frecce: seleziona uno dei quattro slot.
- A: entra nel mondo selezionato, se gia' creato.
- X: crea un mondo nel primo slot libero (massimo 4).

### Dentro un mondo
- Frecce: muovi il cursore sul terreno.
- X / Y: abbassa / alza il livello di lavoro.
- L: rompi il blocco nella posizione del cursore.
- R: piazza il blocco selezionato.
- Touch: seleziona uno dei cinque blocchi.
- B: torna al menu dei mondi.

Il simbolo + sulla mappa indica il cursore di costruzione, non un personaggio.

## Compilazione

Serve devkitPro con devkitARM e il pacchetto nds-dev (libnds) installati. Eseguire `make` dalla cartella del progetto. La GitHub Action installa nds-dev prima di compilare.

## Prossimi passi

Grafica a blocchi, salvataggio dei quattro mondi su scheda SD e controlli piu' rifiniti.

# DSi Craft

Gioco fan-made creativo per Nintendo DSi, realizzato in C con devkitPro/devkitARM e libnds.

## Stato attuale

Prima versione giocabile in stile sandbox 2D testuale:
- mondo a blocchi 16 x 16 con 8 livelli di altezza;
- fino a 4 mondi selezionabili dal menu;
- creazione di mondi con terreno iniziale;
- blocchi: erba, terra, pietra, legno e sabbia;
- cursore di costruzione, senza personaggio visibile o visuale in terza persona;
- rottura e piazzamento dei blocchi;
- selezione del blocco tramite touch screen;
- ambientazione sempre di giorno: non esiste un ciclo giorno/notte.

## Salvataggi

I quattro mondi vengono caricati all'avvio e salvati automaticamente sulla scheda SD del Nintendo DSi nel file `fat:/dsi-craft.sav`. Il salvataggio viene aggiornato quando crei un mondo, rompi un blocco o ne piazzi uno.

La schermata mostra lo stato del salvataggio. Se la scheda SD non è disponibile o la scrittura fallisce, il gioco lo segnala: in quel caso le modifiche restano solo in memoria e non bisogna considerarle salvate. Non rimuovere la scheda SD mentre il gioco sta salvando.

## Modalita' di gioco

DSi Craft e' esclusivamente creativo: sempre giorno, niente ciclo notte, mob, meteo, pioggia o sopravvivenza.

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
- SELECT: passa al blocco successivo.
- B: torna al menu dei mondi.

Il simbolo + sulla mappa indica il cursore di costruzione, non un personaggio.

## Compilazione

Serve devkitPro con devkitARM, libnds e libfat installati. Eseguire `make` dalla cartella del progetto. La GitHub Action installa nds-dev prima di compilare.

## Prossimi passi

Grafica a blocchi e controlli piu' rifiniti.

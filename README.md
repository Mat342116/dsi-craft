# DSi Craft 3D

Gioco sandbox creativo fan-made per Nintendo DS/DSi, scritto in C con devkitPro, devkitARM e libnds.

## Versione in sviluppo

La vecchia schermata sandbox 2D testuale è stata sostituita da un prototipo 3D in prima persona, renderizzato in software sullo schermo superiore:
- mondo a blocchi 16 x 16 con 8 livelli di altezza;
- quattro mondi selezionabili e salvati su scheda SD;
- terreno iniziale con blocchi di erba, terra, pietra e piccole piattaforme;
- visuale in prima persona con rotazione orizzontale e inclinazione;
- movimento relativo alla direzione della visuale;
- collisioni orizzontali con i blocchi più alti;
- rottura e piazzamento dei blocchi;
- selezione del blocco tramite touch screen.

Il renderer è una prima implementazione software leggera, pensata per l'hardware Nintendo DS. Non è ancora un motore 3D completo con texture dettagliate o illuminazione avanzata.

## Avvio

All'accensione il gioco entra automaticamente nel primo mondo salvato. Se non esiste ancora un mondo, ne crea uno nuovo nel primo slot. Premi START per aprire il menu dei mondi.

## Comandi

### Menu dei mondi
- Frecce: seleziona uno dei quattro mondi.
- A: entra nel mondo selezionato.
- X: crea un mondo nel primo slot libero.
- START: torna al menu.

### Dentro un mondo 3D
- Freccia SU / GIÙ: cammina avanti / indietro.
- Freccia SINISTRA / DESTRA: spostamento laterale.
- Y / A: ruota la visuale a sinistra / destra (secondo pad simulato).
- X / B: guarda in alto / in basso.
- L: rompe il blocco davanti al giocatore.
- R: piazza il blocco selezionato davanti al giocatore.
- Touch: seleziona uno dei cinque materiali.
- START: torna al menu principale.
- SELECT: apre la selezione dei mondi.

I quattro pulsanti frontali simulano il secondo analogico: non muovono il personaggio.

## Salvataggi

I mondi vengono caricati all'avvio e salvati automaticamente sulla scheda SD nel file `fat:/dsi-craft.sav`. Se la scheda SD non è disponibile, il gioco continua in memoria ma le modifiche non sono persistenti.

## Compilazione

Serve devkitPro con devkitARM, libnds e libfat installati. Eseguire `make` dalla cartella del progetto. La GitHub Action installa l'ambiente Nintendo DS e prova a compilare il file NDS.

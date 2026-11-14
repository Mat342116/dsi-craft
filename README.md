# DSi Craft

Gioco creativo a blocchi per Nintendo DSi, scritto in C con devkitPro/devkitARM e libnds.

## Versione 3D

- Schermo superiore: mondo tridimensionale a blocchi, con prospettiva e selettore di costruzione.
- Schermo inferiore: tavolozza touch con i blocchi disponibili.
- Toccando un blocco nella tavolozza lo si prende direttamente in mano; il blocco selezionato viene mostrato anche nella visuale di gioco.
- Blocchi semplici: erba, terra, pietra, legno e sabbia.
- Porte e bauli: si possono piazzare e aprire/chiudere con A.
- TNT, botole, fuoco e blocchi troppo complessi non sono inclusi.
- Modalita' creativa sempre di giorno, senza mob, meteo o sopravvivenza.

## Mondi e salvataggi

Sono disponibili fino a quattro mondi, con terreno iniziale 16 x 16 e otto livelli di altezza. I mondi vengono salvati automaticamente sulla scheda SD del Nintendo DSi nel file fat:/dsi-craft.sav.

Il formato di salvataggio nuovo mantiene la compatibilita' con i mondi creati dalla versione precedente: i salvataggi versione 1 con i cinque blocchi base vengono caricati senza cancellare i mondi. Le nuove modifiche vengono salvate nel formato versione 2.

Se la scheda SD non e' disponibile o la scrittura fallisce, il gioco lo segnala. In quel caso le modifiche restano solo in memoria e non bisogna considerarle salvate. Non rimuovere la scheda SD mentre il gioco sta salvando.

## Comandi

### Menu mondi
- Frecce: seleziona uno dei quattro slot.
- A: entra nel mondo selezionato.
- X: crea un mondo nel primo slot libero.

### Nel mondo 3D
- Schermo inferiore / touch: scegli il blocco da tenere in mano.
- Frecce: sposta il puntatore sul terreno.
- X / Y: abbassa o alza il livello di costruzione.
- L: rompi il blocco puntato.
- R: posa il blocco che hai in mano.
- A: apri o chiudi una porta o un baule puntato.
- B: torna al menu dei mondi.

Non c'e' hotbar: il blocco si sceglie direttamente dalla tavolozza sullo schermo inferiore.

## Compilazione

Serve devkitPro con devkitARM, libnds e libfat installati. Eseguire make dalla cartella del progetto. La GitHub Action installa nds-dev prima di compilare.

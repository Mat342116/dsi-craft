# DSi Craft

Gioco fan-made creativo per Nintendo DSi, realizzato in C con **devkitPro/libnds**.

## Stato attuale

Questa e' una prima versione giocabile in stile sandbox 2D testuale:
- mondo a blocchi 16 x 16 con livelli di altezza;
- blocchi: erba, terra, pietra, legno e sabbia;
- movimento del cursore giocatore e regolazione dell'altezza;
- rottura e piazzamento dei blocchi;
- selezione del blocco da usare tramite touch screen.

## Modalita' di gioco

DSi Craft e' pensato esclusivamente per la creativita':
- nessun mob;
- nessuna pioggia o evento atmosferico;
- nessun ciclo meteo o sistema di sopravvivenza.

## Comandi

| Input | Azione |
| --- | --- |
| Frecce | Muovi il giocatore sul mondo |
| X / Y | Abbassa / alza il livello di lavoro |
| L | Rompi il blocco nella posizione corrente |
| R | Piazza il blocco selezionato |
| Touch | Seleziona uno dei cinque blocchi |

## Compilazione

Serve un ambiente devkitPro con libnds installato. Dalla cartella del progetto:

```sh
make
```

Il Makefile usa `DEVKITPRO` e le regole di libnds. La ROM Nintendo DS (.nds) viene generata dal processo di build se l'ambiente e' configurato correttamente.

## Prossimi passi

La base attuale e' volutamente semplice. Le prossime tappe sono una vera visualizzazione grafica dei blocchi, una telecamera/visuale di gioco, un sistema di salvataggio del mondo e controlli piu' rifiniti.

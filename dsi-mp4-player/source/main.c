#include <nds.h>
#include <fat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define VIDEO_DIR "/video"
#define MAX_FILES 64
#define NAME_LEN 256
#define ROWS 10

static char files[MAX_FILES][NAME_LEN];
static int count = 0, selected = 0, menu_open = 0, paused = 1;
static int touch_was_down = 0;

static int is_mp4(const char *name) {
    const char *dot = strrchr(name, '.');
    return dot && strcasecmp(dot, ".mp4") == 0;
}

static void scan_videos(void) {
    DIR *dir = opendir(VIDEO_DIR);
    struct dirent *entry;
    count = 0;
    if (!dir) return;
    while ((entry = readdir(dir)) != NULL && count < MAX_FILES) {
        if (entry->d_type == DT_DIR || !is_mp4(entry->d_name)) continue;
        strncpy(files[count], entry->d_name, NAME_LEN - 1);
        files[count][NAME_LEN - 1] = '\0';
        count++;
    }
    closedir(dir);
    if (selected >= count) selected = count ? count - 1 : 0;
}

static void draw_player(void) {
    consoleClear();
    iprintf("        DSi VIDEO PLAYER\n\n");
    iprintf("  %s\n\n", count ? files[selected] : "Nessun video selezionato");
    iprintf("  Stato: %s\n\n", paused ? "PAUSA" : "PLAY");
    iprintf("  +----------------------+\n");
    iprintf("  |                      |\n");
    iprintf("  |   AREA VIDEO         |\n");
    iprintf("  |                      |\n");
    iprintf("  +----------------------+\n\n");
    iprintf("   [ PLAY ]       [ PAUSA ]\n\n");
    iprintf(" Tocca PLAY/PAUSA. START = FILE");
    iprintf("\n\n Nota: decoder video da integrare.");
}

static void draw_menu(void) {
    consoleClear();
    iprintf("         SCEGLI VIDEO\n");
    iprintf(" Cartella SD: %s\n\n", VIDEO_DIR);
    if (!count) {
        iprintf(" Nessun MP4 trovato.\n");
        iprintf(" Crea /video sulla SD e copia\n");
        iprintf(" i tuoi file .mp4.\n");
    } else {
        int first = selected > ROWS - 1 ? selected - (ROWS - 1) : 0;
        for (int i = first; i < count && i < first + ROWS; ++i)
            iprintf(" %c %s\n", i == selected ? '>' : ' ', files[i]);
    }
    iprintf("\n Tocca un nome per selezionarlo.");
    iprintf("\n START = torna al lettore");
    iprintf("\n R = aggiorna elenco");
}

int main(void) {
    touchPosition touch;
    consoleDemoInit();
    iprintf("DSi VIDEO PLAYER\nAvvio scheda SD...\n");
    if (!fatInitDefault()) {
        iprintf("\nErrore: SD non accessibile.\n");
        while (1) swiWaitForVBlank();
    }

    scan_videos();
    draw_player();

    while (1) {
        scanKeys();
        int keys = keysDown();
        int down = keysHeld() & KEY_TOUCH;

        if (keys & KEY_START) {
            menu_open = !menu_open;
            if (menu_open) { scan_videos(); draw_menu(); }
            else draw_player();
        }

        if (keys & KEY_R) {
            scan_videos();
            if (menu_open) draw_menu();
        }

        if (down && !touch_was_down) {
            touchRead(&touch);
            if (menu_open) {
                int first = selected > ROWS - 1 ? selected - (ROWS - 1) : 0;
                if (touch.py >= 24 && touch.py < 24 + ROWS * 12) {
                    int row = (touch.py - 24) / 12;
                    int choice = first + row;
                    if (choice < count) {
                        selected = choice;
                        menu_open = 0;
                        paused = 1;
                        draw_player();
                    }
                }
            } else if (touch.py >= 145 && touch.py <= 190) {
                if (touch.px >= 12 && touch.px <= 120) {
                    paused = 0;
                    draw_player();
                } else if (touch.px >= 132 && touch.px <= 245) {
                    paused = 1;
                    draw_player();
                }
            }
        }
        touch_was_down = down ? 1 : 0;
        swiWaitForVBlank();
    }
    return 0;
}

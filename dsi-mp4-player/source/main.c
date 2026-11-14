#include <nds.h>
#include <fat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>

#define VIDEO_DIR "/video"
#define MAX_FILES 64
#define NAME_LEN 256
#define VISIBLE_ROWS 12

static char files[MAX_FILES][NAME_LEN];
static int count = 0, selected = 0, page = 0, detail = 0;

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
}

static void draw_list(void) {
    consoleClear();
    iprintf("DSi MP4 PLAYER - prototipo\n\n");
    iprintf("Cartella: %s\nVideo: %d\n\n", VIDEO_DIR, count);
    if (!count) {
        iprintf("Nessun MP4 trovato.\n");
        iprintf("Crea /video sulla SD e copia\n");
        iprintf("qui i file .mp4.\n");
    } else {
        if (selected < page) page = selected;
        if (selected >= page + VISIBLE_ROWS) page = selected - VISIBLE_ROWS + 1;
        for (int i = page; i < count && i < page + VISIBLE_ROWS; i++)
            iprintf("%c %s\n", i == selected ? '>' : ' ', files[i]);
    }
    iprintf("\nSu/Giu: scegli  A: dettagli\nB: aggiorna elenco");
}

static void draw_detail(void) {
    consoleClear();
    iprintf("DSi MP4 PLAYER\n\nFile:\n%s\n\n", files[selected]);
    iprintf("MP4 individuato.\n\n");
    iprintf("Questa build non include ancora\n");
    iprintf("decoder, conversione o playback.\n");
    iprintf("\nB: torna alla lista");
}

int main(void) {
    consoleDemoInit();
    iprintf("DSi MP4 PLAYER\nInizializzazione SD...\n");
    if (!fatInitDefault()) {
        iprintf("\nErrore: SD non accessibile.\n");
        while (1) swiWaitForVBlank();
    }

    scan_videos();
    draw_list();
    while (1) {
        scanKeys();
        int keys = keysDown();
        if (!detail) {
            if ((keys & KEY_UP) && selected > 0) { selected--; draw_list(); }
            if ((keys & KEY_DOWN) && selected < count - 1) { selected++; draw_list(); }
            if ((keys & KEY_A) && count > 0) { detail = 1; draw_detail(); }
            if (keys & KEY_B) {
                scan_videos();
                if (selected >= count) selected = count ? count - 1 : 0;
                page = 0;
                draw_list();
            }
        } else if (keys & KEY_B) {
            detail = 0;
            draw_list();
        }
        swiWaitForVBlank();
    }
    return 0;
}

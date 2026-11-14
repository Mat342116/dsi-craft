#include <nds.h>
#include <fat.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>

#define VIDEO_DIR "fat:/videos"
#define MAX_FILES 8
#define NAME_LEN 96

static char files[MAX_FILES][NAME_LEN];
static int fileCount = 0;
static int selected = 0;
static bool playing = false;
static bool paused = false;
static PrintConsole topScreen, bottomScreen;

static bool has_fv_extension(const char *name) {
    size_t n = strlen(name);
    return n >= 3 && strcasecmp(name + n - 3, ".fv") == 0;
}

static void scan_videos(void) {
    DIR *dir = opendir(VIDEO_DIR);
    struct dirent *entry;
    fileCount = 0;
    if (!dir) return;
    while ((entry = readdir(dir)) != NULL && fileCount < MAX_FILES) {
        if (entry->d_name[0] == '.') continue;
        if (!has_fv_extension(entry->d_name)) continue;
        strncpy(files[fileCount], entry->d_name, NAME_LEN - 1);
        files[fileCount][NAME_LEN - 1] = '\0';
        fileCount++;
    }
    closedir(dir);
}

static void draw_ui(void) {
    consoleSelect(&topScreen);
    consoleClear();
    iprintf("\x1b[2;4HDSi VIDEO PLAYER");
    iprintf("\x1b[5;1HVideo output: upper screen");
    iprintf("\x1b[7;1HSelected file:");
    if (fileCount && selected < fileCount)
        iprintf("\x1b[9;1H%s", files[selected]);
    else
        iprintf("\x1b[9;1HNo .fv files found");
    iprintf("\x1b[13;1HPlayback decoder: not connected yet");

    consoleSelect(&bottomScreen);
    consoleClear();
    iprintf("VIDEO LIBRARY  (SD:/videos)\n\n");
    if (!fileCount) iprintf("No .fv videos found.\n");
    for (int i = 0; i < fileCount; i++)
        iprintf("%c %d. %s\n", i == selected ? '>' : ' ', i + 1, files[i]);
    iprintf("\n\n[ PLAY ]       [ PAUSE ]\n");
    iprintf("\n[ STOP ]       [ RESCAN ]\n");
    iprintf("\nTouch a file, then PLAY.\n");
    iprintf("START: return to file list\n");
    iprintf("\nState: %s%s\n", playing ? "PLAYING" : "STOPPED",
            paused ? " (PAUSED)" : "");
}

int main(void) {
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    consoleInit(&topScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0,
                false, true);
    consoleInit(&bottomScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0,
                true, true);

    if (!fatInitDefault()) {
        consoleSelect(&bottomScreen);
        iprintf("SD card not available.\n");
        while (1) swiWaitForVBlank();
    }

    scan_videos();
    draw_ui();

    while (1) {
        scanKeys();
        u32 down = keysDown();
        if (down & KEY_START) {
            playing = false;
            paused = false;
            scan_videos();
            draw_ui();
        }

        touchPosition touch;
        if (down & KEY_TOUCH) {
            touchRead(&touch);
            int y = touch.py;
            int x = touch.px;

            /* Touch coordinates refer to the lower display. */
            if (y >= 24 && y < 24 + fileCount * 16) {
                int row = (y - 24) / 16;
                if (row >= 0 && row < fileCount) selected = row;
            } else if (y >= 145 && y < 174) {
                if (x < 128 && fileCount) {
                    /* UI state only: a playback backend must be linked. */
                    playing = true;
                    paused = false;
                } else if (x >= 128 && fileCount && playing) {
                    paused = !paused;
                }
            } else if (y >= 174 && x < 128) {
                playing = false;
                paused = false;
            } else if (y >= 174 && x >= 128) {
                scan_videos();
                if (selected >= fileCount) selected = fileCount ? fileCount - 1 : 0;
            }
            draw_ui();
        }
        swiWaitForVBlank();
    }
    return 0;
}

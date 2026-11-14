#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define WORLD_SIZE 16
#define WORLD_HEIGHT 8
#define MAX_WORLDS 4
#define SAVE_PATH "fat:/dsi-craft.sav"
#define SAVE_TEMP_PATH "fat:/dsi-craft.tmp"
#define SAVE_BACKUP_PATH "fat:/dsi-craft.bak"
#define SAVE_MAGIC 0x44534943
#define SAVE_VERSION 1

#define AIR 0
#define GRASS 1
#define DIRT 2
#define STONE 3
#define WOOD 4
#define SAND 5

typedef struct {
    unsigned char blocks[WORLD_SIZE][WORLD_HEIGHT][WORLD_SIZE];
    bool used;
} World;

typedef struct {
    unsigned int magic;
    unsigned int version;
    World worlds[MAX_WORLDS];
} SaveData;

static World worlds[MAX_WORLDS];
static int selectedWorld = 0;
static int activeWorld = -1;
static int selectedBlock = GRASS;
static int cursorX = 8;
static int cursorZ = 8;
static int cursorY = 3;
static bool sdAvailable = false;
static bool saveOk = false;

static char blockSymbol(unsigned char block) {
    switch (block) {
        case GRASS: return 'G';
        case DIRT:  return 'D';
        case STONE: return 'S';
        case WOOD:  return 'W';
        case SAND:  return 'A';
        default:    return '.';
    }
}

static void generateWorld(World *world) {
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            world->blocks[x][0][z] = STONE;
            world->blocks[x][1][z] = DIRT;
            world->blocks[x][2][z] = GRASS;
            for (int y = 3; y < WORLD_HEIGHT; y++) {
                world->blocks[x][y][z] = AIR;
            }
        }
    }
    world->used = true;
}

static bool saveWorlds(void) {
    if (!sdAvailable) {
        saveOk = false;
        return false;
    }

    SaveData data;
    data.magic = SAVE_MAGIC;
    data.version = SAVE_VERSION;
    memcpy(data.worlds, worlds, sizeof(worlds));

    /* Scrive prima un file temporaneo: il salvataggio precedente resta intatto
       se la scheda si riempie o la scrittura viene interrotta. */
    remove(SAVE_TEMP_PATH);
    FILE *file = fopen(SAVE_TEMP_PATH, "wb");
    if (file == NULL) {
        saveOk = false;
        return false;
    }

    bool written = fwrite(&data, 1, sizeof(data), file) == sizeof(data);
    bool closed = fclose(file) == 0;
    if (!written || !closed) {
        remove(SAVE_TEMP_PATH);
        saveOk = false;
        return false;
    }

    /* FAT non consente sempre rename() sopra un file esistente.
       Sposta il vecchio salvataggio in backup e ripristinalo se serve. */
    remove(SAVE_BACKUP_PATH);
    FILE *oldSave = fopen(SAVE_PATH, "rb");
    bool hadOldSave = oldSave != NULL;
    if (oldSave != NULL) fclose(oldSave);

    if (hadOldSave && rename(SAVE_PATH, SAVE_BACKUP_PATH) != 0) {
        remove(SAVE_TEMP_PATH);
        saveOk = false;
        return false;
    }

    if (rename(SAVE_TEMP_PATH, SAVE_PATH) != 0) {
        if (hadOldSave) rename(SAVE_BACKUP_PATH, SAVE_PATH);
        remove(SAVE_TEMP_PATH);
        saveOk = false;
        return false;
    }

    if (hadOldSave) remove(SAVE_BACKUP_PATH);
    saveOk = true;
    return true;
}

static void loadWorlds(void) {
    if (!sdAvailable) return;

    FILE *file = fopen(SAVE_PATH, "rb");
    if (file == NULL) {
        saveOk = true; /* Primo avvio: non esiste ancora un file di salvataggio. */
        return;
    }

    SaveData data;
    size_t bytesRead = fread(&data, 1, sizeof(data), file);
    int extra = fgetc(file);
    fclose(file);

    if (bytesRead != sizeof(data) || extra != EOF ||
        data.magic != SAVE_MAGIC || data.version != SAVE_VERSION) {
        saveOk = false;
        return;
    }

    for (int i = 0; i < MAX_WORLDS; i++) {
        if (data.worlds[i].used) {
            for (int x = 0; x < WORLD_SIZE; x++) {
                for (int y = 0; y < WORLD_HEIGHT; y++) {
                    for (int z = 0; z < WORLD_SIZE; z++) {
                        if (data.worlds[i].blocks[x][y][z] > SAND) {
                            saveOk = false;
                            return;
                        }
                    }
                }
            }
        }
    }

    memcpy(worlds, data.worlds, sizeof(worlds));
    saveOk = true;
}

static void showWorldMenu(void) {
    consoleClear();
    iprintf("          DSi CRAFT\n");
    iprintf("==============================\n");
    iprintf("       SELEZIONE MONDO\n\n");
    iprintf("Scegli un mondo (massimo 4):\n\n");

    for (int i = 0; i < MAX_WORLDS; i++) {
        iprintf("%c Mondo %d: %s\n",
                selectedWorld == i ? '>' : ' ',
                i + 1,
                worlds[i].used ? "SALVATO" : "VUOTO");
    }

    iprintf("\nFRECCE: seleziona mondo\n");
    iprintf("A: entra nel mondo selezionato\n");
    iprintf("X: crea un nuovo mondo\n");
    iprintf("Salvataggio SD: %s\n",
            !sdAvailable ? "SD NON DISPONIBILE" :
            (saveOk ? "OK" : "ERRORE"));
    iprintf("Sempre giorno - nessun ciclo notte\n");
    iprintf("Niente mob, meteo o sopravvivenza\n");
}

static void keepCursorInWorld(void) {
    if (cursorX < 0) cursorX = 0;
    if (cursorX >= WORLD_SIZE) cursorX = WORLD_SIZE - 1;
    if (cursorZ < 0) cursorZ = 0;
    if (cursorZ >= WORLD_SIZE) cursorZ = WORLD_SIZE - 1;
    if (cursorY < 1) cursorY = 1;
    if (cursorY >= WORLD_HEIGHT) cursorY = WORLD_HEIGHT - 1;
}

static void breakBlock(void) {
    worlds[activeWorld].blocks[cursorX][cursorY][cursorZ] = AIR;
}

static void placeBlock(void) {
    if (worlds[activeWorld].blocks[cursorX][cursorY][cursorZ] == AIR) {
        worlds[activeWorld].blocks[cursorX][cursorY][cursorZ] =
            (unsigned char)selectedBlock;
    }
}

static void showWorld(void) {
    consoleClear();
    iprintf("          DSi CRAFT\n");
    iprintf("==============================\n");
    iprintf(" MONDO %d - MODALITA' CREATIVA\n", activeWorld + 1);
    iprintf(" CURSORE X/Z: %d/%d  LIVELLO: %d\n\n",
            cursorX, cursorZ, cursorY);

    iprintf("     ");
    for (int x = 0; x < WORLD_SIZE; x++) iprintf("%X", x);
    iprintf("\n");

    for (int z = 0; z < WORLD_SIZE; z++) {
        iprintf(" %2X  ", z);
        for (int x = 0; x < WORLD_SIZE; x++) {
            if (x == cursorX && z == cursorZ) {
                iprintf("+");
            } else {
                int y;
                for (y = WORLD_HEIGHT - 1; y >= 0; y--) {
                    if (worlds[activeWorld].blocks[x][y][z] != AIR) break;
                }
                iprintf("%c", y >= 0 ?
                    blockSymbol(worlds[activeWorld].blocks[x][y][z]) : '.');
            }
        }
        iprintf("\n");
    }

    iprintf("\n==============================\n");
    iprintf(" BLOCCO: %d (1=G 2=D 3=S 4=W 5=A)\n", selectedBlock);
    iprintf(" TOUCH: scegli blocco nella barra\\n");
    iprintf(" SELECT: blocco successivo\\n");
    iprintf(" FRECCE: muovi cursore X/Y: livello\n");
    iprintf(" L: rompi  R: piazza  B: menu mondi\n");
    iprintf(" + indica il cursore, non un personaggio\n");
    iprintf(" Salvataggio: %s\n",
            !sdAvailable ? "SD non disponibile" :
            (saveOk ? "OK" : "ERRORE"));
}

int main(void) {
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    consoleDemoInit();

    sdAvailable = fatInitDefault();
    if (sdAvailable) {
        loadWorlds();
    }

    showWorldMenu();

    while (1) {
        scanKeys();
        int keys = keysDown();
        bool changed = false;

        if (activeWorld < 0) {
            if (keys & KEY_UP) {
                selectedWorld = (selectedWorld + MAX_WORLDS - 1) % MAX_WORLDS;
                changed = true;
            }
            if (keys & KEY_DOWN) {
                selectedWorld = (selectedWorld + 1) % MAX_WORLDS;
                changed = true;
            }
            if (keys & KEY_A) {
                if (worlds[selectedWorld].used) {
                    activeWorld = selectedWorld;
                    cursorX = 8;
                    cursorZ = 8;
                    cursorY = 3;
                    showWorld();
                }
            }
            if (keys & KEY_X) {
                int freeSlot = -1;
                for (int i = 0; i < MAX_WORLDS; i++) {
                    if (!worlds[i].used) {
                        freeSlot = i;
                        break;
                    }
                }
                if (freeSlot >= 0) {
                    generateWorld(&worlds[freeSlot]);
                    selectedWorld = freeSlot;
                    activeWorld = freeSlot;
                    saveWorlds();
                    cursorX = 8;
                    cursorZ = 8;
                    cursorY = 3;
                    showWorld();
                } else {
                    consoleClear();
                    iprintf("Hai gia' creato 4 mondi!\n");
                    iprintf("Premi un tasto per tornare.");
                    while (1) {
                        scanKeys();
                        if (keysDown()) break;
                        swiWaitForVBlank();
                    }
                    showWorldMenu();
                }
            }
            if (changed) showWorldMenu();
        } else {
            if (keys & KEY_B) {
                activeWorld = -1;
                showWorldMenu();
            } else {
                if (keys & KEY_UP)    { cursorZ--; changed = true; }
                if (keys & KEY_DOWN)  { cursorZ++; changed = true; }
                if (keys & KEY_LEFT)  { cursorX--; changed = true; }
                if (keys & KEY_RIGHT) { cursorX++; changed = true; }
                if (keys & KEY_Y)     { cursorY++; changed = true; }
                if (keys & KEY_SELECT) { selectedBlock = selectedBlock % SAND + 1; changed = true; }
                if (keys & KEY_Y)     { cursorY++; changed = true; }\n                if (keys & KEY_SELECT) { selectedBlock = selectedBlock % SAND + 1; changed = true; }
                if (keys & KEY_L)     { breakBlock(); saveWorlds(); changed = true; }
                if (keys & KEY_R)     { placeBlock(); saveWorlds(); changed = true; }

                if (keys & KEY_TOUCH) {
                    touchPosition touch;
                    touchRead(&touch);
                    if (touch.px < 255 && touch.py < 192) {
                        int slot = touch.px / 51;
                        if (slot >= 0 && slot < 5) {
                            selectedBlock = slot + 1;
                            changed = true;
                        }
                    }
                }

                keepCursorInWorld();
                if (changed) showWorld();
            }
        }

        swiWaitForVBlank();
    }

    return 0;
}

#include <nds.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define WORLD_SIZE 16
#define WORLD_HEIGHT 8
#define MAX_WORLDS 4

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

static World worlds[MAX_WORLDS];
static int selectedWorld = 0;
static int activeWorld = -1;
static int selectedBlock = GRASS;
static int cursorX = 8;
static int cursorZ = 8;
static int cursorY = 3;

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
                worlds[i].used ? "CREATO" : "VUOTO");
    }

    iprintf("\nFRECCE: seleziona mondo\n");
    iprintf("A: entra nel mondo selezionato\n");
    iprintf("X: crea un nuovo mondo\n");
    iprintf("Massimo 4 mondi in memoria\n");
    iprintf("Nessun mob, meteo o sopravvivenza\n");
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
    iprintf(" TOUCH: scegli blocco nella barra\n");
    iprintf(" FRECCE: muovi cursore X/Y: livello\n");
    iprintf(" L: rompi  R: piazza  B: menu mondi\n");
    iprintf(" + indica il cursore, non un personaggio\n");
}

int main(void) {
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    consoleDemoInit();

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
                if (keys & KEY_X)     { cursorY--; changed = true; }
                if (keys & KEY_Y)     { cursorY++; changed = true; }
                if (keys & KEY_L)     { breakBlock(); changed = true; }
                if (keys & KEY_R)     { placeBlock(); changed = true; }

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

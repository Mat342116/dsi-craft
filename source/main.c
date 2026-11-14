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
#define SAVE_VERSION 2

#define AIR 0
#define GRASS 1
#define DIRT 2
#define STONE 3
#define WOOD 4
#define SAND 5
#define DOOR_CLOSED 6
#define DOOR_OPEN 7
#define CHEST_CLOSED 8
#define CHEST_OPEN 9

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

static const int paletteBlocks[7] = {
    GRASS, DIRT, STONE, WOOD, SAND, DOOR_CLOSED, CHEST_CLOSED
};

static const char *blockName(unsigned char block) {
    switch (block) {
        case GRASS: return "ERBA";
        case DIRT: return "TERRA";
        case STONE: return "PIETRA";
        case WOOD: return "LEGNO";
        case SAND: return "SABBIA";
        case DOOR_CLOSED:
        case DOOR_OPEN: return "PORTA";
        case CHEST_CLOSED:
        case CHEST_OPEN: return "BAULE";
        default: return "VUOTO";
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

    FILE *oldSave = fopen(SAVE_PATH, "rb");
    bool hadOldSave = oldSave != NULL;
    if (oldSave != NULL) fclose(oldSave);

    if (!hadOldSave) {
        FILE *backup = fopen(SAVE_BACKUP_PATH, "rb");
        if (backup != NULL) {
            fclose(backup);
            if (rename(SAVE_BACKUP_PATH, SAVE_PATH) == 0)
                hadOldSave = true;
        }
    }

    if (hadOldSave) remove(SAVE_BACKUP_PATH);
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
        if (rename(SAVE_BACKUP_PATH, SAVE_PATH) == 0)
            file = fopen(SAVE_PATH, "rb");
    }
    if (file == NULL) {
        saveOk = true;
        return;
    }

    SaveData data;
    size_t bytesRead = fread(&data, 1, sizeof(data), file);
    int extra = fgetc(file);
    fclose(file);

    /* Version 1 worlds used only the five basic blocks. Their layout is
       unchanged, so old saves can be loaded without resetting the worlds. */
    if (bytesRead != sizeof(data) || extra != EOF ||
        data.magic != SAVE_MAGIC ||
        (data.version != 1 && data.version != SAVE_VERSION)) {
        saveOk = false;
        return;
    }

    unsigned char maxBlock = (data.version == 1) ? SAND : CHEST_OPEN;
    for (int i = 0; i < MAX_WORLDS; i++) {
        if (data.worlds[i].used) {
            for (int x = 0; x < WORLD_SIZE; x++) {
                for (int y = 0; y < WORLD_HEIGHT; y++) {
                    for (int z = 0; z < WORLD_SIZE; z++) {
                        if (data.worlds[i].blocks[x][y][z] > maxBlock) {
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
    iprintf("Sempre giorno - modalita' creativa\n");
    iprintf("Niente TNT, botole o fuoco\n");
}

static void showPalette(void) {
    consoleClear();
    iprintf("       DSi CRAFT - BLOCCHI\n");
    iprintf(" Tocca un blocco per prenderlo\n");
    iprintf("--------------------------------\n");
    iprintf("[ERBA]  [TERRA] [PIETRA] [LEGNO]\n");
    iprintf("   1       2       3       4\n");
    iprintf("\n[SABBIA] [PORTA]  [BAULE]\n");
    iprintf("   5       6       7\n");
    iprintf("--------------------------------\n");
    iprintf("MANO: %s\n", blockName((unsigned char)selectedBlock));
    iprintf("Il blocco selezionato e' in mano\n");
    iprintf("Frecce: muovi il puntatore\n");
    iprintf("X/Y: cambia altezza\n");
    iprintf("L: rompi  R: posa blocco\n");
    iprintf("A: usa porta o baule\n");
    iprintf("B: torna ai mondi\n");
    iprintf("Salvataggio SD: %s\n",
            !sdAvailable ? "non disponibile" :
            (saveOk ? "OK" : "ERRORE"));
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

static void interactBlock(void) {
    unsigned char *block =
        &worlds[activeWorld].blocks[cursorX][cursorY][cursorZ];

    if (*block == DOOR_CLOSED) *block = DOOR_OPEN;
    else if (*block == DOOR_OPEN) *block = DOOR_CLOSED;
    else if (*block == CHEST_CLOSED) *block = CHEST_OPEN;
    else if (*block == CHEST_OPEN) *block = CHEST_CLOSED;
    else return;

    saveWorlds();
}

static void drawQuad(float x1, float y1, float z1,
                     float x2, float y2, float z2,
                     float x3, float y3, float z3,
                     float x4, float y4, float z4,
                     int color) {
    glColor(color);
    glBegin(GL_QUADS);
    glVertex3f(x1, y1, z1);
    glVertex3f(x2, y2, z2);
    glVertex3f(x3, y3, z3);
    glVertex3f(x4, y4, z4);
    glEnd();
}

static void drawCuboid(float x0, float y0, float z0,
                       float x1, float y1, float z1,
                       int topColor, int sideColor, int frontColor) {
    drawQuad(x0,y1,z0, x1,y1,z0, x1,y1,z1, x0,y1,z1, topColor);
    drawQuad(x0,y0,z1, x1,y0,z1, x1,y0,z0, x0,y0,z0, sideColor);
    drawQuad(x0,y0,z0, x0,y1,z0, x0,y1,z1, x0,y0,z1, sideColor);
    drawQuad(x1,y0,z1, x1,y1,z1, x1,y1,z0, x1,y0,z0, sideColor);
    drawQuad(x1,y0,z0, x1,y1,z0, x0,y1,z0, x0,y0,z0, sideColor);
    drawQuad(x0,y0,z1, x0,y1,z1, x1,y1,z1, x1,y0,z1, frontColor);
}

static int blockColor(unsigned char block) {
    switch (block) {
        case GRASS: return RGB15(12,25,7);
        case DIRT: return RGB15(20,11,5);
        case STONE: return RGB15(17,17,19);
        case WOOD: return RGB15(22,14,5);
        case SAND: return RGB15(28,25,13);
        default: return RGB15(20,13,5);
    }
}

static void drawBlock(int x, int y, int z, unsigned char block) {
    float a = (float)x - 8.0f;
    float b = (float)y;
    float c = (float)z - 8.0f;
    float X = a + 1.0f, Y = b + 1.0f, Z = c + 1.0f;
    World *w = &worlds[activeWorld];

    if (block == DOOR_CLOSED || block == DOOR_OPEN) {
        if (block == DOOR_CLOSED) {
            drawCuboid(a,b,c+0.43f, X,Y,c+0.57f,
                       RGB15(22,13,5), RGB15(13,7,2), RGB15(25,15,6));
        } else {
            drawCuboid(a+0.43f,b,c, a+0.57f,Y,Z,
                       RGB15(22,13,5), RGB15(13,7,2), RGB15(25,15,6));
        }
        return;
    }

    if (block == CHEST_CLOSED || block == CHEST_OPEN) {
        drawCuboid(a+0.10f,b,c+0.10f, X-0.10f,b+0.72f,Z-0.10f,
                   RGB15(22,13,4), RGB15(14,8,2), RGB15(25,15,5));
        if (block == CHEST_CLOSED) {
            drawCuboid(a+0.10f,b+0.72f,c+0.10f,
                       X-0.10f,Y,Z-0.10f,
                       RGB15(26,17,6), RGB15(18,10,3), RGB15(28,19,7));
        } else {
            drawCuboid(a+0.10f,b+0.72f,c+0.10f,
                       X-0.10f,Y,c+0.28f,
                       RGB15(26,17,6), RGB15(18,10,3), RGB15(28,19,7));
        }
        return;
    }

    int top = blockColor(block);
    int side = (block == GRASS) ? RGB15(17,10,4) : RGB15(10,10,10);
    int front = (block == GRASS) ? RGB15(17,10,4) : RGB15(15,15,15);

    if (y == WORLD_HEIGHT - 1 || w->blocks[x][y+1][z] == AIR)
        drawQuad(a,Y,c, X,Y,c, X,Y,Z, a,Y,Z, top);
    if (y > 0 && w->blocks[x][y-1][z] == AIR)
        drawQuad(a,b,Z, X,b,Z, X,b,c, a,b,c, RGB15(8,8,8));
    if (x == 0 || w->blocks[x-1][y][z] == AIR)
        drawQuad(a,b,c, a,Y,c, a,Y,Z, a,b,Z, side);
    if (x == WORLD_SIZE-1 || w->blocks[x+1][y][z] == AIR)
        drawQuad(X,b,Z, X,Y,Z, X,Y,c, X,b,c, RGB15(18,18,18));
    if (z == 0 || w->blocks[x][y][z-1] == AIR)
        drawQuad(X,b,c, X,Y,c, a,Y,c, a,b,c, RGB15(15,15,15));
    if (z == WORLD_SIZE-1 || w->blocks[x][y][z+1] == AIR)
        drawQuad(a,b,Z, a,Y,Z, X,Y,Z, X,b,Z, front);
}

static void drawBuildMarker(void) {
    float x = (float)cursorX - 8.0f;
    float y = (float)cursorY + 0.035f;
    float z = (float)cursorZ - 8.0f;
    int marker = RGB15(31,31,0);

    /* Bright X-shaped placement pointer on the selected block. */
    drawQuad(x+0.04f,y,z+0.12f,
             x+0.12f,y,z+0.04f,
             x+0.96f,y,z+0.88f,
             x+0.88f,y,z+0.96f, marker);
    drawQuad(x+0.04f,y,z+0.88f,
             x+0.12f,y,z+0.96f,
             x+0.96f,y,z+0.12f,
             x+0.88f,y,z+0.04f, marker);
}

static void drawBlockInHand(void) {
    int color = blockColor((unsigned char)selectedBlock);

    /* A small hand and the selected block stay visible in the lower-right
       corner of the first-person view. */
    glPushMatrix();
    glLoadIdentity();
    glTranslatef(0.78f, -0.62f, -2.6f);
    drawCuboid(-0.10f,-0.18f,-0.08f, 0.16f,0.02f,0.18f,
               RGB15(26,18,13), RGB15(23,14,9), RGB15(28,20,15));
    glTranslatef(-0.10f, 0.16f, 0.04f);
    drawCuboid(-0.18f,-0.18f,-0.18f, 0.18f,0.18f,0.18f,
               color, color, color);
    glPopMatrix(1);
}

static void renderWorld3D(void) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(58.0f, 256.0f/192.0f, 0.1f, 80.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(12.0f, 11.0f, 14.0f,
              0.0f, 1.5f, 0.0f,
              0.0f, 1.0f, 0.0f);

    glPolyFmt(POLY_ALPHA(31) | POLY_CULL_NONE);

    for (int y = 0; y < WORLD_HEIGHT; y++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            for (int x = 0; x < WORLD_SIZE; x++) {
                unsigned char block = worlds[activeWorld].blocks[x][y][z];
                if (block != AIR) drawBlock(x, y, z, block);
            }
        }
    }

    drawBuildMarker();
    drawBlockInHand();
    glFlush(0);
}

static void selectPaletteBlock(int x, int y) {
    int slot = -1;

    if (y >= 40 && y < 100) {
        slot = x / 64;
        if (slot < 0 || slot > 3) return;
    } else if (y >= 104 && y < 168) {
        slot = 4 + x / 64;
        if (slot < 4 || slot > 6) return;
    } else {
        return;
    }

    selectedBlock = paletteBlocks[slot];
    showPalette();
}

int main(void) {
    videoSetMode(MODE_0_3D);
    videoSetModeSub(MODE_0_2D);
    lcdMainOnTop();

    vramSetBankA(VRAM_A_TEXTURE);
    vramSetBankB(VRAM_B_TEXTURE);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256,
                31, 0, false, true);

    glInit();
    glEnable(GL_ANTIALIAS);
    glClearColor(12, 20, 31, 31);
    glClearPolyID(63);
    glClearDepth(GL_MAX_DEPTH);
    glViewport(0, 0, 255, 191);

    sdAvailable = fatInitDefault();
    if (sdAvailable) loadWorlds();

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
            if ((keys & KEY_A) && worlds[selectedWorld].used) {
                activeWorld = selectedWorld;
                cursorX = 8;
                cursorZ = 8;
                cursorY = 3;
                showPalette();
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
                    saveWorlds();
                    showPalette();
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

                if (keys & KEY_L) {
                    breakBlock();
                    saveWorlds();
                    changed = true;
                }
                if (keys & KEY_R) {
                    placeBlock();
                    saveWorlds();
                    changed = true;
                }
                if (keys & KEY_A) {
                    interactBlock();
                    changed = true;
                }

                if (keys & KEY_TOUCH) {
                    touchPosition touch;
                    touchRead(&touch);
                    selectPaletteBlock(touch.px, touch.py);
                }

                keepCursorInWorld();
                if (changed) showPalette();
            }
        }

        if (activeWorld >= 0) renderWorld3D();
        swiWaitForVBlank();
    }

    return 0;
}

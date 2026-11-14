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

/* Player position and view direction for the first-person camera. */
static float playerX = 8.5f;
static float playerY = 4.0f;
static float playerZ = 8.5f;
static float viewYaw = 0.0f;
static float viewPitch = 0.0f;
static int paletteBg = -1;
static u16 *palettePixels = NULL;

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
    /* Gameplay UI is drawn as pixel-art icons on the lower screen. */
    if (palettePixels == NULL) return;
    for (int i = 0; i < 256 * 256; i++) palettePixels[i] = RGB15(5, 8, 13);
    /* subtle panel and icon wells */
    for (int y = 8; y < 184; y++)
        for (int x = 6; x < 250; x++)
            palettePixels[y * 256 + x] = RGB15(8, 12, 18);
    for (int i = 0; i < 7; i++) {
        int x = (i < 4) ? 12 + i * 60 : 42 + (i - 4) * 60;
        int y = (i < 4) ? 24 : 108;
        int edge = paletteBlocks[i] == selectedBlock ? RGB15(31, 29, 12) : RGB15(15, 18, 23);
        for (int yy = y - 5; yy < y + 53; yy++)
            for (int xx = x - 5; xx < x + 53; xx++)
                if (xx >= 0 && xx < 256 && yy >= 0 && yy < 192)
                    palettePixels[yy * 256 + xx] = edge;
        for (int yy = y; yy < y + 43; yy++)
            for (int xx = x; xx < x + 43; xx++)
                palettePixels[yy * 256 + xx] = RGB15(3, 5, 8);
        int col = blockColor((unsigned char)paletteBlocks[i]);
        if (paletteBlocks[i] == DOOR_CLOSED) {
            for (int yy = y + 5; yy < y + 39; yy++)
                for (int xx = x + 13; xx < x + 30; xx++)
                    palettePixels[yy * 256 + xx] = RGB15(22, 13, 5);
            for (int yy = y + 5; yy < y + 39; yy++) palettePixels[yy * 256 + x + 14] = RGB15(28, 20, 8);
        } else if (paletteBlocks[i] == CHEST_CLOSED) {
            for (int yy = y + 15; yy < y + 36; yy++)
                for (int xx = x + 6; xx < x + 37; xx++)
                    palettePixels[yy * 256 + xx] = RGB15(22, 13, 4);
            for (int xx = x + 6; xx < x + 37; xx++) palettePixels[(y + 15) * 256 + xx] = RGB15(30, 20, 7);
            for (int yy = y + 23; yy < y + 27; yy++)
                for (int xx = x + 19; xx < x + 24; xx++) palettePixels[yy * 256 + xx] = RGB15(30, 25, 12);
        } else {
            /* Three shaded faces make a small voxel-cube illustration. */
            for (int yy = y + 8; yy < y + 29; yy++)
                for (int xx = x + 7; xx < x + 36; xx++) {
                    int shade = (xx < x + 21 && yy < y + 19) ? 2 : 0;
                    palettePixels[yy * 256 + xx] = col;
                    if (shade) palettePixels[yy * 256 + xx] = RGB15(28, 28, 28);
                }
            for (int yy = y + 29; yy < y + 37; yy++)
                for (int xx = x + 7; xx < x + 36; xx++)
                    palettePixels[yy * 256 + xx] = RGB15(10, 10, 10);
        }
    }
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

static void drawCenterCrosshair(void) {
    /* Fixed, thin Minecraft-style crosshair at the exact screen center. */
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof(-1.0f, 1.0f, -1.0f, 1.0f, 0.1f, 10.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    int white = RGB15(31, 31, 31);
    drawQuad(-0.010f, -0.075f, -2.0f, 0.010f, -0.075f, -2.0f,
             0.010f, 0.075f, -2.0f, -0.010f, 0.075f, -2.0f, white);
    drawQuad(-0.075f, -0.010f, -2.0f, 0.075f, -0.010f, -2.0f,
             0.075f, 0.010f, -2.0f, -0.075f, 0.010f, -2.0f, white);
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
    gluPerspective(68.0f, 256.0f/192.0f, 0.1f, 80.0f);

    float cp = cosf(viewPitch);
    float dx = sinf(viewYaw) * cp;
    float dy = sinf(viewPitch);
    float dz = -cosf(viewYaw) * cp;
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(playerX - 8.0f, playerY, playerZ - 8.0f,
              playerX - 8.0f + dx, playerY + dy,
              playerZ - 8.0f + dz,
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
    drawBlockInHand();
    drawCenterCrosshair();
    glFlush(0);
}

static void selectPaletteBlock(int x, int y) {
    int slot = -1;
    if (y >= 19 && y < 76) {
        int col = (x - 7) / 60;
        if (col >= 0 && col < 4 && x >= 7 + col * 60 && x < 60 + col * 60) slot = col;
    } else if (y >= 103 && y < 160) {
        int col = (x - 37) / 60;
        if (col >= 0 && col < 3 && x >= 37 + col * 60 && x < 90 + col * 60) slot = 4 + col;
    }
    if (slot < 0 || slot > 6) return;
    selectedBlock = paletteBlocks[slot];
    showPalette();
}

static bool raycastTarget(int *hitX, int *hitY, int *hitZ,
                          int *placeX, int *placeY, int *placeZ) {
    float cp = cosf(viewPitch);
    float dx = sinf(viewYaw) * cp;
    float dy = sinf(viewPitch);
    float dz = -cosf(viewYaw) * cp;
    int lastX = -1, lastY = -1, lastZ = -1;
    for (float t = 0.15f; t <= 6.0f; t += 0.06f) {
        int x = (int)floorf(playerX + dx * t);
        int y = (int)floorf(playerY + dy * t);
        int z = (int)floorf(playerZ + dz * t);
        if (x < 0 || x >= WORLD_SIZE || y < 0 || y >= WORLD_HEIGHT || z < 0 || z >= WORLD_SIZE) continue;
        if (worlds[activeWorld].blocks[x][y][z] != AIR) {
            *hitX = x; *hitY = y; *hitZ = z;
            *placeX = lastX; *placeY = lastY; *placeZ = lastZ;
            return true;
        }
        lastX = x; lastY = y; lastZ = z;
    }
    return false;
}

static void breakTargetBlock(void) {
    int x,y,z,px,py,pz;
    if (raycastTarget(&x,&y,&z,&px,&py,&pz)) {
        worlds[activeWorld].blocks[x][y][z] = AIR;
        saveWorlds();
    }
}

static void placeTargetBlock(void) {
    int x,y,z,px,py,pz;
    if (raycastTarget(&x,&y,&z,&px,&py,&pz) &&
        px >= 0 && py >= 0 && pz >= 0 &&
        worlds[activeWorld].blocks[px][py][pz] == AIR) {
        worlds[activeWorld].blocks[px][py][pz] = (unsigned char)selectedBlock;
        saveWorlds();
    }
}

static void interactTargetBlock(void) {
    int x,y,z,px,py,pz;
    if (raycastTarget(&x,&y,&z,&px,&py,&pz)) {
        cursorX=x; cursorY=y; cursorZ=z;
        interactBlock();
    }
}

static void initGamePalette(void) {
    videoSetModeSub(MODE_5_2D);
    paletteBg = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    palettePixels = (u16 *)bgGetGfxPtr(paletteBg);
    showPalette();
}

static void enterWorldPalette(void) {
    initGamePalette();
}

static void returnToWorldMenu(void) {
    palettePixels = NULL;
    videoSetModeSub(MODE_0_2D);
    consoleInit(NULL, 0, BgType_Text4bpp, BgSize_T_256x256,
                31, 0, false, true);
    activeWorld = -1;
    showWorldMenu();
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
                playerX = 8.5f; playerY = 4.0f; playerZ = 8.5f;
                viewYaw = 0.0f; viewPitch = 0.0f;
                enterWorldPalette();
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
                    playerX = 8.5f; playerY = 4.0f; playerZ = 8.5f;
                    viewYaw = 0.0f; viewPitch = 0.0f;
                    saveWorlds();
                    enterWorldPalette();
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
                returnToWorldMenu();
            } else {
                /* D-pad controls movement and turning, like a handheld FPS. */
                float forward = 0.0f;
                if (keys & KEY_UP) forward += 0.16f;
                if (keys & KEY_DOWN) forward -= 0.16f;
                if (keys & KEY_LEFT) viewYaw -= 0.09f;
                if (keys & KEY_RIGHT) viewYaw += 0.09f;
                if (forward != 0.0f) {
                    playerX += sinf(viewYaw) * forward;
                    playerZ -= cosf(viewYaw) * forward;
                    if (playerX < 0.3f) playerX = 0.3f;
                    if (playerX > WORLD_SIZE - 0.3f) playerX = WORLD_SIZE - 0.3f;
                    if (playerZ < 0.3f) playerZ = 0.3f;
                    if (playerZ > WORLD_SIZE - 0.3f) playerZ = WORLD_SIZE - 0.3f;
                }
                if (keys & KEY_L) breakTargetBlock();
                if (keys & KEY_R) placeTargetBlock();
                if (keys & KEY_A) interactTargetBlock();
                if (keys & KEY_TOUCH) {
                    touchPosition touch;
                    touchRead(&touch);
                    selectPaletteBlock(touch.px, touch.py);
                }
            }
        }

        if (activeWorld >= 0) renderWorld3D();
        swiWaitForVBlank();
    }

    return 0;
}

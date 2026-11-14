#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

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
#define SCREEN_W 256
#define SCREEN_H 192
#define PI 3.14159265358979323846f

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
static float playerX = 8.5f, playerZ = 8.5f, playerY = 3.0f;
static float yaw = 0.0f, pitch = 0.0f;
static bool sdAvailable = false, saveOk = false;
static u16 *framebuffer;
static PrintConsole bottomConsole;

static u16 rgb15(int r, int g, int b) {
    return RGB15(r, g, b);
}

static int topHeight(int x, int z) {
    if (x < 0 || x >= WORLD_SIZE || z < 0 || z >= WORLD_SIZE) return WORLD_HEIGHT;
    for (int y = WORLD_HEIGHT - 1; y >= 0; --y)
        if (worlds[activeWorld].blocks[x][y][z] != AIR) return y + 1;
    return 0;
}

static unsigned char topBlock(int x, int z) {
    int h = topHeight(x, z);
    if (h <= 0 || h > WORLD_HEIGHT) return STONE;
    return worlds[activeWorld].blocks[x][h - 1][z];
}

static u16 blockColor(unsigned char block, int shade) {
    int r = 10, g = 10, b = 10;
    switch (block) {
        case GRASS: r = 5; g = 25; b = 5; break;
        case DIRT:  r = 20; g = 11; b = 5; break;
        case STONE: r = 15; g = 15; b = 16; break;
        case WOOD:  r = 22; g = 13; b = 5; break;
        case SAND:  r = 28; g = 25; b = 13; break;
        default: break;
    }
    r = r * shade / 16; g = g * shade / 16; b = b * shade / 16;
    if (r > 31) r = 31; if (g > 31) g = 31; if (b > 31) b = 31;
    return rgb15(r, g, b);
}

static void fillRect(int x0, int y0, int x1, int y1, u16 color) {
    if (x0 < 0) x0 = 0; if (x1 > SCREEN_W) x1 = SCREEN_W;
    if (y0 < 0) y0 = 0; if (y1 > SCREEN_H) y1 = SCREEN_H;
    for (int y = y0; y < y1; ++y)
        for (int x = x0; x < x1; ++x)
            framebuffer[y * 256 + x] = color;
}

static void generateWorld(World *world) {
    memset(world, 0, sizeof(*world));
    for (int x = 0; x < WORLD_SIZE; ++x) {
        for (int z = 0; z < WORLD_SIZE; ++z) {
            world->blocks[x][0][z] = STONE;
            world->blocks[x][1][z] = DIRT;
            world->blocks[x][2][z] = GRASS;
        }
    }
    /* Piccole colline e piattaforme: ogni blocco e' una cella solida. */
    for (int x = 3; x < 7; ++x)
        for (int z = 4; z < 8; ++z)
            world->blocks[x][3][z] = STONE;
    for (int x = 10; x < 13; ++x)
        for (int z = 9; z < 12; ++z)
            world->blocks[x][3][z] = WOOD;
    world->used = true;
}

static bool saveWorlds(void) {
    if (!sdAvailable) { saveOk = false; return false; }
    SaveData data;
    data.magic = SAVE_MAGIC; data.version = SAVE_VERSION;
    memcpy(data.worlds, worlds, sizeof(worlds));
    remove(SAVE_TEMP_PATH);
    FILE *f = fopen(SAVE_TEMP_PATH, "wb");
    if (!f) { saveOk = false; return false; }
    bool ok = fwrite(&data, 1, sizeof(data), f) == sizeof(data);
    if (fclose(f) != 0) ok = false;
    if (!ok) { remove(SAVE_TEMP_PATH); saveOk = false; return false; }
    FILE *old = fopen(SAVE_PATH, "rb");
    bool hadOld = old != NULL;
    if (old) fclose(old);
    if (!hadOld) {
        FILE *bak = fopen(SAVE_BACKUP_PATH, "rb");
        if (bak) { fclose(bak); if (rename(SAVE_BACKUP_PATH, SAVE_PATH) == 0) hadOld = true; }
    }
    if (hadOld) remove(SAVE_BACKUP_PATH);
    if (hadOld && rename(SAVE_PATH, SAVE_BACKUP_PATH) != 0) {
        remove(SAVE_TEMP_PATH); saveOk = false; return false;
    }
    if (rename(SAVE_TEMP_PATH, SAVE_PATH) != 0) {
        if (hadOld) rename(SAVE_BACKUP_PATH, SAVE_PATH);
        remove(SAVE_TEMP_PATH); saveOk = false; return false;
    }
    if (hadOld) remove(SAVE_BACKUP_PATH);
    saveOk = true; return true;
}

static void loadWorlds(void) {
    if (!sdAvailable) return;
    FILE *f = fopen(SAVE_PATH, "rb");
    if (!f && rename(SAVE_BACKUP_PATH, SAVE_PATH) == 0) f = fopen(SAVE_PATH, "rb");
    if (!f) { saveOk = true; return; }
    SaveData data;
    size_t n = fread(&data, 1, sizeof(data), f);
    int extra = fgetc(f); fclose(f);
    if (n != sizeof(data) || extra != EOF || data.magic != SAVE_MAGIC ||
        data.version != SAVE_VERSION) { saveOk = false; return; }
    for (int w = 0; w < MAX_WORLDS; ++w) if (data.worlds[w].used)
        for (int x = 0; x < WORLD_SIZE; ++x)
            for (int y = 0; y < WORLD_HEIGHT; ++y)
                for (int z = 0; z < WORLD_SIZE; ++z)
                    if (data.worlds[w].blocks[x][y][z] > SAND) { saveOk = false; return; }
    memcpy(worlds, data.worlds, sizeof(worlds)); saveOk = true;
}

static void clearBottom(void) {
    consoleSelect(&bottomConsole);
    consoleClear();
}

static void showMenu(void) {
    activeWorld = -1;
    clearBottom();
    iprintf("          DSi CRAFT 3D\n");
    iprintf("==============================\n");
    iprintf("     SELEZIONE MONDO\n\n");
    for (int i = 0; i < MAX_WORLDS; ++i)
        iprintf("%c Mondo %d: %s\n", selectedWorld == i ? '>' : ' ',
                i + 1, worlds[i].used ? "SALVATO" : "VUOTO");
    iprintf("\nFRECCE: scegli mondo\n");
    iprintf("A: entra nel mondo\nX: crea mondo\n");
    iprintf("START: menu / SELECT: mondi\n");
    iprintf("SD: %s\n", !sdAvailable ? "NON DISPONIBILE" : (saveOk ? "OK" : "ERRORE"));
    fillRect(0, 0, 256, 192, rgb15(4, 9, 19));
    fillRect(0, 118, 256, 192, rgb15(5, 15, 7));
    fillRect(36, 74, 220, 78, rgb15(12, 25, 18));
    fillRect(44, 66, 212, 70, rgb15(12, 25, 18));
}

static void renderWorld(void) {
    const float horizon = 96.0f + pitch * 55.0f;
    for (int y = 0; y < SCREEN_H; ++y) {
        u16 c;
        if (y < horizon) {
            int k = y * 8 / SCREEN_H;
            c = rgb15(5 + k / 2, 12 + k, 23 + k);
        } else {
            int k = (y - (int)horizon) * 8 / SCREEN_H;
            c = rgb15(8 + k, 13 + k, 6 + k / 2);
        }
        for (int x = 0; x < SCREEN_W; ++x) framebuffer[y * 256 + x] = c;
    }

    float dirX = cosf(yaw), dirZ = sinf(yaw);
    float planeX = -dirZ * 0.72f, planeZ = dirX * 0.72f;
    for (int sx = 0; sx < SCREEN_W; ++sx) {
        float cameraX = 2.0f * sx / (float)SCREEN_W - 1.0f;
        float rayX = dirX + planeX * cameraX;
        float rayZ = dirZ + planeZ * cameraX;
        int hitX = -1, hitZ = -1, side = 0;
        float distance = 0.05f;
        for (int step = 0; step < 180; ++step) {
            distance += 0.075f;
            int cx = (int)floorf(playerX + rayX * distance);
            int cz = (int)floorf(playerZ + rayZ * distance);
            if (cx < 0 || cx >= WORLD_SIZE || cz < 0 || cz >= WORLD_SIZE) {
                hitX = cx; hitZ = cz; side = (fabsf(rayX) > fabsf(rayZ)) ? 0 : 1; break;
            }
            if (topHeight(cx, cz) > 0) {
                hitX = cx; hitZ = cz;
                side = (fabsf(rayX) > fabsf(rayZ)) ? 0 : 1;
                break;
            }
        }
        if (hitX == -1) continue;
        if (distance < 0.12f) distance = 0.12f;
        int h = (hitX < 0 || hitX >= WORLD_SIZE || hitZ < 0 || hitZ >= WORLD_SIZE)
            ? WORLD_HEIGHT : topHeight(hitX, hitZ);
        unsigned char block = (hitX < 0 || hitX >= WORLD_SIZE || hitZ < 0 || hitZ >= WORLD_SIZE)
            ? STONE : topBlock(hitX, hitZ);
        float scale = 72.0f / distance;
        int top = (int)(horizon - (h - playerY + 0.15f) * scale);
        int bottom = (int)(horizon + (playerY - 0.0f) * scale);
        if (top < 0) top = 0; if (bottom > SCREEN_H) bottom = SCREEN_H;
        int shade = side ? 11 : 16;
        if (((hitX + hitZ) & 1) != 0) shade -= 2;
        u16 wall = blockColor(block, shade);
        for (int py = top; py < bottom; ++py) {
            int band = ((py - top) / 12) & 1;
            framebuffer[py * 256 + sx] = band ? blockColor(block, shade - 2) : wall;
        }
        /* Bordo superiore illuminato: rende leggibile il volume dei blocchi. */
        if (top >= 0 && top < SCREEN_H) framebuffer[top * 256 + sx] = blockColor(block, 19);
    }

    /* Mirino centrale. */
    fillRect(125, 95, 131, 97, rgb15(31, 31, 31));
    fillRect(127, 93, 129, 99, rgb15(31, 31, 31));
}

static bool canStand(float x, float z) {
    int cx = (int)floorf(x), cz = (int)floorf(z);
    if (cx < 0 || cx >= WORLD_SIZE || cz < 0 || cz >= WORLD_SIZE) return false;
    return (float)topHeight(cx, cz) <= playerY + 0.15f;
}

static void movePlayer(float forward, float strafe) {
    float dx = cosf(yaw) * forward - sinf(yaw) * strafe;
    float dz = sinf(yaw) * forward + cosf(yaw) * strafe;
    float nx = playerX + dx, nz = playerZ + dz;
    if (canStand(nx, playerZ)) playerX = nx;
    if (canStand(playerX, nz)) playerZ = nz;
    int cx = (int)floorf(playerX), cz = (int)floorf(playerZ);
    if (cx >= 0 && cx < WORLD_SIZE && cz >= 0 && cz < WORLD_SIZE) {
        float floorY = (float)topHeight(cx, cz);
        if (playerY > floorY) playerY -= 0.08f;
        if (playerY < floorY) playerY = floorY;
    }
}

static void editTarget(bool place) {
    float dx = cosf(yaw), dz = sinf(yaw);
    int lastX = -1, lastZ = -1;
    for (float d = 0.35f; d < 4.0f; d += 0.08f) {
        int x = (int)floorf(playerX + dx * d);
        int z = (int)floorf(playerZ + dz * d);
        if (x < 0 || x >= WORLD_SIZE || z < 0 || z >= WORLD_SIZE) break;
        if (x == (int)floorf(playerX) && z == (int)floorf(playerZ)) continue;
        int h = topHeight(x, z);
        if (h > 0 && fabsf((float)h - (playerY + 1.0f)) < 2.5f) {
            if (place) {
                if (lastX >= 0 && topHeight(lastX, lastZ) < WORLD_HEIGHT) {
                    int y = topHeight(lastX, lastZ);
                    worlds[activeWorld].blocks[lastX][y][lastZ] = (unsigned char)selectedBlock;
                }
            } else if (h > 0) {
                worlds[activeWorld].blocks[x][h - 1][z] = AIR;
            }
            saveWorlds();
            return;
        }
        lastX = x; lastZ = z;
    }
}

static void showGameHelp(void) {
    clearBottom();
    iprintf("DSi CRAFT - MONDO %d\n", activeWorld + 1);
    iprintf("Prima persona 3D | blocco: %d\n", selectedBlock);
    iprintf("Frecce: cammina / strafing\n");
    iprintf("Y/A: ruota visuale | X/B: guarda su/giu\n");
    iprintf("L: rompi | R: piazza\n");
    iprintf("START: menu | SELECT: scegli mondo\n");
    iprintf("Salvataggio SD: %s\n", saveOk ? "OK" : "non disponibile/errore");
}

static void enterWorld(int slot) {
    activeWorld = slot;
    playerX = 8.5f; playerZ = 8.5f; playerY = (float)topHeight(8, 8);
    yaw = 0.0f; pitch = 0.0f;
    showGameHelp();
}

int main(void) {
    videoSetMode(MODE_5_2D);
    videoSetModeSub(MODE_0_2D);
    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);
    int bg = bgInit(2, BgType_Bmp16, BgSize_B16_256x256, 0, 0);
    framebuffer = (u16 *)bgGetGfxPtr(bg);
    consoleInit(&bottomConsole, 0, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
    consoleSelect(&bottomConsole);
    sdAvailable = fatInitDefault();
    if (sdAvailable) loadWorlds();
    showMenu();

    while (1) {
        scanKeys();
        int down = keysDown();
        int held = keysHeld();
        if (activeWorld < 0) {
            if (down & KEY_UP) selectedWorld = (selectedWorld + MAX_WORLDS - 1) % MAX_WORLDS;
            if (down & KEY_DOWN) selectedWorld = (selectedWorld + 1) % MAX_WORLDS;
            if (down & KEY_A) {
                if (worlds[selectedWorld].used) enterWorld(selectedWorld);
            }
            if (down & KEY_X) {
                int freeSlot = -1;
                for (int i = 0; i < MAX_WORLDS; ++i)
                    if (!worlds[i].used) { freeSlot = i; break; }
                if (freeSlot >= 0) {
                    generateWorld(&worlds[freeSlot]); selectedWorld = freeSlot;
                    saveWorlds(); enterWorld(freeSlot);
                }
            }
            if (down & KEY_START) showMenu();
            if (activeWorld < 0) showMenu();
        } else {
            if (down & KEY_START) { showMenu(); }
            else if (down & KEY_SELECT) { showMenu(); }
            else {
                const float move = 0.10f;
                float forward = 0.0f, strafe = 0.0f;
                if (held & KEY_UP) forward += move;
                if (held & KEY_DOWN) forward -= move;
                if (held & KEY_LEFT) strafe -= move;
                if (held & KEY_RIGHT) strafe += move;
                if (forward != 0.0f || strafe != 0.0f) movePlayer(forward, strafe);
                if (held & KEY_Y) yaw -= 0.045f;
                if (held & KEY_A) yaw += 0.045f;
                if (held & KEY_X) pitch -= 0.025f;
                if (held & KEY_B) pitch += 0.025f;
                if (pitch < -0.75f) pitch = -0.75f;
                if (pitch > 0.75f) pitch = 0.75f;
                if (down & KEY_L) editTarget(false);
                if (down & KEY_R) editTarget(true);
                if (down & KEY_TOUCH) {
                    touchPosition t; touchRead(&t);
                    if (t.px < 256) selectedBlock = (t.px / 52) % 5 + 1;
                }
                renderWorld();
                showGameHelp();
            }
        }
        swiWaitForVBlank();
    }
    return 0;
}

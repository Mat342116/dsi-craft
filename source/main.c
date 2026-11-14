#include <nds.h>
#include <stdio.h>
#include <stdbool.h>

#define WORLD_SIZE 16
#define WORLD_HEIGHT 8

#define AIR 0
#define GRASS 1
#define DIRT 2
#define STONE 3
#define WOOD 4
#define SAND 5

static unsigned char world[WORLD_SIZE][WORLD_HEIGHT][WORLD_SIZE];

static int selectedBlock = GRASS;
static int playerX = 8;
static int playerZ = 8;
static int playerY = 3;

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

void generateWorld(void) {
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            world[x][0][z] = STONE;
            world[x][1][z] = DIRT;
            world[x][2][z] = GRASS;
            for (int y = 3; y < WORLD_HEIGHT; y++) {
                world[x][y][z] = AIR;
            }
        }
    }
}

static void keepPlayerInWorld(void) {
    if (playerX < 0) playerX = 0;
    if (playerX >= WORLD_SIZE) playerX = WORLD_SIZE - 1;
    if (playerZ < 0) playerZ = 0;
    if (playerZ >= WORLD_SIZE) playerZ = WORLD_SIZE - 1;
    if (playerY < 1) playerY = 1;
    if (playerY >= WORLD_HEIGHT) playerY = WORLD_HEIGHT - 1;
}

static void breakBlock(void) {
    if (world[playerX][playerY][playerZ] != AIR) {
        world[playerX][playerY][playerZ] = AIR;
    }
}

static void placeBlock(void) {
    if (world[playerX][playerY][playerZ] == AIR) {
        world[playerX][playerY][playerZ] = (unsigned char)selectedBlock;
    }
}

static void showWorld(void) {
    consoleClear();
    iprintf("          DSi CRAFT\n");
    iprintf("==============================\n");
    iprintf(" MODALITA' CREATIVA - MONDO\n");
    iprintf(" X/Z: %d/%d   ALTEZZA: %d\n\n", playerX, playerZ, playerY);

    iprintf("     ");
    for (int x = 0; x < WORLD_SIZE; x++) iprintf("%X", x);
    iprintf("\n");

    for (int z = 0; z < WORLD_SIZE; z++) {
        iprintf(" %2X  ", z);
        for (int x = 0; x < WORLD_SIZE; x++) {
            if (x == playerX && z == playerZ) {
                iprintf("@");
            } else {
                int y;
                for (y = WORLD_HEIGHT - 1; y >= 0; y--) {
                    if (world[x][y][z] != AIR) break;
                }
                iprintf("%c", y >= 0 ? blockSymbol(world[x][y][z]) : '.');
            }
        }
        iprintf("\n");
    }

    iprintf("\n==============================\n");
    iprintf(" BLOCCO: %d ", selectedBlock);
    iprintf("(1=G 2=D 3=S 4=W 5=A)\n");
    iprintf(" TAP: scegli blocco (parte bassa)\n");
    iprintf(" FRECCE: muovi  X/Y: altezza\n");
    iprintf(" L: rompi blocco   R: piazza\n");
    iprintf(" Solo creativa: niente mob/meteo\n");
}

int main(void) {
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleDemoInit();

    generateWorld();
    showWorld();

    while (1) {
        scanKeys();
        int keys = keysDown();
        bool changed = false;

        if (keys & KEY_UP)    { playerZ--; changed = true; }
        if (keys & KEY_DOWN)  { playerZ++; changed = true; }
        if (keys & KEY_LEFT)  { playerX--; changed = true; }
        if (keys & KEY_RIGHT) { playerX++; changed = true; }

        if (keys & KEY_X) { playerY--; changed = true; }
        if (keys & KEY_Y) { playerY++; changed = true; }

        if (keys & KEY_L) { breakBlock(); changed = true; }
        if (keys & KEY_R) { placeBlock(); changed = true; }

        if (keys & KEY_TOUCH) {
            touchPosition touch;
            touchRead(&touch);
            if (touch.py >= 0 && touch.py < 192) {
                int slot = touch.px / 51;
                if (slot >= 0 && slot < 5) {
                    selectedBlock = slot + 1;
                    changed = true;
                }
            }
        }

        keepPlayerInWorld();
        if (changed) showWorld();

        swiWaitForVBlank();
    }

    return 0;
}


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
static int playerY = 2;

void generateWorld(void) {
    for (int x = 0; x < WORLD_SIZE; x++) {
        for (int z = 0; z < WORLD_SIZE; z++) {
            world[x][0][z] = STONE;
            world[x][1][z] = DIRT;
            world[x][2][z] = GRASS;
        }
    }
}

void showInventory(void) {
    consoleClear();
    iprintf("     DSi CRAFT\n");
    iprintf("====================\n");
    iprintf(" CREATIVE MODE\n\n");
    iprintf(" 1. GRASS\n");
    iprintf(" 2. DIRT\n");
    iprintf(" 3. STONE\n");
    iprintf(" 4. WOOD\n");
    iprintf(" 5. SAND\n");
    iprintf("====================\n");
    iprintf(" BLOCCO: %d\n", selectedBlock);
}

int main(void) {
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleDemoInit();
    consoleInit(
        NULL, 0,
        BgType_Text4bpp,
        BgSize_T_256x256,
        31, 0,
        true, true
    );

    generateWorld();

    while (1) {
        scanKeys();
        int keys = keysDown();

        if (keys & KEY_UP) playerZ--;
        if (keys & KEY_DOWN) playerZ++;
        if (keys & KEY_LEFT) playerX--;
        if (keys & KEY_RIGHT) playerX++;

        if (keys & KEY_X) playerY--;
        if (keys & KEY_Y) playerY++;

        if (keys & KEY_L) {
            iprintf("\nRottura blocco\n");
        }

        if (keys & KEY_R) {
            iprintf("\nPiazzamento blocco\n");
        }

        touchPosition touch;
        touchRead(&touch);

        if (keys & KEY_TOUCH) {
            int slot = touch.px / 51;
            if (slot >= 0 && slot < 5)
                selectedBlock = slot + 1;
        }

        if (playerX < 0) playerX = 0;
        if (playerX >= WORLD_SIZE) playerX = WORLD_SIZE - 1;
        if (playerZ < 0) playerZ = 0;
        if (playerZ >= WORLD_SIZE) playerZ = WORLD_SIZE - 1;

        swiWaitForVBlank();
    }

    return 0;
}

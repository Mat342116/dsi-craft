#include <nds.h>
#include <stdio.h>

#define WORLD_SIZE 16
#define WORLD_HEIGHT 8

#define AIR   0
#define GRASS 1
#define DIRT  2
#define STONE 3
#define WOOD  4
#define SAND  5

static unsigned char world[WORLD_SIZE][WORLD_HEIGHT][WORLD_SIZE];

static PrintConsole topScreen;
static PrintConsole bottomScreen;

static int selectedBlock = GRASS;
static int playerX = WORLD_SIZE / 2;
static int playerZ = WORLD_SIZE / 2;
static int facing = 0; /* 0=nord, 1=est, 2=sud, 3=ovest */

static const char *blockNames[] = {
    "ARIA", "ERBA", "TERRA", "PIETRA", "LEGNO", "SABBIA"
};

static const char blockSymbols[] = {
    ' ', 'G', 'T', 'P', 'L', 'S'
};

static void generateWorld(void)
{
    int x, z;

    for (x = 0; x < WORLD_SIZE; x++) {
        for (z = 0; z < WORLD_SIZE; z++) {
            world[x][0][z] = STONE;
            world[x][1][z] = DIRT;
            world[x][2][z] = GRASS;
        }
    }
}

static int topBlockY(int x, int z)
{
    int y;

    for (y = WORLD_HEIGHT - 1; y >= 0; y--) {
        if (world[x][y][z] != AIR)
            return y;
    }

    return -1;
}

static void getTarget(int *x, int *z)
{
    *x = playerX;
    *z = playerZ;

    if (facing == 0) (*z)--;
    if (facing == 1) (*x)++;
    if (facing == 2) (*z)++;
    if (facing == 3) (*x)--;

    if (*x < 0) *x = 0;
    if (*x >= WORLD_SIZE) *x = WORLD_SIZE - 1;
    if (*z < 0) *z = 0;
    if (*z >= WORLD_SIZE) *z = WORLD_SIZE - 1;
}

static void breakBlock(void)
{
    int x, z;
    int y;

    getTarget(&x, &z);
    y = topBlockY(x, z);

    if (y >= 0)
        world[x][y][z] = AIR;
}

static void placeBlock(void)
{
    int x, z;
    int y;

    getTarget(&x, &z);
    y = topBlockY(x, z) + 1;

    if (y >= 0 && y < WORLD_HEIGHT)
        world[x][y][z] = (unsigned char)selectedBlock;
}

static void renderWorld(void)
{
    int x, z;
    char symbol;

    consoleSelect(&topScreen);
    consoleClear();

    iprintf("          DSI CRAFT\n");
    iprintf("     MONDO - VISTA DALL'ALTO\n");
    iprintf("   +----------------+\n");

    for (z = 0; z < WORLD_SIZE; z++) {
        iprintf("   |");
        for (x = 0; x < WORLD_SIZE; x++) {
            if (x == playerX && z == playerZ) {
                symbol = '@';
            } else {
                int y = topBlockY(x, z);
                symbol = (y < 0) ? '.' : blockSymbols[world[x][y][z]];
            }
            iprintf("%c", symbol);
        }
        iprintf("|\n");
    }

    iprintf("   +----------------+\n");
    iprintf("@ = TU  G=ERBA T=TERRA\n");
    iprintf("P=PIETRA L=LEGNO S=SABBIA\n");
    iprintf("POSIZIONE: X %d  Z %d\n", playerX, playerZ);
    iprintf("BLOCCO: %s\n", blockNames[selectedBlock]);
    iprintf("FRECCЕ = MUOVI  L=ROMPI  R=PIAZZA");
}

static void renderInventory(void)
{
    consoleSelect(&bottomScreen);
    consoleClear();

    iprintf("       INVENTARIO\n");
    iprintf("==============================\n");
    iprintf("TOCCA UNO SLOT PER SELEZIONARE\n\n");

    iprintf("%c 1 ERBA     %c 2 TERRA\n",
            selectedBlock == GRASS ? '>' : ' ',
            selectedBlock == DIRT ? '>' : ' ');
    iprintf("%c 3 PIETRA   %c 4 LEGNO\n",
            selectedBlock == STONE ? '>' : ' ',
            selectedBlock == WOOD ? '>' : ' ');
    iprintf("%c 5 SABBIA\n",
            selectedBlock == SAND ? '>' : ' ');

    iprintf("\n==============================\n");
    iprintf("FRECCE: MUOVI IL PERSONAGGIO\n");
    iprintf("L: ROMPI IL BLOCCO DAVANTI\n");
    iprintf("R: PIAZZA IL BLOCCO SELEZIONATO\n");
    iprintf("A / B: CAMBIA BLOCCO\n");
}

static void movePlayer(int dx, int dz)
{
    if (dx > 0) facing = 1;
    else if (dx < 0) facing = 3;
    else if (dz < 0) facing = 0;
    else if (dz > 0) facing = 2;

    playerX += dx;
    playerZ += dz;

    if (playerX < 0) playerX = 0;
    if (playerX >= WORLD_SIZE) playerX = WORLD_SIZE - 1;
    if (playerZ < 0) playerZ = 0;
    if (playerZ >= WORLD_SIZE) playerZ = WORLD_SIZE - 1;
}

static void selectNextBlock(int direction)
{
    selectedBlock += direction;

    if (selectedBlock > SAND)
        selectedBlock = GRASS;
    if (selectedBlock < GRASS)
        selectedBlock = SAND;
}

int main(void)
{
    videoSetMode(MODE_0_2D);
    videoSetModeSub(MODE_0_2D);

    vramSetBankA(VRAM_A_MAIN_BG);
    vramSetBankC(VRAM_C_SUB_BG);

    consoleInit(&topScreen, 0, BgType_Text4bpp, BgSize_T_256x256,
                31, 0, true, true);
    consoleInit(&bottomScreen, 0, BgType_Text4bpp, BgSize_T_256x256,
                31, 0, false, true);

    generateWorld();

    while (pmMainLoop()) {
        scanKeys();
        {
            int keys = keysDown();

            if (keys & KEY_UP)    movePlayer(0, -1);
            if (keys & KEY_DOWN)  movePlayer(0, 1);
            if (keys & KEY_LEFT)  movePlayer(-1, 0);
            if (keys & KEY_RIGHT) movePlayer(1, 0);

            if (keys & KEY_L) breakBlock();
            if (keys & KEY_R) placeBlock();
            if (keys & KEY_A) selectNextBlock(1);
            if (keys & KEY_B) selectNextBlock(-1);

            if (keys & KEY_TOUCH) {
                touchPosition touch;
                touchRead(&touch);

                if (touch.px < 52) selectedBlock = GRASS;
                else if (touch.px < 103) selectedBlock = DIRT;
                else if (touch.px < 154) selectedBlock = STONE;
                else if (touch.px < 205) selectedBlock = WOOD;
                else selectedBlock = SAND;
            }
        }

        renderWorld();
        renderInventory();
        swiWaitForVBlank();
    }

    return 0;
}

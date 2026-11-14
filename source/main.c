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
       Prima recupera un eventuale backup lasciato da un'interruzione. */
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

    if (hadOldSave) {
        remove(SAVE_BACKUP_PATH);
    }
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
        /* Recupera il backup se la console si è spenta durante il salvataggio. */
        if (rename(SAVE_BACKUP_PATH, SAVE_PATH) == 0)
            file = fopen(SAVE_PATH, "rb");
    }
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


static void drawQuad(float x1,float y1,float z1,float x2,float y2,float z2,
                     float x3,float y3,float z3,float x4,float y4,float z4,
                     int color) {
    glColor(color);
    glBegin(GL_QUADS);
    glVertex3f(x1,y1,z1); glVertex3f(x2,y2,z2);
    glVertex3f(x3,y3,z3); glVertex3f(x4,y4,z4);
    glEnd();
}

static int blockColor(unsigned char block) {
    switch (block) {
        case GRASS: return RGB15(12,25,7);
        case DIRT:  return RGB15(20,11,5);
        case STONE: return RGB15(17,17,19);
        case WOOD:  return RGB15(22,14,5);
        case SAND:  return RGB15(28,25,13);
        default:    return RGB15(31,31,31);
    }
}

static void drawBlock(int x,int y,int z,unsigned char block) {
    float a=(float)x-8.0f, b=(float)y, c=(float)z-8.0f;
    float X=a+1.0f, Y=b+1.0f, Z=c+1.0f;
    int color=blockColor(block);
    World *w=&worlds[activeWorld];
    /* Render only exposed faces, keeping the DS geometry workload modest. */
    if (y==WORLD_HEIGHT-1 || w->blocks[x][y+1][z]==AIR)
        drawQuad(a,Y,c, X,Y,c, X,Y,Z, a,Y,Z,color);
    if (y==0 || w->blocks[x][y-1][z]==AIR)
        drawQuad(a,b,Z, X,b,Z, X,b,c, a,b,c,RGB15(8,8,8));
    if (x==0 || w->blocks[x-1][y][z]==AIR)
        drawQuad(a,b,c, a,Y,c, a,Y,Z, a,b,Z,RGB15(12,12,12));
    if (x==WORLD_SIZE-1 || w->blocks[x+1][y][z]==AIR)
        drawQuad(X,b,Z, X,Y,Z, X,Y,c, X,b,c,RGB15(18,18,18));
    if (z==0 || w->blocks[x][y][z-1]==AIR)
        drawQuad(X,b,c, X,Y,c, a,Y,c, a,b,c,RGB15(15,15,15));
    if (z==WORLD_SIZE-1 || w->blocks[x][y][z+1]==AIR)
        drawQuad(a,b,Z, a,Y,Z, X,Y,Z, X,b,Z,RGB15(21,21,21));
}

static void renderWorld3D(void) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(58.0f,256.0f/192.0f,0.1f,80.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(20.0f,19.0f,23.0f, 0.0f,2.0f,0.0f, 0.0f,1.0f,0.0f);
    glClearColor(12,20,31,31);
    glClearDepth(GL_MAX_DEPTH);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    glPolyFmt(POLY_ALPHA(31)|POLY_CULL_NONE);
    for (int y=0;y<WORLD_HEIGHT;y++) {
        for (int z=0;z<WORLD_SIZE;z++) {
            for (int x=0;x<WORLD_SIZE;x++) {
                unsigned char block=worlds[activeWorld].blocks[x][y][z];
                if (block!=AIR) drawBlock(x,y,z,block);
            }
        }
    }
    /* A bright wireframe cube marks the selected build position. */
    float x=(float)cursorX-7.5f, y=(float)cursorY+0.02f, z=(float)cursorZ-7.5f;
    glColor(RGB15(31,31,0));
    glBegin(GL_LINES);
    glVertex3f(x,y,z); glVertex3f(x+1,y,z);
    glVertex3f(x+1,y,z); glVertex3f(x+1,y,z+1);
    glVertex3f(x+1,y,z+1); glVertex3f(x,y,z+1);
    glVertex3f(x,y,z+1); glVertex3f(x,y,z);
    glVertex3f(x,y+1,z); glVertex3f(x+1,y+1,z);
    glVertex3f(x+1,y+1,z); glVertex3f(x+1,y+1,z+1);
    glVertex3f(x+1,y+1,z+1); glVertex3f(x,y+1,z+1);
    glVertex3f(x,y+1,z+1); glVertex3f(x,y+1,z);
    glVertex3f(x,y,z); glVertex3f(x,y+1,z);
    glVertex3f(x+1,y,z); glVertex3f(x+1,y+1,z);
    glVertex3f(x+1,y,z+1); glVertex3f(x+1,y+1,z+1);
    glVertex3f(x,y,z+1); glVertex3f(x,y+1,z+1);
    glEnd();
    glFlush(0);
}

static void showWorld(void) {
    consoleClear();
    iprintf("DSi CRAFT  |  MONDO %d\n",activeWorld+1);
    iprintf("   MODALITA' CREATIVA 3D\n");
    iprintf("------------------------------\n");
    iprintf("Posizione: X %d  Y %d  Z %d\n",cursorX,cursorY,cursorZ);
    iprintf("Blocco selezionato: %d\n",selectedBlock);
    iprintf("SELECT: cambia blocco\n");
    iprintf("Frecce: muovi  X/Y: altezza\n");
    iprintf("L: rompi blocco  R: piazza\n");
    iprintf("B: menu mondi\n");
    iprintf("------------------------------\n");
    iprintf("SD: %s\n",!sdAvailable?"non disponibile":(saveOk?"salvataggio OK":"errore salvataggio"));
    iprintf("Schermo superiore: mondo 3D\n");
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
                if (keys & KEY_X)     { cursorY--; changed = true; }
                if (keys & KEY_Y)     { cursorY++; changed = true; }
                if (keys & KEY_SELECT) { selectedBlock = selectedBlock % SAND + 1; changed = true; }
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

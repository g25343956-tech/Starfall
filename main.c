/*
 * STARFALL - Shooter horizontal para Game Boy Advance
 *
 * - Sin librerias externas: acceso directo a registros (estilo Tonc).
 * - Todo el arte esta dibujado con texto en este archivo (busca ART_*),
 *   asi que puedes editarlo sin herramientas de graficos.
 *
 * Controles:  Cruceta = mover   A = disparar (mantener = autofuego)
 *             START = pausa / empezar
 */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

/* ------------------------------------------------------------------ */
/* Registros y memoria                                                */
/* ------------------------------------------------------------------ */
#define REG16(a)       (*(volatile u16 *)(a))
#define REG_DISPCNT    REG16(0x04000000)
#define REG_VCOUNT     REG16(0x04000006)
#define REG_BG0CNT     REG16(0x04000008)
#define REG_BG1CNT     REG16(0x0400000A)
#define REG_BG0HOFS    REG16(0x04000010)
#define REG_BG1HOFS    REG16(0x04000014)
#define REG_KEYINPUT   REG16(0x04000130)

#define REG_SNDDMGCNT  REG16(0x04000080)
#define REG_SNDDSCNT   REG16(0x04000082)
#define REG_SNDSTAT    REG16(0x04000084)
#define REG_SND1CNT_L  REG16(0x04000060)
#define REG_SND1CNT_H  REG16(0x04000062)
#define REG_SND1CNT_X  REG16(0x04000064)
#define REG_SND4CNT_L  REG16(0x04000078)
#define REG_SND4CNT_H  REG16(0x0400007C)

#define PAL_BG         ((volatile u16 *)0x05000000)
#define PAL_OBJ        ((volatile u16 *)0x05000200)
#define BG_TILES       ((volatile u32 *)0x06000000)
#define MAP_BG0        ((volatile u16 *)0x0600E000)   /* screenblock 28 */
#define MAP_BG1        ((volatile u16 *)0x0600E800)   /* screenblock 29 */
#define OBJ_TILES      ((volatile u32 *)0x06010000)
#define OAM_MEM        ((volatile u32 *)0x07000000)

#define RGB15(r, g, b) ((r) | ((g) << 5) | ((b) << 10))

#define KEY_A      0x0001
#define KEY_START  0x0008
#define KEY_RIGHT  0x0010
#define KEY_LEFT   0x0020
#define KEY_UP     0x0040
#define KEY_DOWN   0x0080

#define SCREEN_W   240
#define SCREEN_H   160
#define FIELD_TOP  16      /* las 16 primeras lineas son para el HUD */

/* ------------------------------------------------------------------ */
/* Utilidades                                                         */
/* ------------------------------------------------------------------ */
static u32 rng_state = 12345;

static int rnd(int n)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return (int)((rng_state >> 16) % (u32)n);
}

static int clampi(int v, int lo, int hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}

static int overlap(int ax, int ay, int aw, int ah,
                   int bx, int by, int bw, int bh)
{
    return ax < bx + bw && bx < ax + aw && ay < by + bh && by < ay + ah;
}

static const signed char SIN24[32] = {
     0,  5,  9, 13, 17, 20, 22, 24, 24, 24, 22, 20, 17, 13,  9,  5,
     0, -5, -9,-13,-17,-20,-22,-24,-24,-24,-22,-20,-17,-13, -9, -5
};

/* ------------------------------------------------------------------ */
/* Arte (cada caracter es un indice de paleta en hexadecimal; '.' = 0) */
/* Los sprites de 16x16 se definen con la MITAD SUPERIOR (8 filas) y   */
/* se espejan en vertical.                                             */
/*                                                                    */
/* Paleta: 1 blanco  2 azul claro  3 azul  4 rojo  5 naranja           */
/*         6 amarillo 7 gris 8 gris oscuro 9 verde a verde oscuro      */
/*         b cian  c magenta  d magenta oscuro                         */
/* ------------------------------------------------------------------ */
static const char *const ART_SHIP[8] = {
    "................",
    "...3............",
    "...23...........",
    "....233333......",
    "4...2222223333..",
    "54..22222222bb..",
    "654.222222222bb1",
    "654.222222222221"
};

static const char *const ART_ENEMY[8] = {
    "................",
    "......aaaa......",
    "....aa9999aa....",
    "..aa99999999aa..",
    ".a999999999999a.",
    "a9997777777799a.",
    "a99977666677999a",
    "a99977666677999a"
};

static const char *const ART_BOOM0[8] = {
    "................",
    "......6..6......",
    "...6...55...6...",
    "....5.5445.5....",
    "..6..544445..6..",
    "....55466455....",
    "..6.54466645.6..",
    "...554466664455."
};

static const char *const ART_BOOM1[8] = {
    "................",
    "................",
    "....5......5....",
    "..4...5..5...4..",
    "......4..4......",
    "...5....4....5..",
    "..4....5...4....",
    "....4.....5....."
};

static const char *const ART_PSHOT[8] = {
    "........", "........", "........", "66666666",
    "11111111", "........", "........", "........"
};

static const char *const ART_ESHOT[8] = {
    "........", "..4444..", ".455554.", ".456654.",
    ".456654.", ".455554.", "..4444..", "........"
};

/* Fuente 3x5 (se dibuja al doble de tamano con sprites de 8x16) */
static const struct { char c; u8 r[5]; } FONT[] = {
    {'0',{7,5,5,5,7}}, {'1',{2,6,2,2,7}}, {'2',{7,1,7,4,7}},
    {'3',{7,1,7,1,7}}, {'4',{5,5,7,1,1}}, {'5',{7,4,7,1,7}},
    {'6',{7,4,7,5,7}}, {'7',{7,1,2,2,2}}, {'8',{7,5,7,5,7}},
    {'9',{7,5,7,1,7}},
    {'A',{2,5,7,5,5}}, {'C',{7,4,4,4,7}}, {'E',{7,4,7,4,7}},
    {'F',{7,4,7,4,4}}, {'G',{7,4,5,5,7}}, {'H',{5,5,7,5,5}},
    {'I',{7,2,2,2,7}}, {'L',{4,4,4,4,7}}, {'M',{5,7,7,5,5}},
    {'N',{7,5,5,5,5}}, {'O',{7,5,5,5,7}}, {'P',{7,5,7,4,4}},
    {'R',{6,5,6,5,5}}, {'S',{7,4,7,1,7}}, {'T',{7,2,2,2,2}},
    {'U',{5,5,5,5,7}}, {'V',{5,5,5,5,2}}
};
#define N_FONT ((int)(sizeof(FONT) / sizeof(FONT[0])))

/* Posiciones de tiles en la memoria de sprites (unidades de tile 4bpp) */
#define T_SHIP    0     /* 16x16 -> 4 tiles */
#define T_PSHOT   4
#define T_ESHOT   5
#define T_ENEMY   8     /* 16x16 */
#define T_BOOM0   12    /* 16x16 */
#define T_BOOM1   16    /* 16x16 */
#define T_FONT    32    /* 2 tiles por glifo (8x16) */

/* ------------------------------------------------------------------ */
/* Carga de graficos                                                  */
/* ------------------------------------------------------------------ */
static int hexval(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

/* Convierte arte de texto a tiles 4bpp en modo de mapeo 1D. */
static void load_art(const char *const *art, int w, int h, int mirror, int tile)
{
    int tw = w / 8;
    for (int y = 0; y < h; y++) {
        const char *row = art[(mirror && y >= h / 2) ? h - 1 - y : y];
        int len = 0;
        while (row[len]) len++;
        for (int x = 0; x < w && x < len; x++) {
            int c = hexval(row[x]);
            if (c) {
                int t = tile + (y >> 3) * tw + (x >> 3);
                OBJ_TILES[t * 8 + (y & 7)] |= (u32)c << ((x & 7) * 4);
            }
        }
    }
}

static void load_font(void)
{
    for (int g = 0; g < N_FONT; g++) {
        int base = T_FONT + g * 2;
        for (int r = 0; r < 5; r++)
            for (int c = 0; c < 3; c++) {
                if (!(FONT[g].r[r] & (4 >> c))) continue;
                for (int dy = 0; dy < 2; dy++)
                    for (int dx = 0; dx < 2; dx++) {
                        int px = 1 + c * 2 + dx;
                        int py = 3 + r * 2 + dy;
                        int t  = base + (py >> 3);
                        OBJ_TILES[t * 8 + (py & 7)] |= 1u << (px * 4);
                    }
            }
    }
}

static void init_palettes(void)
{
    static const u16 base[16] = {
        0,
        RGB15(31, 31, 31), RGB15(14, 22, 31), RGB15(6, 10, 24),
        RGB15(31, 6, 6),   RGB15(31, 16, 4),  RGB15(31, 28, 6),
        RGB15(20, 20, 22), RGB15(10, 10, 12), RGB15(8, 26, 10),
        RGB15(3, 14, 5),   RGB15(10, 28, 31), RGB15(28, 8, 26),
        RGB15(14, 3, 14),  0, 0
    };
    for (int i = 0; i < 16; i++) {
        PAL_OBJ[i]      = base[i];          /* banco 0: normal            */
        PAL_OBJ[16 + i] = base[i];          /* banco 1: enemigo magenta   */
        PAL_OBJ[32 + i] = base[i];          /* banco 2: texto amarillo    */
    }
    PAL_OBJ[16 + 9]  = base[12];
    PAL_OBJ[16 + 10] = base[13];
    PAL_OBJ[32 + 1]  = RGB15(31, 28, 8);

    PAL_BG[0] = RGB15(0, 0, 3);
    PAL_BG[1] = RGB15(31, 31, 31);
    PAL_BG[2] = RGB15(12, 12, 18);
    PAL_BG[3] = RGB15(20, 20, 28);
}

static void init_background(void)
{
    for (int i = 0; i < 32; i++) BG_TILES[i] = 0;
    BG_TILES[1 * 8 + 3] = 1u << 12;     /* tile 1: estrella brillante */
    BG_TILES[2 * 8 + 6] = 2u << 20;     /* tile 2: estrella tenue     */
    BG_TILES[3 * 8 + 1] = 3u << 4;      /* tile 3: estrella media     */

    for (int i = 0; i < 1024; i++) {
        int r = rnd(100);
        MAP_BG0[i] = (r < 3) ? 2 : (r < 6 ? 3 : 0);   /* capa lejana  */
        r = rnd(100);
        MAP_BG1[i] = (r < 3) ? 1 : 0;                 /* capa cercana */
    }
    REG_BG0CNT = 2 | (28 << 8);
    REG_BG1CNT = 1 | (29 << 8);
}

/* ------------------------------------------------------------------ */
/* Sprites (buffer en RAM que se copia a la OAM en VBlank)             */
/* ------------------------------------------------------------------ */
typedef struct { u16 a0, a1, a2, pad; } Obj;
static Obj shadow[128] __attribute__((aligned(4)));
static int nspr;

#define SHAPE_SQUARE 0
#define SHAPE_TALL   2

static void spr(int x, int y, int shape, int size, int tile, int pal)
{
    if (nspr >= 128) return;
    if (x <= -16 || x >= SCREEN_W || y <= -16 || y >= SCREEN_H) return;
    Obj *o = &shadow[nspr++];
    o->a0 = (u16)((y & 0xFF) | (shape << 14));
    o->a1 = (u16)((x & 0x1FF) | (size << 14));
    o->a2 = (u16)(tile | (pal << 12));
}

static void oam_flush(void)
{
    for (int i = nspr; i < 128; i++) { shadow[i].a0 = 0x0200; shadow[i].a1 = 0; shadow[i].a2 = 0; }
    const u32 *src = (const u32 *)shadow;
    for (int i = 0; i < 256; i++) OAM_MEM[i] = src[i];
}

/* ------------------------------------------------------------------ */
/* Texto y numeros                                                    */
/* ------------------------------------------------------------------ */
static int glyph_index(char c)
{
    for (int i = 0; i < N_FONT; i++) if (FONT[i].c == c) return i;
    return -1;
}

static void draw_text(int x, int y, const char *s)
{
    for (; *s; s++, x += 8) {
        if (*s == ' ') continue;
        int g = glyph_index(*s);
        if (g >= 0) spr(x, y, SHAPE_TALL, 0, T_FONT + g * 2, 2);
    }
}

static void draw_num(int x, int y, int v, int digits)
{
    for (int i = digits - 1; i >= 0; i--) {
        int d = v % 10;
        v /= 10;
        spr(x + i * 8, y, SHAPE_TALL, 0, T_FONT + d * 2, 2);
    }
}

/* ------------------------------------------------------------------ */
/* Sonido (canal 1 = disparo, canal 4 = ruido para explosiones)       */
/* ------------------------------------------------------------------ */
static void snd_init(void)
{
    REG_SNDSTAT   = 0x0080;
    REG_SNDDMGCNT = 0xFF77;
    REG_SNDDSCNT  = 0x0002;
}

static void snd_shot(void)
{
    REG_SND1CNT_L = 0x0000;
    REG_SND1CNT_H = 0x6180;                 /* duty 50%, vol 6, decae */
    REG_SND1CNT_X = 0x8000 | 1950;          /* reinicia el canal      */
}

static void snd_boom(void)
{
    REG_SND4CNT_L = 0xC200;
    REG_SND4CNT_H = 0x8000 | (4 << 4);
}

static void snd_hit(void)
{
    REG_SND4CNT_L = 0xF500;
    REG_SND4CNT_H = 0x8000 | (6 << 4);
}

/* ------------------------------------------------------------------ */
/* Estado del juego                                                   */
/* ------------------------------------------------------------------ */
#define MAX_SHOTS   10
#define MAX_ENEMIES 8
#define MAX_ESHOTS  12
#define MAX_BOOMS   8

typedef struct { int x, y, on; } Shot;
typedef struct { int x, y, on, type, hp, t, by, cd; } Enemy;
typedef struct { int x, y, vx, vy, on; } EShot;     /* x,y en punto fijo 8.8 */
typedef struct { int x, y, t, on; } Boom;

enum { ST_TITLE, ST_PLAY, ST_OVER };

static int state = ST_TITLE;
static int frame;
static int px, py, lives, inv, cd;
static int score, hi, level, kills, spawn_t, paused, over_t;

static Shot  shots[MAX_SHOTS];
static Enemy enemies[MAX_ENEMIES];
static EShot eshots[MAX_ESHOTS];
static Boom  booms[MAX_BOOMS];

static void add_boom(int x, int y)
{
    for (int i = 0; i < MAX_BOOMS; i++)
        if (!booms[i].on) { booms[i].x = x; booms[i].y = y; booms[i].t = 0; booms[i].on = 1; return; }
}

static void reset_game(void)
{
    for (int i = 0; i < MAX_SHOTS;   i++) shots[i].on = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].on = 0;
    for (int i = 0; i < MAX_ESHOTS;  i++) eshots[i].on = 0;
    for (int i = 0; i < MAX_BOOMS;   i++) booms[i].on = 0;
    px = 24; py = 72;
    lives = 3; inv = 90; cd = 0;
    score = 0; level = 1; kills = 0;
    spawn_t = 60; paused = 0; over_t = 0;
}

static void add_score(int pts)
{
    score += pts;
    if (score > 999999) score = 999999;
    if (score > hi) hi = score;
}

static void hurt_player(void)
{
    add_boom(px, py);
    snd_hit();
    lives--;
    inv = 120;
    if (lives <= 0) {
        lives = 0;
        state = ST_OVER;
        over_t = 0;
    }
}

static void spawn_enemy(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &enemies[i];
        if (e->on) continue;
        e->on = 1;
        e->x = SCREEN_W;
        e->t = 0;
        e->cd = 40 + rnd(80);
        if (level >= 2 && rnd(3) == 0) {
            e->type = 1; e->hp = 2;
            e->by = 40 + rnd(81);
            e->y = e->by;
        } else {
            e->type = 0; e->hp = 1;
            e->y = FIELD_TOP + rnd(129);
            e->by = e->y;
        }
        return;
    }
}

static void enemy_fire(const Enemy *e)
{
    for (int i = 0; i < MAX_ESHOTS; i++) {
        EShot *s = &eshots[i];
        if (s->on) continue;
        int sx = e->x, sy = e->y + 4;
        int dx = px + 4 - sx, dy = py + 4 - sy;
        if (dx > -8) { dx = -100; dy = 0; }          /* jugador detras: recto */
        int adx = dx < 0 ? -dx : dx, ady = dy < 0 ? -dy : dy;
        int dist = adx > ady ? adx + ady / 2 : ady + adx / 2;
        s->x  = sx << 8;
        s->y  = sy << 8;
        s->vx = dx * 384 / dist;
        s->vy = dy * 384 / dist;
        s->on = 1;
        return;
    }
}

static void update_booms(void)
{
    for (int i = 0; i < MAX_BOOMS; i++)
        if (booms[i].on && ++booms[i].t >= 16) booms[i].on = 0;
}

static void update_play(u16 held)
{
    if (held & KEY_LEFT)  px -= 2;
    if (held & KEY_RIGHT) px += 2;
    if (held & KEY_UP)    py -= 2;
    if (held & KEY_DOWN)  py += 2;
    px = clampi(px, 0, SCREEN_W - 16);
    py = clampi(py, FIELD_TOP, SCREEN_H - 16);

    if (cd > 0) cd--;
    if ((held & KEY_A) && cd == 0) {
        for (int i = 0; i < MAX_SHOTS; i++)
            if (!shots[i].on) {
                shots[i].on = 1; shots[i].x = px + 14; shots[i].y = py + 4;
                break;
            }
        cd = 7;
        snd_shot();
    }
    if (inv > 0) inv--;

    for (int i = 0; i < MAX_SHOTS; i++)
        if (shots[i].on) {
            shots[i].x += 5;
            if (shots[i].x >= SCREEN_W) shots[i].on = 0;
        }

    if (--spawn_t <= 0) {
        spawn_enemy();
        int base = 80 - level * 5;
        spawn_t = (base < 22 ? 22 : base) + rnd(25);
    }

    int spd0 = clampi(1 + level / 3, 1, 3);
    int spd1 = clampi(1 + level / 8, 1, 2);

    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy *e = &enemies[i];
        if (!e->on) continue;
        e->t++;
        if (e->type == 0) {
            e->x -= spd0;
        } else {
            e->x -= spd1;
            e->y = e->by + SIN24[(e->t >> 1) & 31];
        }
        if (e->x < -16) { e->on = 0; continue; }

        if (level >= 3 && e->x > px + 24 && e->x < SCREEN_W - 16) {
            if (--e->cd <= 0) {
                enemy_fire(e);
                e->cd = clampi(110 - level * 4, 45, 110) + rnd(40);
            }
        }

        for (int j = 0; j < MAX_SHOTS; j++) {
            Shot *s = &shots[j];
            if (!s->on) continue;
            if (overlap(s->x, s->y + 2, 8, 4, e->x + 1, e->y + 2, 14, 12)) {
                s->on = 0;
                if (--e->hp <= 0) {
                    e->on = 0;
                    add_boom(e->x, e->y);
                    snd_boom();
                    add_score(e->type ? 200 : 100);
                    kills++;
                    level = clampi(1 + kills / 10, 1, 15);
                    break;
                }
            }
        }
        if (!e->on) continue;

        if (inv == 0 && lives > 0 &&
            overlap(px + 3, py + 4, 10, 8, e->x + 2, e->y + 3, 12, 10)) {
            e->on = 0;
            add_boom(e->x, e->y);
            hurt_player();
        }
    }

    for (int i = 0; i < MAX_ESHOTS; i++) {
        EShot *s = &eshots[i];
        if (!s->on) continue;
        s->x += s->vx;
        s->y += s->vy;
        int ex = s->x >> 8, ey = s->y >> 8;
        if (ex < -8 || ey < FIELD_TOP - 8 || ey > SCREEN_H) { s->on = 0; continue; }
        if (inv == 0 && lives > 0 &&
            overlap(px + 4, py + 5, 8, 6, ex + 2, ey + 2, 4, 4)) {
            s->on = 0;
            hurt_player();
        }
    }
    update_booms();
}

/* ------------------------------------------------------------------ */
/* Dibujo                                                             */
/* ------------------------------------------------------------------ */
static void draw_world(void)
{
    for (int i = 0; i < MAX_ENEMIES; i++)
        if (enemies[i].on) spr(enemies[i].x, enemies[i].y, SHAPE_SQUARE, 1, T_ENEMY, enemies[i].type);
    for (int i = 0; i < MAX_SHOTS; i++)
        if (shots[i].on) spr(shots[i].x, shots[i].y, SHAPE_SQUARE, 0, T_PSHOT, 0);
    for (int i = 0; i < MAX_ESHOTS; i++)
        if (eshots[i].on) spr(eshots[i].x >> 8, eshots[i].y >> 8, SHAPE_SQUARE, 0, T_ESHOT, 0);
    for (int i = 0; i < MAX_BOOMS; i++)
        if (booms[i].on) spr(booms[i].x, booms[i].y, SHAPE_SQUARE, 1, booms[i].t < 8 ? T_BOOM0 : T_BOOM1, 0);
    if (state == ST_PLAY && (inv == 0 || ((inv >> 2) & 1) == 0))
        spr(px, py, SHAPE_SQUARE, 1, T_SHIP, 0);
}

static void draw_hud(void)
{
    draw_text(4, 0, "SCORE");
    draw_num(48, 0, score, 6);
    draw_text(108, 0, "LV");
    draw_num(128, 0, level, 2);
    for (int i = 0; i < lives; i++)
        spr(SCREEN_W - 18 * (i + 1), 0, SHAPE_SQUARE, 1, T_SHIP, 0);
}

/* ------------------------------------------------------------------ */
/* Inicializacion y bucle principal                                    */
/* ------------------------------------------------------------------ */
static void init(void)
{
    REG_DISPCNT = 0x0080;                       /* pantalla apagada mientras cargamos */
    init_palettes();
    for (int i = 0; i < 100 * 8; i++) OBJ_TILES[i] = 0;
    load_art(ART_SHIP,  16, 16, 1, T_SHIP);
    load_art(ART_ENEMY, 16, 16, 1, T_ENEMY);
    load_art(ART_BOOM0, 16, 16, 1, T_BOOM0);
    load_art(ART_BOOM1, 16, 16, 1, T_BOOM1);
    load_art(ART_PSHOT, 8, 8, 0, T_PSHOT);
    load_art(ART_ESHOT, 8, 8, 0, T_ESHOT);
    load_font();
    init_background();
    snd_init();
    REG_DISPCNT = 0x1340;                       /* modo 0, BG0+BG1+OBJ, mapeo 1D */
}

static void vsync(void)
{
    while (REG_VCOUNT >= 160) { }
    while (REG_VCOUNT <  160) { }
}

int main(void)
{
    u16 prev = 0;
    init();

    for (;;) {
        vsync();
        oam_flush();
        frame++;
        REG_BG1HOFS = (u16)frame;
        REG_BG0HOFS = (u16)(frame >> 1);

        u16 now = (u16)(~REG_KEYINPUT & 0x03FF);
        u16 pressed = (u16)(now & ~prev);
        prev = now;
        nspr = 0;

        switch (state) {
        case ST_TITLE:
            rng_state += (u32)frame;                       /* semilla variable */
            draw_text(88, 40, "STARFALL");
            spr(112, 66 + SIN24[(frame >> 2) & 31] / 4, SHAPE_SQUARE, 1, T_SHIP, 0);
            if ((frame >> 5) & 1) draw_text(76, 96, "PRESS START");
            draw_text(84, 124, "HI");
            draw_num(100, 124, hi, 6);
            if (pressed & KEY_START) { reset_game(); state = ST_PLAY; }
            break;

        case ST_PLAY:
            if (pressed & KEY_START) paused ^= 1;
            if (!paused) update_play(now);
            draw_world();
            draw_hud();
            if (paused) draw_text(100, 70, "PAUSE");
            break;

        case ST_OVER:
            update_booms();
            over_t++;
            draw_world();
            draw_hud();
            draw_text(84, 50, "GAME OVER");
            draw_text(72, 74, "SCORE");
            draw_num(116, 74, score, 6);
            draw_text(84, 92, "HI");
            draw_num(100, 92, hi, 6);
            if (over_t > 45) {
                if ((frame >> 5) & 1) draw_text(76, 120, "PRESS START");
                if (pressed & KEY_START) { reset_game(); state = ST_PLAY; }
            }
            break;
        }
    }
    return 0;
}

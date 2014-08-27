#ifndef __TETRIS_H__
#define __TETRIS_H__

#include <stdio.h>

#define MAP_X_MAX   22
#define MAP_Y_MAX   12
#define MAP_SPACE   0x0
#define MAP_POINT   0x1
#define MAP_WALL    0xFF

/*
 * 1100
 * 1100
 * 0000
 * 0000
 */
#define T_O_VAL_0   0xcc00

/*
 * 1111     1000
 * 0000     1000
 * 0000     1000
 * 0000     1000
 */
#define T_I_VAL_0   0xf000
#define T_I_VAL_1   0x8888

/*
 * 1000     0110
 * 1100     1100
 * 0100     0000
 * 0000     0000
 */
#define T_S_VAL_0   0x8c40
#define T_S_VAL_1   0x6c00

/*
 * 0100     1100
 * 1100     0110
 * 1000     0000
 * 0000     0000
 */
#define T_Z_VAL_0   0x4c80
#define T_Z_VAL_1   0xc600

/*
 * 1000     1100    1110    0100
 * 1110     1000    0010    0100
 * 0000     1000    0000    1100
 * 0000     0000    0000    0000
 */
#define T_J_VAL_0   0x8e00
#define T_J_VAL_1   0xc880
#define T_J_VAL_2   0xe200
#define T_J_VAL_3   0x44c0

/*
 * 0010     1000    1110    1100
 * 1110     1000    1000    0100
 * 0000     1100    0000    0100
 * 0000     0000    0000    0000
 */
#define T_L_VAL_0   0x2e00
#define T_L_VAL_1   0x88c0
#define T_L_VAL_2   0xe800
#define T_L_VAL_3   0xc440

/*
 * 0100     1000    1110    0100
 * 1110     1100    0100    1100
 * 0000     1000    0000    0100
 * 0000     0000    0000    0000
 */
#define T_T_VAL_0   0x4e00
#define T_T_VAL_1   0x8c80
#define T_T_VAL_2   0xe400
#define T_T_VAL_3   0x4c40

typedef enum pattern_e {
    TETRIS_O = 0,
    TETRIS_I,
    TETRIS_S,
    TETRIS_Z,
    TETRIS_J,
    TETRIS_L,
    TETRIS_T,

    TETRIS_PATTERN_MAX,
} pattern_t;

typedef struct figure_s {
    pattern_t type;
    unsigned short val;
    unsigned char x, y;
} figure_t;

typedef enum movement_e {
    ROTATE = 0,
    DOWN,
    LEFT,
    RIGHT,
    UNKNOWN,
} movement_t;

typedef enum stage_e {
    STAGE_INIT,
    STAGE_FALLING,
    STAGE_CALC_POINT,
} stage_t;

#define BUG() \
    do { \
        fprintf(stderr, "BUG: %s <%d>\n", __func__, __LINE__); \
        exit(-1); \
    } while (0)

#endif /* __TETRIS_H__ */


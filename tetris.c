#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <curses.h>

#include "tetris.h"

static unsigned short g_pattern_val[TETRIS_PATTERN_MAX][5] = {
    { T_O_VAL_0, 0, 0, 0, 0 },
    { T_I_VAL_0, T_I_VAL_1, 0, 0, 0 },
    { T_S_VAL_0, T_S_VAL_1, 0, 0, 0 },
    { T_Z_VAL_0, T_Z_VAL_1, 0, 0, 0 },
    { T_J_VAL_0, T_J_VAL_1, T_J_VAL_2, T_J_VAL_3, 0 },
    { T_L_VAL_0, T_L_VAL_1, T_L_VAL_2, T_L_VAL_3, 0 },
    { T_T_VAL_0, T_T_VAL_1, T_T_VAL_2, T_T_VAL_3, 0 },
};

static unsigned char g_map[MAP_X_MAX][MAP_Y_MAX];
static figure_t g_curr_figure = {0};
static figure_t g_new_figure = {0};
static stage_t g_curr_stage = STAGE_INIT;
static volatile int g_tetris_point = 0;

static unsigned char empty_row[MAP_Y_MAX] = {MAP_WALL,
                                             MAP_SPACE,
                                             MAP_SPACE,
                                             MAP_SPACE,
                                             MAP_SPACE,
                                             MAP_SPACE,
                                             MAP_SPACE,
                                             MAP_SPACE,
                                             MAP_SPACE,
                                             MAP_SPACE,
                                             MAP_SPACE,
                                             MAP_WALL,};

static unsigned char point_row[MAP_Y_MAX] = {MAP_WALL,
                                             MAP_POINT,
                                             MAP_POINT,
                                             MAP_POINT,
                                             MAP_POINT,
                                             MAP_POINT,
                                             MAP_POINT,
                                             MAP_POINT,
                                             MAP_POINT,
                                             MAP_POINT,
                                             MAP_POINT,
                                             MAP_WALL,};

static unsigned char wall_row[MAP_Y_MAX] = {MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,
                                            MAP_WALL,};

static void gen_new_figure()
{
    int i, j;

    i = rand() % TETRIS_PATTERN_MAX;
    switch (i) {
    case TETRIS_O:
        j = 0;
        break;
    case TETRIS_I:
    case TETRIS_S:
    case TETRIS_Z:
        j = rand() % 2;
        break;
    default:
        j = rand() % 4;
        break;
    }

    g_new_figure.type = i;
    g_new_figure.val = g_pattern_val[i][j];
    g_new_figure.x = 1;
    g_new_figure.y = MAP_Y_MAX + 1;
}

static unsigned short get_rotate_val(figure_t *fig)
{
    int i;

    if (fig == NULL
         || fig->type >= TETRIS_PATTERN_MAX
         || fig->type < 0) {
        BUG();
        return 0;
    }

    for (i = 0; i < 4; i++) {
        if (g_pattern_val[fig->type][i] == fig->val) {
            break;
        }
    }
    if (i == 4) {
        BUG();
        return 0;
    }

    i++;
    if (g_pattern_val[fig->type][i] == 0) {
        return g_pattern_val[fig->type][0];
    } else {
        return g_pattern_val[fig->type][i];
    }
}

static void gen_next_figure(figure_t *orig, movement_t move, figure_t *next)
{
    if (orig == NULL || next == NULL) {
        BUG();
        return;
    }

    memcpy(next, orig, sizeof(figure_t));
    switch (move) {
    case ROTATE:
        next->val = get_rotate_val(next);
        break;
    case DOWN:
        next->x = next->x + 1;
        break;
    case LEFT:
        next->y = next->y - 1;
        break;
    case RIGHT:
        next->y = next->y + 1;
        break;
    default:
        BUG();
        break;
    }

    return;
}

/* 允许将figure放置到地图上 */
static int permit_placement(figure_t *fig)
{
    unsigned char hex_seg;
    int step;

    if (fig == NULL) {
        BUG();
        return 0;
    }

    for (step = 0; step < 4; step++) {
        hex_seg = (fig->val >> ((3 - step) << 2)) & 0xF;
        if ((g_map[fig->x + step][fig->y] && (hex_seg & 0x8))
             || (g_map[fig->x + step][fig->y + 1] && (hex_seg & 0x4))
             || (g_map[fig->x + step][fig->y + 2] && (hex_seg & 0x2))
             || (g_map[fig->x + step][fig->y + 3] && (hex_seg & 0x1))) {
            return 0;
        }
    }

    return 1;
}

/* 如果figure不能再DOWN，则视当前figure已到生命尽头，否则继续执行move */
static int permit_movement(figure_t *fig)
{
    figure_t tmp_fig;

    gen_next_figure(fig, DOWN, &tmp_fig);

    return permit_placement(&tmp_fig);
}

static void do_placement(figure_t *fig)
{
    unsigned char hex_seg;
    int step, digit;

    if (fig == NULL) {
        BUG();
        return;
    }

    for (step = 0; step < 4; step++) {
        hex_seg = (fig->val >> ((3 - step) << 2)) & 0xF;
        for (digit = 0; digit < 4; digit++) {
            if (hex_seg & (0x1 << digit)) {
                g_map[fig->x + step][fig->y + 3 - digit] = MAP_POINT;
            }
        }
    }
}

static int is_point_row(unsigned char (*row)[MAP_Y_MAX])
{
    if (row == NULL) {
        BUG();
        return 0;
    }
    if (memcmp(row, point_row, MAP_Y_MAX) == 0) {
        return 1;
    } else {
        return 0;
    }
}

static int is_empty_row(unsigned char (*row)[MAP_Y_MAX])
{
    if (row == NULL) {
        BUG();
        return 0;
    }
    if (memcmp(row, empty_row, MAP_Y_MAX) == 0) {
        return 1;
    } else {
        return 0;
    }
}

static int is_wall_row(unsigned char (*row)[MAP_Y_MAX])
{
    if (row == NULL) {
        BUG();
        return 0;
    }
    if (memcmp(row, wall_row, MAP_Y_MAX) == 0) {
        return 1;
    } else {
        return 0;
    }
}

/* 返回值为获取的分数 */
static int calc_point_and_update_map()
{
    int point_row_cnt = 0;
    unsigned char (*row)[MAP_Y_MAX];
    unsigned char (*tmp_row)[MAP_Y_MAX];

    for (row = &g_map[MAP_X_MAX - 2]; !is_empty_row(row) && !is_wall_row(row); row--) {
        if (is_point_row(row)) {
            point_row_cnt++;
        }
    }

    if (point_row_cnt == 0) {
        return 0;
    }

    for (row = &g_map[MAP_X_MAX - 2]; !is_empty_row(row) && !is_wall_row(row); ) {
        if (is_point_row(row)) {
            for (tmp_row = row; !is_wall_row(tmp_row - 1); tmp_row--) {
                memcpy(tmp_row, tmp_row - 1, MAP_Y_MAX);
            }
            memcpy(tmp_row, empty_row, MAP_Y_MAX);
        } else {
            row--;
        }
    }

    return point_row_cnt;
}

static void draw_curr_figure()
{
    unsigned char hex_seg;
    int step, digit;

    if (g_curr_figure.val == 0) {
        return;
    }

    for (step = 0; step < 4; step++) {
        hex_seg = (g_curr_figure.val >> ((3 - step) << 2)) & 0xF;
        for (digit = 0; digit < 4; digit++) {
            if (hex_seg & (0x1 << digit)) {
                attron(A_REVERSE);
                mvaddch(g_curr_figure.x + step, (g_curr_figure.y + 3 - digit) * 2, ' ');
                mvaddch(g_curr_figure.x + step, (g_curr_figure.y + 3 - digit) * 2 + 1, ' ');
                attroff(A_REVERSE);
            }
        }
    }
}

static void draw_new_figure()
{
    unsigned char hex_seg;
    int step, digit;

    if (g_new_figure.val == 0) {
        return;
    }

    for (step = 0; step < 4; step++) {
        hex_seg = (g_new_figure.val >> ((3 - step) << 2)) & 0xF;
        for (digit = 0; digit < 4; digit++) {
            if (hex_seg & (0x1 << digit)) {
                attron(A_REVERSE);
                mvaddch(g_new_figure.x + step, (g_new_figure.y + 3 - digit) * 2, ' ');
                mvaddch(g_new_figure.x + step, (g_new_figure.y + 3 - digit) * 2 + 1, ' ');
                attroff(A_REVERSE);
            } else {
                mvaddch(g_new_figure.x + step, (g_new_figure.y + 3 - digit) * 2, ' ');
                mvaddch(g_new_figure.x + step, (g_new_figure.y + 3 - digit) * 2 + 1, ' ');
            }
        }
    }
}

static void draw_point()
{
    char buf[32];

    bzero(buf, sizeof(buf));
    sprintf(buf, "Point:%-4d", g_tetris_point);

    mvaddstr(MAP_X_MAX + 1, 1, buf);
}

static void draw_map()
{
    int i, j;

    for (i = 0; i < MAP_X_MAX; i++) {
        for (j = 0; j < MAP_Y_MAX; j++) {
            switch (g_map[i][j]) {
            case MAP_SPACE:
                mvaddch(i, 2 * j, ' ');
                mvaddch(i, 2 * j + 1, ' ');
                break;
            case MAP_POINT:
                attron(A_REVERSE);
                mvaddch(i, 2 * j, ' ');
                mvaddch(i, 2 * j + 1, ' ');
                attroff(A_REVERSE);
                break;
            case MAP_WALL:
                attron(A_REVERSE);
                mvaddch(i, 2 * j, ' ');
                mvaddch(i, 2 * j + 1, ' ');
                attroff(A_REVERSE);
                break;
            }
        }
    }

    draw_curr_figure();
    draw_new_figure();
    draw_point();
    refresh();

    return;
}

static void init_map()
{
    int x, y;

    for (x = 0; x < MAP_X_MAX; x++) {
        for (y = 0; y < MAP_Y_MAX; y++) {
            if (x == 0 || y == 0
                 || x == (MAP_X_MAX - 1) || y == (MAP_Y_MAX - 1)) {
                g_map[x][y] = MAP_WALL;
            } else {
                g_map[x][y] = MAP_SPACE;
            }
        }
    }
}

static movement_t get_command()
{
    movement_t ret;
    int ch;

    ch = getch();

    switch (ch) {
    case KEY_UP:
        ret = ROTATE;
        break;
    case KEY_DOWN:
        ret = DOWN;
        break;
    case KEY_RIGHT:
        ret = RIGHT;
        break;
    case KEY_LEFT:
        ret = LEFT;
        break;
    case 27:
        endwin();
        exit(-1);
        break;
    default:
        ret = UNKNOWN;
        break;
    }

    return ret;
}

static void exec_proc(int level)
{
    fd_set read_set;
    int maxfd;
    long usec;
    struct timeval tv;
    int ret;
    movement_t move;
    figure_t tmp_fig;

    if (level < 0 || level > 9) {
        BUG();
        return;
    }

    usec = 200000 * (10 - level);

    while (1) {
        draw_map();
        FD_ZERO(&read_set);
        FD_SET(STDIN_FILENO, &read_set);
        maxfd = STDIN_FILENO;

        bzero(&tv, sizeof(tv));
        tv.tv_sec = usec / 1000000;
        tv.tv_usec = usec % 1000000;

        ret = select(maxfd + 1, &read_set, NULL, NULL, &tv);
        if (ret < 0) {
            fprintf(stderr, "Select %d \r\n", ret);
            return;
        }

        if (ret == 0) {
            /* 超时，自动执行DOWN操作 */
            move = DOWN;
        } else {
            if (FD_ISSET(STDIN_FILENO, &read_set)) {
                move = get_command();
                if (move == UNKNOWN) {
                    continue;
                }
            }
        }

again:
        draw_map();
        switch (g_curr_stage) {
        case STAGE_INIT:
            memcpy(&g_curr_figure, &g_new_figure, sizeof(figure_t));
            gen_new_figure();
            g_curr_figure.x = 1;
            g_curr_figure.y = 4;
            if (!permit_placement(&g_curr_figure)) {
                /* 新生成的图案已不能放置在地图上，结束 */
                goto game_over;
            }
            g_curr_stage = STAGE_FALLING;
            break;
        case STAGE_FALLING:
            if (!permit_movement(&g_curr_figure)) {
                /* 不能移动时，将图案放置在地图上 */
                do_placement(&g_curr_figure);
                g_curr_stage = STAGE_CALC_POINT;
                goto again;
            } else {
                gen_next_figure(&g_curr_figure, move, &tmp_fig);
                if (permit_placement(&tmp_fig)) {
                    memcpy(&g_curr_figure, &tmp_fig, sizeof(figure_t));
                }
            }
            break;
        case STAGE_CALC_POINT:
            g_tetris_point = g_tetris_point + calc_point_and_update_map();
            g_curr_stage = STAGE_INIT;
            goto again;
            break;
        default:
            goto game_over;
            break;
        }
    }

game_over:
    draw_map();
    fprintf(stderr, "Game over.\r\n");
}

static void print_usage(const char *cmd)
{
    printf("Usage: %s <level>\n"
           "  Level [0, 9], and default level is 0\n", cmd);
}

void init_win()
{
    initscr();
    cbreak();
    nonl();
    noecho();
    intrflush(stdscr,false);
    keypad(stdscr,true);
    refresh();
}

int main(int argc, char *argv[])
{
    int level;

    if (argc == 1) {
        level = 0;
    } else if (argc == 2) {
        level = atoi(argv[1]);
        if (level < 0 || level > 9) {
            print_usage(argv[0]);
            return -1;
        }
    } else {
        print_usage(argv[0]);
        return -1;
    }

    srand(time(NULL));

    init_win();
    init_map();
    gen_new_figure();
    g_curr_stage = STAGE_INIT;
    draw_map();

    exec_proc(level);

    endwin();

    return 0;
}



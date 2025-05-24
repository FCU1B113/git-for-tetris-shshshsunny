#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <windows.h>

#define CANVAS_WIDTH 10
#define CANVAS_HEIGHT 20

#define FALL_DELAY 500
#define RENDER_DELAY 100

#define LEFT_KEY 0x25
#define RIGHT_KEY 0x27
#define ROTATE_KEY 0x26
#define DOWN_KEY 0x28
#define FALL_KEY 0x20
#define PAUSE_KEY 0x50

#define LEFT_FUNC() GetAsyncKeyState(LEFT_KEY) & 0x8000
#define RIGHT_FUNC() GetAsyncKeyState(RIGHT_KEY) & 0x8000
#define ROTATE_FUNC() GetAsyncKeyState(ROTATE_KEY) & 0x8000
#define DOWN_FUNC() GetAsyncKeyState(DOWN_KEY) & 0x8000
#define FALL_FUNC() GetAsyncKeyState(FALL_KEY) & 0x8000
#define PAUSE_FUNC() GetAsyncKeyState(PAUSE_KEY) & 0x8000

typedef enum { RED = 41, GREEN, YELLOW, BLUE, PURPLE, CYAN, WHITE, BLACK = 0 } Color;
typedef enum { EMPTY = -1, I, J, L, O, S, T, Z } ShapeId;

typedef struct {
    ShapeId shape;
    Color color;
    int size;
    char rotates[4][4][4];
} Shape;

typedef struct {
    int x, y, score, rotate, fallTime, combo;
    ShapeId queue[4];
    bool paused;
} State;

typedef struct {
    Color color;
    ShapeId shape;
    bool current;
    bool ghost;
} Block;

Shape shapes[7] = {
    {.shape = I, .color = CYAN, .size = 4, .rotates = {{{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},{{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},{{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},{{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}}},
    {.shape = J, .color = BLUE, .size = 3, .rotates = {{{1,0,0},{1,1,1},{0,0,0}},{{0,1,1},{0,1,0},{0,1,0}},{{0,0,0},{1,1,1},{0,0,1}},{{0,1,0},{0,1,0},{1,1,0}}}},
    {.shape = L, .color = YELLOW, .size = 3, .rotates = {{{0,0,1},{1,1,1},{0,0,0}},{{0,1,0},{0,1,0},{0,1,1}},{{0,0,0},{1,1,1},{1,0,0}},{{1,1,0},{0,1,0},{0,1,0}}}},
    {.shape = O, .color = WHITE, .size = 2, .rotates = {{{1,1},{1,1}},{{1,1},{1,1}},{{1,1},{1,1}},{{1,1},{1,1}}}},
    {.shape = S, .color = GREEN, .size = 3, .rotates = {{{0,1,1},{1,1,0},{0,0,0}},{{0,1,0},{0,1,1},{0,0,1}},{{0,0,0},{0,1,1},{1,1,0}},{{1,0,0},{1,1,0},{0,1,0}}}},
    {.shape = T, .color = PURPLE, .size = 3, .rotates = {{{0,1,0},{1,1,1},{0,0,0}},{{0,1,0},{0,1,1},{0,1,0}},{{0,0,0},{1,1,1},{0,1,0}},{{0,1,0},{1,1,0},{0,1,0}}}},
    {.shape = Z, .color = RED, .size = 3, .rotates = {{{1,1,0},{0,1,1},{0,0,0}},{{0,0,1},{0,1,1},{0,1,0}},{{0,0,0},{1,1,0},{0,1,1}},{{0,1,0},{1,1,0},{1,0,0}}}}
};

void setBlock(Block* block, Color color, ShapeId shape, bool current, bool ghost) {
    block->color = ghost ? 100 : color;
    block->shape = shape;
    block->current = current;
    block->ghost = ghost;
}

void resetBlock(Block* block) {
    block->color = BLACK;
    block->shape = EMPTY;
    block->current = false;
    block->ghost = false;
}

void clearGhost(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH]) {
    for (int i = 0; i < CANVAS_HEIGHT; i++)
        for (int j = 0; j < CANVAS_WIDTH; j++)
            if (canvas[i][j].ghost) resetBlock(&canvas[i][j]);
}

void drawGhost(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], int x, int y, int rotate, ShapeId shapeId) {
    clearGhost(canvas);
    Shape shapeData = shapes[shapeId];
    int size = shapeData.size;

    int ghostY = y;
    while (true) {
        bool canMove = true;
        for (int i = 0; i < size && canMove; i++)
            for (int j = 0; j < size && canMove; j++)
                if (shapeData.rotates[rotate][i][j]) {
                    int newY = ghostY + i + 1;
                    int newX = x + j;
                    if (newY >= CANVAS_HEIGHT || (canvas[newY][newX].shape != EMPTY && !canvas[newY][newX].current))
                        canMove = false;
                }
        if (!canMove) break;
        ghostY++;
    }

    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            if (shapeData.rotates[rotate][i][j])
                setBlock(&canvas[ghostY + i][x + j], shapeData.color, shapeId, false, true);
}

bool move(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], int ox, int oy, int or , int nx, int ny, int nr, ShapeId shapeId) {
    Shape shape = shapes[shapeId];
    int size = shape.size;

    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            if (shape.rotates[nr][i][j]) {
                int x = nx + j, y = ny + i;
                if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) return false;
                if (!canvas[y][x].current && canvas[y][x].shape != EMPTY) return false;
            }

    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            if (shape.rotates[or ][i][j])
                resetBlock(&canvas[oy + i][ox + j]);

    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++)
            if (shape.rotates[nr][i][j])
                setBlock(&canvas[ny + i][nx + j], shape.color, shapeId, true, false);

    return true;
}

int clearLine(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state) {
    int linesCleared = 0;

    for (int i = 0; i < CANVAS_HEIGHT; i++)
        for (int j = 0; j < CANVAS_WIDTH; j++)
            canvas[i][j].current = false;

    for (int i = CANVAS_HEIGHT - 1; i >= 0; i--) {
        bool isFull = true;
        for (int j = 0; j < CANVAS_WIDTH; j++)
            if (canvas[i][j].shape == EMPTY) isFull = false;

        if (isFull) {
            linesCleared++;
            for (int j = i; j > 0; j--)
                for (int k = 0; k < CANVAS_WIDTH; k++)
                    canvas[j][k] = canvas[j - 1][k];
            i++;
        }
    }

    if (linesCleared > 0) {
        state->combo++;
        state->score += linesCleared * 10 + state->combo * 5;
    }
    else {
        state->combo = 0;
    }

    return linesCleared;
}

void printCanvas(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state) {
    printf("\033[0;0H");
    for (int i = 0; i < CANVAS_HEIGHT; i++) {
        printf("|");
        for (int j = 0; j < CANVAS_WIDTH; j++)
            printf("\033[%dm\u3000", canvas[i][j].color);
        printf("\033[0m|\n");
    }

    printf("\033[%d;%dHScore: %d", 2, CANVAS_WIDTH * 2 + 5, state->score);
    if (state->combo > 1)
        printf("\033[%d;%dH🔥 Combo x%d!", 3, CANVAS_WIDTH * 2 + 5, state->combo);

    printf("\033[%d;%dHNext:", 5, CANVAS_WIDTH * 2 + 5);
    for (int i = 1; i <= 3; i++) {
        Shape s = shapes[state->queue[i]];
        for (int j = 0; j < 4; j++) {
            printf("\033[%d;%dH", 6 + i * 4 + j, CANVAS_WIDTH * 2 + 10);
            for (int k = 0; k < 4; k++) {
                if (j < s.size && k < s.size && s.rotates[0][j][k])
                    printf("\033[%dm  ", s.color);
                else
                    printf("\033[0m  ");
            }
        }
    }
    if (state->paused)
        printf("\033[%d;%dH\x1b[47m PAUSED \x1b[0m", 1, CANVAS_WIDTH * 2 + 5);
}

void logic(Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH], State* state) {
    if (PAUSE_FUNC()) {
        state->paused = !state->paused;
        Sleep(200);
        return;
    }

    if (state->paused) return;

    if (ROTATE_FUNC()) {
        int nr = (state->rotate + 1) % 4;
        if (move(canvas, state->x, state->y, state->rotate, state->x, state->y, nr, state->queue[0]))
            state->rotate = nr;
    }
    else if (LEFT_FUNC()) {
        if (move(canvas, state->x, state->y, state->rotate, state->x - 1, state->y, state->rotate, state->queue[0]))
            state->x--;
    }
    else if (RIGHT_FUNC()) {
        if (move(canvas, state->x, state->y, state->rotate, state->x + 1, state->y, state->rotate, state->queue[0]))
            state->x++;
    }
    else if (DOWN_FUNC()) {
        state->fallTime = FALL_DELAY;
    }
    else if (FALL_FUNC()) {
        state->fallTime += FALL_DELAY * CANVAS_HEIGHT;
    }

    state->fallTime += RENDER_DELAY;

    drawGhost(canvas, state->x, state->y, state->rotate, state->queue[0]);

    while (state->fallTime >= FALL_DELAY) {
        state->fallTime -= FALL_DELAY;
        if (move(canvas, state->x, state->y, state->rotate, state->x, state->y + 1, state->rotate, state->queue[0]))
            state->y++;
        else {
            clearLine(canvas, state);
            state->x = CANVAS_WIDTH / 2;
            state->y = 0;
            state->rotate = 0;
            state->fallTime = 0;
            state->queue[0] = state->queue[1];
            state->queue[1] = state->queue[2];
            state->queue[2] = state->queue[3];
            state->queue[3] = rand() % 7;

            if (!move(canvas, state->x, state->y, state->rotate, state->x, state->y, state->rotate, state->queue[0])) {
                printf("\033[%d;%dH\x1b[41m GAME OVER \x1b[0m\n", CANVAS_HEIGHT / 2, CANVAS_WIDTH * 2 + 5);
                exit(0);
            }
        }
    }
}

int main() {
    srand(time(NULL));
    State state = { .x = CANVAS_WIDTH / 2, .y = 0, .score = 0, .rotate = 0, .fallTime = 0, .combo = 0, .paused = false };

    for (int i = 0; i < 4; i++) state.queue[i] = rand() % 7;

    Block canvas[CANVAS_HEIGHT][CANVAS_WIDTH];
    for (int i = 0; i < CANVAS_HEIGHT; i++)
        for (int j = 0; j < CANVAS_WIDTH; j++)
            resetBlock(&canvas[i][j]);

    Shape shape = shapes[state.queue[0]];
    for (int i = 0; i < shape.size; i++)
        for (int j = 0; j < shape.size; j++)
            if (shape.rotates[0][i][j])
                setBlock(&canvas[state.y + i][state.x + j], shape.color, state.queue[0], true, false);

    while (1) {
        printCanvas(canvas, &state);
        logic(canvas, &state);
        Sleep(RENDER_DELAY);
    }
    return 0;
}

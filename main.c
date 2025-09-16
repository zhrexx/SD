#include <stdint.h>

#include "sd.h"

typedef struct {
    Point pos;
    float angle;
    Point vel;
    unsigned int color;
    int lifetime;
} Confetti;

Confetti confettis[512];
int confetti_count = 0;

void SpawnConfetti(Point center, int count, int lifetime) {
    for (int i = 0; i < count && confetti_count < 512; i++) {
        float angle = ((float)rand() / RAND_MAX) * 2.0f * 3.14159f;
        float speed = 2.0f + ((float)rand() / RAND_MAX) * 3.0f;
        Point vel = { cosf(angle) * speed, sinf(angle) * speed };
        unsigned int colors[] = { SD_RED, SD_GREEN, SD_BLUE, SD_YELLOW, SD_CYAN, SD_MAGENTA, SD_ORANGE, SD_PURPLE };
        unsigned int color = colors[rand() % 8];

        confettis[confetti_count++] = (Confetti){ center, 0, vel, color, lifetime };
    }
}

void UpdateDrawConfetti() {
    for (int i = 0; i < confetti_count; i++) {
        Confetti* c = &confettis[i];

        c->pos.x += c->vel.x;
        c->pos.y += c->vel.y;

        c->vel.y += 0.1f;

        SD_Rect((int)c->pos.x, (int)c->pos.y, 4, 4, c->color);

        c->lifetime--;
    }

    int j = 0;
    for (int i = 0; i < confetti_count; i++) {
        if (confettis[i].lifetime > 0) {
            confettis[j++] = confettis[i];
        }
    }
    confetti_count = j;
}


int main() {
    SD_CreateWindow(800, 600, "SD Demo", true);

    uint32_t start = timeGetTime();

    Point pos = SD_Point(SD_Width() / 2 - 20, SD_Height() / 2 - 20);
    int step = 2;
    int fps = 120;
    int point_size = 20;
    int score = 0;

    Point points[256] = {0};
    int point_spawn_count = 16;

    for (int point_count = 0; point_count < 255; point_count++) {
        points[point_count].x = rand() % SD_Width() - point_size;
        points[point_count].y = rand() % SD_Height() - point_size;
    }

    while (SD_WindowActive()) {
        SD_StartFrame();
            SD_SetFPS(fps);

            if (SD_Key('W') && pos.y > 0) pos.y -= step;
            if (SD_Key('S') && pos.y < SD_Height() - 20) pos.y += step;
            if (SD_Key('A') && pos.x > 0) pos.x -= step;
            if (SD_Key('D') && pos.x < SD_Width() - 20) pos.x += step;
            if (SD_KeyDouble('M')) { // More fps
                fps += 30;
            }
            if (SD_KeyDouble('L')) { // Less fps
                fps -= 30;
            }
            if (SD_KeyDouble('F')) { // Faster
                step += 1;
            }
            if (SD_KeyDouble('G')) { // Slower
                step -= 1;
            }
            if (SD_KeyDouble('P')) { // Spawn more points
                if (point_spawn_count + 8 < 256) {
                    point_spawn_count += 8;
                }
            }
            if (SD_KeyDouble('O')) {
                for (int point_count = 0; point_count < 255; point_count++) {
                    points[point_count].x = rand() % SD_Width() - point_size;
                    points[point_count].y = rand() % SD_Height() - point_size;
                }
            }
            if (SD_KeyDouble('R')) { // Reset Position
                pos.x = SD_Width() / 2 - 20;
                pos.y = SD_Height() / 2 - 20;
            }

            SD_Fill(SD_BLACK);
            SD_Rect(pos.x, pos.y, 20, 20, SD_BLUE);



            for (int i = 0; i < point_spawn_count; i++) {
                Point p = points[i];
                SD_Rect(SD_EXPAND_POINT(p), point_size, point_size, SD_RED);

                if (SD_RectOverlap(pos, point_size, point_size, p, point_size, point_size)) {
                    points[i] = SD_Point(rand() % SD_Width() - point_size, rand() % SD_Height() - point_size);
                    score++;
                    SpawnConfetti(pos, 20, 180);
                }

            }

            UpdateDrawConfetti();

            float current = SD_GetFPS();
            SD_DrawFText(25, 50, SD_WHITE, 1.0f, "FPS: %d | LIMITER == %d | STEP == %d\nPOS == (%.1f, %.1f) | SCORE == %d | %d points spawed | PLAYING %u seconds",
                (int)current, fps, step, pos.x, pos.y, score, point_spawn_count, (timeGetTime() - start) / 1000);

            if (SD_MouseLeft()) {
                Point mouse = SD_MousePos();
                SD_Circle(SD_EXPAND_POINT(mouse), 10, 0xFF0000FF);
            }

            if (SD_MouseRight()) {
                Point mouse = SD_MousePos();
                SD_Circle(SD_EXPAND_POINT(mouse), 10, SD_GREEN);
            }

        SD_EndFrame();
    }

    SD_DestroyWindow();
    return 0;
}
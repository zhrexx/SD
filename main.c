#include "sd.h"

int main() {
    SD_CreateWindow(800, 600, "SD Demo", true);

    Point pos = SD_Point(SD_Width() / 2 - 20, SD_Height() / 2 - 20);
    int step = 2;
    int fps = 120;


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
        if (SD_KeyDouble('R')) { // Reset Position
            pos.x = SD_Width() / 2 - 20;
            pos.y = SD_Height() / 2 - 20;
        }

        SD_Fill(SD_BLACK);
        float current = SD_GetFPS();
        SD_DrawFText(25, 50, SD_WHITE, 1.0f, "FPS: %d | LIMITER == %d | STEP == %d | POS == (%.1f, %.1f)", (int)current, fps, step, pos.x, pos.y);

        SD_Rect(pos.x, pos.y, 20, 20, SD_RED);

        // TODO: Example
        if (SD_MouseLeft() && SD_InsideSquare(SD_MousePos(), SD_Point(pos.x, pos.y), 20)) {

        }

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
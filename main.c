#include "sd.h"

int main() {
    SD_CreateWindow(800, 600, "SD Demo");

    int x = SD_Width() / 2, y = SD_Height() / 2;
    int fps = 120;


    while (SD_WindowActive()) {
        SD_StartFrame();
        SD_SetFPS(fps);

        if (SD_Key('W') && y > 0) y -= 2;
        if (SD_Key('S') && y < SD_Height() - 20) y += 2;
        if (SD_Key('A') && x > 0) x -= 2;
        if (SD_Key('D') && x < SD_Width() - 20) x += 2;
        if (SD_KeyDouble('M')) { // More fps
            fps += 30;
        }

        SD_Fill(SD_BLACK);
        float current = SD_GetFPS();
        SD_DrawFText(25, 50, SD_WHITE, 2.0f, "FPS: %d | %d", (int)current, fps);

        SD_Rect(x, y, 20, 20, SD_RED);

        if (SD_MouseLeft()) {
            SD_Circle(SD_MouseX(), SD_MouseY(), 10, SD_YELLOW);
        }

        SD_EndFrame();
    }

    SD_DestroyWindow();
    return 0;
}
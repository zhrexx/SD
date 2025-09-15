// SimpleDraw library

#ifndef SD_H
#define SD_H

#define FONT8x16_IMPLEMENTATION
#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#pragma comment(lib, "winmm.lib")
#include "font8x16.h"

static int g_width = 800;
static int g_height = 600;
static HBITMAP g_hBitmap = NULL;
static void *g_pixels = NULL;
static HWND g_hwnd = NULL;
static int g_should_close = 0;
static int g_mouse_x = 0;
static int g_mouse_y = 0;
static bool g_mouse_left = false;
static bool g_mouse_ldouble = false;
static bool g_mouse_right = false;
static bool g_keys[256] = {0};
static double g_key_last_time[256] = {0};
static bool g_key_double[256] = {0};


static int g_target_fps = 60;
static LARGE_INTEGER g_frequency;
static LARGE_INTEGER g_last_frame_time;
static LARGE_INTEGER g_frame_start_time;
static int g_frame_count = 0;
static float g_fps = 0.0f;
static LARGE_INTEGER g_fps_timer;

#define SD_RED     0xFF0000
#define SD_GREEN   0x00FF00
#define SD_BLUE    0x0000FF
#define SD_WHITE   0xFFFFFF
#define SD_BLACK   0x000000
#define SD_YELLOW  0xFFFF00
#define SD_CYAN    0x00FFFF
#define SD_MAGENTA 0xFF00FF
#define SD_ORANGE  0xFFA500
#define SD_PURPLE  0x800080
#define SD_GRAY    0x808080
#define SD_BROWN   0x8B4513

static LRESULT CALLBACK SD_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            if (g_hBitmap) {
                HDC mdc = CreateCompatibleDC(dc);
                SelectObject(mdc, g_hBitmap);
                BitBlt(dc, 0, 0, g_width, g_height, mdc, 0, 0, SRCCOPY);
                DeleteDC(mdc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            g_should_close = 1;
            PostQuitMessage(0);
            return 0;
        case WM_MOUSEMOVE:
            g_mouse_x = GET_X_LPARAM(lParam);
            g_mouse_y = GET_Y_LPARAM(lParam);
            return 0;
        case WM_LBUTTONDOWN:
            g_mouse_left = true;
            return 0;
        case WM_LBUTTONUP:
            g_mouse_left = false;
            return 0;
        case WM_RBUTTONDOWN:
            g_mouse_right = true;
            return 0;
        case WM_RBUTTONUP:
            g_mouse_right = false;
            return 0;
        case WM_LBUTTONDBLCLK:
            g_mouse_ldouble = true;
            return 0;
        case WM_KEYDOWN:
            if (wParam < 256) {
                g_keys[wParam] = true;

                LARGE_INTEGER now;
                QueryPerformanceCounter(&now);
                double current = (double)now.QuadPart / g_frequency.QuadPart;

                if (current - g_key_last_time[wParam] < 0.3) {
                    g_key_double[wParam] = true;
                }

                g_key_last_time[wParam] = current;
            }
            return 0;
            return 0;
        case WM_KEYUP:
            if (wParam < 256) g_keys[wParam] = false;
            return 0;
        case WM_CLOSE:
            g_should_close = 1;
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SD_CreateWindow(int width, int height, const char* title) {
    g_width = width;
    g_height = height;
    g_should_close = 0;

    QueryPerformanceFrequency(&g_frequency);
    QueryPerformanceCounter(&g_last_frame_time);
    QueryPerformanceCounter(&g_fps_timer);

    timeBeginPeriod(1);

    HINSTANCE hInst = GetModuleHandle(NULL);
    const wchar_t CLASS_NAME[] = L"SDWindow";

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = SD_WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    int len = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t* wtitle = malloc(len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, len);

    RECT rect = {0, 0, width, height};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    g_hwnd = CreateWindowW(
        CLASS_NAME, wtitle,
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInst, NULL
    );

    free(wtitle);

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);

    BITMAPINFO bi = {0};
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = g_width;
    bi.bmiHeader.biHeight = -g_height;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    bi.bmiHeader.biCompression = BI_RGB;

    HDC dc = GetDC(g_hwnd);
    g_hBitmap = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &g_pixels, NULL, 0);
    ReleaseDC(g_hwnd, dc);
}

bool SD_WindowActive() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return !g_should_close;
}

void SD_DestroyWindow() {
    timeEndPeriod(1);
    if (g_hBitmap) {
        DeleteObject(g_hBitmap);
        g_hBitmap = NULL;
    }
    g_pixels = NULL;
    if (g_hwnd) {
        DestroyWindow(g_hwnd);
        g_hwnd = NULL;
    }
}

void SD_StartFrame() {
    QueryPerformanceCounter(&g_frame_start_time);
}

void SD_EndFrame() {
    LARGE_INTEGER current_time;
    QueryPerformanceCounter(&current_time);

    g_frame_count++;

    double elapsed_since_fps = (double)(current_time.QuadPart - g_fps_timer.QuadPart) / g_frequency.QuadPart;
    if (elapsed_since_fps >= 1.0) {
        g_fps = g_frame_count / elapsed_since_fps;
        g_frame_count = 0;
        g_fps_timer = current_time;
    }

    double frame_time = (double)(current_time.QuadPart - g_frame_start_time.QuadPart) / g_frequency.QuadPart;
    double target_frame_time = 1.0 / g_target_fps;

    if (frame_time < target_frame_time) {
        double remaining_time = target_frame_time - frame_time;

        if (remaining_time > 0.002) {
            int sleep_ms = (int)((remaining_time - 0.001) * 1000.0);
            if (sleep_ms > 0) {
                Sleep(sleep_ms);
            }
        }

        LARGE_INTEGER spin_start;
        QueryPerformanceCounter(&spin_start);

        while (true) {
            LARGE_INTEGER spin_current;
            QueryPerformanceCounter(&spin_current);
            double spin_elapsed = (double)(spin_current.QuadPart - g_frame_start_time.QuadPart) / g_frequency.QuadPart;
            if (spin_elapsed >= target_frame_time) break;
            YieldProcessor();
        }
    }

    if (g_hwnd) {
        InvalidateRect(g_hwnd, NULL, FALSE);
        UpdateWindow(g_hwnd);
    }

    g_last_frame_time = current_time;
}

void SD_SetFPS(int fps) {
    if (fps > 0 && fps <= 100000) {
        g_target_fps = fps + 4;
    }
}

float SD_GetFPS() {
    return g_fps;
}

void SD_Fill(unsigned int color) {
    if (!g_pixels) return;
    unsigned char *p = (unsigned char *)g_pixels;
    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;

    for (int i = 0; i < g_width * g_height * 4; i += 4) {
        p[i + 0] = b;
        p[i + 1] = g;
        p[i + 2] = r;
        p[i + 3] = 255;
    }
}

void SD_Plot(int x, int y, unsigned int color) {
    if (!g_pixels || x < 0 || x >= g_width || y < 0 || y >= g_height) return;

    unsigned char *p = (unsigned char *)g_pixels;
    unsigned char r = (color >> 16) & 0xFF;
    unsigned char g = (color >> 8) & 0xFF;
    unsigned char b = color & 0xFF;

    int idx = (y * g_width + x) * 4;
    p[idx + 0] = b;
    p[idx + 1] = g;
    p[idx + 2] = r;
    p[idx + 3] = 255;
}

void SD_Rect(int x, int y, int w, int h, unsigned int color) {
    for (int yy = y; yy < y + h && yy < g_height; yy++) {
        if (yy < 0) continue;
        for (int xx = x; xx < x + w && xx < g_width; xx++) {
            if (xx < 0) continue;
            SD_Plot(xx, yy, color);
        }
    }
}

void SD_RectOutline(int x, int y, int w, int h, unsigned int color) {
    for (int xx = x; xx < x + w; xx++) {
        SD_Plot(xx, y, color);
        SD_Plot(xx, y + h - 1, color);
    }
    for (int yy = y; yy < y + h; yy++) {
        SD_Plot(x, yy, color);
        SD_Plot(x + w - 1, yy, color);
    }
}

void SD_Circle(int x, int y, int radius, unsigned int color) {
    for (int yy = -radius; yy <= radius; yy++) {
        for (int xx = -radius; xx <= radius; xx++) {
            if (xx*xx + yy*yy <= radius*radius) {
                SD_Plot(x + xx, y + yy, color);
            }
        }
    }
}

void SD_CircleOutline(int x, int y, int radius, unsigned int color) {
    int xx = 0, yy = radius;
    int d = 3 - 2 * radius;

    while (yy >= xx) {
        SD_Plot(x + xx, y + yy, color);
        SD_Plot(x - xx, y + yy, color);
        SD_Plot(x + xx, y - yy, color);
        SD_Plot(x - xx, y - yy, color);
        SD_Plot(x + yy, y + xx, color);
        SD_Plot(x - yy, y + xx, color);
        SD_Plot(x + yy, y - xx, color);
        SD_Plot(x - yy, y - xx, color);

        xx++;
        if (d > 0) {
            yy--;
            d = d + 4 * (xx - yy) + 10;
        } else {
            d = d + 4 * xx + 6;
        }
    }
}

void SD_Line(int x1, int y1, int x2, int y2, unsigned int color) {
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1;
    int sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        SD_Plot(x1, y1, color);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void SD_Triangle(int x1, int y1, int x2, int y2, int x3, int y3, unsigned int color) {
    int min_x = (x1 < x2) ? (x1 < x3 ? x1 : x3) : (x2 < x3 ? x2 : x3);
    int max_x = (x1 > x2) ? (x1 > x3 ? x1 : x3) : (x2 > x3 ? x2 : x3);
    int min_y = (y1 < y2) ? (y1 < y3 ? y1 : y3) : (y2 < y3 ? y2 : y3);
    int max_y = (y1 > y2) ? (y1 > y3 ? y1 : y3) : (y2 > y3 ? y2 : y3);

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            int d1 = (x - x2) * (y1 - y2) - (x1 - x2) * (y - y2);
            int d2 = (x - x3) * (y2 - y3) - (x2 - x3) * (y - y3);
            int d3 = (x - x1) * (y3 - y1) - (x3 - x1) * (y - y1);

            bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            if (!(has_neg && has_pos)) {
                SD_Plot(x, y, color);
            }
        }
    }
}

void SD_TriangleOutline(int x1, int y1, int x2, int y2, int x3, int y3, unsigned int color) {
    SD_Line(x1, y1, x2, y2, color);
    SD_Line(x2, y2, x3, y3, color);
    SD_Line(x3, y3, x1, y1, color);
}

void SD_DrawText(int x, int y, const char* text, unsigned int color, float scale) {
    if (!text || scale <= 0.0f) return;

    int start_x = x;
    int scaled_width = (int)(8 * scale);
    int scaled_height = (int)(16 * scale);

    for (const char* p = text; *p; p++) {
        unsigned char c = (unsigned char)*p;

        if (c == '\n') {
            x = start_x;
            y += scaled_height;
            continue;
        }

        if (c < 32 || c > 126) {
            x += scaled_width;
            continue;
        }

        const unsigned char* font_data = font8x16[c];

        for (int row = 0; row < 16; row++) {
            unsigned char byte = font_data[row];

            for (int bit = 0; bit < 8; bit++) {
                if (byte & (0x80 >> bit)) {
                    for (int sy = 0; sy < (int)scale; sy++) {
                        for (int sx = 0; sx < (int)scale; sx++) {
                            SD_Plot(x + bit * (int)scale + sx, y + row * (int)scale + sy, color);
                        }
                    }
                }
            }
        }

        x += scaled_width;
    }
}

void SD_DrawFText(int x, int y, unsigned int color, float scale, const char* fmt, ...) {
    char buffer[1024];
    va_list args;

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    SD_DrawText(x, y, buffer, color, scale);
}

int SD_Width() {
    return g_width;
}

int SD_Height() {
    return g_height;
}

int SD_MouseX() {
    return g_mouse_x;
}

int SD_MouseY() {
    return g_mouse_y;
}

bool SD_MouseLeft() {
    return g_mouse_left;
}

bool SD_MouseLeftDouble() {
    bool ret = g_mouse_ldouble;
    g_mouse_ldouble = false;
    return ret;
}

bool SD_MouseRight() {
    return g_mouse_right;
}

bool SD_Ctrl() {
    return g_keys[VK_LCONTROL] || g_keys[VK_RCONTROL];
}

bool SD_Key(int key) {
    if (key < 0 || key >= 256) return false;
    return g_keys[key];
}

bool SD_KeyDouble(int key) {
    if (key < 0 || key >= 256) return false;
    bool ret = g_key_double[key];
    g_key_double[key] = false;
    return ret;
}


unsigned int SD_RGB(unsigned char r, unsigned char g, unsigned char b) {
    return (r << 16) | (g << 8) | b;
}

unsigned int SD_RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (r << 16) | (g << 8) | b;
}

void SD_Sleep(int ms) {
    Sleep(ms);
}

#endif
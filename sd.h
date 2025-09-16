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
static bool g_resize_pending = false;
static int g_new_width, g_new_height;

static int g_target_fps = 60;
static LARGE_INTEGER g_frequency;
static LARGE_INTEGER g_last_frame_time;
static LARGE_INTEGER g_frame_start_time;
static int g_frame_count = 0;
static float g_fps = 0.0f;
static LARGE_INTEGER g_fps_timer;
static bool g_in_sizemove = false;

typedef struct {
    float x, y;
} Point;

unsigned int SD_RGB(unsigned char r, unsigned char g, unsigned char b) {
    return (r << 16) | (g << 8) | b;
}

unsigned int SD_RGBA(unsigned char r, unsigned char g, unsigned char b, unsigned char a) {
    return (r << 16) | (g << 8) | b;
}

unsigned int SD_RGBA64(unsigned long long rgba) {
    unsigned char r = (rgba >> 24) & 0xFF;
    unsigned char g = (rgba >> 16) & 0xFF;
    unsigned char b = (rgba >> 8)  & 0xFF;
    unsigned char a = (rgba)       & 0xFF;

    return (r << 16) | (g << 8) | b;
}



#define SD_RED     SD_RGBA64(0xFF0000FF)
#define SD_GREEN   SD_RGBA64(0x00FF00FF)
#define SD_BLUE    SD_RGBA64(0x0000FFFF)
#define SD_WHITE   SD_RGBA64(0xFFFFFFFF)
#define SD_BLACK   SD_RGBA64(0x000000FF)
#define SD_YELLOW  SD_RGBA64(0xFFFF00FF)
#define SD_CYAN    SD_RGBA64(0x00FFFFFF)
#define SD_MAGENTA SD_RGBA64(0xFF00FFFF)
#define SD_ORANGE  SD_RGBA64(0xFFA500FF)
#define SD_PURPLE  SD_RGBA64(0x800080FF)
#define SD_GRAY    SD_RGBA64(0x808080FF)
#define SD_BROWN   SD_RGBA64(0x8B4513FF)

static inline unsigned int SD_RGBToBGRA(unsigned int rgb_color) {
    return ((rgb_color & 0xFF0000) >> 16) |
           (rgb_color & 0x00FF00) |
           ((rgb_color & 0x0000FF) << 16) |
           0xFF000000;
}

static inline void SD_HandleResize() {
    if (g_resize_pending && !g_in_sizemove) {
        g_width = g_new_width;
        g_height = g_new_height;

        if (g_hBitmap) {
            DeleteObject(g_hBitmap);
            g_hBitmap = NULL;
            g_pixels = NULL;
        }

        HDC dc = GetDC(g_hwnd);
        BITMAPINFO bi = {0};
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = g_width;
        bi.bmiHeader.biHeight = -g_height;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        g_hBitmap = CreateDIBSection(dc, &bi, DIB_RGB_COLORS, &g_pixels, NULL, 0);
        ReleaseDC(g_hwnd, dc);

        g_resize_pending = false;
    }
}

static LRESULT CALLBACK SD_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC dc = BeginPaint(hwnd, &ps);
            if (g_hBitmap) {
                HDC mdc = CreateCompatibleDC(dc);
                HGDIOBJ old_obj = SelectObject(mdc, g_hBitmap);
                BitBlt(dc, ps.rcPaint.left, ps.rcPaint.top,
                       ps.rcPaint.right - ps.rcPaint.left,
                       ps.rcPaint.bottom - ps.rcPaint.top,
                       mdc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
                SelectObject(mdc, old_obj);
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
        case WM_KEYUP:
            if (wParam < 256) g_keys[wParam] = false;
            return 0;
        case WM_CLOSE:
            g_should_close = 1;
            return 0;
        case WM_SIZE: {
            if (wParam != SIZE_MINIMIZED) {
                g_new_width = LOWORD(lParam);
                g_new_height = HIWORD(lParam);
                g_resize_pending = true;
                SD_HandleResize();
                InvalidateRect(hwnd, NULL, FALSE);
            }
            return 0;
        }
        case WM_ENTERSIZEMOVE:
            g_in_sizemove = true;
            return 0;
        case WM_EXITSIZEMOVE:
            g_in_sizemove = false;
            SD_HandleResize();
            InvalidateRect(hwnd, NULL, FALSE);
            return 0;
        case WM_ERASEBKGND:
            return 1;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void SD_CreateWindow(int width, int height, const char* title, bool resizable) {
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
    wc.hbrBackground = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;

    RegisterClassW(&wc);

    int len = MultiByteToWideChar(CP_UTF8, 0, title, -1, NULL, 0);
    wchar_t* wtitle = malloc(len * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, title, -1, wtitle, len);

    RECT rect = {0, 0, width, height};

    DWORD style;
    if (!resizable) {
        style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    } else {
        style = WS_OVERLAPPEDWINDOW;
    }

    AdjustWindowRect(&rect, style, FALSE);

    g_hwnd = CreateWindowW(
        CLASS_NAME, wtitle,
        style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, hInst, NULL
    );

    free(wtitle);

    ShowWindow(g_hwnd, SW_SHOW);

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

    if (g_hwnd && !g_in_sizemove) {
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

static inline void SD_FastFill32(void* dest, unsigned int value, size_t count) {
    unsigned int* ptr = (unsigned int*)dest;
    for (size_t i = 0; i < count; i++) {
        ptr[i] = value;
    }
}

void SD_Fill(unsigned int color) {
    if (!g_pixels) return;
    unsigned int bgra_color = SD_RGBToBGRA(color);
    SD_FastFill32(g_pixels, bgra_color, g_width * g_height);
}

static inline void SD_PlotUnsafe(int x, int y, unsigned int color) {
    unsigned int* pixels = (unsigned int*)g_pixels;
    pixels[y * g_width + x] = SD_RGBToBGRA(color);
}

void SD_Plot(int x, int y, unsigned int color) {
    if (!g_pixels || x < 0 || x >= g_width || y < 0 || y >= g_height) return;
    SD_PlotUnsafe(x, y, color);
}

void SD_Rect(int x, int y, int w, int h, unsigned int color) {
    if (!g_pixels || w <= 0 || h <= 0) return;

    int x1 = x < 0 ? 0 : x;
    int y1 = y < 0 ? 0 : y;
    int x2 = x + w > g_width ? g_width : x + w;
    int y2 = y + h > g_height ? g_height : y + h;

    if (x1 >= x2 || y1 >= y2) return;

    unsigned int bgra = SD_RGBToBGRA(color);
    unsigned int* pixels = (unsigned int*)g_pixels;
    int width = x2 - x1;

    for (int yy = y1; yy < y2; yy++) {
        SD_FastFill32(&pixels[yy * g_width + x1], bgra, width);
    }
}

void SD_RectOutline(int x, int y, int w, int h, unsigned int color) {
    if (w <= 0 || h <= 0) return;

    for (int xx = x; xx < x + w; xx++) {
        SD_Plot(xx, y, color);
        SD_Plot(xx, y + h - 1, color);
    }
    for (int yy = y; yy < y + h; yy++) {
        SD_Plot(x, yy, color);
        SD_Plot(x + w - 1, yy, color);
    }
}

void SD_RectAtAngle(int cx, int cy, int w, int h, float angle, unsigned int color) {
    if (w <= 0 || h <= 0) return;

    float cosA = cosf(angle);
    float sinA = sinf(angle);
    float hw = w / 2.0f;
    float hh = h / 2.0f;

    float corners[4][2] = {
        {-hw, -hh}, { hw, -hh}, { hw, hh}, {-hw, hh}
    };
    float minX = 1e30f, minY = 1e30f, maxX = -1e30f, maxY = -1e30f;
    for (int i = 0; i < 4; i++) {
        float x = cx + corners[i][0] * cosA - corners[i][1] * sinA;
        float y = cy + corners[i][0] * sinA + corners[i][1] * cosA;
        if (x < minX) minX = x;
        if (y < minY) minY = y;
        if (x > maxX) maxX = x;
        if (y > maxY) maxY = y;
    }

    int x1 = max(0, (int)minX);
    int y1 = max(0, (int)minY);
    int x2 = min(g_width, (int)maxX + 1);
    int y2 = min(g_height, (int)maxY + 1);

    unsigned int* pixels = (unsigned int*)g_pixels;
    unsigned int bgra = SD_RGBToBGRA(color);

    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {
            float dx = x - cx;
            float dy = y - cy;
            float localX =  dx * cosA + dy * sinA;
            float localY = -dx * sinA + dy * cosA;

            if (localX >= -hw && localX < hw && localY >= -hh && localY < hh) {
                pixels[y * g_width + x] = bgra;
            }
        }
    }
}


void SD_Circle(int x, int y, int radius, unsigned int color) {
    if (radius <= 0) return;

    int x1 = x - radius;
    int y1 = y - radius;
    int x2 = x + radius;
    int y2 = y + radius;

    if (x1 >= g_width || y1 >= g_height || x2 < 0 || y2 < 0) return;

    int r_sq = radius * radius;

    for (int yy = y1; yy <= y2; yy++) {
        if (yy < 0 || yy >= g_height) continue;

        int dy = yy - y;
        int dy_sq = dy * dy;

        for (int xx = x1; xx <= x2; xx++) {
            if (xx < 0 || xx >= g_width) continue;

            int dx = xx - x;
            if (dx*dx + dy_sq <= r_sq) {
                SD_PlotUnsafe(xx, yy, color);
            }
        }
    }
}

void SD_CircleOutline(int x, int y, int radius, unsigned int color) {
    if (radius <= 0) return;

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
        int e2 = err << 1;
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

    if (min_x >= g_width || max_x < 0 || min_y >= g_height || max_y < 0) return;

    min_x = min_x < 0 ? 0 : min_x;
    max_x = max_x >= g_width ? g_width - 1 : max_x;
    min_y = min_y < 0 ? 0 : min_y;
    max_y = max_y >= g_height ? g_height - 1 : max_y;

    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            int d1 = (x - x2) * (y1 - y2) - (x1 - x2) * (y - y2);
            int d2 = (x - x3) * (y2 - y3) - (x2 - x3) * (y - y3);
            int d3 = (x - x1) * (y3 - y1) - (x3 - x1) * (y - y1);

            bool has_neg = (d1 < 0) || (d2 < 0) || (d3 < 0);
            bool has_pos = (d1 > 0) || (d2 > 0) || (d3 > 0);

            if (!(has_neg && has_pos)) {
                SD_PlotUnsafe(x, y, color);
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
    if (!text || scale <= 0.0f || !g_pixels) return;

    int start_x = x;
    int scale_int = (int)scale;
    int scaled_width = 8 * scale_int;
    int scaled_height = 16 * scale_int;

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

        if (scale_int == 1) {
            for (int row = 0; row < 16; row++) {
                unsigned char byte = font_data[row];
                int py = y + row;
                if (py < 0 || py >= g_height) continue;

                for (int bit = 0; bit < 8; bit++) {
                    if (byte & (0x80 >> bit)) {
                        int px = x + bit;
                        if (px >= 0 && px < g_width) {
                            SD_PlotUnsafe(px, py, color);
                        }
                    }
                }
            }
        } else {
            for (int row = 0; row < 16; row++) {
                unsigned char byte = font_data[row];

                for (int bit = 0; bit < 8; bit++) {
                    if (byte & (0x80 >> bit)) {
                        int base_x = x + bit * scale_int;
                        int base_y = y + row * scale_int;

                        for (int sy = 0; sy < scale_int; sy++) {
                            int py = base_y + sy;
                            if (py < 0 || py >= g_height) continue;

                            for (int sx = 0; sx < scale_int; sx++) {
                                int px = base_x + sx;
                                if (px >= 0 && px < g_width) {
                                    SD_PlotUnsafe(px, py, color);
                                }
                            }
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


// Callbacks draw_fn every steps specifyed times and gives angle in degrees
void SD_DrawAroundPoint(void (*draw_fn)(Point p, float angle), Point p, const int steps) {
    for (int i = 0; i < steps; i++) {
        float angle = (360.0f / steps) * i;
        draw_fn(p, angle);
    }
}


// UTILS

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

Point SD_MousePos() {
    return (Point){(float)g_mouse_x, (float)g_mouse_y};
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

void SD_Sleep(int ms) {
    Sleep(ms);
}

bool SD_InsideSquare(Point p, Point min, float size) {
    return (p.x >= min.x && p.x <= min.x + size &&
            p.y >= min.y && p.y <= min.y + size);
}

bool SD_RectOverlap(Point a, int aw, int ah, Point b, int bw, int bh) {
    return (a.x < b.x + bw &&
            a.x + aw > b.x &&
            a.y < b.y + bh &&
            a.y + ah > b.y);
}


Point SD_Point(float x, float y) {
    return (Point){x, y};
}



#define SD_EXPAND_POINT(point) point.x, point.y
int __sd__select__counter = 0;
#define SD_SELECT(a, b) (__sd__select__counter++ % 2 ? a : b) // Pseudo-random selector (uses incrementation)

#define SD_GET_RECT_CENTER(start_p, size) (Point){(start_p.x + size.x / 2.0f), (start_p.y + size.y / 2.0f)}

#endif
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <GL/gl.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#define PI         3.14159265358979323846f
#define TWO_PI     6.28318530717958647692f
#define NUM_SPIKES 160
#define RING_R     0.26f
#define INNER_R    0.18f
#define MAX_SPIKE  0.15f
#define FRAME_MS   42        /* ~24 FPS */

static HWND  g_hwnd  = NULL;
static HDC   g_hdc   = NULL;
static HGLRC g_hglrc = NULL;
static int   g_W, g_H;
static float g_time  = 0.0f;

static float g_spikes[NUM_SPIKES];
static float g_targets[NUM_SPIKES];
static float g_keyPulse      = 0.0f;
static int   g_prevKeys[256] = {0};
static float g_mx = 0.0f, g_my = 0.0f;

/* ---- helpers ---- */
static float randf(void)                        { return (float)rand()/(float)RAND_MAX; }
static float lerpf(float a, float b, float t)   { return a+(b-a)*t; }
static float clampf(float v, float lo, float hi){ return v<lo?lo:v>hi?hi:v; }

/* ---- input ---- */
static void pollInput(void) {
    POINT p;
    GetCursorPos(&p);
    g_mx =  (float)p.x / g_W * 2.0f - 1.0f;
    g_my = -((float)p.y / g_H * 2.0f - 1.0f);

    int anyNew = 0;
    for (int k = 8; k < 256; k++) {
        int down = (GetAsyncKeyState(k) & 0x8000) ? 1 : 0;
        if (down && !g_prevKeys[k]) anyNew = 1;
        g_prevKeys[k] = down;
    }
    if (anyNew) {
        g_keyPulse = 1.0f;
        for (int i = 0; i < NUM_SPIKES; i++)
            if (randf() < 0.6f) g_targets[i] = randf() * MAX_SPIKE;
    }
}

/* ---- spikes ---- */
static void initSpikes(void) {
    for (int i = 0; i < NUM_SPIKES; i++) {
        g_spikes[i]  = 0.0f;
        g_targets[i] = randf() * 0.02f;
    }
}

static void updateSpikes(float dt) {
    g_keyPulse = lerpf(g_keyPulse, 0.0f, dt * 5.0f);
    if (g_keyPulse < 0.001f) g_keyPulse = 0.0f;

    float mouseAngle = atan2f(g_my, g_mx);
    float mouseDist  = clampf(sqrtf(g_mx*g_mx + g_my*g_my), 0.0f, 1.0f);

    for (int i = 0; i < NUM_SPIKES; i++) {
        float angle = (float)i / NUM_SPIKES * TWO_PI - PI;
        float diff  = angle - mouseAngle;
        while (diff >  PI) diff -= TWO_PI;
        while (diff < -PI) diff += TWO_PI;

        float prox = expf(-diff*diff * 10.0f) * mouseDist * 0.22f;
        if (randf() < 0.015f)
            g_targets[i] = randf() * 0.025f + prox * MAX_SPIKE * 0.4f;

        float want = g_targets[i]
                   + prox * MAX_SPIKE
                   + g_keyPulse * randf() * MAX_SPIKE * 0.55f;
        g_spikes[i] = lerpf(g_spikes[i], want, dt * 7.0f);
    }
}

/* ---- render ---- */
static void render(void) {
    float aspect = (float)g_W / (float)g_H;

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    glLineWidth(1.0f);

    /* outer spike ring */
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < NUM_SPIKES; i++) {
        float a    = (float)i / NUM_SPIKES * TWO_PI;
        float wave = sinf(g_time * 0.7f + a * 2.5f) * 0.004f;
        float r    = RING_R + g_spikes[i] + wave;
        float br   = 0.45f + g_spikes[i] / MAX_SPIKE * 0.55f;
        glColor4f(br, br, br, 0.85f);
        glVertex2f(cosf(a) * r / aspect, sinf(a) * r);
    }
    glEnd();

    /* glow pass */
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < NUM_SPIKES; i++) {
        float a = (float)i / NUM_SPIKES * TWO_PI;
        float r = RING_R + g_spikes[i] + 0.008f;
        glColor4f(1, 1, 1, 0.04f);
        glVertex2f(cosf(a) * r / aspect, sinf(a) * r);
    }
    glEnd();

    /* inner ring */
    glColor4f(0.45f, 0.45f, 0.45f, 0.30f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 100; i++) {
        float a = (float)i / 100.0f * TWO_PI;
        glVertex2f(cosf(a) * INNER_R / aspect, sinf(a) * INNER_R);
    }
    glEnd();

    /* radial lines */
    glBegin(GL_LINES);
    for (int i = 0; i < NUM_SPIKES; i += 4) {
        float a  = (float)i / NUM_SPIKES * TWO_PI;
        float br = 0.25f + g_spikes[i] / MAX_SPIKE * 0.5f;
        glColor4f(br, br, br, 0.55f);
        glVertex2f(cosf(a) * INNER_R / aspect, sinf(a) * INNER_R);
        glColor4f(br, br, br, 0.0f);
        glVertex2f(cosf(a) * (RING_R + g_spikes[i]) / aspect,
                   sinf(a) * (RING_R + g_spikes[i]));
    }
    glEnd();

    SwapBuffers(g_hdc);
}

/* ---- OpenGL init ---- */
static BOOL setupGL(HWND hwnd) {
    g_hdc = GetDC(hwnd);
    PIXELFORMATDESCRIPTOR pfd = {0};
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    int fmt = ChoosePixelFormat(g_hdc, &pfd);
    if (!fmt || !SetPixelFormat(g_hdc, fmt, &pfd)) return FALSE;
    g_hglrc = wglCreateContext(g_hdc);
    return g_hglrc && wglMakeCurrent(g_hdc, g_hglrc);
}

/* ---- autostart ---- */
static void registerAutostart(void) {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(NULL, path, MAX_PATH);
    HKEY key;
    if (RegOpenKeyExW(HKEY_CURRENT_USER,
            L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
            0, KEY_SET_VALUE, &key) == ERROR_SUCCESS) {
        RegSetValueExW(key, L"AereReact", 0, REG_SZ,
            (BYTE*)path, (DWORD)((wcslen(path)+1) * sizeof(wchar_t)));
        RegCloseKey(key);
    }
}

/* ---- window proc ---- */
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
        case WM_DESTROY:
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(g_hglrc);
            ReleaseDC(hwnd, g_hdc);
            PostQuitMessage(0);
            return 0;
        case WM_SIZE:
            g_W = LOWORD(lp);
            g_H = HIWORD(lp);
            if (g_H == 0) g_H = 1;
            glViewport(0, 0, g_W, g_H);
            return 0;
        case WM_ERASEBKGND:
            return 1;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            EndPaint(hwnd, &ps);
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

/* ---- entry ---- */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR cmd, int show) {
    (void)hPrev; (void)cmd; (void)show;
    srand((unsigned)time(NULL));

    g_W = GetSystemMetrics(SM_CXSCREEN);
    g_H = GetSystemMetrics(SM_CYSCREEN);

    registerAutostart();

    WNDCLASSW wc     = {0};
    wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = L"AereReact";
    RegisterClassW(&wc);

    /*
     * WS_POPUP + HWND_BOTTOM — confirmed working on Win11 25H2 build 26200.
     * WS_CHILD into Progman fails silently on this build.
     * WS_EX_TOOLWINDOW    = hidden from Alt-Tab
     * WS_EX_NOACTIVATE    = never steals focus
     * WS_EX_TRANSPARENT   = mouse clicks pass through to desktop
     * WS_EX_LAYERED       = required for HWND_BOTTOM to stick reliably
     */
    g_hwnd = CreateWindowExW(
        WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE |
        WS_EX_TRANSPARENT | WS_EX_LAYERED,
        L"AereReact", L"AereReact",
        WS_POPUP | WS_VISIBLE,
        0, 0, g_W, g_H,
        NULL, NULL, hInst, NULL
    );

    if (!g_hwnd) return 1;

    /* Full opacity — layered flag is only needed to anchor HWND_BOTTOM */
    SetLayeredWindowAttributes(g_hwnd, 0, 255, LWA_ALPHA);

    /* Push behind everything permanently */
    SetWindowPos(g_hwnd, HWND_BOTTOM, 0, 0, g_W, g_H,
        SWP_NOACTIVATE | SWP_NOSENDCHANGING);

    ShowWindow(g_hwnd, SW_SHOWNOACTIVATE);
    UpdateWindow(g_hwnd);

    if (!setupGL(g_hwnd)) return 1;
    glViewport(0, 0, g_W, g_H);
    initSpikes();

    LARGE_INTEGER freq, t0, t1;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&t0);

    MSG msg;
    while (1) {
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) goto done;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }

        QueryPerformanceCounter(&t1);
        float dt = (float)(t1.QuadPart - t0.QuadPart) / (float)freq.QuadPart;

        if (dt < FRAME_MS * 0.001f) {
            DWORD ms = (DWORD)(FRAME_MS - dt * 1000.0f);
            if (ms > 1) Sleep(ms - 1);
            continue;
        }
        if (dt > 0.1f) dt = 0.1f;
        t0 = t1;

        g_time += dt;
        pollInput();
        updateSpikes(dt);
        render();

        /* Re-anchor every frame — prevents Win+D from pushing us up */
        SetWindowPos(g_hwnd, HWND_BOTTOM, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);
    }
done:
    return 0;
}

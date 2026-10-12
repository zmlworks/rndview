// File: rndview.c
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <bcrypt.h>
#include <commctrl.h>

#ifdef _MSC_VER
#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#pragma function(memset)
#endif

/* CRT 非使用のため memset を自前実装（構造体のゼロ初期化でコンパイラが呼ぶことがある） */
void *__cdecl memset(void *dst, int c, size_t n)
{
    volatile unsigned char *p = (volatile unsigned char *)dst;
    while (n--) *p++ = (unsigned char)c;
    return dst;
}

#define BUF_SIZE    512
#define OUT_MAX     (BUF_SIZE * 8 + 1)
#define MODE_COUNT  7
#define ID_RADIO    100
#define ID_REFRESH  200

typedef struct {
    const WCHAR *label;
    int          bits;
    const WCHAR *alpha;
    unsigned     n;
} MODE;

#define ALPHA(s) s, (unsigned)(sizeof(s) / sizeof(WCHAR) - 1)

static const MODE g_modes[MODE_COUNT] = {
    { L"2進数",                1, ALPHA(L"01") },
    { L"ダイス(1-6)",          3, ALPHA(L"123456") },
    { L"10進数",               4, ALPHA(L"0123456789") },
    { L"16進数",               4, ALPHA(L"0123456789ABCDEF") },
    { L"英小文字+数字",        6, ALPHA(L"0123456789abcdefghijklmnopqrstuvwxyz") },
    { L"英大小文字+数字",      6, ALPHA(L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ") },
    { L"英大小文字+数字+記号", 7, ALPHA(L"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~") },
};

static BYTE     g_buf[BUF_SIZE];
static WCHAR    g_out[OUT_MAX];
static NTSTATUS g_status = -1;
static int      g_mode = 3; /* 起動時は16進数 */
static int      g_dpi = 96;
static HWND     g_radio[MODE_COUNT], g_btn, g_info, g_edit;
static HFONT    g_uiFont, g_monoFont;

static int S(int v) { return MulDiv(v, g_dpi, 96); }

static void Fetch(void)
{
    g_status = BCryptGenRandom(NULL, g_buf, BUF_SIZE, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
}

static void Render(void)
{
    const MODE *m = &g_modes[g_mode];
    const int total = BUF_SIZE * 8;
    int pos = 0, n = 0, chunks = 0, i;
    WCHAR info[128];

    if (!BCRYPT_SUCCESS(g_status)) {
        wsprintfW(info, L"BCryptGenRandom 失敗 (0x%08lX)", (unsigned long)g_status);
        SetWindowTextW(g_info, info);
        SetWindowTextW(g_edit, L"");
        return;
    }

    while (pos + m->bits <= total) {
        unsigned v = 0;
        for (i = 0; i < m->bits; i++, pos++) {
            v = (v << 1) | ((g_buf[pos >> 3] >> (7 - (pos & 7))) & 1u);
        }
        chunks++;
        if (v < m->n) g_out[n++] = m->alpha[v];
    }
    g_out[n] = 0;

    SetWindowTextW(g_edit, g_out);
    wsprintfW(info, L"%d 文字   ( %d bit 単位で読取: %d 個中 %d 個を棄却 )",
              n, m->bits, chunks, chunks - n);
    SetWindowTextW(g_info, info);
}

static void Layout(int w, int h)
{
    MoveWindow(g_info, S(190), S(68), w - S(198), S(20), TRUE);
    MoveWindow(g_edit, S(8), S(98), w - S(16), h - S(106), TRUE);
}

static void CreateControls(HWND hwnd)
{
    HINSTANCE hi = GetModuleHandleW(NULL);
    NONCLIENTMETRICSW ncm;
    int i;

    ncm.cbSize = sizeof(ncm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
    g_uiFont = CreateFontIndirectW(&ncm.lfMessageFont);
    g_monoFont = CreateFontW(-MulDiv(11, g_dpi, 72), 0, 0, 0, FW_NORMAL, 0, 0, 0,
                             DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                             CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

    for (i = 0; i < MODE_COUNT; i++) {
        int x = (i < 4) ? S(8 + i * 130) : S(8 + (i - 4) * 190);
        int y = (i < 4) ? S(8) : S(36);
        int w = (i < 4) ? S(125) : S(185);
        g_radio[i] = CreateWindowExW(0, L"BUTTON", g_modes[i].label,
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | (i == 0 ? WS_GROUP | WS_TABSTOP : 0),
            x, y, w, S(24), hwnd, (HMENU)(INT_PTR)(ID_RADIO + i), hi, NULL);
        SendMessageW(g_radio[i], WM_SETFONT, (WPARAM)g_uiFont, FALSE);
    }
    SendMessageW(g_radio[g_mode], BM_SETCHECK, BST_CHECKED, 0);

    g_btn = CreateWindowExW(0, L"BUTTON", L"乱数再取得 (512 Byte)",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_GROUP | BS_PUSHBUTTON,
        S(8), S(64), S(170), S(28), hwnd, (HMENU)ID_REFRESH, hi, NULL);
    SendMessageW(g_btn, WM_SETFONT, (WPARAM)g_uiFont, FALSE);

    g_info = CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE | SS_LEFT | SS_CENTERIMAGE,
        0, 0, 0, 0, hwnd, NULL, hi, NULL);
    SendMessageW(g_info, WM_SETFONT, (WPARAM)g_uiFont, FALSE);

    g_edit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | ES_MULTILINE | ES_READONLY,
        0, 0, 0, 0, hwnd, NULL, hi, NULL);
    SendMessageW(g_edit, WM_SETFONT, (WPARAM)g_monoFont, FALSE);
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    switch (msg) {
    case WM_CREATE:
        CreateControls(hwnd);
        Fetch();
        Render();
        return 0;

    case WM_SIZE:
        Layout(LOWORD(lp), HIWORD(lp));
        return 0;

    case WM_COMMAND:
        if (HIWORD(wp) == BN_CLICKED) {
            int id = LOWORD(wp);
            if (id >= ID_RADIO && id < ID_RADIO + MODE_COUNT) {
                g_mode = id - ID_RADIO;
                Render();
            } else if (id == ID_REFRESH) {
                Fetch();
                Render();
            }
        }
        return 0;

    case WM_CTLCOLORSTATIC:
        if ((HWND)lp == g_edit) { /* 読み取り専用エディットを白背景に */
            SetTextColor((HDC)wp, GetSysColor(COLOR_WINDOWTEXT));
            SetBkColor((HDC)wp, GetSysColor(COLOR_WINDOW));
            return (LRESULT)GetSysColorBrush(COLOR_WINDOW);
        }
        break;

    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lp)->ptMinTrackSize.x = S(600);
        ((MINMAXINFO *)lp)->ptMinTrackSize.y = S(300);
        return 0;

    case WM_DESTROY:
        SetWindowTextW(g_edit, L"");
        SecureZeroMemory(g_buf, sizeof(g_buf));
        SecureZeroMemory(g_out, sizeof(g_out));
        DeleteObject(g_uiFont);
        DeleteObject(g_monoFont);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

static int Run(void)
{
    HINSTANCE hi = GetModuleHandleW(NULL);
    INITCOMMONCONTROLSEX icc;
    WNDCLASSW wc = { 0 };
    HWND hwnd;
    HDC hdc;
    MSG msg;

    SetProcessDPIAware();
    hdc = GetDC(NULL);
    g_dpi = GetDeviceCaps(hdc, LOGPIXELSY);
    ReleaseDC(NULL, hdc);

    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hi;
    wc.hCursor       = LoadCursorW(NULL, (LPCWSTR)IDC_ARROW);
    wc.hIcon         = LoadIconW(hi, MAKEINTRESOURCEW(1)); /* app.ico (resource ID 1) */
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = L"RndViewWnd";
    RegisterClassW(&wc);

    hwnd = CreateWindowExW(0, wc.lpszClassName, L"Random Buffer Viewer",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, CW_USEDEFAULT,
        S(720), S(520), NULL, NULL, hi, NULL);
    /* タイトルバー/タスクバー用の小アイコン (16x16 相当) */
    SendMessageW(hwnd, WM_SETICON, ICON_SMALL,
        (LPARAM)LoadImageW(hi, MAKEINTRESOURCEW(1), IMAGE_ICON,
                           GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
    ShowWindow(hwnd, SW_SHOWNORMAL);

    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(hwnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    return (int)msg.wParam;
}

void Entry(void)
{
    ExitProcess((UINT)Run());
}
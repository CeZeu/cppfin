/*  Bond Calculator – two‑page Win32 GUI
 *  – Page 1: pick formula with four radio buttons.
 *  – Page 2: fill in the known variables, click Calculate.
 *  – “⬅ Go Back” returns to page 1 with clean state.
 *
 *  Builds with:  g++ main.cpp -mwindows -O2 -o BondCalc.exe
 *  (MinGW–w64 or MSVC with UNICODE defined)
 */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cmath>
#include <functional>
#include <sstream>
#include <iomanip>

//=========================== control IDs ====================================
enum {
    IDC_RADIO_PV_COUPON = 1001,   // PV of coupon bond (solve P)
    IDC_RADIO_YTM_ZERO,           // YTM of zero‑coupon (solve y)
    IDC_RADIO_YTM_COUPON,         // YTM of coupon bond (solve y)
    IDC_RADIO_PV_ZERO,            // PV of zero‑coupon (solve P)

    IDC_BTN_NEXT,
    IDC_BTN_BACK,
    IDC_LBL_FORMULA,

    IDC_LBL_P,   IDC_EDIT_P,
    IDC_LBL_FV,  IDC_EDIT_FV,
    IDC_LBL_CPN, IDC_EDIT_CPN,
    IDC_LBL_N,   IDC_EDIT_N,
    IDC_LBL_Y,   IDC_EDIT_Y,

    IDC_BTN_CALC,
    IDC_LBL_RESULT, IDC_EDIT_RESULT
};

//================== forward declarations ====================================
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
double bisect(std::function<double(double)> f, double a, double b);

//================== helper: show / hide a group =============================
void ShowGroup(HWND hWnd, const int* ids, int count, BOOL show)
{
    for (int i = 0; i < count; ++i)
        ShowWindow(GetDlgItem(hWnd, ids[i]), show ? SW_SHOW : SW_HIDE);
}

//================== helper: read / write numbers ============================
double GetDouble(HWND parent, int id)
{
    wchar_t buf[64];
    GetWindowTextW(GetDlgItem(parent, id), buf, 64);
    return _wtof(buf);
}

void SetDouble(HWND parent, int id, double v, bool asPercent = false)
{
    std::wostringstream ss;
    ss.setf(std::ios::fixed); ss.precision(6);
    if (asPercent)
        ss << v << L"  (" << v * 100.0 << L" %)";
    else
        ss << v;
    SetWindowTextW(GetDlgItem(parent, id), ss.str().c_str());
}

//================== helper: tiny bisection solver ===========================
double bisect(std::function<double(double)> f, double a, double b)
{
    double fa = f(a), fb = f(b);
    if (fa * fb > 0) throw std::runtime_error("f(a) and f(b) same sign");
    for (int i = 0; i < 60; ++i)
    {
        double m  = 0.5 * (a + b);
        double fm = f(m);
        if (std::fabs(fm) < 1e-10) return m;
        if (fa * fm < 0) { b = m; fb = fm; }
        else             { a = m; fa = fm; }
    }
    return 0.5 * (a + b);
}

//=========================== main WinMain ===================================
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow)
{
    WNDCLASSW wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"BondCalcWnd";
    wc.hCursor       = LoadCursorW(nullptr, IDC_ARROW);
    RegisterClassW(&wc);

    HWND hMain = CreateWindowExW(
        0, wc.lpszClassName, L"Bond Calculator",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 420,
        nullptr, nullptr, hInst, nullptr);

    if (!hMain) return 0;
    ShowWindow(hMain, nCmdShow);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

//========================= page helpers =====================================
void ClearEdits(HWND h)
{
    const int eds[] = { IDC_EDIT_P, IDC_EDIT_FV, IDC_EDIT_CPN,
                        IDC_EDIT_N, IDC_EDIT_Y, IDC_EDIT_RESULT };
    for (int id : eds) SetWindowTextW(GetDlgItem(h, id), L"");
}

void UpdateInputPage(HWND h)
{
    bool pvCoupon = SendMessageW(GetDlgItem(h, IDC_RADIO_PV_COUPON), BM_GETCHECK, 0,0)==BST_CHECKED;
    bool ytmZero  = SendMessageW(GetDlgItem(h, IDC_RADIO_YTM_ZERO  ), BM_GETCHECK, 0,0)==BST_CHECKED;
    bool ytmCoup  = SendMessageW(GetDlgItem(h, IDC_RADIO_YTM_COUPON), BM_GETCHECK, 0,0)==BST_CHECKED;
    bool pvZero   = SendMessageW(GetDlgItem(h, IDC_RADIO_PV_ZERO   ), BM_GETCHECK, 0,0)==BST_CHECKED;

    const wchar_t* formula =
        pvCoupon ? L"P = CPN·(1/y)·(1 − 1/(1+y)^N) + FV/(1+y)^N" :
        ytmZero  ? L"y = (FV/P)^(1/N) − 1"                        :
        ytmCoup  ? L"0 = CPN·(1/y)·(1 − 1/(1+y)^N) + FV/(1+y)^N − P" :
                   L"P = FV / (1+y)^N";
    SetWindowTextW(GetDlgItem(h, IDC_LBL_FORMULA), formula);

    // hide everything, then un‑hide the required inputs
    const int all[] = {
        IDC_LBL_P,IDC_EDIT_P, IDC_LBL_FV,IDC_EDIT_FV,
        IDC_LBL_CPN,IDC_EDIT_CPN, IDC_LBL_N,IDC_EDIT_N,
        IDC_LBL_Y,IDC_EDIT_Y
    };
    ShowGroup(h, all, _countof(all), FALSE);

    if (!pvCoupon && !pvZero)           // solving for y (need P,FV,CPN,N)
    {
        const int ids[] = {
            IDC_LBL_P,IDC_EDIT_P, IDC_LBL_FV,IDC_EDIT_FV,
            IDC_LBL_CPN,IDC_EDIT_CPN, IDC_LBL_N,IDC_EDIT_N
        };
        ShowGroup(h, ids, _countof(ids), TRUE);
    }
    else                                // solving for P (need FV,N,y, [CPN])
    {
        const int ids[] = {
            IDC_LBL_FV,IDC_EDIT_FV, IDC_LBL_N,IDC_EDIT_N,
            IDC_LBL_Y,IDC_EDIT_Y
        };
        ShowGroup(h, ids, _countof(ids), TRUE);
        if (pvCoupon) {
            ShowWindow(GetDlgItem(h, IDC_LBL_CPN),  SW_SHOW);
            ShowWindow(GetDlgItem(h, IDC_EDIT_CPN), SW_SHOW);
        }
    }

    // disable the unknown edit box
    EnableWindow(GetDlgItem(h, IDC_EDIT_P),  !(pvCoupon||pvZero));
    EnableWindow(GetDlgItem(h, IDC_EDIT_Y),   pvCoupon||pvZero);

    ClearEdits(h);
}

//========================= calculation ======================================
void DoCalculation(HWND h)
{
    bool pvCoupon = SendMessageW(GetDlgItem(h, IDC_RADIO_PV_COUPON), BM_GETCHECK, 0,0)==BST_CHECKED;
    bool ytmZero  = SendMessageW(GetDlgItem(h, IDC_RADIO_YTM_ZERO  ), BM_GETCHECK, 0,0)==BST_CHECKED;
    bool ytmCoup  = SendMessageW(GetDlgItem(h, IDC_RADIO_YTM_COUPON), BM_GETCHECK, 0,0)==BST_CHECKED;
    bool pvZero   = SendMessageW(GetDlgItem(h, IDC_RADIO_PV_ZERO   ), BM_GETCHECK, 0,0)==BST_CHECKED;

    double P   = GetDouble(h, IDC_EDIT_P);
    double FV  = GetDouble(h, IDC_EDIT_FV);
    double CPN = GetDouble(h, IDC_EDIT_CPN);
    double N   = GetDouble(h, IDC_EDIT_N);
    double y   = GetDouble(h, IDC_EDIT_Y);
    double answer = 0.0;

    try {
        if (pvCoupon) {
            answer = CPN*(1.0/y)*(1.0 - std::pow(1.0+y,-N)) + FV*std::pow(1.0+y,-N);
            SetDouble(h, IDC_EDIT_RESULT, answer);
        }
        else if (pvZero) {
            answer = FV * std::pow(1.0+y, -N);
            SetDouble(h, IDC_EDIT_RESULT, answer);
        }
        else if (ytmZero) {
            answer = std::pow(FV/P, 1.0/N) - 1.0;
            SetDouble(h, IDC_EDIT_RESULT, answer, true);     // show % as well
        }
        else {                                              // ytmCoupon
            auto f = [&](double yy){
                return CPN*(1.0/yy)*(1 - std::pow(1+yy,-N))
                     + FV*std::pow(1+yy,-N) - P;
            };
            answer = bisect(f, 1e-6, 5.0);
            SetDouble(h, IDC_EDIT_RESULT, answer, true);    // show % as well
        }
    }
    catch(...) {
        MessageBoxW(h, L"Computation failed. Check inputs.",
                       L"Error", MB_OK | MB_ICONERROR);
    }
}

/* ------------------------------------------------------------------------- */
/*                              Window Proc                                  */
/* ------------------------------------------------------------------------- */
LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
    static const int page1[] = { IDC_RADIO_PV_COUPON, IDC_RADIO_YTM_ZERO,
                                 IDC_RADIO_YTM_COUPON, IDC_RADIO_PV_ZERO,
                                 IDC_BTN_NEXT };
    static const int page2[] = { IDC_BTN_BACK, IDC_LBL_FORMULA,
        IDC_LBL_P,IDC_EDIT_P, IDC_LBL_FV,IDC_EDIT_FV, IDC_LBL_CPN,IDC_EDIT_CPN,
        IDC_LBL_N,IDC_EDIT_N, IDC_LBL_Y,IDC_EDIT_Y, IDC_BTN_CALC,
        IDC_LBL_RESULT, IDC_EDIT_RESULT };

    switch(msg)
    {
    case WM_CREATE:
    {
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(h, GWLP_HINSTANCE);

        // ---------- Page 1 ----------
        CreateWindowW(L"BUTTON", L"PV of Coupon Bond",
            WS_CHILD|WS_VISIBLE|WS_GROUP|BS_AUTORADIOBUTTON,
            20, 30, 200, 20, h, (HMENU)IDC_RADIO_PV_COUPON, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"YTM of Zero‑Coupon Bond",
            WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
            20, 55, 200, 20, h, (HMENU)IDC_RADIO_YTM_ZERO, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"YTM of Coupon Bond",
            WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
            20, 80, 200, 20, h, (HMENU)IDC_RADIO_YTM_COUPON, hInst, nullptr);

        CreateWindowW(L"BUTTON", L"PV of Zero‑Coupon Bond",
            WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
            20, 105, 200, 20, h, (HMENU)IDC_RADIO_PV_ZERO, hInst, nullptr);

        // select first radio by default
        SendMessageW(GetDlgItem(h, IDC_RADIO_PV_COUPON), BM_SETCHECK, BST_CHECKED, 0);

        CreateWindowW(L"BUTTON", L"Next ⮕",
            WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
            380, 100, 90, 28, h, (HMENU)IDC_BTN_NEXT, hInst, nullptr);

        // ---------- Page 2 ----------
        CreateWindowW(L"BUTTON", L"⬅ Go Back",
            WS_CHILD,
            20, 10, 90, 26, h, (HMENU)IDC_BTN_BACK, hInst, nullptr);

        CreateWindowW(L"STATIC", L"",
            WS_CHILD|SS_LEFT,
            130, 12, 360, 22, h, (HMENU)IDC_LBL_FORMULA, hInst, nullptr);

        // labels + edits laid out as two equal columns
        auto makeLbl = [&](int id, LPCWSTR txt, int col, int row){
            CreateWindowW(L"STATIC", txt, WS_CHILD|SS_RIGHT,
                20+col*260, 50+row*28, 70, 20,
                h, (HMENU)id, hInst, nullptr);
        };
        auto makeEd  = [&](int id, int col, int row){
            CreateWindowW(L"EDIT", L"", WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,
                100+col*260, 50+row*28, 120, 20,
                h, (HMENU)id, hInst, nullptr);
        };

        makeLbl(IDC_LBL_P,   L"P",      0, 0); makeEd(IDC_EDIT_P,   0, 0);
        makeLbl(IDC_LBL_FV,  L"FV",     1, 0); makeEd(IDC_EDIT_FV,  1, 0);
        makeLbl(IDC_LBL_CPN, L"CPN",    0, 1); makeEd(IDC_EDIT_CPN, 0, 1);
        makeLbl(IDC_LBL_N,   L"N (yrs)",1, 1); makeEd(IDC_EDIT_N,   1, 1);
        makeLbl(IDC_LBL_Y,   L"y",      0, 2); makeEd(IDC_EDIT_Y,   0, 2);

        CreateWindowW(L"BUTTON", L"Calculate",
            WS_CHILD|BS_DEFPUSHBUTTON,
            190, 145, 120, 30, h, (HMENU)IDC_BTN_CALC, hInst, nullptr);

        makeLbl(IDC_LBL_RESULT, L"Result", 0, 4);
        CreateWindowW(L"EDIT", L"", WS_CHILD|WS_BORDER|ES_READONLY|ES_AUTOHSCROLL,
            100, 50+4*28, 320, 22,
            h, (HMENU)IDC_EDIT_RESULT, hInst, nullptr);

        // hide page 2 initially
        ShowGroup(h, page2, _countof(page2), FALSE);
        break;
    }

    /* ------------------ Commands ------------------- */
    case WM_COMMAND:
        switch(LOWORD(wp))
        {
        case IDC_BTN_NEXT:
            ShowGroup(h, page1, _countof(page1), FALSE);
            ShowGroup(h, page2, _countof(page2), TRUE);
            UpdateInputPage(h);
            break;

        case IDC_BTN_BACK:
            ShowGroup(h, page2, _countof(page2), FALSE);
            ShowGroup(h, page1, _countof(page1), TRUE);
            break;

        case IDC_BTN_CALC:
            DoCalculation(h);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }
    return DefWindowProcW(h, msg, wp, lp);
}

// main.cpp
// Win32 GUI bond calculator, Unicode build

#define UNICODE
#define _UNICODE

#include <windows.h>
#include <cmath>
#include <functional>
#include <sstream>

// control IDs
enum {
    IDC_RADIO1 = 1001,
    IDC_RADIO2,
    IDC_RADIO3,
    IDC_RADIO4,
    IDC_BUTTON_NEXT,
    IDC_BUTTON_BACK,
    IDC_LABEL_FORMULA,
    IDC_LABEL_P,
    IDC_EDIT_P,
    IDC_LABEL_FV,
    IDC_EDIT_FV,
    IDC_LABEL_CPN,
    IDC_EDIT_CPN,
    IDC_LABEL_N,
    IDC_EDIT_N,
    IDC_LABEL_Y,
    IDC_EDIT_Y,
    IDC_BUTTON_CALC,
    IDC_LABEL_RESULT,
    IDC_EDIT_RESULT,
};

// forward declarations
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
double bisect(std::function<double(double)> f, double low, double high);

// helper to show/hide multiple controls
void ShowGroup(HWND hwnd, const int ids[], int count, BOOL bShow) {
    for (int i = 0; i < count; ++i)
        ShowWindow(GetDlgItem(hwnd, ids[i]), bShow ? SW_SHOW : SW_HIDE);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    // register window class
    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = L"BondCalcWnd";
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    // create main window
    HWND hwnd = CreateWindowEx(
        0,
        L"BondCalcWnd",
        L"Bond Calculator",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 380,
        nullptr, nullptr, hInst, nullptr
    );
    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);

    // message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// simplest bisection root‐finder
double bisect(std::function<double(double)> f, double low, double high) {
    double fl = f(low), fh = f(high);
    if (fl * fh > 0) throw std::runtime_error("no sign change");
    for (int i = 0; i < 60; ++i) {
        double mid = 0.5 * (low + high);
        double fm  = f(mid);
        if (fabs(fm) < 1e-10) return mid;
        if (fl * fm < 0) { high = mid; fh = fm; }
        else             { low  = mid; fl = fm; }
    }
    return 0.5 * (low + high);
}

// read/write a double in an EDIT control
double GetEd(HWND hwnd, int id) {
    wchar_t buf[64];
    GetWindowText(hwnd, buf, 64);
    return _wtof(buf);
}
void SetEd(HWND hwnd, int id, double v) {
    std::wostringstream ss;
    ss.precision(8);
    ss << v;
    SetWindowText(hwnd, ss.str().c_str());
}

// prepare and clear page 2 inputs based on selected formula
void UpdatePage2(HWND hwnd) {
    bool pvCoupon = SendMessage(GetDlgItem(hwnd, IDC_RADIO1), BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmZero  = SendMessage(GetDlgItem(hwnd, IDC_RADIO2), BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmCoup  = SendMessage(GetDlgItem(hwnd, IDC_RADIO3), BM_GETCHECK,0,0)==BST_CHECKED;
    bool pvZero   = SendMessage(GetDlgItem(hwnd, IDC_RADIO4), BM_GETCHECK,0,0)==BST_CHECKED;

    // display the appropriate formula
    const wchar_t* formula =
        pvCoupon ? L"P = CPN*(1/y)*(1 - 1/pow(1+y, N)) + FV/pow(1+y, N)" :
        ytmZero  ? L"y = pow(FV/P, 1.0/N) - 1" :
        ytmCoup  ? L"0 = CPN*(1/y)*(1 - 1/pow(1+y,N)) + FV/pow(1+y,N) - P" :
                   L"P = FV / pow(1+y, N)";
    SetWindowText(GetDlgItem(hwnd, IDC_LABEL_FORMULA), formula);

    // define all input IDs
    const int allIds[] = {
        IDC_LABEL_P,    IDC_EDIT_P,
        IDC_LABEL_FV,   IDC_EDIT_FV,
        IDC_LABEL_CPN,  IDC_EDIT_CPN,
        IDC_LABEL_N,    IDC_EDIT_N,
        IDC_LABEL_Y,    IDC_EDIT_Y
    };
    // hide them all
    ShowGroup(hwnd, allIds, _countof(allIds), FALSE);

    // show only the ones needed
    if (!pvCoupon && !pvZero) {
        // solving for y: need P, FV, CPN, N
        const int ids1[] = {
            IDC_LABEL_P, IDC_EDIT_P,
            IDC_LABEL_FV, IDC_EDIT_FV,
            IDC_LABEL_CPN, IDC_EDIT_CPN,
            IDC_LABEL_N, IDC_EDIT_N
        };
        ShowGroup(hwnd, ids1, _countof(ids1), TRUE);
    }
    else {
        // solving for P: need FV, (CPN?), N, y
        const int ids2[] = {
            IDC_LABEL_FV, IDC_EDIT_FV,
            IDC_LABEL_N,  IDC_EDIT_N,
            IDC_LABEL_Y,  IDC_EDIT_Y
        };
        ShowGroup(hwnd, ids2, _countof(ids2), TRUE);
        if (pvCoupon) {
            ShowWindow(GetDlgItem(hwnd, IDC_LABEL_CPN),  SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_EDIT_CPN),   SW_SHOW);
        }
    }

    // disable the field we will solve for
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_P), !(pvCoupon||pvZero));
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_Y),  (pvCoupon||pvZero));

    // clear all inputs + result
    for (int id : allIds)
        SetWindowText(GetDlgItem(hwnd, id), L"");
    SetWindowText(GetDlgItem(hwnd, IDC_EDIT_RESULT), L"");
}

// perform the calculation and display it
void DoCalc(HWND hwnd) {
    bool pvCoupon = SendMessage(GetDlgItem(hwnd, IDC_RADIO1), BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmZero  = SendMessage(GetDlgItem(hwnd, IDC_RADIO2), BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmCoup  = SendMessage(GetDlgItem(hwnd, IDC_RADIO3), BM_GETCHECK,0,0)==BST_CHECKED;
    bool pvZero   = SendMessage(GetDlgItem(hwnd, IDC_RADIO4), BM_GETCHECK,0,0)==BST_CHECKED;

    double P   = GetEd(hwnd, IDC_EDIT_P);
    double FV  = GetEd(hwnd, IDC_EDIT_FV);
    double CPN = GetEd(hwnd, IDC_EDIT_CPN);
    double N   = GetEd(hwnd, IDC_EDIT_N);
    double y   = GetEd(hwnd, IDC_EDIT_Y);
    double result = 0;

    try {
        if (pvCoupon) {
            result = CPN*(1.0/y)*(1 - pow(1+y,-N)) + FV*pow(1+y,-N);
        }
        else if (pvZero) {
            result = FV * pow(1+y, -N);
        }
        else if (ytmZero) {
            result = pow(FV/P, 1.0/N) - 1.0;
        }
        else { // ytmCoup
            auto f = [&](double yy) {
                return CPN*(1.0/yy)*(1 - pow(1+yy,-N))
                     + FV*pow(1+yy,-N) - P;
            };
            result = bisect(f, 1e-6, 5.0);
        }
        SetEd(hwnd, IDC_EDIT_RESULT, result);
    }
    catch(...) {
        MessageBox(
            hwnd,
            L"Calculation failed (bad input or no convergence).",
            L"Error",
            MB_OK | MB_ICONERROR
        );
    }
}

// window procedure
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    // IDs for page1
    static const int panel1[] = {
        IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4, IDC_BUTTON_NEXT
    };
    // IDs for page2
    static const int panel2[] = {
        IDC_BUTTON_BACK, IDC_LABEL_FORMULA,
        IDC_LABEL_P, IDC_EDIT_P,
        IDC_LABEL_FV, IDC_EDIT_FV,
        IDC_LABEL_CPN, IDC_EDIT_CPN,
        IDC_LABEL_N, IDC_EDIT_N,
        IDC_LABEL_Y, IDC_EDIT_Y,
        IDC_BUTTON_CALC,
        IDC_LABEL_RESULT, IDC_EDIT_RESULT
    };

    switch(msg) {
    case WM_CREATE:
        // --- Page 1 controls ---
        CreateWindow(L"BUTTON", L"PV Coupon Bond",
            WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON,
            20, 20, 200, 20, hwnd, (HMENU)IDC_RADIO1, nullptr, nullptr);
        CreateWindow(L"BUTTON", L"YTM Zero‑Coupon",
            WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON,
            20, 50, 200, 20, hwnd, (HMENU)IDC_RADIO2, nullptr, nullptr);
        CreateWindow(L"BUTTON", L"YTM Coupon Bond",
            WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON,
            20, 80, 200, 20, hwnd, (HMENU)IDC_RADIO3, nullptr, nullptr);
        CreateWindow(L"BUTTON", L"PV Zero‑Coupon",
            WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON,
            20,110,200, 20, hwnd, (HMENU)IDC_RADIO4, nullptr, nullptr);
        CreateWindow(L"BUTTON", L"Next ➔",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            360,110,80,30, hwnd, (HMENU)IDC_BUTTON_NEXT, nullptr, nullptr);
        SendMessage(GetDlgItem(hwnd,IDC_RADIO1), BM_SETCHECK, BST_CHECKED, 0);

        // --- Page 2 controls (hidden) ---
        CreateWindow(L"BUTTON", L"⬅ Go Back",
            WS_CHILD, 20,10,80,25, hwnd, (HMENU)IDC_BUTTON_BACK, nullptr, nullptr);
        CreateWindow(L"STATIC", L"",
            WS_CHILD, 120,15,330,20, hwnd, (HMENU)IDC_LABEL_FORMULA, nullptr, nullptr);

        // labels & edits
        auto MK_LBL = [&](int id, LPCWSTR txt, int x, int y){
            CreateWindow(L"STATIC", txt, WS_CHILD, x,y,60,20, hwnd, (HMENU)id, nullptr, nullptr);
        };
        auto MK_ED = [&](int id, int x, int y){
            CreateWindow(L"EDIT", L"", WS_CHILD|WS_BORDER, x,y,100,20, hwnd, (HMENU)id, nullptr, nullptr);
        };
        MK_LBL(IDC_LABEL_P,  L"P:",      20, 50);   MK_ED(IDC_EDIT_P,   120,50);
        MK_LBL(IDC_LABEL_FV, L"FV:",     260,50);   MK_ED(IDC_EDIT_FV,  320,50);
        MK_LBL(IDC_LABEL_CPN,L"CPN:",     20, 80);   MK_ED(IDC_EDIT_CPN, 120,80);
        MK_LBL(IDC_LABEL_N,  L"N (yrs):",260,80);   MK_ED(IDC_EDIT_N,   320,80);
        MK_LBL(IDC_LABEL_Y,  L"y:",       20,110);   MK_ED(IDC_EDIT_Y,   120,110);

        CreateWindow(L"BUTTON", L"Calculate",
            WS_CHILD, 180,150,100,30, hwnd, (HMENU)IDC_BUTTON_CALC, nullptr, nullptr);

        MK_LBL(IDC_LABEL_RESULT, L"Result:", 20,190);
        CreateWindow(L"EDIT", L"", WS_CHILD|WS_BORDER|ES_READONLY,
            120,190,200,20, hwnd, (HMENU)IDC_EDIT_RESULT, nullptr, nullptr);

        ShowGroup(hwnd, panel2, _countof(panel2), FALSE);
        break;

    case WM_COMMAND:
        switch(LOWORD(wParam)) {
        case IDC_BUTTON_NEXT:
            ShowGroup(hwnd, panel1, _countof(panel1), FALSE);
            ShowGroup(hwnd, panel2, _countof(panel2), TRUE);
            UpdatePage2(hwnd);
            break;
        case IDC_BUTTON_BACK:
            ShowGroup(hwnd, panel2, _countof(panel2), FALSE);
            ShowGroup(hwnd, panel1, _countof(panel1), TRUE);
            break;
        case IDC_BUTTON_CALC:
            DoCalc(hwnd);
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

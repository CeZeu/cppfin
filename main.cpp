// main.cpp
// Win32 GUI application — use W‐suffix APIs and no compound literals

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

// forward
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
double bisect(std::function<double(double)> f, double low, double high);

// helpers to show/hide groups
void ShowGroup(HWND hwnd, const int ids[], int count, BOOL bShow) {
    for (int i = 0; i < count; i++)
        ShowWindow(GetDlgItem(hwnd, ids[i]), bShow ? SW_SHOW : SW_HIDE);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int nCmdShow) {
    static const wchar_t CLASS_NAME[] = L"BondCalcWnd";

    // --- Register window class (wide) ---
    WNDCLASSW wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor       = LoadCursorW(NULL, IDC_ARROW);
    RegisterClassW(&wc);

    // --- Create main window (wide) ---
    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"Bond Calculator",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 380,
        NULL, NULL, hInst, NULL
    );
    if (!hwnd) return 0;
    ShowWindow(hwnd, nCmdShow);

    // --- Standard message loop (wide) ---
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

// bisection root‑finder for coupon‑bond YTM
double bisect(std::function<double(double)> f, double low, double high) {
    double fl = f(low), fh = f(high);
    if (fl * fh > 0) throw std::runtime_error("no sign change");
    for (int i = 0; i < 60; i++) {
        double m  = 0.5 * (low + high);
        double fm = f(m);
        if (fabs(fm) < 1e-10) return m;
        if (fl * fm < 0) { high = m; fh = fm; }
        else             { low  = m; fl = fm; }
    }
    return 0.5 * (low + high);
}

// read/write double from/to an EDIT
double GetEd(HWND hwnd, int id) {
    wchar_t buf[64];
    GetWindowTextW(GetDlgItem(hwnd, id), buf, 64);
    return _wtof(buf);
}
void SetEd(HWND hwnd, int id, double v) {
    std::wostringstream ss;
    ss.precision(8);
    ss << v;
    SetWindowTextW(GetDlgItem(hwnd, id), ss.str().c_str());
}

// update page2: show formula, show only relevant inputs, enable/disable the unknown
void UpdatePage2(HWND hwnd) {
    bool pvCoupon = SendMessageW(GetDlgItem(hwnd, IDC_RADIO1), BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool ytmZero  = SendMessageW(GetDlgItem(hwnd, IDC_RADIO2), BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool ytmCoup  = SendMessageW(GetDlgItem(hwnd, IDC_RADIO3), BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool pvZero   = SendMessageW(GetDlgItem(hwnd, IDC_RADIO4), BM_GETCHECK, 0, 0) == BST_CHECKED;

    // set formula text
    const wchar_t* fm =
      pvCoupon ? L"P = CPN*(1/y)*(1 - 1/pow(1+y, N)) + FV/pow(1+y, N)" :
      ytmZero  ? L"y = pow(FV/P, 1.0/N) - 1" :
      ytmCoup  ? L"0 = CPN*(1/y)*(1 - 1/pow(1+y,N)) + FV/pow(1+y,N) - P" :
                 L"P = FV / pow(1+y, N)";
    SetWindowTextW(GetDlgItem(hwnd, IDC_LABEL_FORMULA), fm);

    // hide all inputs
    const int allIds[] = {
      IDC_LABEL_P,IDC_EDIT_P,IDC_LABEL_FV,IDC_EDIT_FV,
      IDC_LABEL_CPN,IDC_EDIT_CPN,IDC_LABEL_N,IDC_EDIT_N,
      IDC_LABEL_Y,IDC_EDIT_Y
    };
    ShowGroup(hwnd, allIds, _countof(allIds), FALSE);

    // show only what we need
    if (!pvCoupon && !pvZero) {
        // solving for y → need P, FV, CPN, N
        const int ids1[] = {IDC_LABEL_P,IDC_EDIT_P,IDC_LABEL_FV,IDC_EDIT_FV,IDC_LABEL_CPN,IDC_EDIT_CPN,IDC_LABEL_N,IDC_EDIT_N};
        ShowGroup(hwnd, ids1, _countof(ids1), TRUE);
    }
    else {
        // solving for P → need FV, (CPN?), N, y
        const int ids2[] = {IDC_LABEL_FV,IDC_EDIT_FV,IDC_LABEL_N,IDC_EDIT_N,IDC_LABEL_Y,IDC_EDIT_Y};
        ShowGroup(hwnd, ids2, _countof(ids2), TRUE);
        if (pvCoupon) {
            ShowWindow(GetDlgItem(hwnd, IDC_LABEL_CPN), SW_SHOW);
            ShowWindow(GetDlgItem(hwnd, IDC_EDIT_CPN), SW_SHOW);
        }
    }

    // disable the field we're solving for
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_P),    (pvCoupon||pvZero) ? FALSE : TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_EDIT_Y),    (ytmZero||ytmCoup) ? FALSE : TRUE);

    // clear all inputs & result
    for (int id: allIds) SetWindowTextW(GetDlgItem(hwnd, id), L"");
    SetWindowTextW(GetDlgItem(hwnd, IDC_EDIT_RESULT), L"");
}

// do the calculation on page2
void DoCalc(HWND hwnd) {
    bool pvCoupon = SendMessageW(GetDlgItem(hwnd, IDC_RADIO1), BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool ytmZero  = SendMessageW(GetDlgItem(hwnd, IDC_RADIO2), BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool ytmCoup  = SendMessageW(GetDlgItem(hwnd, IDC_RADIO3), BM_GETCHECK, 0, 0) == BST_CHECKED;
    bool pvZero   = SendMessageW(GetDlgItem(hwnd, IDC_RADIO4), BM_GETCHECK, 0, 0) == BST_CHECKED;

    double P   = GetEd(hwnd, IDC_EDIT_P);
    double FV  = GetEd(hwnd, IDC_EDIT_FV);
    double CPN = GetEd(hwnd, IDC_EDIT_CPN);
    double N   = GetEd(hwnd, IDC_EDIT_N);
    double y   = GetEd(hwnd, IDC_EDIT_Y);
    double res = 0;

    try {
      if (pvCoupon) {
        res = CPN*(1.0/y)*(1 - pow(1+y,-N)) + FV*pow(1+y,-N);
      }
      else if (pvZero) {
        res = FV * pow(1+y, -N);
      }
      else if (ytmZero) {
        res = pow(FV/P, 1.0/N) - 1.0;
      }
      else if (ytmCoup) {
        auto f = [&](double yy){
          return CPN*(1.0/yy)*(1 - pow(1+yy,-N)) + FV*pow(1+yy,-N) - P;
        };
        res = bisect(f, 1e-6, 5.0);
      }
      SetEd(hwnd, IDC_EDIT_RESULT, res);
    }
    catch(...) {
      MessageBoxW(hwnd,
        L"Calculation failed (bad input or no convergence).",
        L"Error",
        MB_OK|MB_ICONERROR
      );
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM w, LPARAM l) {
    // panel1 IDs
    static const int panel1[] = {
      IDC_RADIO1,IDC_RADIO2,IDC_RADIO3,IDC_RADIO4,IDC_BUTTON_NEXT
    };
    // panel2 IDs
    static const int panel2[] = {
      IDC_BUTTON_BACK,IDC_LABEL_FORMULA,
      IDC_LABEL_P,IDC_EDIT_P,IDC_LABEL_FV,IDC_EDIT_FV,
      IDC_LABEL_CPN,IDC_EDIT_CPN,IDC_LABEL_N,IDC_EDIT_N,
      IDC_LABEL_Y,IDC_EDIT_Y,IDC_BUTTON_CALC,
      IDC_LABEL_RESULT,IDC_EDIT_RESULT
    };

    switch(msg) {
    case WM_CREATE: {
        // --- Page 1 ---
        CreateWindowW(L"BUTTON", L"PV Coupon Bond", 
          WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON, 20, 20, 200, 20,
          hwnd, (HMENU)IDC_RADIO1, NULL, NULL);
        CreateWindowW(L"BUTTON", L"YTM Zero‐Coupon", 
          WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON, 20, 50, 200, 20,
          hwnd, (HMENU)IDC_RADIO2, NULL, NULL);
        CreateWindowW(L"BUTTON", L"YTM Coupon Bond", 
          WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON, 20, 80, 200, 20,
          hwnd, (HMENU)IDC_RADIO3, NULL, NULL);
        CreateWindowW(L"BUTTON", L"PV Zero‐Coupon", 
          WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON, 20,110,200,20,
          hwnd, (HMENU)IDC_RADIO4, NULL, NULL);
        CreateWindowW(L"BUTTON", L"Next ➔", 
          WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON, 360,110,80,30,
          hwnd, (HMENU)IDC_BUTTON_NEXT, NULL, NULL);
        SendMessageW(GetDlgItem(hwnd,IDC_RADIO1), BM_SETCHECK, BST_CHECKED, 0);

        // --- Page 2 (hidden) ---
        CreateWindowW(L"BUTTON", L"⬅ Go Back", WS_CHILD, 20,10,80,25,
          hwnd, (HMENU)IDC_BUTTON_BACK, NULL, NULL);
        CreateWindowW(L"STATIC", L"", WS_CHILD, 120,15,330,20,
          hwnd, (HMENU)IDC_LABEL_FORMULA, NULL, NULL);

        #define MAKE_LBL_ED(idLbl, txt, x,y) \
          CreateWindowW(L"STATIC", txt, WS_CHILD, x,y,60,20, hwnd, (HMENU)idLbl, NULL, NULL)
        #define MAKE_ED(idEd, x,y) \
          CreateWindowW(L"EDIT", L"", WS_CHILD|WS_BORDER, x,y,100,20, hwnd, (HMENU)idEd, NULL, NULL)

        MAKE_LBL_ED(IDC_LABEL_P,  L"P:",     20, 50);
        MAKE_ED    (IDC_EDIT_P,             120,50);
        MAKE_LBL_ED(IDC_LABEL_FV, L"FV:",    260,50);
        MAKE_ED    (IDC_EDIT_FV,            320,50);
        MAKE_LBL_ED(IDC_LABEL_CPN,L"CPN:",    20,80);
        MAKE_ED    (IDC_EDIT_CPN,           120,80);
        MAKE_LBL_ED(IDC_LABEL_N,  L"N (yrs):",260,80);
        MAKE_ED    (IDC_EDIT_N,            320,80);
        MAKE_LBL_ED(IDC_LABEL_Y,  L"y:",     20,110);
        MAKE_ED    (IDC_EDIT_Y,             120,110);

        CreateWindowW(L"BUTTON", L"Calculate", WS_CHILD, 180,150,100,30,
          hwnd,(HMENU)IDC_BUTTON_CALC,NULL,NULL);

        MAKE_LBL_ED(IDC_LABEL_RESULT, L"Result:", 20,190);
        CreateWindowW(L"EDIT", L"", WS_CHILD|WS_BORDER|ES_READONLY,
          120,190,200,20, hwnd,(HMENU)IDC_EDIT_RESULT,NULL,NULL);
        #undef MAKE_LBL_ED
        #undef MAKE_ED

        ShowGroup(hwnd, panel2, _countof(panel2), FALSE);
        break;
    }

    case WM_COMMAND:
        switch(LOWORD(w)) {
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
    return DefWindowProcW(hwnd, msg, w, l);
}

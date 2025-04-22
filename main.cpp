#define UNICODE
#define _UNICODE

#include <windows.h>
#include <cmath>
#include <functional>
#include <sstream>

// ------------------- control IDs -------------------
enum {
    IDC_RADIO1 = 1001, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4,
    IDC_BUTTON_NEXT,   IDC_BUTTON_BACK,
    IDC_LABEL_FORMULA,
    IDC_LABEL_P,   IDC_EDIT_P,
    IDC_LABEL_FV,  IDC_EDIT_FV,
    IDC_LABEL_CPN, IDC_EDIT_CPN,
    IDC_LABEL_N,   IDC_EDIT_N,
    IDC_LABEL_Y,   IDC_EDIT_Y,
    IDC_BUTTON_CALC,
    IDC_LABEL_RESULT, IDC_EDIT_RESULT,
};

// ---------------- forward decls --------------------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
double bisect(std::function<double(double)> f, double a, double b);

// ------------- helper to show/hide sets ------------
void ShowGroup(HWND h, const int* ids, int n, BOOL show)
{
    for (int i = 0; i < n; ++i)
        ShowWindow(GetDlgItem(h, ids[i]), show ? SW_SHOW : SW_HIDE);
}

// ================= WinMain =========================
int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int nCmd)
{
    WNDCLASS wc{};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hi;
    wc.lpszClassName = L"BondCalcWnd";
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    HWND win = CreateWindowEx(
        0, L"BondCalcWnd", L"Bond Calculator",
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 380,
        nullptr, nullptr, hi, nullptr);
    if (!win) return 0;
    ShowWindow(win, nCmd);

    MSG m;
    while (GetMessage(&m, nullptr, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    return 0;
}

// ===================================================
// ================= implementation ==================
// ===================================================

double bisect(std::function<double(double)> f, double a, double b)
{
    double fa = f(a), fb = f(b);
    if (fa * fb > 0) throw std::runtime_error("no sign change");
    for (int i = 0; i < 60; ++i) {
        double m = .5 * (a + b), fm = f(m);
        if (std::fabs(fm) < 1e-10) return m;
        (fa * fm < 0 ? b : a) = m;
        (fa * fm < 0 ? fb : fa) = fm;
    }
    return .5 * (a + b);
}

double GetEd(HWND h, int id)
{
    wchar_t buf[64]; GetWindowText(GetDlgItem(h, id), buf, 64);
    return _wtof(buf);
}
void SetEd(HWND h, int id, double v)
{
    std::wostringstream ss; ss.precision(8); ss << v;
    SetWindowText(GetDlgItem(h, id), ss.str().c_str());
}

void UpdatePage2(HWND h)
{
    bool pvCoupon = SendMessage(GetDlgItem(h, IDC_RADIO1), BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmZero  = SendMessage(GetDlgItem(h, IDC_RADIO2), BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmCoup  = SendMessage(GetDlgItem(h, IDC_RADIO3), BM_GETCHECK,0,0)==BST_CHECKED;
    bool pvZero   = SendMessage(GetDlgItem(h, IDC_RADIO4), BM_GETCHECK,0,0)==BST_CHECKED;

    const wchar_t* f =
        pvCoupon ? L"P = CPN*(1/y)*(1-1/(1+y)^N)+FV/(1+y)^N" :
        ytmZero  ? L"y = (FV/P)^(1/N) - 1"                   :
        ytmCoup  ? L"0 = CPN*(1/y)*(1-1/(1+y)^N)+FV/(1+y)^N-P":
                   L"P = FV/(1+y)^N";
    SetWindowText(GetDlgItem(h, IDC_LABEL_FORMULA), f);

    const int all[] = {IDC_LABEL_P,IDC_EDIT_P, IDC_LABEL_FV,IDC_EDIT_FV,
                       IDC_LABEL_CPN,IDC_EDIT_CPN, IDC_LABEL_N,IDC_EDIT_N,
                       IDC_LABEL_Y,IDC_EDIT_Y};
    ShowGroup(h, all, _countof(all), FALSE);

    if (!pvCoupon && !pvZero) {
        const int ids[] = {IDC_LABEL_P,IDC_EDIT_P, IDC_LABEL_FV,IDC_EDIT_FV,
                           IDC_LABEL_CPN,IDC_EDIT_CPN, IDC_LABEL_N,IDC_EDIT_N};
        ShowGroup(h, ids, _countof(ids), TRUE);
    } else {
        const int ids[] = {IDC_LABEL_FV,IDC_EDIT_FV, IDC_LABEL_N,IDC_EDIT_N,
                           IDC_LABEL_Y,IDC_EDIT_Y};
        ShowGroup(h, ids, _countof(ids), TRUE);
        if (pvCoupon) {
            ShowWindow(GetDlgItem(h, IDC_LABEL_CPN), SW_SHOW);
            ShowWindow(GetDlgItem(h, IDC_EDIT_CPN),  SW_SHOW);
        }
    }
    EnableWindow(GetDlgItem(h, IDC_EDIT_P), !(pvCoupon||pvZero));
    EnableWindow(GetDlgItem(h, IDC_EDIT_Y),  (pvCoupon||pvZero));

    for (int id: all) SetWindowText(GetDlgItem(h,id), L"");
    SetWindowText(GetDlgItem(h, IDC_EDIT_RESULT), L"");
}

void DoCalc(HWND h)
{
    bool pvCoupon = SendMessage(GetDlgItem(h, IDC_RADIO1), BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmZero  = SendMessage(GetDlgItem(h, IDC_RADIO2), BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmCoup  = SendMessage(GetDlgItem(h, IDC_RADIO3), BM_GETCHECK,0,0)==BST_CHECKED;
    bool pvZero   = SendMessage(GetDlgItem(h, IDC_RADIO4), BM_GETCHECK,0,0)==BST_CHECKED;

    double P   = GetEd(h, IDC_EDIT_P);
    double FV  = GetEd(h, IDC_EDIT_FV);
    double CPN = GetEd(h, IDC_EDIT_CPN);
    double N   = GetEd(h, IDC_EDIT_N);
    double y   = GetEd(h, IDC_EDIT_Y);
    double r=0;

    try {
        if (pvCoupon)           r = CPN*(1/y)*(1-std::pow(1+y,-N))+FV*std::pow(1+y,-N);
        else if (pvZero)        r = FV*std::pow(1+y,-N);
        else if (ytmZero)       r = std::pow(FV/P,1.0/N)-1;
        else {                  // ytmCoup
            auto f=[&](double yy){
                return CPN*(1/yy)*(1-std::pow(1+yy,-N))+FV*std::pow(1+yy,-N)-P;
            };
            r = bisect(f,1e-6,5);
        }
        SetEd(h, IDC_EDIT_RESULT, r);
    } catch(...) {
        MessageBox(h,L"Calculation failed.",L"Error",MB_OK|MB_ICONERROR);
    }
}

// ================= window procedure =================
LRESULT CALLBACK WndProc(HWND h, UINT u, WPARAM w, LPARAM l)
{
    static const int p1[]={IDC_RADIO1,IDC_RADIO2,IDC_RADIO3,IDC_RADIO4,IDC_BUTTON_NEXT};
    static const int p2[]={IDC_BUTTON_BACK,IDC_LABEL_FORMULA,
                           IDC_LABEL_P,IDC_EDIT_P,IDC_LABEL_FV,IDC_EDIT_FV,
                           IDC_LABEL_CPN,IDC_EDIT_CPN,IDC_LABEL_N,IDC_EDIT_N,
                           IDC_LABEL_Y,IDC_EDIT_Y,IDC_BUTTON_CALC,
                           IDC_LABEL_RESULT,IDC_EDIT_RESULT};

    switch(u)
    {
    case WM_CREATE:{
        // ------- Page1 -------
        auto btn=[&](LPCWSTR txt,int y,int id){
            CreateWindow(L"BUTTON",txt,WS_CHILD|WS_VISIBLE|BS_RADIOBUTTON,
                         20,y,200,20,h,(HMENU)(INT_PTR)id,nullptr,nullptr);
        };
        btn(L"PV Coupon Bond",20, IDC_RADIO1);
        btn(L"YTM Zero‑Coupon",50, IDC_RADIO2);
        btn(L"YTM Coupon Bond",80, IDC_RADIO3);
        btn(L"PV Zero‑Coupon",110, IDC_RADIO4);

        CreateWindow(L"BUTTON",L"Next ➔",
                     WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
                     360,110,80,30,h,(HMENU)(INT_PTR)IDC_BUTTON_NEXT,nullptr,nullptr);
        SendMessage(GetDlgItem(h,IDC_RADIO1),BM_SETCHECK,BST_CHECKED,0);

        // ------- Page2 (hidden) -------
        CreateWindow(L"BUTTON",L"⬅ Go Back",WS_CHILD,
                     20,10,80,25,h,(HMENU)(INT_PTR)IDC_BUTTON_BACK,nullptr,nullptr);
        CreateWindow(L"STATIC",L"",WS_CHILD,120,15,330,20,
                     h,(HMENU)(INT_PTR)IDC_LABEL_FORMULA,nullptr,nullptr);

        auto lbl=[&](int id,LPCWSTR t,int x,int y){
            CreateWindow(L"STATIC",t,WS_CHILD,x,y,60,20,
                         h,(HMENU)(INT_PTR)id,nullptr,nullptr);
        };
        auto ed=[&](int id,int x,int y){
            CreateWindow(L"EDIT",L"",WS_CHILD|WS_BORDER,x,y,100,20,
                         h,(HMENU)(INT_PTR)id,nullptr,nullptr);
        };
        lbl(IDC_LABEL_P,  L"P:",      20,50);  ed(IDC_EDIT_P,  120,50);
        lbl(IDC_LABEL_FV, L"FV:",     260,50); ed(IDC_EDIT_FV, 320,50);
        lbl(IDC_LABEL_CPN,L"CPN:",    20,80);  ed(IDC_EDIT_CPN,120,80);
        lbl(IDC_LABEL_N,  L"N (yrs):",260,80); ed(IDC_EDIT_N,  320,80);
        lbl(IDC_LABEL_Y,  L"y:",      20,110); ed(IDC_EDIT_Y,  120,110);

        CreateWindow(L"BUTTON",L"Calculate",WS_CHILD,
                     180,150,100,30,h,(HMENU)(INT_PTR)IDC_BUTTON_CALC,nullptr,nullptr);

        lbl(IDC_LABEL_RESULT,L"Result:",20,190);
        CreateWindow(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_READONLY,
                     120,190,200,20, h,(HMENU)(INT_PTR)IDC_EDIT_RESULT,nullptr,nullptr);

        ShowGroup(h,p2,_countof(p2),FALSE);
        break;}
    case WM_COMMAND:
        switch(LOWORD(w)){
        case IDC_BUTTON_NEXT:
            ShowGroup(h,p1,_countof(p1),FALSE);
            ShowGroup(h,p2,_countof(p2),TRUE);
            UpdatePage2(h);
            break;
        case IDC_BUTTON_BACK:
            ShowGroup(h,p2,_countof(p2),FALSE);
            ShowGroup(h,p1,_countof(p1),TRUE);
            break;
        case IDC_BUTTON_CALC:
            DoCalc(h); break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0); break;
    }
    return DefWindowProc(h,u,w,l);
}

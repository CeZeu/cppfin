/* --------- Bond Calculator (3‑formula, spacious layout) ---------------
 *  Build:  g++ main.cpp -mwindows -O2 -o BondCalc.exe        (MinGW‑w64)
 * ---------------------------------------------------------------------*/
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cmath>
#include <functional>
#include <sstream>

/* ──────────── control IDs ─────────────────────────────── */
enum {
    IDC_RADIO_CPN = 1001,
    IDC_RADIO_YTM_ZERO,
    IDC_RADIO_YTM_COUPON,

    IDC_BTN_NEXT,
    IDC_BTN_BACK,
    IDC_LBL_FORMULA,

    IDC_LBL_RATE,   IDC_EDIT_RATE,
    IDC_LBL_FV,     IDC_EDIT_FV,
    IDC_LBL_PAYPYR, IDC_EDIT_PAYPYR,

    IDC_LBL_P,      IDC_EDIT_P,
    IDC_LBL_CPN,    IDC_EDIT_CPN,
    IDC_LBL_N,      IDC_EDIT_N,
    IDC_LBL_Y,      IDC_EDIT_Y,

    IDC_BTN_CALC,
    IDC_LBL_RESULT, IDC_EDIT_RESULT
};

/* ──────────── helpers ─────────────────────────────────── */
double bisect(std::function<double(double)> f, double a, double b)
{
    double fa=f(a), fb=f(b);
    if (fa*fb > 0) throw std::runtime_error("no sign change");
    for(int i=0;i<60;++i){
        double m=0.5*(a+b), fm=f(m);
        if (std::fabs(fm) < 1e-10) return m;
        (fa*fm<0? b:a)=m;
        (fa*fm<0? fb:fa)=fm;
    }
    return 0.5*(a+b);
}
void ShowGroup(HWND h,const int*ids,int n,BOOL show){
    for(int i=0;i<n;++i) ShowWindow(GetDlgItem(h,ids[i]),show?SW_SHOW:SW_HIDE);
}
double GetD(HWND h,int id){ wchar_t b[64]; GetWindowText(GetDlgItem(h,id),b,64); return _wtof(b);}
void SetD(HWND h,int id,double v,bool pct=false){
    std::wostringstream ss; ss.setf(std::ios::fixed); ss.precision(6);
    pct? ss<<v<<"  ("<<v*100.0<<" %)": ss<<v;
    SetWindowText(GetDlgItem(h,id), ss.str().c_str());
}

/* ──────────── Window procedure ────────────────────────── */
LRESULT CALLBACK WndProc(HWND h,UINT u,WPARAM w,LPARAM l)
{
    static const int page1[]={IDC_RADIO_CPN,IDC_RADIO_YTM_ZERO,IDC_RADIO_YTM_COUPON,IDC_BTN_NEXT};
    static const int page2[]={IDC_BTN_BACK,IDC_LBL_FORMULA,
      IDC_LBL_RATE,IDC_EDIT_RATE, IDC_LBL_FV,IDC_EDIT_FV, IDC_LBL_PAYPYR,IDC_EDIT_PAYPYR,
      IDC_LBL_P,IDC_EDIT_P, IDC_LBL_CPN,IDC_EDIT_CPN, IDC_LBL_N,IDC_EDIT_N, IDC_LBL_Y,IDC_EDIT_Y,
      IDC_BTN_CALC, IDC_LBL_RESULT,IDC_EDIT_RESULT};

    switch(u)
    {
    /* ───── create controls ───── */
    case WM_CREATE:{
        HINSTANCE hi=(HINSTANCE)GetWindowLongPtr(h,GWLP_HINSTANCE);

        /* page‑1 (formula chooser) */
        CreateWindowW(L"BUTTON",L"Coupon Payment",
          WS_CHILD|WS_VISIBLE|WS_GROUP|BS_AUTORADIOBUTTON,
          40,60,200,24,h,(HMENU)IDC_RADIO_CPN,hi,nullptr);
        CreateWindowW(L"BUTTON",L"YTM – Zero‑Coupon",
          WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
          40,100,200,24,h,(HMENU)IDC_RADIO_YTM_ZERO,hi,nullptr);
        CreateWindowW(L"BUTTON",L"YTM – Coupon Bond",
          WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
          40,140,200,24,h,(HMENU)IDC_RADIO_YTM_COUPON,hi,nullptr);
        SendMessageW(GetDlgItem(h,IDC_RADIO_CPN),BM_SETCHECK,BST_CHECKED,0);

        CreateWindowW(L"BUTTON",L"Next ⮕",
          WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
          480,140,90,30,h,(HMENU)IDC_BTN_NEXT,hi,nullptr);

        /* page‑2 (hidden initially) */
        CreateWindowW(L"BUTTON",L"⬅ Go Back",
          WS_CHILD, 20,15,90,28,h,(HMENU)IDC_BTN_BACK,hi,nullptr);
        CreateWindowW(L"STATIC",L"",WS_CHILD|SS_LEFT,
          130,18,480,22,h,(HMENU)IDC_LBL_FORMULA,hi,nullptr);

        auto lbl=[&](int id,LPCWSTR t,int x,int y){
            CreateWindowW(L"STATIC",t,WS_CHILD|SS_RIGHT, x,y,110,20,h,(HMENU)id,hi,nullptr);};
        auto ed=[&](int id,int x,int y){
            CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL, x,y,120,22,h,(HMENU)id,hi,nullptr);};

        int y0=70, dy=40;
        lbl(IDC_LBL_RATE,L"Coupon Rate",  20,y0);               ed(IDC_EDIT_RATE,150,y0);
        lbl(IDC_LBL_FV,  L"Face Value",   320,y0);               ed(IDC_EDIT_FV,  450,y0);

        lbl(IDC_LBL_PAYPYR,L"# Pay/Year", 20,y0+dy);             ed(IDC_EDIT_PAYPYR,150,y0+dy);
        lbl(IDC_LBL_P,   L"Price P",      320,y0+dy);            ed(IDC_EDIT_P,  450,y0+dy);

        lbl(IDC_LBL_CPN, L"Coupon CPN",   20,y0+2*dy);           ed(IDC_EDIT_CPN,150,y0+2*dy);
        lbl(IDC_LBL_N,   L"N (years)",    320,y0+2*dy);          ed(IDC_EDIT_N,  450,y0+2*dy);

        lbl(IDC_LBL_Y,   L"Yield y",      20,y0+3*dy);           ed(IDC_EDIT_Y,  150,y0+3*dy);

        CreateWindowW(L"BUTTON",L"Calculate",WS_CHILD|BS_DEFPUSHBUTTON,
                      260, y0+4*dy, 120,32, h,(HMENU)IDC_BTN_CALC,hi,nullptr);

        lbl(IDC_LBL_RESULT,L"Result",20,y0+5*dy);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_READONLY|ES_AUTOHSCROLL,
                      150,y0+5*dy, 420,24, h,(HMENU)IDC_EDIT_RESULT,hi,nullptr);

        ShowGroup(h,page2,_countof(page2),FALSE);
        break;}

    /* ───── button logic ───── */
    case WM_COMMAND:
        switch(LOWORD(w))
        {
        /* ----- page navigation ----- */
        case IDC_BTN_NEXT:{
            ShowGroup(h,page1,_countof(page1),FALSE);
            ShowGroup(h,page2,_countof(page2),TRUE);

            /* prepare inputs for selected formula */
            BOOL cpn   = SendMessageW(GetDlgItem(h,IDC_RADIO_CPN),BM_GETCHECK,0,0)==BST_CHECKED;
            BOOL y0    = SendMessageW(GetDlgItem(h,IDC_RADIO_YTM_ZERO),BM_GETCHECK,0,0)==BST_CHECKED;

            /* hide all variable rows first */
            const int all[]= {
                IDC_LBL_RATE,IDC_EDIT_RATE, IDC_LBL_FV,IDC_EDIT_FV, IDC_LBL_PAYPYR,IDC_EDIT_PAYPYR,
                IDC_LBL_P,IDC_EDIT_P, IDC_LBL_CPN,IDC_EDIT_CPN, IDC_LBL_N,IDC_EDIT_N, IDC_LBL_Y,IDC_EDIT_Y };
            ShowGroup(h,all,_countof(all),FALSE);

            if (cpn){ /* Coupon Payment formula */
                SetWindowText(GetDlgItem(h,IDC_LBL_FORMULA),
                  L"CPN = (Coupon Rate × FV) / (# Payments per Year)");
                const int vis[]={IDC_LBL_RATE,IDC_EDIT_RATE, IDC_LBL_FV,IDC_EDIT_FV,
                                 IDC_LBL_PAYPYR,IDC_EDIT_PAYPYR};
                ShowGroup(h,vis,_countof(vis),TRUE);
                EnableWindow(GetDlgItem(h,IDC_EDIT_RATE),TRUE);
                EnableWindow(GetDlgItem(h,IDC_EDIT_FV),TRUE);
                EnableWindow(GetDlgItem(h,IDC_EDIT_PAYPYR),TRUE);
            }
            else if (y0){ /* YTM zero coupon */
                SetWindowText(GetDlgItem(h,IDC_LBL_FORMULA),
                  L"y = (FV / P)^(1/N) – 1");
                const int vis[]={IDC_LBL_FV,IDC_EDIT_FV, IDC_LBL_P,IDC_EDIT_P,
                                 IDC_LBL_N,IDC_EDIT_N};
                ShowGroup(h,vis,_countof(vis),TRUE);
            }
            else { /* YTM coupon bond */
                SetWindowText(GetDlgItem(h,IDC_LBL_FORMULA),
 L"0 = CPN·(1/y)(1 − 1/(1+y)^N) + FV/(1+y)^N − P");
                const int vis[]={IDC_LBL_P,IDC_EDIT_P, IDC_LBL_FV,IDC_EDIT_FV,
                                 IDC_LBL_CPN,IDC_EDIT_CPN, IDC_LBL_N,IDC_EDIT_N};
                ShowGroup(h,vis,_countof(vis),TRUE);
            }
            /* clear all edit boxes & result */
            for (int id : page2) if (HIWORD(id)==0xFFFF){} // none
            const int clr[]={IDC_EDIT_RATE,IDC_EDIT_FV,IDC_EDIT_PAYPYR,
                             IDC_EDIT_P,IDC_EDIT_CPN,IDC_EDIT_N,IDC_EDIT_Y,IDC_EDIT_RESULT};
            for(int id:clr) SetWindowText(GetDlgItem(h,id),L"");
            break;}

        case IDC_BTN_BACK:
            ShowGroup(h,page2,_countof(page2),FALSE);
            ShowGroup(h,page1,_countof(page1),TRUE);
            break;

        /* ----- Calculate ----- */
        case IDC_BTN_CALC:{
            BOOL cpn   = SendMessageW(GetDlgItem(h,IDC_RADIO_CPN),BM_GETCHECK,0,0)==BST_CHECKED;
            BOOL y0    = SendMessageW(GetDlgItem(h,IDC_RADIO_YTM_ZERO),BM_GETCHECK,0,0)==BST_CHECKED;
            try{
                if (cpn){
                    double rate = GetD(h,IDC_EDIT_RATE);
                    double fv   = GetD(h,IDC_EDIT_FV);
                    double m    = GetD(h,IDC_EDIT_PAYPYR);
                    SetD(h,IDC_EDIT_RESULT, rate*fv/m);
                }else if (y0){
                    double fv=GetD(h,IDC_EDIT_FV), p=GetD(h,IDC_EDIT_P), N=GetD(h,IDC_EDIT_N);
                    double y= std::pow(fv/p,1.0/N)-1.0;
                    SetD(h,IDC_EDIT_RESULT,y,true);
                }else{
                    double P=GetD(h,IDC_EDIT_P), FV=GetD(h,IDC_EDIT_FV);
                    double CPN=GetD(h,IDC_EDIT_CPN), N=GetD(h,IDC_EDIT_N);
                    auto f=[&](double y){return CPN*(1.0/y)*(1-std::pow(1+y,-N))+FV*std::pow(1+y,-N)-P;};
                    double y=bisect(f,1e-6,5);
                    SetD(h,IDC_EDIT_RESULT,y,true);
                }
            }catch(...){
                MessageBoxW(h,L"Computation failed. Check inputs.",L"Error",
                            MB_OK|MB_ICONERROR);
            }
            break;}
        }
        break;

    case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProcW(h,u,w,l);
}

/* ───────────── WinMain registration & window size ───────────── */
int WINAPI wWinMain(HINSTANCE hi,HINSTANCE,LPWSTR,int)
{
    WNDCLASSW wc{}; wc.lpfnWndProc=WndProc; wc.hInstance=hi;
    wc.lpszClassName=L"BondCalcWnd"; wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    RegisterClassW(&wc);
    HWND w=CreateWindowW(L"BondCalcWnd",L"Bond Calculator",
        WS_OVERLAPPEDWINDOW&~WS_THICKFRAME,
        CW_USEDEFAULT,CW_USEDEFAULT,640,500,
        nullptr,nullptr,hi,nullptr);
    ShowWindow(w,SW_SHOW);
    MSG m; while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    return 0;
}

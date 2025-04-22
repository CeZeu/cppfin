#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cmath>
#include <functional>
#include <sstream>
#include <stdexcept>

/* ─────── control IDs ─────── */
enum { IDC_RADIO_CPN = 1001, IDC_RADIO_YTM_ZERO, IDC_RADIO_YTM_COUPON,
       IDC_BTN_NEXT, IDC_BTN_BACK, IDC_LBL_FORMULA,
       IDC_LBL_RATE,   IDC_EDIT_RATE,
       IDC_LBL_FV,     IDC_EDIT_FV,
       IDC_LBL_PAYPYR, IDC_EDIT_PAYPYR,
       IDC_LBL_P,      IDC_EDIT_P,
       IDC_LBL_CPN,    IDC_EDIT_CPN,
       IDC_LBL_N,      IDC_EDIT_N,
       IDC_LBL_Y,      IDC_EDIT_Y,
       IDC_BTN_CALC,   IDC_LBL_RESULT, IDC_EDIT_RESULT };

/* ─────── helpers ─────── */
double bisect(std::function<double(double)> f,double a,double b){
    double fa=f(a),fb=f(b); if(fa*fb>0)throw std::runtime_error("range");
    for(int i=0;i<60;++i){ double m=.5*(a+b),fm=f(m);
        if(fabs(fm)<1e-10)return m;
        if(fa*fm<0){b=m;fb=fm;}else{a=m;fa=fm;}}
    return .5*(a+b);
}
void ShowGroup(HWND h,const int*ids,int n,BOOL show){
    for(int i=0;i<n;++i) ShowWindow(GetDlgItem(h,ids[i]),show?SW_SHOW:SW_HIDE);}
double GetD(HWND h,int id){wchar_t b[64];GetWindowTextW(GetDlgItem(h,id),b,64);return _wtof(b);}
void SetD(HWND h,int id,double v,bool pct=false){
    std::wostringstream ss;ss.setf(std::ios::fixed);ss.precision(6);
    if(pct) ss<<v<<"  ("<<v*100.0<<" %)"; else ss<<v;
    SetWindowTextW(GetDlgItem(h,id),ss.str().c_str());
}
void ClearEdits(HWND h){
    const int eds[]={IDC_EDIT_RATE,IDC_EDIT_FV,IDC_EDIT_PAYPYR,
                     IDC_EDIT_P,IDC_EDIT_CPN,IDC_EDIT_N,IDC_EDIT_Y,IDC_EDIT_RESULT};
    for(int id:eds) SetWindowTextW(GetDlgItem(h,id),L"");
}

/* ─────── window proc ─────── */
LRESULT CALLBACK WndProc(HWND h,UINT u,WPARAM w,LPARAM){
    static const int pg1[]={IDC_RADIO_CPN,IDC_RADIO_YTM_ZERO,IDC_RADIO_YTM_COUPON,IDC_BTN_NEXT};
    static const int pg2[]={IDC_BTN_BACK,IDC_LBL_FORMULA,
        IDC_LBL_RATE,IDC_EDIT_RATE, IDC_LBL_FV,IDC_EDIT_FV, IDC_LBL_PAYPYR,IDC_EDIT_PAYPYR,
        IDC_LBL_P,IDC_EDIT_P, IDC_LBL_CPN,IDC_EDIT_CPN, IDC_LBL_N,IDC_EDIT_N, IDC_LBL_Y,IDC_EDIT_Y,
        IDC_BTN_CALC, IDC_LBL_RESULT,IDC_EDIT_RESULT};

    switch(u){
    case WM_CREATE:{
        HINSTANCE hi=(HINSTANCE)GetWindowLongPtrW(h,GWLP_HINSTANCE);

        /* page‑1 */
        CreateWindowW(L"BUTTON",L"Coupon Payment",
          WS_CHILD|WS_VISIBLE|WS_GROUP|BS_AUTORADIOBUTTON,
          40,60,220,24,h,(HMENU)(INT_PTR)IDC_RADIO_CPN,hi,nullptr);
        CreateWindowW(L"BUTTON",L"YTM – Zero‑Coupon Bond",
          WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
          40,100,220,24,h,(HMENU)(INT_PTR)IDC_RADIO_YTM_ZERO,hi,nullptr);
        CreateWindowW(L"BUTTON",L"YTM – Coupon Bond",
          WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
          40,140,220,24,h,(HMENU)(INT_PTR)IDC_RADIO_YTM_COUPON,hi,nullptr);
        SendMessageW(GetDlgItem(h,IDC_RADIO_CPN),BM_SETCHECK,BST_CHECKED,0);
        CreateWindowW(L"BUTTON",L"Next ⮕",
          WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
          480,140,90,30,h,(HMENU)(INT_PTR)IDC_BTN_NEXT,hi,nullptr);

        /* page‑2 */
        CreateWindowW(L"BUTTON",L"⬅ Go Back",WS_CHILD,
          20,15,90,28,h,(HMENU)(INT_PTR)IDC_BTN_BACK,hi,nullptr);
        CreateWindowW(L"STATIC",L"",WS_CHILD|SS_LEFT,
          130,18,480,22,h,(HMENU)(INT_PTR)IDC_LBL_FORMULA,hi,nullptr);

        /* labels */
        CreateWindowW(L"STATIC",L"Coupon Rate",WS_CHILD|SS_RIGHT, 20, 70,110,20,h,(HMENU)(INT_PTR)IDC_LBL_RATE,hi,nullptr);
        CreateWindowW(L"STATIC",L"Face Value", WS_CHILD|SS_RIGHT,320, 70,110,20,h,(HMENU)(INT_PTR)IDC_LBL_FV,hi,nullptr);
        CreateWindowW(L"STATIC",L"# Pay/Year", WS_CHILD|SS_RIGHT, 20,110,110,20,h,(HMENU)(INT_PTR)IDC_LBL_PAYPYR,hi,nullptr);
        CreateWindowW(L"STATIC",L"Price P",    WS_CHILD|SS_RIGHT,320,110,110,20,h,(HMENU)(INT_PTR)IDC_LBL_P,hi,nullptr);
        CreateWindowW(L"STATIC",L"Coupon CPN", WS_CHILD|SS_RIGHT, 20,150,110,20,h,(HMENU)(INT_PTR)IDC_LBL_CPN,hi,nullptr);
        CreateWindowW(L"STATIC",L"N (years)",  WS_CHILD|SS_RIGHT,320,150,110,20,h,(HMENU)(INT_PTR)IDC_LBL_N,hi,nullptr);
        CreateWindowW(L"STATIC",L"Yield y",    WS_CHILD|SS_RIGHT, 20,190,110,20,h,(HMENU)(INT_PTR)IDC_LBL_Y,hi,nullptr);

        /* edits */
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,150, 70,120,22,h,(HMENU)(INT_PTR)IDC_EDIT_RATE,hi,nullptr);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,450, 70,120,22,h,(HMENU)(INT_PTR)IDC_EDIT_FV,hi,nullptr);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,150,110,120,22,h,(HMENU)(INT_PTR)IDC_EDIT_PAYPYR,hi,nullptr);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,450,110,120,22,h,(HMENU)(INT_PTR)IDC_EDIT_P,hi,nullptr);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,150,150,120,22,h,(HMENU)(INT_PTR)IDC_EDIT_CPN,hi,nullptr);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,450,150,120,22,h,(HMENU)(INT_PTR)IDC_EDIT_N,hi,nullptr);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,150,190,120,22,h,(HMENU)(INT_PTR)IDC_EDIT_Y,hi,nullptr);

        CreateWindowW(L"BUTTON",L"Calculate",WS_CHILD|BS_DEFPUSHBUTTON,
            260,240,120,30,h,(HMENU)(INT_PTR)IDC_BTN_CALC,hi,nullptr);

        CreateWindowW(L"STATIC",L"Result",WS_CHILD|SS_RIGHT,
            20,290,110,20,h,(HMENU)(INT_PTR)IDC_LBL_RESULT,hi,nullptr);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_READONLY|ES_AUTOHSCROLL,
            150,290,420,24,h,(HMENU)(INT_PTR)IDC_EDIT_RESULT,hi,nullptr);

        ShowGroup(h,pg2,_countof(pg2),FALSE);
        break;}

    /* ---------- Commands ---------- */
    case WM_COMMAND:
        switch(LOWORD(w)){
        case IDC_BTN_NEXT:{
            ShowGroup(h,pg1,_countof(pg1),FALSE);
            ShowGroup(h,pg2,_countof(pg2),TRUE); ClearEdits(h);

            bool cpn=SendMessageW(GetDlgItem(h,IDC_RADIO_CPN),BM_GETCHECK,0,0)==BST_CHECKED;
            bool y0 =SendMessageW(GetDlgItem(h,IDC_RADIO_YTM_ZERO),BM_GETCHECK,0,0)==BST_CHECKED;

            const int all[]={IDC_LBL_RATE,IDC_EDIT_RATE,IDC_LBL_FV,IDC_EDIT_FV,
                             IDC_LBL_PAYPYR,IDC_EDIT_PAYPYR,IDC_LBL_P,IDC_EDIT_P,
                             IDC_LBL_CPN,IDC_EDIT_CPN,IDC_LBL_N,IDC_EDIT_N,IDC_LBL_Y,IDC_EDIT_Y};
            ShowGroup(h,all,_countof(all),FALSE);

            if(cpn){
                SetWindowTextW(GetDlgItem(h,IDC_LBL_FORMULA),
                    L"CPN = (Coupon Rate × FV) / (# Payments per Year)");
                const int vis[]={IDC_LBL_RATE,IDC_EDIT_RATE,IDC_LBL_FV,IDC_EDIT_FV,
                                 IDC_LBL_PAYPYR,IDC_EDIT_PAYPYR};
                ShowGroup(h,vis,_countof(vis),TRUE);
            }else if(y0){
                SetWindowTextW(GetDlgItem(h,IDC_LBL_FORMULA),
                    L"y = (FV / P)^(1/N) – 1");
                const int vis[]={IDC_LBL_FV,IDC_EDIT_FV,IDC_LBL_P,IDC_EDIT_P,
                                 IDC_LBL_N,IDC_EDIT_N};
                ShowGroup(h,vis,_countof(vis),TRUE);
            }else{
                SetWindowTextW(GetDlgItem(h,IDC_LBL_FORMULA),
                    L"0 = CPN·(1/y)(1 − 1/(1+y)^N) + FV/(1+y)^N − P");
                const int vis[]={IDC_LBL_P,IDC_EDIT_P,IDC_LBL_FV,IDC_EDIT_FV,
                                 IDC_LBL_CPN,IDC_EDIT_CPN,IDC_LBL_N,IDC_EDIT_N};
                ShowGroup(h,vis,_countof(vis),TRUE);
            }
            break;}

        case IDC_BTN_BACK:
            ShowGroup(h,pg2,_countof(pg2),FALSE);
            ShowGroup(h,pg1,_countof(pg1),TRUE);
            break;

        case IDC_BTN_CALC:{
            bool cpn=SendMessageW(GetDlgItem(h,IDC_RADIO_CPN),BM_GETCHECK,0,0)==BST_CHECKED;
            bool y0 =SendMessageW(GetDlgItem(h,IDC_RADIO_YTM_ZERO),BM_GETCHECK,0,0)==BST_CHECKED;
            try{
                if(cpn){
                    double r=GetD(h,IDC_EDIT_RATE), fv=GetD(h,IDC_EDIT_FV), m=GetD(h,IDC_EDIT_PAYPYR);
                    SetD(h,IDC_EDIT_RESULT,r*fv/m);
                }else if(y0){
                    double fv=GetD(h,IDC_EDIT_FV),P=GetD(h,IDC_EDIT_P),N=GetD(h,IDC_EDIT_N);
                    SetD(h,IDC_EDIT_RESULT,std::pow(fv/P,1.0/N)-1.0,true);
                }else{
                    double P=GetD(h,IDC_EDIT_P),FV=GetD(h,IDC_EDIT_FV),CPN=GetD(h,IDC_EDIT_CPN),N=GetD(h,IDC_EDIT_N);
                    auto f=[&](double y){return CPN*(1.0/y)*(1-std::pow(1+y,-N))+FV*std::pow(1+y,-N)-P;};
                    SetD(h,IDC_EDIT_RESULT,bisect(f,1e-6,5.0),true);
                }
            }catch(...){
                MessageBoxW(h,L"Computation failed. Check inputs.",L"Error",MB_OK|MB_ICONERROR);
            }
            break;}
        }
        break;

    case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProcW(h,u,w,l);
}

/* ─────── WinMain ─────── */
int WINAPI WinMain(HINSTANCE hi,HINSTANCE,LPSTR,int){
    WNDCLASSW wc{};wc.lpfnWndProc=WndProc;wc.hInstance=hi;
    wc.lpszClassName=L"BondCalcWnd";wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    RegisterClassW(&wc);
    HWND w=CreateWindowW(L"BondCalcWnd",L"Bond Calculator",
        WS_OVERLAPPEDWINDOW&~WS_THICKFRAME,
        CW_USEDEFAULT,CW_USEDEFAULT,640,500,
        nullptr,nullptr,hi,nullptr);
    ShowWindow(w,SW_SHOW);
    MSG m;while(GetMessageW(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessageW(&m);}
    return 0;
}

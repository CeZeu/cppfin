/*  Bond Calculator – three formulas, big clean UI               */
/*  Build:  g++ main.cpp -mwindows -O2 -o BondCalc.exe            */
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cmath>
#include <functional>
#include <sstream>
#include <iomanip>

/* ────────── control IDs ────────── */
enum {
    IDC_RADIO_CPN = 1001, IDC_RADIO_YTM_ZERO, IDC_RADIO_YTM_COUPON,
    IDC_BTN_NEXT, IDC_BTN_BACK, IDC_LBL_FORMULA,
    IDC_LBL_RATE, IDC_EDIT_RATE, IDC_LBL_FV, IDC_EDIT_FV,
    IDC_LBL_PAYPYR, IDC_EDIT_PAYPYR, IDC_LBL_P, IDC_EDIT_P,
    IDC_LBL_CPN, IDC_EDIT_CPN, IDC_LBL_N, IDC_EDIT_N,
    IDC_LBL_Y, IDC_EDIT_Y, IDC_BTN_CALC, IDC_LBL_RESULT, IDC_EDIT_RESULT
};

/* ────────── helpers ────────── */
double bisect(std::function<double(double)> f,double a,double b){
    double fa=f(a),fb=f(b); if(fa*fb>0)throw std::runtime_error("no sign");
    for(int i=0;i<60;++i){ double m=.5*(a+b),fm=f(m);
        if(fabs(fm)<1e-10) return m;
        (fa*fm<0? b:a)=m; (fa*fm<0? fb:fa)=fm;}
    return .5*(a+b);
}
void ShowGroup(HWND h,const int*ids,int n,BOOL on){
    for(int i=0;i<n;++i) ShowWindow(GetDlgItem(h,ids[i]),on?SW_SHOW:SW_HIDE);}
double GetD(HWND h,int id){ wchar_t buf[64]; GetWindowTextW(GetDlgItem(h,id),buf,64); return _wtof(buf);}
void SetD(HWND h,int id,double v,bool pct=false){
    std::wostringstream ss; ss.setf(std::ios::fixed); ss<<std::setprecision(6);
    ss<<v; if(pct) ss<<"  ("<<std::setprecision(4)<<v*100.0<<" %)";
    SetWindowTextW(GetDlgItem(h,id), ss.str().c_str());
}
void ClearEdits(HWND h){
    const int eds[]={IDC_EDIT_RATE,IDC_EDIT_FV,IDC_EDIT_PAYPYR,
                     IDC_EDIT_P,IDC_EDIT_CPN,IDC_EDIT_N,IDC_EDIT_Y,IDC_EDIT_RESULT};
    for(int id:eds) SetWindowTextW(GetDlgItem(h,id),L"");
}

/* ────────── window proc ────────── */
LRESULT CALLBACK WndProc(HWND hWnd,UINT msg,WPARAM wp,LPARAM lp)
{
    static const int pg1[]={IDC_RADIO_CPN,IDC_RADIO_YTM_ZERO,IDC_RADIO_YTM_COUPON,IDC_BTN_NEXT};
    static const int pg2[]={IDC_BTN_BACK,IDC_LBL_FORMULA,
        IDC_LBL_RATE,IDC_EDIT_RATE,IDC_LBL_FV,IDC_EDIT_FV,
        IDC_LBL_PAYPYR,IDC_EDIT_PAYPYR,IDC_LBL_P,IDC_EDIT_P,
        IDC_LBL_CPN,IDC_EDIT_CPN,IDC_LBL_N,IDC_EDIT_N,
        IDC_LBL_Y,IDC_EDIT_Y,IDC_BTN_CALC,IDC_LBL_RESULT,IDC_EDIT_RESULT};

    switch(msg){
    case WM_CREATE:{
        HINSTANCE hi=(HINSTANCE)GetWindowLongPtrW(hWnd,GWLP_HINSTANCE);

        /* ── First page ── */
        CreateWindowW(L"BUTTON",L"Coupon Payment",
          WS_CHILD|WS_VISIBLE|WS_GROUP|BS_AUTORADIOBUTTON,
          60,80,260,28,hWnd,(HMENU)(INT_PTR)IDC_RADIO_CPN,hi,nullptr);

        CreateWindowW(L"BUTTON",L"YTM – Zero‑Coupon Bond",
          WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
          60,130,260,28,hWnd,(HMENU)(INT_PTR)IDC_RADIO_YTM_ZERO,hi,nullptr);

        CreateWindowW(L"BUTTON",L"YTM – Coupon Bond",
          WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
          60,180,260,28,hWnd,(HMENU)(INT_PTR)IDC_RADIO_YTM_COUPON,hi,nullptr);

        SendMessageW(GetDlgItem(hWnd,IDC_RADIO_CPN),BM_SETCHECK,BST_CHECKED,0);

        CreateWindowW(L"BUTTON",L"Next ⮕",WS_CHILD|WS_VISIBLE|BS_DEFPUSHBUTTON,
          720,180,110,36,hWnd,(HMENU)(INT_PTR)IDC_BTN_NEXT,hi,nullptr);

        /* ── Second page (hidden) ── */
        CreateWindowW(L"BUTTON",L"⬅ Go Back",WS_CHILD,
          30,20,110,32,hWnd,(HMENU)(INT_PTR)IDC_BTN_BACK,hi,nullptr);

        CreateWindowW(L"STATIC",L"",WS_CHILD|SS_LEFT,
          160,24,680,30,hWnd,(HMENU)(INT_PTR)IDC_LBL_FORMULA,hi,nullptr);

        int xLbl1=30,xEd1=180,xLbl2=470,xEd2=620,row=90,dh=48;

        #define MAKE_LBL(id,text,x,y) \
          CreateWindowW(L"STATIC",text,WS_CHILD|SS_RIGHT,x,y,140,24,hWnd,(HMENU)(INT_PTR)id,hi,nullptr)
        #define MAKE_ED(id,x,y) \
          CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,x,y,150,26,hWnd,(HMENU)(INT_PTR)id,hi,nullptr)

        MAKE_LBL(IDC_LBL_RATE,  L"Coupon Rate",  xLbl1,row); MAKE_ED(IDC_EDIT_RATE,  xEd1,row);
        MAKE_LBL(IDC_LBL_FV,    L"Face Value",   xLbl2,row); MAKE_ED(IDC_EDIT_FV,    xEd2,row);

        MAKE_LBL(IDC_LBL_PAYPYR,L"# Pay/Year",   xLbl1,row+dh); MAKE_ED(IDC_EDIT_PAYPYR,xEd1,row+dh);
        MAKE_LBL(IDC_LBL_P,     L"Price P",      xLbl2,row+dh); MAKE_ED(IDC_EDIT_P,     xEd2,row+dh);

        MAKE_LBL(IDC_LBL_CPN,   L"Coupon CPN",   xLbl1,row+2*dh); MAKE_ED(IDC_EDIT_CPN, xEd1,row+2*dh);
        MAKE_LBL(IDC_LBL_N,     L"N (years)",    xLbl2,row+2*dh); MAKE_ED(IDC_EDIT_N,   xEd2,row+2*dh);

        MAKE_LBL(IDC_LBL_Y,     L"Yield y",      xLbl1,row+3*dh); MAKE_ED(IDC_EDIT_Y,   xEd1,row+3*dh);

        CreateWindowW(L"BUTTON",L"Calculate",WS_CHILD|BS_DEFPUSHBUTTON,
          390,row+4*dh,120,34,hWnd,(HMENU)(INT_PTR)IDC_BTN_CALC,hi,nullptr);

        MAKE_LBL(IDC_LBL_RESULT,L"Result",xLbl1,row+5*dh);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_READONLY|ES_AUTOHSCROLL,
          xEd1,row+5*dh,560,26,hWnd,(HMENU)(INT_PTR)IDC_EDIT_RESULT,hi,nullptr);

        #undef MAKE_LBL
        #undef MAKE_ED

        ShowGroup(hWnd,pg2,_countof(pg2),FALSE);
        break;}

    /* ── COMMANDS ── */
    case WM_COMMAND:
        switch(LOWORD(wp)){
        case IDC_BTN_NEXT:{
            ShowGroup(hWnd,pg1,_countof(pg1),FALSE);
            EnableWindow(GetDlgItem(hWnd,IDC_BTN_NEXT),FALSE);
            ShowGroup(hWnd,pg2,_countof(pg2),TRUE);
            ClearEdits(hWnd);

            bool isCPN = SendMessageW(GetDlgItem(hWnd,IDC_RADIO_CPN),BM_GETCHECK,0,0)==BST_CHECKED;
            bool isZero= SendMessageW(GetDlgItem(hWnd,IDC_RADIO_YTM_ZERO),BM_GETCHECK,0,0)==BST_CHECKED;

            /* hide all rows */
            const int allRows[]={IDC_LBL_RATE,IDC_EDIT_RATE,IDC_LBL_FV,IDC_EDIT_FV,
                                 IDC_LBL_PAYPYR,IDC_EDIT_PAYPYR,IDC_LBL_P,IDC_EDIT_P,
                                 IDC_LBL_CPN,IDC_EDIT_CPN,IDC_LBL_N,IDC_EDIT_N,
                                 IDC_LBL_Y,IDC_EDIT_Y};
            ShowGroup(hWnd,allRows,_countof(allRows),FALSE);

            if(isCPN){
                SetWindowTextW(GetDlgItem(hWnd,IDC_LBL_FORMULA),
                    L"CPN = (Coupon Rate × FV) / (# Payments per Year)");
                const int v[]={IDC_LBL_RATE,IDC_EDIT_RATE,IDC_LBL_FV,IDC_EDIT_FV,
                               IDC_LBL_PAYPYR,IDC_EDIT_PAYPYR};
                ShowGroup(hWnd,v,_countof(v),TRUE);
            }else if(isZero){
                SetWindowTextW(GetDlgItem(hWnd,IDC_LBL_FORMULA),
                    L"y = (FV / P)^(1/N) – 1");
                const int v[]={IDC_LBL_FV,IDC_EDIT_FV,IDC_LBL_P,IDC_EDIT_P,
                               IDC_LBL_N,IDC_EDIT_N};
                ShowGroup(hWnd,v,_countof(v),TRUE);
            }else{
                SetWindowTextW(GetDlgItem(hWnd,IDC_LBL_FORMULA),
                    L"0 = CPN·(1/y)(1 − 1/(1+y)^N) + FV/(1+y)^N − P");
                const int v[]={IDC_LBL_P,IDC_EDIT_P,IDC_LBL_FV,IDC_EDIT_FV,
                               IDC_LBL_CPN,IDC_EDIT_CPN,IDC_LBL_N,IDC_EDIT_N};
                ShowGroup(hWnd,v,_countof(v),TRUE);
            }
            break;}

        case IDC_BTN_BACK:
            ShowGroup(hWnd,pg2,_countof(pg2),FALSE);
            ShowGroup(hWnd,pg1,_countof(pg1),TRUE);
            EnableWindow(GetDlgItem(hWnd,IDC_BTN_NEXT),TRUE);
            ClearEdits(hWnd);
            break;

        case IDC_BTN_CALC:{
            bool cpn  = SendMessageW(GetDlgItem(hWnd,IDC_RADIO_CPN),BM_GETCHECK,0,0)==BST_CHECKED;
            bool y0   = SendMessageW(GetDlgItem(hWnd,IDC_RADIO_YTM_ZERO),BM_GETCHECK,0,0)==BST_CHECKED;
            try{
                if(cpn){
                    double rate = GetD(hWnd,IDC_EDIT_RATE);
                    if(rate>1) rate /= 100.0;            // treat 5 → 0.05
                    double fv   = GetD(hWnd,IDC_EDIT_FV);
                    double m    = GetD(hWnd,IDC_EDIT_PAYPYR);
                    SetD(hWnd,IDC_EDIT_RESULT, rate*fv/m );
                }else if(y0){
                    double fv=GetD(hWnd,IDC_EDIT_FV),
                           P =GetD(hWnd,IDC_EDIT_P),
                           N =GetD(hWnd,IDC_EDIT_N);
                    SetD(hWnd,IDC_EDIT_RESULT, std::pow(fv/P,1.0/N)-1.0, true);
                }else{
                    double P  =GetD(hWnd,IDC_EDIT_P),
                           FV =GetD(hWnd,IDC_EDIT_FV),
                           CPN=GetD(hWnd,IDC_EDIT_CPN),
                           N  =GetD(hWnd,IDC_EDIT_N);
                    auto f=[&](double y){return CPN*(1.0/y)*(1-std::pow(1+y,-N))+FV*std::pow(1+y,-N)-P;};
                    SetD(hWnd,IDC_EDIT_RESULT, bisect(f,1e-6,5.0), true);
                }
            }catch(...){
                MessageBoxW(hWnd,L"Computation failed. Check inputs.",
                               L"Error",MB_OK|MB_ICONERROR);
            }
            break;}
        }
        break;

    case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProcW(hWnd,msg,wp,lp);
}

/* ────────── WinMain ────────── */
int WINAPI WinMain(HINSTANCE hi,HINSTANCE,LPSTR,int)
{
    WNDCLASSW wc{}; wc.lpfnWndProc=WndProc; wc.hInstance=hi;
    wc.lpszClassName=L"BondCalcWnd"; wc.hCursor=LoadCursorW(nullptr,IDC_ARROW);
    RegisterClassW(&wc);

    HWND w=CreateWindowW(L"BondCalcWnd",L"Bond Calculator",
        WS_OVERLAPPEDWINDOW&~WS_THICKFRAME,
        CW_USEDEFAULT,CW_USEDEFAULT,900,650,
        nullptr,nullptr,hi,nullptr);
    ShowWindow(w,SW_SHOW);

    MSG m; while(GetMessageW(&m,nullptr,0,0)){
        TranslateMessage(&m); DispatchMessageW(&m);}
    return 0;
}

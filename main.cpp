/*  main.cpp – Bond calculator, nicer UI, Unicode build
    g++ main.cpp -mwindows -O2 -o BondCalc.exe
*/
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cmath>
#include <functional>
#include <sstream>
#include <iomanip>

// ---------- control IDs ----------
enum {
    IDC_RADIO1 = 1001, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4,
    IDC_NEXT, IDC_BACK,
    IDC_FORMULA,
    IDC_L_P,   IDC_E_P,
    IDC_L_FV,  IDC_E_FV,
    IDC_L_CPN, IDC_E_CPN,
    IDC_L_N,   IDC_E_N,
    IDC_L_Y,   IDC_E_Y,
    IDC_CALC,
    IDC_L_RES, IDC_E_RES
};

// ---------- prototypes -----------
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
double bisect(std::function<double(double)>, double, double);

// ---------- helpers --------------
inline void ShowGroup(HWND h, const int* ids, int n, BOOL show)
{
    for (int i = 0; i < n; ++i)
        ShowWindow(GetDlgItem(h, ids[i]), show ? SW_SHOW : SW_HIDE);
}
inline double GetDouble(HWND hEd)
{
    wchar_t b[64]; GetWindowTextW(hEd, b, 64); return _wtof(b);
}
inline void SetText(HWND hEd, const std::wstring& s)
{ SetWindowTextW(hEd, s.c_str()); }

double bisect(std::function<double(double)> f,double a,double b)
{
    double fa=f(a), fb=f(b); if(fa*fb>0) throw std::runtime_error("no sign");
    for(int i=0;i<60;++i){ double m=.5*(a+b), fm=f(m);
        if(fabs(fm)<1e-11) return m;
        (fa*fm<0? b:a)=m; (fa*fm<0? fb:fa)=fm;
    } return .5*(a+b);
}

// ============ WinMain ============
int WINAPI WinMain(HINSTANCE hi, HINSTANCE, LPSTR, int nCmd)
{
    WNDCLASS wc{0}; wc.lpfnWndProc=WndProc; wc.hInstance=hi;
    wc.lpszClassName=L"BondWnd"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    RegisterClass(&wc);

    HWND w=CreateWindowEx(0,L"BondWnd",L"Bond Calculator",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU, CW_USEDEFAULT,CW_USEDEFAULT,
        500, 400, nullptr,nullptr,hi,nullptr);
    ShowWindow(w,nCmd);

    MSG m; while(GetMessage(&m,nullptr,0,0)){TranslateMessage(&m);DispatchMessage(&m);}
    return 0;
}

// ============ logic helpers =================
struct Inputs {
    double P,FV,CPN,N,y;
};
static Inputs readInputs(HWND h)
{
    Inputs in{
        GetDouble(GetDlgItem(h,IDC_E_P)),
        GetDouble(GetDlgItem(h,IDC_E_FV)),
        GetDouble(GetDlgItem(h,IDC_E_CPN)),
        GetDouble(GetDlgItem(h,IDC_E_N)),
        GetDouble(GetDlgItem(h,IDC_E_Y))
    };
    return in;
}
static void writeResult(HWND h,double val,bool isYield)
{
    std::wostringstream ss; ss<<std::setprecision(8)<<val;
    if(isYield){
        ss<<L"  ("<<std::fixed<<std::setprecision(2)<<val*100<<L" %)";
    }
    SetText(GetDlgItem(h,IDC_E_RES), ss.str());
}

// ========= Update page 2 ==========
static void prepareInputs(HWND h)
{
    bool pvC = SendMessageW(GetDlgItem(h,IDC_RADIO1),BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmZ = SendMessageW(GetDlgItem(h,IDC_RADIO2),BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmC = SendMessageW(GetDlgItem(h,IDC_RADIO3),BM_GETCHECK,0,0)==BST_CHECKED;
    bool pvZ  = SendMessageW(GetDlgItem(h,IDC_RADIO4),BM_GETCHECK,0,0)==BST_CHECKED;

    const wchar_t* f =
        pvC ? L"P = CPN·(1/y)·(1‑1/(1+y)^N) + FV/(1+y)^N":
        ytmZ? L"y = (FV/P)^(1/N) − 1":
        ytmC? L"0 = CPN·(1/y)·(1‑1/(1+y)^N)+FV/(1+y)^N − P":
              L"P = FV/(1+y)^N";
    SetText(GetDlgItem(h,IDC_FORMULA), f);

    // hide *all* inputs
    const int all[]={IDC_L_P,IDC_E_P, IDC_L_FV,IDC_E_FV, IDC_L_CPN,IDC_E_CPN,
                     IDC_L_N,IDC_E_N, IDC_L_Y,IDC_E_Y};
    ShowGroup(h,all,_countof(all),FALSE);

    if(!pvC && !pvZ){
        int ids[]={IDC_L_P,IDC_E_P, IDC_L_FV,IDC_E_FV,
                   IDC_L_CPN,IDC_E_CPN, IDC_L_N,IDC_E_N};
        ShowGroup(h,ids,_countof(ids),TRUE);
    }else{
        int ids[]={IDC_L_FV,IDC_E_FV, IDC_L_N,IDC_E_N, IDC_L_Y,IDC_E_Y};
        ShowGroup(h,ids,_countof(ids),TRUE);
        if(pvC){
            ShowWindow(GetDlgItem(h,IDC_L_CPN),SW_SHOW);
            ShowWindow(GetDlgItem(h,IDC_E_CPN),SW_SHOW);
        }
    }
    EnableWindow(GetDlgItem(h,IDC_E_P), !(pvC||pvZ));
    EnableWindow(GetDlgItem(h,IDC_E_Y),  (pvC||pvZ)?FALSE:TRUE);

    // clear everything
    for(int id:all) SetText(GetDlgItem(h,id),L"");
    SetText(GetDlgItem(h,IDC_E_RES),L"");
}

// ============ Do calculation ===========
static void calculate(HWND h)
{
    bool pvC = SendMessageW(GetDlgItem(h,IDC_RADIO1),BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmZ= SendMessageW(GetDlgItem(h,IDC_RADIO2),BM_GETCHECK,0,0)==BST_CHECKED;
    bool ytmC= SendMessageW(GetDlgItem(h,IDC_RADIO3),BM_GETCHECK,0,0)==BST_CHECKED;
    bool pvZ = SendMessageW(GetDlgItem(h,IDC_RADIO4),BM_GETCHECK,0,0)==BST_CHECKED;

    Inputs in=readInputs(h); double out=0; bool isYield=false;

    try{
        if(pvC){
            out=in.CPN*(1.0/in.y)*(1-std::pow(1+in.y,-in.N))+in.FV*std::pow(1+in.y,-in.N);
        }else if(pvZ){
            out=in.FV*std::pow(1+in.y,-in.N);
        }else if(ytmZ){
            out=std::pow(in.FV/in.P,1.0/in.N)-1; isYield=true;
        }else{ // ytmC
            auto f=[&](double y){
                return in.CPN*(1.0/y)*(1-std::pow(1+y,-in.N))
                     + in.FV*std::pow(1+y,-in.N)-in.P;
            };
            out=bisect(f,1e-6,5); isYield=true;
        }
        writeResult(h,out,isYield);
    }catch(...){
        MessageBoxW(h,L"Bad input or no convergence.",L"Error",MB_OK|MB_ICONERROR);
    }
}

// ============ Window proc ============
LRESULT CALLBACK WndProc(HWND h, UINT m, WPARAM w, LPARAM)
{
    static const int page1[]={IDC_RADIO1,IDC_RADIO2,IDC_RADIO3,IDC_RADIO4,IDC_NEXT};
    static const int page2[]={IDC_BACK,IDC_FORMULA,
        IDC_L_P,IDC_E_P, IDC_L_FV,IDC_E_FV, IDC_L_CPN,IDC_E_CPN,
        IDC_L_N,IDC_E_N, IDC_L_Y,IDC_E_Y,
        IDC_CALC, IDC_L_RES,IDC_E_RES};

    switch(m){
    case WM_CREATE:{
        HINSTANCE hi=(HINSTANCE)GetWindowLongPtrW(h,GWLP_HINSTANCE);
        // ------ page 1 ------
        CreateWindowW(L"BUTTON",L"PV Coupon Bond",
            WS_CHILD|WS_VISIBLE|WS_GROUP|BS_AUTORADIOBUTTON,
            30,20,180,22,h,(HMENU)IDC_RADIO1,hi,nullptr);
        CreateWindowW(L"BUTTON",L"YTM Zero‑Coupon",
            WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
            30,50,180,22,h,(HMENU)IDC_RADIO2,hi,nullptr);
        CreateWindowW(L"BUTTON",L"YTM Coupon Bond",
            WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
            30,80,180,22,h,(HMENU)IDC_RADIO3,hi,nullptr);
        CreateWindowW(L"BUTTON",L"PV Zero‑Coupon",
            WS_CHILD|WS_VISIBLE|BS_AUTORADIOBUTTON,
            30,110,180,22,h,(HMENU)IDC_RADIO4,hi,nullptr);

        CreateWindowW(L"BUTTON",L"Next ➔",
            WS_CHILD|WS_VISIBLE|BS_PUSHBUTTON,
            350,105,90,30,h,(HMENU)IDC_NEXT,hi,nullptr);

        SendMessageW(GetDlgItem(h,IDC_RADIO1),BM_SETCHECK,BST_CHECKED,0);

        // ------ page 2 (hidden) ------
        CreateWindowW(L"BUTTON",L"⬅ Go Back",
            WS_CHILD,20,12,90,24,h,(HMENU)IDC_BACK,hi,nullptr);
        CreateWindowW(L"STATIC",L"",WS_CHILD|SS_LEFT,
            120,16,340,20,h,(HMENU)IDC_FORMULA,hi,nullptr);

        // thin horizontal line
        CreateWindowExW(0,L"STATIC",nullptr,
            WS_CHILD|SS_ETCHEDHORZ,20,38,440,2,h,nullptr,hi,nullptr);

        auto lbl=[&](int id,LPCWSTR t,int x,int y){
            CreateWindowW(L"STATIC",t,WS_CHILD|SS_RIGHT,x,y,80,22,
                h,(HMENU)id,hi,nullptr);
        };
        auto edit=[&](int id,int x,int y){
            CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_AUTOHSCROLL,x,y,100,22,
                h,(HMENU)id,hi,nullptr);
        };
        int left=120,right=310;
        lbl(IDC_L_P,  L"P:",        left, 60);   edit(IDC_E_P,  right, 60);
        lbl(IDC_L_FV, L"FV:",       left, 90);   edit(IDC_E_FV, right, 90);
        lbl(IDC_L_CPN,L"CPN:",      left,120);   edit(IDC_E_CPN,right,120);
        lbl(IDC_L_N,  L"N (yrs):",  left,150);   edit(IDC_E_N,  right,150);
        lbl(IDC_L_Y,  L"y:",        left,180);   edit(IDC_E_Y,  right,180);

        CreateWindowW(L"BUTTON",L"Calculate",
            WS_CHILD,200,225,100,30,h,(HMENU)IDC_CALC,hi,nullptr);

        lbl(IDC_L_RES,L"Result:",   left-60,270);
        CreateWindowW(L"EDIT",L"",WS_CHILD|WS_BORDER|ES_READONLY,
            right-60,270,160,22,h,(HMENU)IDC_E_RES,hi,nullptr);

        ShowGroup(h,page2,_countof(page2),FALSE);
        break;}

    case WM_COMMAND:
        switch(LOWORD(w)){
        case IDC_NEXT:
            ShowGroup(h,page1,_countof(page1),FALSE);
            ShowGroup(h,page2,_countof(page2),TRUE);
            prepareInputs(h); break;

        case IDC_BACK:
            ShowGroup(h,page2,_countof(page2),FALSE);
            ShowGroup(h,page1,_countof(page1),TRUE); break;

        case IDC_CALC:
            calculate(h); break;
        }
        break;

    case WM_DESTROY: PostQuitMessage(0); break;
    }
    return DefWindowProcW(h,m,w,0);
}

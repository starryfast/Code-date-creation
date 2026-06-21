#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0603
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
#include <gdiplus.h>
#include <uxtheme.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <set>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <io.h>
#include <shellapi.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "dwmapi.lib")

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

using namespace std;
using namespace Gdiplus;

// 全局变量
HWND g_hMainWnd, g_hLogEdit, g_hPreviewEdit;
HINSTANCE g_hInst;
HFONT g_hFontNormal, g_hFontLog;
COLORREF g_textColor = RGB(0, 0, 0);
COLORREF g_bgColor = RGB(245, 245, 245);
ULONG_PTR g_gdiplusToken;
Image* g_pBgImage = NULL;
wstring g_bgImagePath;
int g_scrollPos = 0;
int g_totalContentHeight = 3200;

// 控件ID（原有 + 新增70个工具按钮）
#define IDC_EDIT_START         101
#define IDC_EDIT_END           102
#define IDC_COMBO_TYPE         103
#define IDC_EDIT_RANGE1        104
#define IDC_EDIT_RANGE2        105
#define IDC_EDIT_STRLEN        106
#define IDC_EDIT_COUNT         107
#define IDC_CHECK_WRITE        108
#define IDC_BUTTON_GEN         109
#define IDC_EDIT_CPP_PATH      111
#define IDC_BUTTON_BROWSE      112
#define IDC_STATIC_STATUS      118
#define IDC_STATIC_AUTHOR      119
#define IDC_EDIT_VAR_TEXT      120
#define IDC_EDIT_GPP_PATH      121
#define IDC_BUTTON_GPP_BROWSE  122
#define IDC_BUTTON_GPP_AUTO    123
#define IDC_EDIT_OUTDIR        124
#define IDC_BUTTON_OUTDIR_BROWSE 125

#define IDC_RADIO_MODE_RANDOM   130
#define IDC_RADIO_MODE_MANUAL   131
#define IDC_RADIO_MODE_PACKZIP  132
#define IDC_RADIO_MODE_TOOLS    133

#define IDC_EDIT_MANUAL_INNAME   140
#define IDC_EDIT_MANUAL_INCONTENT 141
#define IDC_BUTTON_MANUAL_SAVEIN 142

#define IDC_BUTTON_UNIFIED_COMPILE 150
#define IDC_BUTTON_COLOR_SETTING   151
#define IDC_BUTTON_DEFAULT_COLOR   152
#define IDC_BUTTON_CHANGE_THEME    153
#define IDC_BUTTON_SAVEBG          154

#define IDC_THEME_WHITE      155
#define IDC_THEME_DARK       156
#define IDC_THEME_GREEN      157
#define IDC_THEME_CREAM      158
#define IDC_THEME_BLUE       159
#define IDC_THEME_ORANGE     160
#define IDC_THEME_FOREST     161
#define IDC_THEME_GREY       162
#define IDC_THEME_PINK       163
#define IDC_THEME_GOLD       164

#define IDC_BUTTON_PREVIEW      170
#define IDC_BUTTON_DELETEALL    171
#define IDC_BUTTON_EXPORTCSV    172
#define IDC_EDIT_CUSTOM_CHARSET 173
#define IDC_STATIC_CUSTOM_LABEL 174

#define IDC_STATIC_PACK_LABEL    180
#define IDC_EDIT_PACK_ZIPNAME    181
#define IDC_BUTTON_PACK_EXECUTE  182

#define IDC_STATIC_START_LABEL   201
#define IDC_STATIC_END_LABEL     202
#define IDC_STATIC_TYPE_LABEL    203
#define IDC_STATIC_RANGE_LABEL   204
#define IDC_STATIC_RANGESEP      205
#define IDC_STATIC_COUNT_LABEL   206
#define IDC_STATIC_VAR_LABEL     207
#define IDC_STATIC_OUTDIR_LABEL  208
#define IDC_STATIC_MANUAL_LABEL1 209
#define IDC_STATIC_MANUAL_LABEL2 210
#define IDC_STATIC_GPP_LABEL     211
#define IDC_EDIT_LOG             212
#define IDC_EDIT_PREVIEW         213

#define IDC_BUTTON_STATISTICS    300
#define IDC_BUTTON_SAVECONF      301
#define IDC_BUTTON_LOADCONF      302
#define IDC_BUTTON_RENAME        303
#define IDC_BUTTON_BIGFILE       304
#define IDC_BUTTON_VIEWFILE      305
#define IDC_BUTTON_PROGRESS      306
#define IDC_BUTTON_HELP          307
#define IDC_BUTTON_TIMESTAMP     308
#define IDC_BUTTON_CMDLINE       309

#define IDC_BUTTON_FLOATGEN      320
#define IDC_BUTTON_DATETIMEGEN   321
#define IDC_BUTTON_DEDUP         322
#define IDC_BUTTON_SPLITFILE     323
#define IDC_BUTTON_CHANGEEXT     324

// 原有10个实用工具
#define IDC_BUTTON_BAIDUSEARCH   325
#define IDC_BUTTON_OPENURL       326
#define IDC_BUTTON_PASSWORDGEN   327
#define IDC_BUTTON_CALCULATOR    328
#define IDC_BUTTON_NOTEPAD       329
#define IDC_BUTTON_FILEENCRYPT   330
#define IDC_BUTTON_TIMER_SHUTDOWN 331
#define IDC_BUTTON_WEATHER       332
#define IDC_BUTTON_UNITCONVERT   333
#define IDC_BUTTON_QRCODE        334

// 新增40个小工具
#define IDC_BUTTON_SNIPPING      335
#define IDC_BUTTON_COLORPICKER   336
#define IDC_BUTTON_OSK           337
#define IDC_BUTTON_CLEANTEMP     338
#define IDC_BUTTON_IPLOOKUP      339
#define IDC_BUTTON_MD5           340
#define IDC_BUTTON_FILECOMPARE   341
#define IDC_BUTTON_TEXTREPLACE   342
#define IDC_BUTTON_COUNTDOWN     343
#define IDC_BUTTON_SYSINFO       344
#define IDC_BUTTON_TASKMGR       345
#define IDC_BUTTON_REGEDIT       346
#define IDC_BUTTON_SERVICES      347
#define IDC_BUTTON_DISKCLEAN     348
#define IDC_BUTTON_TASKSCHED     349
#define IDC_BUTTON_SHORTCUT      350
#define IDC_BUTTON_FILEATTR      351
#define IDC_BUTTON_HASH          352
#define IDC_BUTTON_BASE64        353
#define IDC_BUTTON_JSONFORMAT    354
#define IDC_BUTTON_PAINT         355
#define IDC_BUTTON_WORDPAD       356
#define IDC_BUTTON_RDP           357
#define IDC_BUTTON_CONTROLPANEL  358
#define IDC_BUTTON_DEVMGR        359
#define IDC_BUTTON_DISKMGMT      360
#define IDC_BUTTON_MSCONFIG      361
#define IDC_BUTTON_PERFMON       362
#define IDC_BUTTON_RESMON        363
#define IDC_BUTTON_FIREWALL      364
#define IDC_BUTTON_USERACCOUNTS  365
#define IDC_BUTTON_PROGRAMSFEA   366
#define IDC_BUTTON_POWEROPT      367
#define IDC_BUTTON_NETCONN       368
#define IDC_BUTTON_TIMEDATE      369
#define IDC_BUTTON_MOUSE         370
#define IDC_BUTTON_KEYBOARD      371
#define IDC_BUTTON_FONTS         372
#define IDC_BUTTON_FOLDEROPT     373
#define IDC_BUTTON_SEARCHINDEX   374

// 新增20个小工具
#define IDC_BUTTON_EXPLORER     375
#define IDC_BUTTON_POWERSHELL   376
#define IDC_BUTTON_CMD          377
#define IDC_BUTTON_CHARMAP      378
#define IDC_BUTTON_MAGNIFY      379
#define IDC_BUTTON_UTILMAN      380
#define IDC_BUTTON_WINVER       381
#define IDC_BUTTON_DXDIAG       382
#define IDC_BUTTON_DISKPART     383
#define IDC_BUTTON_GPEDIT       384
#define IDC_BUTTON_LUSRMGR      385
#define IDC_BUTTON_SYSDM        386
#define IDC_BUTTON_WINDOWS_TOOLS 387
#define IDC_BUTTON_NETPLWIZ     388
#define IDC_BUTTON_MSRA         389
#define IDC_BUTTON_MSDT         390
#define IDC_BUTTON_SDCLT        391
#define IDC_BUTTON_JOYCPL       392
#define IDC_BUTTON_RECOVERY     393
#define IDC_BUTTON_SYSTEMPROPERTIESADVANCED 394

struct ExtraVar { wstring name, value; };
int g_currentMode = 0;
wstring g_outputDir;
vector<wstring> g_allGeneratedFiles;

// 前置声明（省略，所有函数实现均在下方）

// InputBox 修正版
BOOL InputBox(HWND hWnd, const wchar_t* title, const wchar_t* prompt, wchar_t* out, int maxLen);
LRESULT CALLBACK InputBoxProc(HWND, UINT, WPARAM, LPARAM);

#define INPUTBOX_CLASS L"CDCInputBox"
#define IDC_INPUTEDIT 101
struct INPUTBOX_PARAM { const wchar_t* prompt; wchar_t* out; int maxLen; int result; };
BOOL InputBox(HWND hOwner, const wchar_t* title, const wchar_t* prompt, wchar_t* out, int maxLen) {
    static bool registered = false;
    if (!registered) {
        WNDCLASSEXW wc = { sizeof(wc) };
        wc.lpfnWndProc = InputBoxProc; wc.hInstance = GetModuleHandleW(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW); wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszClassName = INPUTBOX_CLASS; RegisterClassExW(&wc); registered = true;
    }
    INPUTBOX_PARAM param = { prompt, out, maxLen, IDCANCEL };
    HWND hDlg = CreateWindowExW(WS_EX_DLGMODALFRAME, INPUTBOX_CLASS, title,
        WS_CAPTION | WS_SYSMENU | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 520, 200,
        hOwner, NULL, GetModuleHandleW(NULL), &param);
    if (!hDlg) return FALSE;
    MSG msg; while (GetMessage(&msg, NULL, 0, 0)) { if (!IsWindow(hDlg)) break; TranslateMessage(&msg); DispatchMessage(&msg); }
    return (param.result == IDOK);
}
LRESULT CALLBACK InputBoxProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static INPUTBOX_PARAM* pParam = NULL; static HWND hEdit = NULL;
    switch (msg) {
    case WM_CREATE: {
        pParam = (INPUTBOX_PARAM*)((CREATESTRUCT*)lParam)->lpCreateParams;
        CreateWindowW(L"STATIC", pParam->prompt, WS_CHILD | WS_VISIBLE, 10, 10, 480, 40, hWnd, NULL, GetModuleHandleW(NULL), NULL);
        hEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            10, 60, 480, 35, hWnd, (HMENU)IDC_INPUTEDIT, GetModuleHandleW(NULL), NULL);
        CreateWindowW(L"BUTTON", L"确定", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 150, 110, 90, 35, hWnd, (HMENU)IDOK, GetModuleHandleW(NULL), NULL);
        CreateWindowW(L"BUTTON", L"取消", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 280, 110, 90, 35, hWnd, (HMENU)IDCANCEL, GetModuleHandleW(NULL), NULL);
        SetFocus(hEdit); return 0;
    }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK) { GetWindowTextW(hEdit, pParam->out, pParam->maxLen); pParam->result = IDOK; DestroyWindow(hWnd); }
        else if (LOWORD(wParam) == IDCANCEL) { pParam->result = IDCANCEL; DestroyWindow(hWnd); } return 0;
    case WM_CLOSE: pParam->result = IDCANCEL; DestroyWindow(hWnd); return 0;
    } return DefWindowProc(hWnd, msg, wParam, lParam);
}

// 工具函数
bool FileExists(const wstring& path) { return _waccess(path.c_str(), 0) == 0; }
void AddGeneratedFile(const wstring& path) { if (!path.empty() && FileExists(path)) g_allGeneratedFiles.push_back(path); }
void LogMessage(const wchar_t* msg) { if (!g_hLogEdit) return; int len = GetWindowTextLengthW(g_hLogEdit); SendMessageW(g_hLogEdit, EM_SETSEL, len, len); SendMessageW(g_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)msg); SendMessageW(g_hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n"); }
int randomInt(int min, int max) { return min + rand() % (max - min + 1); }
long long randomBetween(long long a, long long b) { if (a > b) swap(a, b); unsigned long long range = (unsigned long long)(b - a) + 1; unsigned long long r = ((unsigned long long)rand() << 30) | ((unsigned long long)rand() << 15) | (unsigned long long)rand(); return a + (long long)(r % range); }
wstring randomString(int length, const wstring& charset) { wstring res; res.reserve(length); for (int i = 0;i < length;++i) res += charset[rand() % charset.size()]; return res; }
wstring GetDesktopPath() { wchar_t path[MAX_PATH]; if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, path))) return wstring(path); return L"."; }
vector<ExtraVar> ParseExtraVars(HWND hEdit) { vector<ExtraVar> ret; int len = GetWindowTextLengthW(hEdit); if (!len) return ret; wchar_t* buf = new wchar_t[len + 2]; GetWindowTextW(hEdit, buf, len + 2); wstringstream ss(buf); wstring line; while (getline(ss, line)) { if (line.empty()) continue; size_t eq = line.find(L'='); if (eq == wstring::npos) { size_t sp = line.find_first_of(L" \t"); if (sp != wstring::npos) ret.push_back({ line.substr(0,sp), line.substr(sp + 1) }); else ret.push_back({ line, L"" }); } else ret.push_back({ line.substr(0,eq), line.substr(eq + 1) }); } delete[] buf; return ret; }

bool GenerateInputFiles(int start, int end, int dataType, long long range1, long long range2, int strLen, int countPerFile, bool writeCount, const vector<ExtraVar>& vars, const wstring& customCharset) {
    if (g_outputDir.empty()) g_outputDir = GetDesktopPath();
    if (dataType == 2) { unsigned long long rangeSize = (range2 >= range1) ? (unsigned long long)(range2 - range1 + 1) : 0; if ((unsigned long long)countPerFile > rangeSize) return false; }
    const wstring low = L"abcdefghijklmnopqrstuvwxyz", up = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ", mix = low + up + L"0123456789";
    const wstring* charset = (dataType == 3) ? &low : (dataType == 4) ? &up : (dataType == 5) ? &mix : NULL;
    if (!customCharset.empty()) charset = &customCharset;
    for (int i = start; i <= end; ++i) {
        wchar_t path[MAX_PATH]; swprintf(path, MAX_PATH, L"%s\\%d.in", g_outputDir.c_str(), i);
        FILE* fp = _wfopen(path, L"w"); if (!fp) return false;
        for (auto& v : vars) { if (v.value.empty()) fwprintf(fp, L"%s\n", v.name.c_str()); else fwprintf(fp, L"%s %s\n", v.name.c_str(), v.value.c_str()); }
        if (writeCount) fwprintf(fp, L"%d\n", countPerFile);
        if (dataType <= 2) { if (dataType == 1) { for (int j = 0; j < countPerFile; ++j) fwprintf(fp, L"%lld ", randomBetween(range1, range2)); } else { set<long long> used; while (used.size() < (size_t)countPerFile) used.insert(randomBetween(range1, range2)); for (auto& v : used) fwprintf(fp, L"%lld ", v); } }
        else { for (int j = 0; j < countPerFile; ++j) fwprintf(fp, L"%s ", randomString(strLen, *charset).c_str()); }
        fwprintf(fp, L"\n"); fclose(fp); AddGeneratedFile(path);
    }
    return true;
}

// g++ 检测（跳过 RedPanda 自带）
wstring FindGppInPath() {
    wchar_t* env = _wgetenv(L"PATH"); if (!env) return L""; wstringstream ss(env); wstring dir;
    while (getline(ss, dir, L';')) { if (dir.empty()) continue; wstring full = dir + L"\\g++.exe"; if (FileExists(full) && dir.find(L"RedPanda") == wstring::npos) return full; }
    return L"";
}
void SearchGppInDir(const wstring& dir, int depth, vector<wstring>& out) {
    if (depth > 6 || dir.empty()) return; WIN32_FIND_DATAW fd; HANDLE hFind = FindFirstFileW((dir + L"\\*").c_str(), &fd); if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue; wstring full = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) { if (_wcsicmp(fd.cFileName, L"Windows") == 0 || _wcsicmp(fd.cFileName, L"System32") == 0 || _wcsicmp(fd.cFileName, L"WinSxS") == 0 || _wcsicmp(fd.cFileName, L"$Recycle.Bin") == 0) continue; SearchGppInDir(full, depth + 1, out); }
        else if (_wcsicmp(fd.cFileName, L"g++.exe") == 0) { out.push_back(full); break; }
    } while (FindNextFileW(hFind, &fd)); FindClose(hFind);
}
wstring AutoFindGpp() {
    wchar_t exe[MAX_PATH]; GetModuleFileNameW(NULL, exe, MAX_PATH); wstring dir = exe; size_t pos = dir.find_last_of(L"\\/");
    if (pos != wstring::npos) { dir = dir.substr(0, pos); if (FileExists(dir + L"\\g++.exe")) return dir + L"\\g++.exe"; wstring t = dir + L"\\..\\mingw64\\bin\\g++.exe"; if (FileExists(t)) return t; t = dir + L"\\..\\..\\mingw64\\bin\\g++.exe"; if (FileExists(t)) return t; }
    wstring pathEnv = FindGppInPath(); if (!pathEnv.empty()) return pathEnv;
    const wchar_t* paths[] = { L"C:\\MinGW\\bin\\g++.exe", L"C:\\mingw64\\bin\\g++.exe", L"D:\\MinGW\\bin\\g++.exe", L"D:\\mingw64\\bin\\g++.exe", L"C:\\TDM-GCC-64\\bin\\g++.exe", L"C:\\Program Files\\mingw-w64\\mingw64\\bin\\g++.exe", L"C:\\Program Files\\CodeBlocks\\MinGW\\bin\\g++.exe", L"C:\\Dev-Cpp\\MinGW64\\bin\\g++.exe" };
    for (auto p : paths) if (FileExists(p)) return p;
    DWORD drives = GetLogicalDrives(); vector<wstring> found;
    for (int i = 0; i < 26; ++i) { if (drives & (1 << i)) { wstring root = wstring(1, L'A' + i) + L":\\"; UINT type = GetDriveTypeW(root.c_str()); if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) { SearchGppInDir(root, 0, found); if (!found.empty()) return found[0]; } } }
    return L"";
}
bool CompileAndRunBatch(const wstring& gpp, const wstring& cpp, int start, int end, HWND hWnd) {
    if (!FileExists(cpp)) { MessageBoxW(hWnd, L"找不到源文件", L"错误", MB_ICONERROR); return false; }
    if (g_outputDir.empty()) g_outputDir = GetDesktopPath(); wstring exe = g_outputDir + L"\\temp_ac.exe"; wstring cmd = L"\"" + gpp + L"\" \"" + cpp + L"\" -o \"" + exe + L"\"";
    HANDLE hRead, hWrite; SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE }; if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    HANDLE hNull = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    STARTUPINFOW si = { sizeof(si) }; si.dwFlags = STARTF_USESTDHANDLES; si.hStdOutput = hWrite; si.hStdError = hWrite; si.hStdInput = hNull;
    PROCESS_INFORMATION pi = { 0 }; wchar_t* buf = new wchar_t[cmd.size() + 1]; wcscpy(buf, cmd.c_str()); BOOL b = CreateProcessW(NULL, buf, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi); delete[] buf; CloseHandle(hWrite); CloseHandle(hNull);
    if (!b) { CloseHandle(hRead); return false; } CloseHandle(hRead); WaitForSingleObject(pi.hProcess, INFINITE); DWORD code; GetExitCodeProcess(pi.hProcess, &code); CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    if (code != 0) return false;
    for (int i = start; i <= end; ++i) { wchar_t inPath[MAX_PATH], outPath[MAX_PATH]; swprintf(inPath, MAX_PATH, L"%s\\%d.in", g_outputDir.c_str(), i); swprintf(outPath, MAX_PATH, L"%s\\%d.out", g_outputDir.c_str(), i); wstring run = L"cd /d \"" + g_outputDir + L"\" && \"" + exe + L"\" < \"" + inPath + L"\" > \"" + outPath + L"\""; _wsystem(run.c_str()); AddGeneratedFile(outPath); }
    DeleteFileW(exe.c_str()); return true;
}
bool CompileAndRunSingle(const wstring& gpp, const wstring& cpp, const wstring& in, const wstring& out, HWND hWnd) {
    if (!FileExists(cpp)) return false; wstring exe = g_outputDir + L"\\temp_single.exe"; wstring cmd = L"\"" + gpp + L"\" \"" + cpp + L"\" -o \"" + exe + L"\"";
    HANDLE hRead, hWrite; SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE }; if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    HANDLE hNull = CreateFileW(L"NUL", GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    STARTUPINFOW si = { sizeof(si) }; si.dwFlags = STARTF_USESTDHANDLES; si.hStdOutput = hWrite; si.hStdError = hWrite; si.hStdInput = hNull;
    PROCESS_INFORMATION pi = { 0 }; wchar_t* buf = new wchar_t[cmd.size() + 1]; wcscpy(buf, cmd.c_str()); BOOL b = CreateProcessW(NULL, buf, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi); delete[] buf; CloseHandle(hWrite); CloseHandle(hNull);
    if (!b) { CloseHandle(hRead); return false; } CloseHandle(hRead); WaitForSingleObject(pi.hProcess, INFINITE); DWORD code; GetExitCodeProcess(pi.hProcess, &code); CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    if (code != 0) return false;
    wstring run = L"cd /d \"" + g_outputDir + L"\" && \"" + exe + L"\" < \"" + in + L"\" > \"" + out + L"\""; _wsystem(run.c_str()); AddGeneratedFile(out); DeleteFileW(exe.c_str()); return true;
}
// 保存/加载 g++ 路径（持久化）
void SaveGppPath(const wstring& path) {
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\TestDataGenerator", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        RegSetValueExW(hKey, L"GppPath", 0, REG_SZ, (BYTE*)path.c_str(), (DWORD)((path.size() + 1) * sizeof(wchar_t)));
        RegCloseKey(hKey);
    }
}
wstring LoadGppPath() {
    HKEY hKey;
    wchar_t buf[MAX_PATH] = { 0 };
    DWORD size = sizeof(buf);
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TestDataGenerator", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"GppPath", NULL, NULL, (BYTE*)buf, &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return wstring(buf);
        }
        RegCloseKey(hKey);
    }
    return L"";
}
// 颜色/主题/背景
void SaveColorSettings() { HKEY hKey; if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\TestDataGenerator", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) { RegSetValueExW(hKey, L"TextColor", 0, REG_DWORD, (BYTE*)&g_textColor, sizeof(COLORREF)); RegSetValueExW(hKey, L"BgColor", 0, REG_DWORD, (BYTE*)&g_bgColor, sizeof(COLORREF)); RegCloseKey(hKey); } }
void LoadColorSettings() { HKEY hKey; if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TestDataGenerator", 0, KEY_READ, &hKey) == ERROR_SUCCESS) { DWORD size = sizeof(COLORREF); RegQueryValueExW(hKey, L"TextColor", NULL, NULL, (BYTE*)&g_textColor, &size); RegQueryValueExW(hKey, L"BgColor", NULL, NULL, (BYTE*)&g_bgColor, &size); RegCloseKey(hKey); } }
void ApplyColorSettings(HWND hWnd) { InvalidateRect(hWnd, NULL, TRUE); UpdateWindow(hWnd); }
void SetPresetTheme(COLORREF text, COLORREF bg) { g_textColor = text; g_bgColor = bg; SaveColorSettings(); }
void SetDefaultColors(HWND hWnd) { SetPresetTheme(RGB(0, 0, 0), RGB(245, 245, 245)); ApplyColorSettings(hWnd); }
void ShowColorDialog(HWND hWnd) { MessageBoxW(hWnd, L"请依次选择文字颜色和背景颜色。", L"颜色设置", MB_OK); CHOOSECOLOR cc = { sizeof(cc) }; COLORREF cust[16] = { 0 }; cc.hwndOwner = hWnd; cc.rgbResult = g_textColor; cc.lpCustColors = cust; cc.Flags = CC_RGBINIT | CC_FULLOPEN; if (ChooseColor(&cc)) { g_textColor = cc.rgbResult; cc.rgbResult = g_bgColor; if (ChooseColor(&cc)) { g_bgColor = cc.rgbResult; SaveColorSettings(); ApplyColorSettings(hWnd); } } }
void LoadBackgroundImage(const wstring& path) { if (g_pBgImage) { delete g_pBgImage; g_pBgImage = NULL; } if (!FileExists(path)) return; g_pBgImage = Image::FromFile(path.c_str()); if (g_pBgImage && g_pBgImage->GetLastStatus() == Ok) { g_bgImagePath = path; HKEY hKey; if (RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\TestDataGenerator", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) { RegSetValueExW(hKey, L"BackgroundImage", 0, REG_SZ, (BYTE*)path.c_str(), (DWORD)((path.size() + 1) * sizeof(wchar_t))); RegCloseKey(hKey); } } else { delete g_pBgImage; g_pBgImage = NULL; } }
void LoadBackgroundPath() { HKEY hKey; wchar_t buf[MAX_PATH] = { 0 }; DWORD size = sizeof(buf); if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TestDataGenerator", 0, KEY_READ, &hKey) == ERROR_SUCCESS) { if (RegQueryValueExW(hKey, L"BackgroundImage", NULL, NULL, (BYTE*)buf, &size) == ERROR_SUCCESS) LoadBackgroundImage(buf); RegCloseKey(hKey); } }
void ClearBackgroundImage() { if (g_pBgImage) { delete g_pBgImage; g_pBgImage = NULL; } g_bgImagePath.clear(); HKEY hKey; if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\TestDataGenerator", 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) { RegDeleteValueW(hKey, L"BackgroundImage"); RegCloseKey(hKey); } }
void ChangeTheme(HWND hWnd) { OPENFILENAMEW ofn = { sizeof(ofn) }; wchar_t file[MAX_PATH] = { 0 }; ofn.hwndOwner = hWnd; ofn.lpstrFilter = L"图片\0*.jpg;*.jpeg;*.png;*.bmp\0"; ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH; ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY; if (GetOpenFileNameW(&ofn)) { LoadBackgroundImage(file); ApplyColorSettings(hWnd); } }
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid) { UINT num = 0, size = 0; ImageCodecInfo* pImageCodecInfo = NULL; GetImageEncodersSize(&num, &size); if (size == 0) return -1; pImageCodecInfo = (ImageCodecInfo*)(malloc(size)); if (pImageCodecInfo == NULL) return -1; GetImageEncoders(num, size, pImageCodecInfo); for (UINT j = 0; j < num; ++j) { if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) { *pClsid = pImageCodecInfo[j].Clsid; free(pImageCodecInfo); return j; } } free(pImageCodecInfo); return -1; }
void SaveBackground(HWND hWnd) { if (!g_pBgImage) { MessageBoxW(hWnd, L"当前没有背景图片", L"提示", MB_OK); return; } OPENFILENAMEW ofn = { sizeof(ofn) }; wchar_t file[MAX_PATH] = L"background.png"; ofn.hwndOwner = hWnd; ofn.lpstrFilter = L"PNG\0*.png\0JPEG\0*.jpg\0BMP\0*.bmp\0"; ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH; ofn.Flags = OFN_OVERWRITEPROMPT; if (GetSaveFileNameW(&ofn)) { CLSID clsid; wstring ext = file; size_t dot = ext.find_last_of(L'.'); if (dot != wstring::npos) ext = ext.substr(dot); else ext = L".png"; if (_wcsicmp(ext.c_str(), L".png") == 0) GetEncoderClsid(L"image/png", &clsid); else if (_wcsicmp(ext.c_str(), L".jpg") == 0 || _wcsicmp(ext.c_str(), L".jpeg") == 0) GetEncoderClsid(L"image/jpeg", &clsid); else GetEncoderClsid(L"image/bmp", &clsid); g_pBgImage->Save(file, &clsid, NULL); LogMessage(L"背景图片已保存"); } }
void DrawBackground(HDC hdc, int w, int h) { if (g_pBgImage) { Graphics g(hdc); int iw = g_pBgImage->GetWidth(), ih = g_pBgImage->GetHeight(); if (iw > 0 && ih > 0) { float scale = max((float)w / iw, (float)h / ih); int dw = (int)(iw * scale), dh = (int)(ih * scale); g.DrawImage(g_pBgImage, (w - dw) / 2, (h - dh) / 2, dw, dh); } } else { HBRUSH brush = CreateSolidBrush(g_bgColor); RECT rc = { 0,0,w,h }; FillRect(hdc, &rc, brush); DeleteObject(brush); } }

// 模式切换
void SwitchMode(HWND hWnd, int mode) {
    g_currentMode = mode;
    int randomCtrls[] = { IDC_EDIT_START, IDC_EDIT_END, IDC_COMBO_TYPE, IDC_EDIT_RANGE1, IDC_EDIT_RANGE2, IDC_EDIT_STRLEN, IDC_EDIT_COUNT, IDC_CHECK_WRITE, IDC_EDIT_VAR_TEXT, IDC_BUTTON_GEN, IDC_STATIC_START_LABEL, IDC_STATIC_END_LABEL, IDC_STATIC_TYPE_LABEL, IDC_STATIC_RANGE_LABEL, IDC_STATIC_RANGESEP, IDC_STATIC_COUNT_LABEL, IDC_STATIC_VAR_LABEL, IDC_EDIT_CUSTOM_CHARSET, IDC_STATIC_CUSTOM_LABEL };
    int manualCtrls[] = { IDC_EDIT_MANUAL_INNAME, IDC_EDIT_MANUAL_INCONTENT, IDC_BUTTON_MANUAL_SAVEIN, IDC_STATIC_MANUAL_LABEL1, IDC_STATIC_MANUAL_LABEL2 };
    int packCtrls[] = { IDC_STATIC_PACK_LABEL, IDC_EDIT_PACK_ZIPNAME, IDC_BUTTON_PACK_EXECUTE };
    int toolsCtrls[] = {
        IDC_BUTTON_BAIDUSEARCH, IDC_BUTTON_OPENURL, IDC_BUTTON_PASSWORDGEN, IDC_BUTTON_CALCULATOR,
        IDC_BUTTON_NOTEPAD, IDC_BUTTON_FILEENCRYPT, IDC_BUTTON_TIMER_SHUTDOWN, IDC_BUTTON_WEATHER,
        IDC_BUTTON_UNITCONVERT, IDC_BUTTON_QRCODE,
        IDC_BUTTON_SNIPPING, IDC_BUTTON_COLORPICKER, IDC_BUTTON_OSK, IDC_BUTTON_CLEANTEMP,
        IDC_BUTTON_IPLOOKUP, IDC_BUTTON_MD5, IDC_BUTTON_FILECOMPARE, IDC_BUTTON_TEXTREPLACE,
        IDC_BUTTON_COUNTDOWN, IDC_BUTTON_SYSINFO, IDC_BUTTON_TASKMGR, IDC_BUTTON_REGEDIT,
        IDC_BUTTON_SERVICES, IDC_BUTTON_DISKCLEAN, IDC_BUTTON_TASKSCHED, IDC_BUTTON_SHORTCUT,
        IDC_BUTTON_FILEATTR, IDC_BUTTON_HASH, IDC_BUTTON_BASE64, IDC_BUTTON_JSONFORMAT,
        IDC_BUTTON_PAINT, IDC_BUTTON_WORDPAD, IDC_BUTTON_RDP, IDC_BUTTON_CONTROLPANEL,
        IDC_BUTTON_DEVMGR, IDC_BUTTON_DISKMGMT, IDC_BUTTON_MSCONFIG, IDC_BUTTON_PERFMON,
        IDC_BUTTON_RESMON, IDC_BUTTON_FIREWALL, IDC_BUTTON_USERACCOUNTS, IDC_BUTTON_PROGRAMSFEA,
        IDC_BUTTON_POWEROPT, IDC_BUTTON_NETCONN, IDC_BUTTON_TIMEDATE, IDC_BUTTON_MOUSE,
        IDC_BUTTON_KEYBOARD, IDC_BUTTON_FONTS, IDC_BUTTON_FOLDEROPT, IDC_BUTTON_SEARCHINDEX,
        IDC_BUTTON_EXPLORER, IDC_BUTTON_POWERSHELL, IDC_BUTTON_CMD, IDC_BUTTON_CHARMAP,
        IDC_BUTTON_MAGNIFY, IDC_BUTTON_UTILMAN, IDC_BUTTON_WINVER, IDC_BUTTON_DXDIAG,
        IDC_BUTTON_DISKPART, IDC_BUTTON_GPEDIT, IDC_BUTTON_LUSRMGR, IDC_BUTTON_SYSDM,
        IDC_BUTTON_WINDOWS_TOOLS, IDC_BUTTON_NETPLWIZ, IDC_BUTTON_MSRA, IDC_BUTTON_MSDT,
        IDC_BUTTON_SDCLT, IDC_BUTTON_JOYCPL, IDC_BUTTON_RECOVERY, IDC_BUTTON_SYSTEMPROPERTIESADVANCED
    };
    int commonCtrls[] = { IDC_BUTTON_UNIFIED_COMPILE, IDC_STATIC_STATUS, IDC_EDIT_CPP_PATH, IDC_BUTTON_BROWSE, IDC_EDIT_GPP_PATH, IDC_BUTTON_GPP_BROWSE, IDC_BUTTON_GPP_AUTO, IDC_STATIC_GPP_LABEL, IDC_EDIT_LOG, IDC_EDIT_PREVIEW };
    int always[] = { IDC_STATIC_AUTHOR, IDC_EDIT_OUTDIR, IDC_STATIC_OUTDIR_LABEL, IDC_BUTTON_OUTDIR_BROWSE, IDC_BUTTON_COLOR_SETTING, IDC_BUTTON_DEFAULT_COLOR, IDC_BUTTON_CHANGE_THEME, IDC_BUTTON_SAVEBG, IDC_THEME_WHITE, IDC_THEME_DARK, IDC_THEME_GREEN, IDC_THEME_CREAM, IDC_THEME_BLUE, IDC_THEME_ORANGE, IDC_THEME_FOREST, IDC_THEME_GREY, IDC_THEME_PINK, IDC_THEME_GOLD, IDC_BUTTON_PREVIEW, IDC_BUTTON_DELETEALL, IDC_BUTTON_EXPORTCSV, IDC_BUTTON_STATISTICS, IDC_BUTTON_SAVECONF, IDC_BUTTON_LOADCONF, IDC_BUTTON_RENAME, IDC_BUTTON_BIGFILE, IDC_BUTTON_VIEWFILE, IDC_BUTTON_PROGRESS, IDC_BUTTON_TIMESTAMP, IDC_BUTTON_CMDLINE, IDC_BUTTON_HELP, IDC_BUTTON_FLOATGEN, IDC_BUTTON_DATETIMEGEN, IDC_BUTTON_DEDUP, IDC_BUTTON_SPLITFILE, IDC_BUTTON_CHANGEEXT };

    for (int id : randomCtrls) ShowWindow(GetDlgItem(hWnd, id), SW_HIDE);
    for (int id : manualCtrls) ShowWindow(GetDlgItem(hWnd, id), SW_HIDE);
    for (int id : packCtrls)   ShowWindow(GetDlgItem(hWnd, id), SW_HIDE);
    for (int id : toolsCtrls)  ShowWindow(GetDlgItem(hWnd, id), SW_HIDE);
    for (int id : commonCtrls) ShowWindow(GetDlgItem(hWnd, id), SW_HIDE);

    if (mode == 0) { for (int id : randomCtrls) ShowWindow(GetDlgItem(hWnd, id), SW_SHOW); for (int id : commonCtrls) ShowWindow(GetDlgItem(hWnd, id), SW_SHOW); }
    else if (mode == 1) { for (int id : manualCtrls) ShowWindow(GetDlgItem(hWnd, id), SW_SHOW); for (int id : commonCtrls) ShowWindow(GetDlgItem(hWnd, id), SW_SHOW); }
    else if (mode == 2) { for (int id : packCtrls) ShowWindow(GetDlgItem(hWnd, id), SW_SHOW); }
    else if (mode == 3) { for (int id : toolsCtrls) ShowWindow(GetDlgItem(hWnd, id), SW_SHOW); }

    SendMessage(GetDlgItem(hWnd, IDC_RADIO_MODE_RANDOM), BM_SETCHECK, mode == 0, 0);
    SendMessage(GetDlgItem(hWnd, IDC_RADIO_MODE_MANUAL), BM_SETCHECK, mode == 1, 0);
    SendMessage(GetDlgItem(hWnd, IDC_RADIO_MODE_PACKZIP), BM_SETCHECK, mode == 2, 0);
    SendMessage(GetDlgItem(hWnd, IDC_RADIO_MODE_TOOLS), BM_SETCHECK, mode == 3, 0);
    InvalidateRect(hWnd, NULL, TRUE);
}

// ZIP 打包
#pragma pack(push, 1)
struct ZipLocal { uint32_t sig; uint16_t vn, fl, mt; uint16_t mt2, md; uint32_t crc; uint32_t cs, us; uint16_t nl, el; };
struct ZipCentral { uint32_t sig; uint16_t vm, vn, fl, mt; uint16_t mt2, md; uint32_t crc; uint32_t cs, us; uint16_t nl, el, cl; uint16_t ds, ia; uint32_t ea, lo; };
struct ZipEOCD { uint32_t sig; uint16_t dn, ds, ne, te; uint32_t ds2, do2; uint16_t cl; };
#pragma pack(pop)
static uint32_t crc32_table[256];
static void init_crc32() { for (uint32_t i = 0; i < 256; ++i) { uint32_t c = i; for (int j = 0; j < 8; ++j) c = (c >> 1) ^ ((c & 1) ? 0xEDB88320 : 0); crc32_table[i] = c; } }
static uint32_t calc_crc32(const void* d, size_t len) { uint32_t c = 0xFFFFFFFF; auto p = (const uint8_t*)d; for (size_t i = 0; i < len; ++i) c = (c >> 8) ^ crc32_table[(c ^ p[i]) & 0xFF]; return c ^ 0xFFFFFFFF; }
static bool WriteData(HANDLE h, const void* d, DWORD sz) { DWORD w; return WriteFile(h, d, sz, &w, NULL) && w == sz; }
bool PackRecordedFilesToZip(const wstring& zipPath) {
    if (g_allGeneratedFiles.empty()) return false;
    static bool inited = false; if (!inited) { init_crc32(); inited = true; }
    HANDLE hZip = CreateFileW(zipPath.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hZip == INVALID_HANDLE_VALUE) return false;
    vector<ZipCentral> dirs; vector<string> names; uint32_t off = 0;
    for (auto& src : g_allGeneratedFiles) {
        HANDLE hf = CreateFileW(src.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL); if (hf == INVALID_HANDLE_VALUE) { CloseHandle(hZip); return false; }
        DWORD sz = GetFileSize(hf, NULL); vector<uint8_t> buf(sz); DWORD rd; if (!ReadFile(hf, buf.data(), sz, &rd, NULL) || rd != sz) { CloseHandle(hf); CloseHandle(hZip); return false; } CloseHandle(hf);
        wstring nameW = src.substr(src.find_last_of(L"\\/") + 1);
        uint32_t crc = calc_crc32(buf.data(), sz);
        string utf8; int req = WideCharToMultiByte(CP_UTF8, 0, nameW.c_str(), -1, NULL, 0, NULL, NULL); utf8.resize(req - 1); WideCharToMultiByte(CP_UTF8, 0, nameW.c_str(), -1, &utf8[0], req, NULL, NULL);
        ZipLocal lh = { 0 }; lh.sig = 0x04034b50; lh.vn = 20; lh.mt = 0; lh.crc = crc; lh.cs = sz; lh.us = sz; lh.nl = (uint16_t)utf8.size();
        if (!WriteData(hZip, &lh, sizeof(lh)) || !WriteData(hZip, utf8.c_str(), lh.nl) || !WriteData(hZip, buf.data(), sz)) { CloseHandle(hZip); return false; }
        ZipCentral cd = { 0 }; cd.sig = 0x02014b50; cd.vm = 20; cd.vn = 20; cd.mt = 0; cd.crc = crc; cd.cs = sz; cd.us = sz; cd.nl = lh.nl; cd.lo = off; dirs.push_back(cd); names.push_back(utf8); off += sizeof(lh) + lh.nl + sz;
    }
    uint32_t dirOff = off;
    for (size_t i = 0; i < dirs.size(); ++i) { WriteData(hZip, &dirs[i], sizeof(ZipCentral)); WriteData(hZip, names[i].c_str(), dirs[i].nl); }
    ZipEOCD eocd = { 0 }; eocd.sig = 0x06054b50; eocd.ne = (uint16_t)dirs.size(); eocd.te = eocd.ne;
    DWORD end = SetFilePointer(hZip, 0, NULL, FILE_CURRENT); eocd.ds2 = end - dirOff; eocd.do2 = dirOff;
    WriteData(hZip, &eocd, sizeof(eocd)); CloseHandle(hZip); return true;
}

// 所有辅助功能函数（原有+新增70个小工具）
void PreviewData(HWND hWnd) { /* 略，同之前 */ }
void DeleteAllGeneratedFiles(HWND hWnd) { for (auto& f : g_allGeneratedFiles) DeleteFileW(f.c_str()); g_allGeneratedFiles.clear(); LogMessage(L"已清空所有生成的文件"); }
void ExportToCSV(HWND hWnd) { /* 略 */ }
void DataStatistics(HWND hWnd) { int inCount = 0, outCount = 0; for (auto& f : g_allGeneratedFiles) { if (f.find(L".in") != wstring::npos) inCount++; else if (f.find(L".out") != wstring::npos) outCount++; } wchar_t msg[128]; swprintf(msg, 128, L"共生成 %d 个 .in 文件，%d 个 .out 文件。", inCount, outCount); MessageBoxW(hWnd, msg, L"数据统计", MB_OK); }
void SaveConfig(HWND hWnd) { /* 略 */ }
void LoadConfig(HWND hWnd) { /* 略 */ }
void BatchRename(HWND hWnd) { int idx = 1; for (auto& f : g_allGeneratedFiles) { if (f.find(L".in") == wstring::npos) continue; wstring newName = g_outputDir + L"\\renamed_" + to_wstring(idx++) + L".in"; MoveFileW(f.c_str(), newName.c_str()); } LogMessage(L"批量重命名完成"); }
void GenerateBigFile(HWND hWnd) { GenerateInputFiles(1, 1, 1, 1, 100000, 0, 100000, false, {}, L""); LogMessage(L"大文件生成完毕"); }
void ViewFileContent(HWND hWnd) { /* 略 */ }
void ProgressGenerate(HWND hWnd) { LogMessage(L"进度生成功能演示（实际需多线程）"); }
wstring GetTimestampDir() { time_t now = time(0); tm ltm; localtime_s(&ltm, &now); wchar_t buf[64]; swprintf(buf, 64, L"%04d%02d%02d_%02d%02d%02d", ltm.tm_year + 1900, ltm.tm_mon + 1, ltm.tm_mday, ltm.tm_hour, ltm.tm_min, ltm.tm_sec); return wstring(buf); }
void UseTimestampDir(HWND hWnd) { g_outputDir = GetDesktopPath() + L"\\" + GetTimestampDir(); CreateDirectoryW(g_outputDir.c_str(), NULL); SetDlgItemTextW(hWnd, IDC_EDIT_OUTDIR, g_outputDir.c_str()); LogMessage(L"已切换到时间戳子目录"); }
void ShowHelp(HWND hWnd) { MessageBoxW(hWnd, L"测试数据生成器 6.0.0\n作者：starryfast\nBilibili 同名\nLuogu 2070675", L"帮助与关于", MB_OK); }
void GenerateFloatData(HWND hWnd) { /* 略 */ }
void GenerateDateTimeData(HWND hWnd) { /* 略 */ }
void DeduplicateFiles(HWND hWnd) { /* 略 */ }
void SplitFile(HWND hWnd) { /* 略 */ }
void ChangeFileExtension(HWND hWnd) { for (auto& f : g_allGeneratedFiles) { if (f.find(L".in") != wstring::npos) { wstring newName = f.substr(0, f.size() - 3) + L".txt"; MoveFileW(f.c_str(), newName.c_str()); } } LogMessage(L"扩展名修改完成"); }

// 原有10个
void BaiduSearch(HWND hWnd) { wchar_t keyword[256] = { 0 }; if (InputBox(hWnd, L"百度搜索", L"请输入搜索关键词：", keyword, 256) && wcslen(keyword) > 0) { wstring url = L"https://www.baidu.com/s?wd=" + wstring(keyword); ShellExecuteW(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL); } }
void OpenUrl(HWND hWnd) { wchar_t url[512] = { 0 }; if (InputBox(hWnd, L"打开网址", L"请输入完整网址：", url, 512) && wcslen(url) > 0) { ShellExecuteW(NULL, L"open", url, NULL, NULL, SW_SHOWNORMAL); } }
void PasswordGenerator(HWND hWnd) { const wchar_t* chars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789!@#$%^&*"; wstring pwd; for (int i = 0; i < 16; ++i) pwd += chars[rand() % wcslen(chars)]; MessageBoxW(hWnd, (L"随机密码：" + pwd).c_str(), L"密码生成器", MB_OK); }
void OpenCalculator(HWND hWnd) { ShellExecuteW(NULL, L"open", L"calc.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenNotepad(HWND hWnd) { ShellExecuteW(NULL, L"open", L"notepad.exe", NULL, NULL, SW_SHOWNORMAL); }
void FileEncrypt(HWND hWnd) { MessageBoxW(hWnd, L"文件加密功能示例：\n选择一个文件，输入密码，将内容异或加密后保存为新文件。\n（此功能需自行实现具体加密算法）", L"文件加密", MB_OK); }
void TimerShutdown(HWND hWnd) { wchar_t time[32] = { 0 }; if (InputBox(hWnd, L"定时关机", L"请输入倒计时秒数（0取消）：", time, 32)) { int seconds = _wtoi(time); if (seconds > 0) { wchar_t cmd[64]; swprintf(cmd, 64, L"shutdown -s -t %d", seconds); _wsystem(cmd); LogMessage(L"已设置定时关机"); } else { _wsystem(L"shutdown -a"); LogMessage(L"已取消定时关机"); } } }
void WeatherQuery(HWND hWnd) { ShellExecuteW(NULL, L"open", L"https://tianqi.qq.com", NULL, NULL, SW_SHOWNORMAL); }
void UnitConvert(HWND hWnd) { MessageBoxW(hWnd, L"常用单位换算：\n1 英寸 = 2.54 厘米\n1 磅 = 0.4536 千克\n1 加仑 = 3.785 升", L"单位换算", MB_OK); }
void QRCodeGenerator(HWND hWnd) { wchar_t text[256] = { 0 }; if (InputBox(hWnd, L"二维码生成", L"请输入要编码的文字或网址：", text, 256) && wcslen(text) > 0) { wstring url = L"https://api.qrserver.com/v1/create-qr-code/?size=200x200&data=" + wstring(text); ShellExecuteW(NULL, L"open", url.c_str(), NULL, NULL, SW_SHOWNORMAL); } }

// 新增40个
void OpenSnippingTool(HWND hWnd) { ShellExecuteW(NULL, L"open", L"snippingtool.exe", NULL, NULL, SW_SHOWNORMAL); }
void ColorPicker(HWND hWnd) { MessageBoxW(hWnd, L"取色器：请使用截图工具或第三方软件获取颜色值。", L"取色器", MB_OK); }
void OpenOSK(HWND hWnd) { ShellExecuteW(NULL, L"open", L"osk.exe", NULL, NULL, SW_SHOWNORMAL); }
void CleanTempFiles(HWND hWnd) { _wsystem(L"del /q /f %temp%\\*.*"); LogMessage(L"临时文件已清理"); }
void IPLookup(HWND hWnd) { ShellExecuteW(NULL, L"open", L"https://www.ip.cn", NULL, NULL, SW_SHOWNORMAL); }
void ComputeMD5(HWND hWnd) { MessageBoxW(hWnd, L"MD5校验：请使用 certutil -hashfile 文件名 MD5", L"MD5", MB_OK); }
void FileCompare(HWND hWnd) { MessageBoxW(hWnd, L"文件比较：请使用 fc 命令或专业比较工具。", L"文件比较", MB_OK); }
void TextReplaceInFiles(HWND hWnd) { LogMessage(L"文本替换功能已调用（示例）"); }
void CountdownTimer(HWND hWnd) { wchar_t sec[32] = { 0 }; if (InputBox(hWnd, L"倒计时", L"输入秒数：", sec, 32)) { int s = _wtoi(sec); wchar_t cmd[128]; swprintf(cmd, 128, L"timeout /t %d && msg * 时间到！", s); _wsystem(cmd); } }
void SystemInfo(HWND hWnd) { _wsystem(L"msinfo32.exe"); }
void OpenTaskManager(HWND hWnd) { ShellExecuteW(NULL, L"open", L"taskmgr.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenRegEdit(HWND hWnd) { ShellExecuteW(NULL, L"open", L"regedit.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenServices(HWND hWnd) { ShellExecuteW(NULL, L"open", L"services.msc", NULL, NULL, SW_SHOWNORMAL); }
void DiskCleanup(HWND hWnd) { ShellExecuteW(NULL, L"open", L"cleanmgr.exe", NULL, NULL, SW_SHOWNORMAL); }
void TaskScheduler(HWND hWnd) { ShellExecuteW(NULL, L"open", L"taskschd.msc", NULL, NULL, SW_SHOWNORMAL); }
void CreateShortcut(HWND hWnd) { MessageBoxW(hWnd, L"创建快捷方式：请右键拖拽文件到目标位置。", L"快捷方式", MB_OK); }
void ChangeFileAttributes(HWND hWnd) { LogMessage(L"文件属性修改已调用（示例）"); }
void ComputeHash(HWND hWnd) { MessageBoxW(hWnd, L"哈希计算：请使用 certutil -hashfile 文件名 SHA256", L"哈希", MB_OK); }
void Base64EncodeDecode(HWND hWnd) { MessageBoxW(hWnd, L"Base64：请使用在线工具或编程实现。", L"Base64", MB_OK); }
void JSONFormatter(HWND hWnd) { MessageBoxW(hWnd, L"JSON格式化：请使用在线工具。", L"JSON", MB_OK); }
void OpenPaint(HWND hWnd) { ShellExecuteW(NULL, L"open", L"mspaint.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenWordpad(HWND hWnd) { ShellExecuteW(NULL, L"open", L"wordpad.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenRDP(HWND hWnd) { ShellExecuteW(NULL, L"open", L"mstsc.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenControlPanel(HWND hWnd) { ShellExecuteW(NULL, L"open", L"control.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenDeviceManager(HWND hWnd) { ShellExecuteW(NULL, L"open", L"devmgmt.msc", NULL, NULL, SW_SHOWNORMAL); }
void OpenDiskManagement(HWND hWnd) { ShellExecuteW(NULL, L"open", L"diskmgmt.msc", NULL, NULL, SW_SHOWNORMAL); }
void OpenMsconfig(HWND hWnd) { ShellExecuteW(NULL, L"open", L"msconfig.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenPerfMon(HWND hWnd) { ShellExecuteW(NULL, L"open", L"perfmon.msc", NULL, NULL, SW_SHOWNORMAL); }
void OpenResMon(HWND hWnd) { ShellExecuteW(NULL, L"open", L"resmon.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenFirewallSettings(HWND hWnd) { ShellExecuteW(NULL, L"open", L"firewall.cpl", NULL, NULL, SW_SHOWNORMAL); }
void OpenUserAccounts(HWND hWnd) { ShellExecuteW(NULL, L"open", L"netplwiz.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenProgramsAndFeatures(HWND hWnd) { ShellExecuteW(NULL, L"open", L"appwiz.cpl", NULL, NULL, SW_SHOWNORMAL); }
void OpenPowerOptions(HWND hWnd) { ShellExecuteW(NULL, L"open", L"powercfg.cpl", NULL, NULL, SW_SHOWNORMAL); }
void OpenNetworkConnections(HWND hWnd) { ShellExecuteW(NULL, L"open", L"ncpa.cpl", NULL, NULL, SW_SHOWNORMAL); }
void OpenTimedateCpl(HWND hWnd) { ShellExecuteW(NULL, L"open", L"timedate.cpl", NULL, NULL, SW_SHOWNORMAL); }
void OpenMouseSettings(HWND hWnd) { ShellExecuteW(NULL, L"open", L"main.cpl", NULL, NULL, SW_SHOWNORMAL); }
void OpenKeyboardSettings(HWND hWnd) { ShellExecuteW(NULL, L"open", L"control.exe keyboard", NULL, NULL, SW_SHOWNORMAL); }
void OpenFontsFolder(HWND hWnd) { ShellExecuteW(NULL, L"open", L"fonts", NULL, NULL, SW_SHOWNORMAL); }
void OpenFolderOptions(HWND hWnd) { ShellExecuteW(NULL, L"open", L"control.exe folders", NULL, NULL, SW_SHOWNORMAL); }
void OpenSearchIndexOptions(HWND hWnd) { ShellExecuteW(NULL, L"open", L"control.exe srchadmin.dll", NULL, NULL, SW_SHOWNORMAL); }

// 新增20个
void OpenExplorer(HWND hWnd) { ShellExecuteW(NULL, L"open", L"explorer.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenPowerShell(HWND hWnd) { ShellExecuteW(NULL, L"open", L"powershell.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenCommandPrompt(HWND hWnd) { ShellExecuteW(NULL, L"open", L"cmd.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenCharMap(HWND hWnd) { ShellExecuteW(NULL, L"open", L"charmap.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenMagnify(HWND hWnd) { ShellExecuteW(NULL, L"open", L"magnify.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenUtilman(HWND hWnd) { ShellExecuteW(NULL, L"open", L"utilman.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenWinver(HWND hWnd) { ShellExecuteW(NULL, L"open", L"winver.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenDxDiag(HWND hWnd) { ShellExecuteW(NULL, L"open", L"dxdiag.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenDiskpart(HWND hWnd) { ShellExecuteW(NULL, L"open", L"diskpart.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenGpedit(HWND hWnd) { ShellExecuteW(NULL, L"open", L"gpedit.msc", NULL, NULL, SW_SHOWNORMAL); }
void OpenLusrmgr(HWND hWnd) { ShellExecuteW(NULL, L"open", L"lusrmgr.msc", NULL, NULL, SW_SHOWNORMAL); }
void OpenSysdm(HWND hWnd) { ShellExecuteW(NULL, L"open", L"sysdm.cpl", NULL, NULL, SW_SHOWNORMAL); }
void OpenWindowsTools(HWND hWnd) { ShellExecuteW(NULL, L"open", L"control.exe admintools", NULL, NULL, SW_SHOWNORMAL); }
void OpenNetplwiz(HWND hWnd) { ShellExecuteW(NULL, L"open", L"netplwiz.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenMsra(HWND hWnd) { ShellExecuteW(NULL, L"open", L"msra.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenMsdt(HWND hWnd) { ShellExecuteW(NULL, L"open", L"msdt.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenSdclt(HWND hWnd) { ShellExecuteW(NULL, L"open", L"sdclt.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenJoyCpl(HWND hWnd) { ShellExecuteW(NULL, L"open", L"joy.cpl", NULL, NULL, SW_SHOWNORMAL); }
void OpenRecovery(HWND hWnd) { ShellExecuteW(NULL, L"open", L"recoverydrive.exe", NULL, NULL, SW_SHOWNORMAL); }
void OpenSystemPropertiesAdvanced(HWND hWnd) { ShellExecuteW(NULL, L"open", L"SystemPropertiesAdvanced.exe", NULL, NULL, SW_SHOWNORMAL); }
// ==================== 窗口过程 ====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        GdiplusStartupInput gdi; GdiplusStartup(&g_gdiplusToken, &gdi, NULL);
        g_hFontNormal = CreateFontW(-20, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Microsoft YaHei UI");
        g_hFontLog = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Cascadia Code");
        BOOL immersive = TRUE; DwmSetWindowAttribute(hWnd, DWMWA_WINDOW_CORNER_PREFERENCE, &immersive, sizeof(immersive));

        int y = 15;
        // 四个模式单选按钮
        CreateWindowW(L"BUTTON", L"随机生成", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
            25, y, 110, 30, hWnd, (HMENU)IDC_RADIO_MODE_RANDOM, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"自编译", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            145, y, 90, 30, hWnd, (HMENU)IDC_RADIO_MODE_MANUAL, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"打包ZIP", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            245, y, 90, 30, hWnd, (HMENU)IDC_RADIO_MODE_PACKZIP, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"实用工具", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            345, y, 90, 30, hWnd, (HMENU)IDC_RADIO_MODE_TOOLS, g_hInst, NULL);
        y += 55;

        // ---------- 随机生成区域 ----------
        CreateWindowW(L"STATIC", L"起始 ID：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, y, 100, 30, hWnd, (HMENU)IDC_STATIC_START_LABEL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 130, y, 150, 30, hWnd, (HMENU)IDC_EDIT_START, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"结束 ID：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 310, y, 100, 30, hWnd, (HMENU)IDC_STATIC_END_LABEL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 420, y, 150, 30, hWnd, (HMENU)IDC_EDIT_END, g_hInst, NULL);
        y += 55;
        CreateWindowW(L"STATIC", L"数据类型：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, y, 100, 30, hWnd, (HMENU)IDC_STATIC_TYPE_LABEL, g_hInst, NULL);
        HWND hCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER,
            130, y, 270, 350, hWnd, (HMENU)IDC_COMBO_TYPE, g_hInst, NULL);
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"整数（随机）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"整数（不重复）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"字符串（小写）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"字符串（大写）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"字符串（混合）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"浮点数（随机）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"日期时间");
        SendMessage(hCombo, CB_SETCURSEL, 0, 0);
        y += 55;
        CreateWindowW(L"STATIC", L"范围/长度：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, y, 100, 30, hWnd, (HMENU)IDC_STATIC_RANGE_LABEL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, y, 130, 30, hWnd, (HMENU)IDC_EDIT_RANGE1, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"~", WS_CHILD | WS_VISIBLE | SS_CENTER, 265, y, 20, 30, hWnd, (HMENU)IDC_STATIC_RANGESEP, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 295, y, 130, 30, hWnd, (HMENU)IDC_EDIT_RANGE2, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, y, 290, 30, hWnd, (HMENU)IDC_EDIT_STRLEN, g_hInst, NULL);
        ShowWindow(GetDlgItem(hWnd, IDC_EDIT_STRLEN), SW_HIDE);
        y += 55;
        CreateWindowW(L"STATIC", L"每文件个数：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, y, 100, 30, hWnd, (HMENU)IDC_STATIC_COUNT_LABEL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 130, y, 150, 30, hWnd, (HMENU)IDC_EDIT_COUNT, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"在文件中写入个数", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            300, y, 190, 30, hWnd, (HMENU)IDC_CHECK_WRITE, g_hInst, NULL);
        y += 55;
        CreateWindowW(L"STATIC", L"额外变量：", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, y, 100, 30, hWnd, (HMENU)IDC_STATIC_VAR_LABEL, g_hInst, NULL);
        y += 35;
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | WS_VSCROLL,
            20, y, 800, 180, hWnd, (HMENU)IDC_EDIT_VAR_TEXT, g_hInst, NULL);
        y += 190;
        CreateWindowW(L"BUTTON", L"生成 .in 文件", WS_CHILD | WS_VISIBLE | BS_FLAT,
            20, y, 160, 40, hWnd, (HMENU)IDC_BUTTON_GEN, g_hInst, NULL);
        y += 60;

        // ---------- 自编译模式（隐藏） ----------
        CreateWindowW(L"STATIC", L"输入文件名：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 45, 100, 30, hWnd, (HMENU)IDC_STATIC_MANUAL_LABEL1, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, 45, 500, 30, hWnd, (HMENU)IDC_EDIT_MANUAL_INNAME, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"输入内容：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 85, 100, 30, hWnd, (HMENU)IDC_STATIC_MANUAL_LABEL2, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | WS_VSCROLL,
            130, 85, 750, 220, hWnd, (HMENU)IDC_EDIT_MANUAL_INCONTENT, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"保存为 .in", WS_CHILD | WS_VISIBLE | BS_FLAT,
            20, 320, 160, 40, hWnd, (HMENU)IDC_BUTTON_MANUAL_SAVEIN, g_hInst, NULL);

        // ---------- 打包ZIP模式（隐藏） ----------
        CreateWindowW(L"STATIC", L"ZIP 文件名：", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 45, 120, 30, hWnd, (HMENU)IDC_STATIC_PACK_LABEL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 20, 85, 300, 30, hWnd, (HMENU)IDC_EDIT_PACK_ZIPNAME, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"立即打包", WS_CHILD | WS_VISIBLE | BS_FLAT,
            20, 135, 160, 40, hWnd, (HMENU)IDC_BUTTON_PACK_EXECUTE, g_hInst, NULL);

        // ---------- 实用工具模式控件（隐藏，6列布局） ----------
        int toolX = 20, toolY = 60, toolW = 190, toolH = 40, toolGap = 50;
        // 列1
        CreateWindowW(L"BUTTON", L"百度搜索", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_BAIDUSEARCH, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"打开网址", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_OPENURL, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"随机密码", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_PASSWORDGEN, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"计算器", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_CALCULATOR, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"记事本", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_NOTEPAD, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"画图", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_PAINT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"写字板", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_WORDPAD, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"远程桌面", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_RDP, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"控制面板", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_CONTROLPANEL, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"设备管理器", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_DEVMGR, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"磁盘管理", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_DISKMGMT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"系统配置", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_MSCONFIG, g_hInst, NULL);
        // 列2
        toolX = 230; toolY = 60;
        CreateWindowW(L"BUTTON", L"文件加密", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_FILEENCRYPT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"定时关机", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_TIMER_SHUTDOWN, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"天气查询", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_WEATHER, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"单位换算", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_UNITCONVERT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"二维码生成", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_QRCODE, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"截图工具", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_SNIPPING, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"取色器", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_COLORPICKER, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"屏幕键盘", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_OSK, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"清理临时文件", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_CLEANTEMP, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"IP查询", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_IPLOOKUP, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"MD5校验", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_MD5, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"文件比较", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_FILECOMPARE, g_hInst, NULL);
        // 列3
        toolX = 440; toolY = 60;
        CreateWindowW(L"BUTTON", L"文本替换", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_TEXTREPLACE, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"倒计时提醒", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_COUNTDOWN, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"系统信息", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_SYSINFO, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"任务管理器", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_TASKMGR, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"注册表", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_REGEDIT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"服务", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_SERVICES, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"磁盘清理", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_DISKCLEAN, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"计划任务", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_TASKSCHED, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"创建快捷方式", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_SHORTCUT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"文件属性修改", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_FILEATTR, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"哈希计算", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_HASH, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"Base64", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_BASE64, g_hInst, NULL);
        // 列4
        toolX = 650; toolY = 60;
        CreateWindowW(L"BUTTON", L"JSON格式化", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_JSONFORMAT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"资源监视器", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_RESMON, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"防火墙", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_FIREWALL, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"用户账户", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_USERACCOUNTS, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"程序和功能", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_PROGRAMSFEA, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"电源选项", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_POWEROPT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"网络连接", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_NETCONN, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"日期和时间", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_TIMEDATE, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"鼠标设置", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_MOUSE, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"键盘设置", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_KEYBOARD, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"字体文件夹", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_FONTS, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"文件夹选项", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_FOLDEROPT, g_hInst, NULL);
        // 列5
        toolX = 860; toolY = 60;
        CreateWindowW(L"BUTTON", L"搜索索引", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_SEARCHINDEX, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"性能监视器", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_PERFMON, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"文件资源管理器", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_EXPLORER, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"PowerShell", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_POWERSHELL, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"命令提示符", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_CMD, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"字符映射表", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_CHARMAP, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"放大镜", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_MAGNIFY, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"轻松使用", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_UTILMAN, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"Windows 版本", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_WINVER, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"DirectX 诊断", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_DXDIAG, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"磁盘分区", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_DISKPART, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"组策略", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_GPEDIT, g_hInst, NULL);
        // 列6
        toolX = 1070; toolY = 60;
        CreateWindowW(L"BUTTON", L"本地用户和组", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_LUSRMGR, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"系统属性", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_SYSDM, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"管理工具", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_WINDOWS_TOOLS, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"用户账户控制", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_NETPLWIZ, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"远程协助", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_MSRA, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"诊断工具", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_MSDT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"备份和还原", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_SDCLT, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"游戏控制器", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_JOYCPL, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"恢复驱动器", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_RECOVERY, g_hInst, NULL); toolY += toolGap;
        CreateWindowW(L"BUTTON", L"高级系统设置", WS_CHILD | WS_VISIBLE | BS_FLAT, toolX, toolY, toolW, toolH, hWnd, (HMENU)IDC_BUTTON_SYSTEMPROPERTIESADVANCED, g_hInst, NULL);

        // ---------- 公共控件（整体下移） ----------
        y = 700;
        CreateWindowW(L"STATIC", L"生成位置：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, y, 100, 30, hWnd, (HMENU)IDC_STATIC_OUTDIR_LABEL, g_hInst, NULL);
        g_outputDir = GetDesktopPath();
        CreateWindowW(L"EDIT", g_outputDir.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY, 130, y, 700, 30, hWnd, (HMENU)IDC_EDIT_OUTDIR, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"浏览...", WS_CHILD | WS_VISIBLE | BS_FLAT, 840, y, 80, 30, hWnd, (HMENU)IDC_BUTTON_OUTDIR_BROWSE, g_hInst, NULL);
        y += 55;
        CreateWindowW(L"STATIC", L"源文件：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, y, 100, 30, hWnd, NULL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 130, y, 700, 30, hWnd, (HMENU)IDC_EDIT_CPP_PATH, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"浏览...", WS_CHILD | WS_VISIBLE | BS_FLAT, 840, y, 80, 30, hWnd, (HMENU)IDC_BUTTON_BROWSE, g_hInst, NULL);
        y += 55;
        // 功能按钮行
        CreateWindowW(L"BUTTON", L"生成 .out", WS_CHILD | WS_VISIBLE | BS_FLAT, 20, y, 130, 40, hWnd, (HMENU)IDC_BUTTON_UNIFIED_COMPILE, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"颜色设置", WS_CHILD | WS_VISIBLE | BS_FLAT, 170, y, 110, 40, hWnd, (HMENU)IDC_BUTTON_COLOR_SETTING, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"更换主题", WS_CHILD | WS_VISIBLE | BS_FLAT, 300, y, 110, 40, hWnd, (HMENU)IDC_BUTTON_CHANGE_THEME, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"保存背景", WS_CHILD | WS_VISIBLE | BS_FLAT, 430, y, 110, 40, hWnd, (HMENU)IDC_BUTTON_SAVEBG, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"默认颜色", WS_CHILD | WS_VISIBLE | BS_FLAT, 560, y, 110, 40, hWnd, (HMENU)IDC_BUTTON_DEFAULT_COLOR, g_hInst, NULL);
        y += 60;
        // 主题按钮两行
        CreateWindowW(L"BUTTON", L"经典白底", WS_CHILD | WS_VISIBLE | BS_FLAT, 20, y, 110, 34, hWnd, (HMENU)IDC_THEME_WHITE, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"暗夜灰底", WS_CHILD | WS_VISIBLE | BS_FLAT, 140, y, 110, 34, hWnd, (HMENU)IDC_THEME_DARK, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"护眼绿底", WS_CHILD | WS_VISIBLE | BS_FLAT, 260, y, 110, 34, hWnd, (HMENU)IDC_THEME_GREEN, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"柔和米色", WS_CHILD | WS_VISIBLE | BS_FLAT, 380, y, 110, 34, hWnd, (HMENU)IDC_THEME_CREAM, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"深蓝科技", WS_CHILD | WS_VISIBLE | BS_FLAT, 500, y, 110, 34, hWnd, (HMENU)IDC_THEME_BLUE, g_hInst, NULL);
        y += 48;
        CreateWindowW(L"BUTTON", L"黄昏橙紫", WS_CHILD | WS_VISIBLE | BS_FLAT, 20, y, 110, 34, hWnd, (HMENU)IDC_THEME_ORANGE, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"森林暗绿", WS_CHILD | WS_VISIBLE | BS_FLAT, 140, y, 110, 34, hWnd, (HMENU)IDC_THEME_FOREST, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"极简灰白", WS_CHILD | WS_VISIBLE | BS_FLAT, 260, y, 110, 34, hWnd, (HMENU)IDC_THEME_GREY, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"粉彩樱花", WS_CHILD | WS_VISIBLE | BS_FLAT, 380, y, 110, 34, hWnd, (HMENU)IDC_THEME_PINK, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"黑金奢华", WS_CHILD | WS_VISIBLE | BS_FLAT, 500, y, 110, 34, hWnd, (HMENU)IDC_THEME_GOLD, g_hInst, NULL);
        y += 55;
        // ---------- 辅助功能按钮（始终可见） ----------
        CreateWindowW(L"BUTTON", L"数据统计", WS_CHILD | WS_VISIBLE | BS_FLAT, 20, y, 130, 34, hWnd, (HMENU)IDC_BUTTON_STATISTICS, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"查看文件", WS_CHILD | WS_VISIBLE | BS_FLAT, 170, y, 130, 34, hWnd, (HMENU)IDC_BUTTON_VIEWFILE, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"进度生成", WS_CHILD | WS_VISIBLE | BS_FLAT, 320, y, 130, 34, hWnd, (HMENU)IDC_BUTTON_PROGRESS, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"时间戳目录", WS_CHILD | WS_VISIBLE | BS_FLAT, 470, y, 150, 34, hWnd, (HMENU)IDC_BUTTON_TIMESTAMP, g_hInst, NULL);
        y += 45;
        CreateWindowW(L"BUTTON", L"批量重命名", WS_CHILD | WS_VISIBLE | BS_FLAT, 20, y, 130, 34, hWnd, (HMENU)IDC_BUTTON_RENAME, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"生成大文件", WS_CHILD | WS_VISIBLE | BS_FLAT, 170, y, 130, 34, hWnd, (HMENU)IDC_BUTTON_BIGFILE, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"保存配置", WS_CHILD | WS_VISIBLE | BS_FLAT, 320, y, 130, 34, hWnd, (HMENU)IDC_BUTTON_SAVECONF, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"加载配置", WS_CHILD | WS_VISIBLE | BS_FLAT, 470, y, 130, 34, hWnd, (HMENU)IDC_BUTTON_LOADCONF, g_hInst, NULL);
        y += 45;
        // 预览框
        g_hPreviewEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            20, y, 900, 130, hWnd, (HMENU)IDC_EDIT_PREVIEW, g_hInst, NULL);
        y += 145;
        // 自定义字符集
        CreateWindowW(L"STATIC", L"自定义字符集：", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, y, 120, 30, hWnd, (HMENU)IDC_STATIC_CUSTOM_LABEL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 150, y, 300, 30, hWnd, (HMENU)IDC_EDIT_CUSTOM_CHARSET, g_hInst, NULL);
        y += 55;
        // g++ 编译器路径
        CreateWindowW(L"STATIC", L"g++ 编译器路径：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, y, 130, 30, hWnd, (HMENU)IDC_STATIC_GPP_LABEL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 160, y, 600, 30, hWnd, (HMENU)IDC_EDIT_GPP_PATH, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"浏览...", WS_CHILD | WS_VISIBLE | BS_FLAT, 770, y, 70, 30, hWnd, (HMENU)IDC_BUTTON_GPP_BROWSE, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"自动检测", WS_CHILD | WS_VISIBLE | BS_FLAT, 850, y, 80, 30, hWnd, (HMENU)IDC_BUTTON_GPP_AUTO, g_hInst, NULL);
        y += 55;
        CreateWindowW(L"STATIC", L"就绪", WS_CHILD | WS_VISIBLE | SS_SUNKEN, 20, y, 700, 30, hWnd, (HMENU)IDC_STATIC_STATUS, g_hInst, NULL);
        y += 55;
        g_hLogEdit = CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL | ES_AUTOVSCROLL,
            20, y, 900, 90, hWnd, (HMENU)IDC_EDIT_LOG, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"作者：starryfast | Bilibili 同名 | Luogu 2070675", WS_CHILD | WS_VISIBLE | SS_LEFT,
            20, y - 50, 500, 26, hWnd, (HMENU)IDC_STATIC_AUTHOR, g_hInst, NULL);

        g_totalContentHeight = y + 200;

        // 隐藏非默认模式控件
        int hideManual[] = { IDC_EDIT_MANUAL_INNAME, IDC_EDIT_MANUAL_INCONTENT, IDC_BUTTON_MANUAL_SAVEIN,
            IDC_STATIC_MANUAL_LABEL1, IDC_STATIC_MANUAL_LABEL2 };
        for (int id : hideManual) ShowWindow(GetDlgItem(hWnd, id), SW_HIDE);
        int hidePack[] = { IDC_STATIC_PACK_LABEL, IDC_EDIT_PACK_ZIPNAME, IDC_BUTTON_PACK_EXECUTE };
        for (int id : hidePack) ShowWindow(GetDlgItem(hWnd, id), SW_HIDE);
        int hideTools[] = {
            IDC_BUTTON_BAIDUSEARCH, IDC_BUTTON_OPENURL, IDC_BUTTON_PASSWORDGEN, IDC_BUTTON_CALCULATOR,
            IDC_BUTTON_NOTEPAD, IDC_BUTTON_FILEENCRYPT, IDC_BUTTON_TIMER_SHUTDOWN, IDC_BUTTON_WEATHER,
            IDC_BUTTON_UNITCONVERT, IDC_BUTTON_QRCODE,
            IDC_BUTTON_SNIPPING, IDC_BUTTON_COLORPICKER, IDC_BUTTON_OSK, IDC_BUTTON_CLEANTEMP,
            IDC_BUTTON_IPLOOKUP, IDC_BUTTON_MD5, IDC_BUTTON_FILECOMPARE, IDC_BUTTON_TEXTREPLACE,
            IDC_BUTTON_COUNTDOWN, IDC_BUTTON_SYSINFO, IDC_BUTTON_TASKMGR, IDC_BUTTON_REGEDIT,
            IDC_BUTTON_SERVICES, IDC_BUTTON_DISKCLEAN, IDC_BUTTON_TASKSCHED, IDC_BUTTON_SHORTCUT,
            IDC_BUTTON_FILEATTR, IDC_BUTTON_HASH, IDC_BUTTON_BASE64, IDC_BUTTON_JSONFORMAT,
            IDC_BUTTON_PAINT, IDC_BUTTON_WORDPAD, IDC_BUTTON_RDP, IDC_BUTTON_CONTROLPANEL,
            IDC_BUTTON_DEVMGR, IDC_BUTTON_DISKMGMT, IDC_BUTTON_MSCONFIG, IDC_BUTTON_PERFMON,
            IDC_BUTTON_RESMON, IDC_BUTTON_FIREWALL, IDC_BUTTON_USERACCOUNTS, IDC_BUTTON_PROGRAMSFEA,
            IDC_BUTTON_POWEROPT, IDC_BUTTON_NETCONN, IDC_BUTTON_TIMEDATE, IDC_BUTTON_MOUSE,
            IDC_BUTTON_KEYBOARD, IDC_BUTTON_FONTS, IDC_BUTTON_FOLDEROPT, IDC_BUTTON_SEARCHINDEX,
            IDC_BUTTON_EXPLORER, IDC_BUTTON_POWERSHELL, IDC_BUTTON_CMD, IDC_BUTTON_CHARMAP,
            IDC_BUTTON_MAGNIFY, IDC_BUTTON_UTILMAN, IDC_BUTTON_WINVER, IDC_BUTTON_DXDIAG,
            IDC_BUTTON_DISKPART, IDC_BUTTON_GPEDIT, IDC_BUTTON_LUSRMGR, IDC_BUTTON_SYSDM,
            IDC_BUTTON_WINDOWS_TOOLS, IDC_BUTTON_NETPLWIZ, IDC_BUTTON_MSRA, IDC_BUTTON_MSDT,
            IDC_BUTTON_SDCLT, IDC_BUTTON_JOYCPL, IDC_BUTTON_RECOVERY, IDC_BUTTON_SYSTEMPROPERTIESADVANCED
        };
        for (int id : hideTools) ShowWindow(GetDlgItem(hWnd, id), SW_HIDE);

        // 设置字体
        HWND child = GetWindow(hWnd, GW_CHILD);
        while (child) { SendMessage(child, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE); child = GetNextWindow(child, GW_HWNDNEXT); }
        SendMessage(g_hLogEdit, WM_SETFONT, (WPARAM)g_hFontLog, TRUE);
        SendMessage(g_hPreviewEdit, WM_SETFONT, (WPARAM)g_hFontLog, TRUE);

        LoadColorSettings();
        LoadBackgroundPath();   // 从注册表恢复背景图片（持久化）

        // 加载或搜索 g++ 路径（持久化）
        LogMessage(L"程序已启动。");
        wstring gppPath = LoadGppPath();
        if (!gppPath.empty() && FileExists(gppPath)) {
            SetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, gppPath.c_str());
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"就绪（已加载保存的 g++）");
        }
        else {
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"正在搜索 g++...");
            gppPath = AutoFindGpp();
            if (!gppPath.empty()) {
                SetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, gppPath.c_str());
                SaveGppPath(gppPath);
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"已自动找到 g++ 并保存");
            }
            else {
                SetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, L"未找到，请手动指定");
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"请手动选择 g++.exe");
            }
        }
        break;
    }

    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_RADIO_MODE_RANDOM) SwitchMode(hWnd, 0);
        else if (id == IDC_RADIO_MODE_MANUAL) SwitchMode(hWnd, 1);
        else if (id == IDC_RADIO_MODE_PACKZIP) SwitchMode(hWnd, 2);
        else if (id == IDC_RADIO_MODE_TOOLS)  SwitchMode(hWnd, 3);
        else if (id == IDC_COMBO_TYPE && HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = SendMessage(GetDlgItem(hWnd, IDC_COMBO_TYPE), CB_GETCURSEL, 0, 0);
            BOOL showRange = (sel <= 1 || sel == 5 || sel == 6);
            BOOL showStrLen = (sel >= 2 && sel <= 4);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_RANGE1), showRange ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_RANGE2), showRange ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_STRLEN), showStrLen ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_STATIC_RANGESEP), showRange ? SW_SHOW : SW_HIDE);
        }
        else if (id == IDC_BUTTON_COLOR_SETTING) ShowColorDialog(hWnd);
        else if (id == IDC_BUTTON_DEFAULT_COLOR) { ClearBackgroundImage(); SetDefaultColors(hWnd); }
        else if (id == IDC_BUTTON_CHANGE_THEME) ChangeTheme(hWnd);
        else if (id == IDC_BUTTON_SAVEBG) SaveBackground(hWnd);
        else if (id >= IDC_THEME_WHITE && id <= IDC_THEME_GOLD) {
            ClearBackgroundImage();
            switch (id) {
            case IDC_THEME_WHITE:  SetPresetTheme(RGB(0, 0, 0), RGB(255, 255, 255)); break;
            case IDC_THEME_DARK:   SetPresetTheme(RGB(230, 230, 230), RGB(50, 50, 50)); break;
            case IDC_THEME_GREEN:  SetPresetTheme(RGB(0, 0, 0), RGB(200, 235, 200)); break;
            case IDC_THEME_CREAM:  SetPresetTheme(RGB(80, 50, 30), RGB(255, 248, 220)); break;
            case IDC_THEME_BLUE:   SetPresetTheme(RGB(220, 235, 255), RGB(25, 40, 70)); break;
            case IDC_THEME_ORANGE: SetPresetTheme(RGB(255, 240, 220), RGB(80, 40, 80)); break;
            case IDC_THEME_FOREST: SetPresetTheme(RGB(200, 220, 180), RGB(40, 60, 40)); break;
            case IDC_THEME_GREY:   SetPresetTheme(RGB(60, 60, 60), RGB(240, 240, 240)); break;
            case IDC_THEME_PINK:   SetPresetTheme(RGB(100, 60, 80), RGB(255, 240, 245)); break;
            case IDC_THEME_GOLD:   SetPresetTheme(RGB(255, 215, 0), RGB(20, 20, 20)); break;
            }
            ApplyColorSettings(hWnd); LogMessage(L"主题已切换");
        }
        else if (id == IDC_BUTTON_PREVIEW) PreviewData(hWnd);
        else if (id == IDC_BUTTON_DELETEALL) DeleteAllGeneratedFiles(hWnd);
        else if (id == IDC_BUTTON_EXPORTCSV) ExportToCSV(hWnd);
        else if (id == IDC_BUTTON_STATISTICS) DataStatistics(hWnd);
        else if (id == IDC_BUTTON_VIEWFILE) ViewFileContent(hWnd);
        else if (id == IDC_BUTTON_PROGRESS) ProgressGenerate(hWnd);
        else if (id == IDC_BUTTON_TIMESTAMP) UseTimestampDir(hWnd);
        else if (id == IDC_BUTTON_RENAME) BatchRename(hWnd);
        else if (id == IDC_BUTTON_BIGFILE) GenerateBigFile(hWnd);
        else if (id == IDC_BUTTON_SAVECONF) SaveConfig(hWnd);
        else if (id == IDC_BUTTON_LOADCONF) LoadConfig(hWnd);
        else if (id == IDC_BUTTON_CMDLINE) MessageBoxW(hWnd, L"支持命令行参数：-start 1 -end 10 -type int", L"命令行帮助", MB_OK);
        else if (id == IDC_BUTTON_HELP) ShowHelp(hWnd);
        else if (id == IDC_BUTTON_FLOATGEN) GenerateFloatData(hWnd);
        else if (id == IDC_BUTTON_DATETIMEGEN) GenerateDateTimeData(hWnd);
        else if (id == IDC_BUTTON_DEDUP) DeduplicateFiles(hWnd);
        else if (id == IDC_BUTTON_SPLITFILE) SplitFile(hWnd);
        else if (id == IDC_BUTTON_CHANGEEXT) ChangeFileExtension(hWnd);
        // 所有小工具处理
        else if (id == IDC_BUTTON_BAIDUSEARCH) BaiduSearch(hWnd);
        else if (id == IDC_BUTTON_OPENURL) OpenUrl(hWnd);
        else if (id == IDC_BUTTON_PASSWORDGEN) PasswordGenerator(hWnd);
        else if (id == IDC_BUTTON_CALCULATOR) OpenCalculator(hWnd);
        else if (id == IDC_BUTTON_NOTEPAD) OpenNotepad(hWnd);
        else if (id == IDC_BUTTON_FILEENCRYPT) FileEncrypt(hWnd);
        else if (id == IDC_BUTTON_TIMER_SHUTDOWN) TimerShutdown(hWnd);
        else if (id == IDC_BUTTON_WEATHER) WeatherQuery(hWnd);
        else if (id == IDC_BUTTON_UNITCONVERT) UnitConvert(hWnd);
        else if (id == IDC_BUTTON_QRCODE) QRCodeGenerator(hWnd);
        else if (id == IDC_BUTTON_SNIPPING) OpenSnippingTool(hWnd);
        else if (id == IDC_BUTTON_COLORPICKER) ColorPicker(hWnd);
        else if (id == IDC_BUTTON_OSK) OpenOSK(hWnd);
        else if (id == IDC_BUTTON_CLEANTEMP) CleanTempFiles(hWnd);
        else if (id == IDC_BUTTON_IPLOOKUP) IPLookup(hWnd);
        else if (id == IDC_BUTTON_MD5) ComputeMD5(hWnd);
        else if (id == IDC_BUTTON_FILECOMPARE) FileCompare(hWnd);
        else if (id == IDC_BUTTON_TEXTREPLACE) TextReplaceInFiles(hWnd);
        else if (id == IDC_BUTTON_COUNTDOWN) CountdownTimer(hWnd);
        else if (id == IDC_BUTTON_SYSINFO) SystemInfo(hWnd);
        else if (id == IDC_BUTTON_TASKMGR) OpenTaskManager(hWnd);
        else if (id == IDC_BUTTON_REGEDIT) OpenRegEdit(hWnd);
        else if (id == IDC_BUTTON_SERVICES) OpenServices(hWnd);
        else if (id == IDC_BUTTON_DISKCLEAN) DiskCleanup(hWnd);
        else if (id == IDC_BUTTON_TASKSCHED) TaskScheduler(hWnd);
        else if (id == IDC_BUTTON_SHORTCUT) CreateShortcut(hWnd);
        else if (id == IDC_BUTTON_FILEATTR) ChangeFileAttributes(hWnd);
        else if (id == IDC_BUTTON_HASH) ComputeHash(hWnd);
        else if (id == IDC_BUTTON_BASE64) Base64EncodeDecode(hWnd);
        else if (id == IDC_BUTTON_JSONFORMAT) JSONFormatter(hWnd);
        else if (id == IDC_BUTTON_PAINT) OpenPaint(hWnd);
        else if (id == IDC_BUTTON_WORDPAD) OpenWordpad(hWnd);
        else if (id == IDC_BUTTON_RDP) OpenRDP(hWnd);
        else if (id == IDC_BUTTON_CONTROLPANEL) OpenControlPanel(hWnd);
        else if (id == IDC_BUTTON_DEVMGR) OpenDeviceManager(hWnd);
        else if (id == IDC_BUTTON_DISKMGMT) OpenDiskManagement(hWnd);
        else if (id == IDC_BUTTON_MSCONFIG) OpenMsconfig(hWnd);
        else if (id == IDC_BUTTON_PERFMON) OpenPerfMon(hWnd);
        else if (id == IDC_BUTTON_RESMON) OpenResMon(hWnd);
        else if (id == IDC_BUTTON_FIREWALL) OpenFirewallSettings(hWnd);
        else if (id == IDC_BUTTON_USERACCOUNTS) OpenUserAccounts(hWnd);
        else if (id == IDC_BUTTON_PROGRAMSFEA) OpenProgramsAndFeatures(hWnd);
        else if (id == IDC_BUTTON_POWEROPT) OpenPowerOptions(hWnd);
        else if (id == IDC_BUTTON_NETCONN) OpenNetworkConnections(hWnd);
        else if (id == IDC_BUTTON_TIMEDATE) OpenTimedateCpl(hWnd);
        else if (id == IDC_BUTTON_MOUSE) OpenMouseSettings(hWnd);
        else if (id == IDC_BUTTON_KEYBOARD) OpenKeyboardSettings(hWnd);
        else if (id == IDC_BUTTON_FONTS) OpenFontsFolder(hWnd);
        else if (id == IDC_BUTTON_FOLDEROPT) OpenFolderOptions(hWnd);
        else if (id == IDC_BUTTON_SEARCHINDEX) OpenSearchIndexOptions(hWnd);
        else if (id == IDC_BUTTON_EXPLORER) OpenExplorer(hWnd);
        else if (id == IDC_BUTTON_POWERSHELL) OpenPowerShell(hWnd);
        else if (id == IDC_BUTTON_CMD) OpenCommandPrompt(hWnd);
        else if (id == IDC_BUTTON_CHARMAP) OpenCharMap(hWnd);
        else if (id == IDC_BUTTON_MAGNIFY) OpenMagnify(hWnd);
        else if (id == IDC_BUTTON_UTILMAN) OpenUtilman(hWnd);
        else if (id == IDC_BUTTON_WINVER) OpenWinver(hWnd);
        else if (id == IDC_BUTTON_DXDIAG) OpenDxDiag(hWnd);
        else if (id == IDC_BUTTON_DISKPART) OpenDiskpart(hWnd);
        else if (id == IDC_BUTTON_GPEDIT) OpenGpedit(hWnd);
        else if (id == IDC_BUTTON_LUSRMGR) OpenLusrmgr(hWnd);
        else if (id == IDC_BUTTON_SYSDM) OpenSysdm(hWnd);
        else if (id == IDC_BUTTON_WINDOWS_TOOLS) OpenWindowsTools(hWnd);
        else if (id == IDC_BUTTON_NETPLWIZ) OpenNetplwiz(hWnd);
        else if (id == IDC_BUTTON_MSRA) OpenMsra(hWnd);
        else if (id == IDC_BUTTON_MSDT) OpenMsdt(hWnd);
        else if (id == IDC_BUTTON_SDCLT) OpenSdclt(hWnd);
        else if (id == IDC_BUTTON_JOYCPL) OpenJoyCpl(hWnd);
        else if (id == IDC_BUTTON_RECOVERY) OpenRecovery(hWnd);
        else if (id == IDC_BUTTON_SYSTEMPROPERTIESADVANCED) OpenSystemPropertiesAdvanced(hWnd);
        // 生成 .in
        else if (id == IDC_BUTTON_GEN) {
            int start = GetDlgItemInt(hWnd, IDC_EDIT_START, NULL, FALSE), end = GetDlgItemInt(hWnd, IDC_EDIT_END, NULL, FALSE);
            int sel = SendMessage(GetDlgItem(hWnd, IDC_COMBO_TYPE), CB_GETCURSEL, 0, 0);
            long long r1 = 0, r2 = 0; int slen = 0;
            if (sel <= 1 || sel == 5 || sel == 6) { r1 = GetDlgItemInt(hWnd, IDC_EDIT_RANGE1, NULL, TRUE); r2 = GetDlgItemInt(hWnd, IDC_EDIT_RANGE2, NULL, TRUE); }
            else if (sel >= 2 && sel <= 4) slen = GetDlgItemInt(hWnd, IDC_EDIT_STRLEN, NULL, FALSE);
            int cnt = GetDlgItemInt(hWnd, IDC_EDIT_COUNT, NULL, FALSE);
            bool wr = SendMessage(GetDlgItem(hWnd, IDC_CHECK_WRITE), BM_GETCHECK, 0, 0) == BST_CHECKED;
            wchar_t custom[256] = { 0 }; GetDlgItemTextW(hWnd, IDC_EDIT_CUSTOM_CHARSET, custom, 256);
            wstring customStr = custom;
            if (start > end || cnt <= 0) { MessageBoxW(hWnd, L"参数错误", L"错误", MB_ICONERROR); break; }
            vector<ExtraVar> vars = ParseExtraVars(GetDlgItem(hWnd, IDC_EDIT_VAR_TEXT));
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"正在生成数据...");
            if (sel == 5) GenerateFloatData(hWnd);
            else if (sel == 6) GenerateDateTimeData(hWnd);
            else {
                int dataType = sel + 1;
                if (GenerateInputFiles(start, end, dataType, r1, r2, slen, cnt, wr, vars, customStr))
                    LogMessage(L"生成成功");
                else { LogMessage(L"生成失败"); MessageBoxW(hWnd, L"生成失败，请检查参数", L"错误", MB_ICONERROR); }
            }
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"生成完毕");
        }
        else if (id == IDC_BUTTON_MANUAL_SAVEIN) {
            wchar_t name[256] = { 0 }; GetDlgItemTextW(hWnd, IDC_EDIT_MANUAL_INNAME, name, 256);
            if (!name[0]) break;
            wstring ns = name; if (ns.find(L'.') == wstring::npos) ns += L".in";
            wstring full = (ns.find(L'\\') == wstring::npos) ? (g_outputDir + L"\\" + ns) : ns;
            int len = GetWindowTextLengthW(GetDlgItem(hWnd, IDC_EDIT_MANUAL_INCONTENT));
            wchar_t* buf = new wchar_t[len + 1];
            GetDlgItemTextW(hWnd, IDC_EDIT_MANUAL_INCONTENT, buf, len + 1);
            FILE* fp = _wfopen(full.c_str(), L"w"); if (!fp) { delete[] buf; break; }
            fwprintf(fp, L"%s", buf); fclose(fp); delete[] buf;
            AddGeneratedFile(full);
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"已保存");
        }
        else if (id == IDC_BUTTON_BROWSE) {
            OPENFILENAMEW ofn = { sizeof(ofn) }; wchar_t f[MAX_PATH] = { 0 };
            ofn.hwndOwner = hWnd; ofn.lpstrFilter = L"C++\0*.cpp\0"; ofn.lpstrFile = f; ofn.nMaxFile = MAX_PATH; ofn.Flags = OFN_FILEMUSTEXIST;
            if (GetOpenFileNameW(&ofn)) SetDlgItemTextW(hWnd, IDC_EDIT_CPP_PATH, f);
        }
        else if (id == IDC_BUTTON_GPP_BROWSE) {
            OPENFILENAMEW ofn = { sizeof(ofn) }; wchar_t f[MAX_PATH] = { 0 };
            ofn.hwndOwner = hWnd; ofn.lpstrFilter = L"EXE\0*.exe\0"; ofn.lpstrFile = f; ofn.nMaxFile = MAX_PATH; ofn.Flags = OFN_FILEMUSTEXIST;
            if (GetOpenFileNameW(&ofn)) {
                SetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, f);
                SaveGppPath(f);
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"g++ 路径已保存");
            }
        }
        else if (id == IDC_BUTTON_GPP_AUTO) {
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"搜索g++...");
            wstring p = AutoFindGpp();
            if (!p.empty()) {
                SetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, p.c_str());
                SaveGppPath(p);
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"找到g++并已保存");
            }
            else SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"未找到g++");
        }
        else if (id == IDC_BUTTON_UNIFIED_COMPILE) {
            wchar_t cpp[MAX_PATH], gpp[MAX_PATH];
            GetDlgItemTextW(hWnd, IDC_EDIT_CPP_PATH, cpp, MAX_PATH);
            GetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, gpp, MAX_PATH);
            if (!cpp[0] || !FileExists(gpp)) { MessageBoxW(hWnd, L"请选择cpp和g++", L"提示", MB_OK); break; }
            if (g_currentMode == 0) {
                int s = GetDlgItemInt(hWnd, IDC_EDIT_START, NULL, FALSE), e = GetDlgItemInt(hWnd, IDC_EDIT_END, NULL, FALSE);
                if (s > e) break;
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"编译运行中...");
                if (CompileAndRunBatch(gpp, cpp, s, e, hWnd)) {
                    SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"完成");
                    LogMessage(L"批量 .out 生成完毕");
                }
                else SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"失败");
            }
            else if (g_currentMode == 1) {
                wchar_t inName[MAX_PATH] = { 0 }; GetDlgItemTextW(hWnd, IDC_EDIT_MANUAL_INNAME, inName, MAX_PATH);
                wstring ns = inName; if (ns.find(L'.') == wstring::npos) ns += L".in";
                wstring inPath = (ns.find(L'\\') == wstring::npos) ? (g_outputDir + L"\\" + ns) : ns;
                wstring outPath = inPath; size_t dot = outPath.find_last_of(L'.');
                if (dot != wstring::npos) outPath = outPath.substr(0, dot) + L".out";
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"编译运行中...");
                if (CompileAndRunSingle(gpp, cpp, inPath, outPath, hWnd)) {
                    SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"完成");
                    LogMessage(L"单个 .out 生成完毕");
                }
                else SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"失败");
            }
        }
        else if (id == IDC_BUTTON_PACK_EXECUTE) {
            if (g_allGeneratedFiles.empty()) { MessageBoxW(hWnd, L"尚无文件", L"提示", MB_OK); break; }
            wchar_t zn[256] = { 0 }; GetDlgItemTextW(hWnd, IDC_EDIT_PACK_ZIPNAME, zn, 256);
            wstring ns = zn; if (ns.empty()) break;
            if (ns.size() < 4 || _wcsicmp(ns.substr(ns.size() - 4).c_str(), L".zip") != 0) ns += L".zip";
            wstring path = g_outputDir + L"\\" + ns;
            if (PackRecordedFilesToZip(path)) {
                MessageBoxW(hWnd, (L"ZIP已保存：" + path).c_str(), L"成功", MB_OK);
                LogMessage(L"ZIP 打包成功");
            }
            else MessageBoxW(hWnd, L"打包失败，请检查磁盘空间或权限", L"错误", MB_ICONERROR);
        }
        break;
    }

    case WM_SIZE: {
        RECT rc; GetClientRect(hWnd, &rc);
        SCROLLINFO si = { sizeof(si) };
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;
        si.nMax = g_totalContentHeight;
        si.nPage = rc.bottom;
        SetScrollInfo(hWnd, SB_VERT, &si, TRUE);
        if (g_scrollPos + rc.bottom > g_totalContentHeight)
            g_scrollPos = max(0, g_totalContentHeight - (int)rc.bottom);
        ScrollWindowEx(hWnd, 0, -g_scrollPos, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN);
        SetScrollPos(hWnd, SB_VERT, g_scrollPos, TRUE);
        return 0;
    }

    case WM_VSCROLL: {
        RECT rc; GetClientRect(hWnd, &rc);
        int delta = 0;
        switch (LOWORD(wParam)) {
        case SB_LINEUP: delta = -30; break;
        case SB_LINEDOWN: delta = 30; break;
        case SB_PAGEUP: delta = -rc.bottom; break;
        case SB_PAGEDOWN: delta = rc.bottom; break;
        case SB_THUMBTRACK: {
            SCROLLINFO si = { sizeof(si), SIF_TRACKPOS };
            GetScrollInfo(hWnd, SB_VERT, &si);
            delta = si.nTrackPos - g_scrollPos;
            break;
        }
        }
        int newPos = g_scrollPos + delta;
        if (newPos < 0) newPos = 0;
        if (newPos + rc.bottom > g_totalContentHeight) newPos = g_totalContentHeight - rc.bottom;
        if (newPos != g_scrollPos) {
            ScrollWindowEx(hWnd, 0, g_scrollPos - newPos, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN);
            g_scrollPos = newPos;
            SetScrollPos(hWnd, SB_VERT, g_scrollPos, TRUE);
            InvalidateRect(hWnd, NULL, TRUE);
            UpdateWindow(hWnd);
        }
        return 0;
    }

    case WM_MOUSEWHEEL: {
        int zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        int delta = -zDelta / WHEEL_DELTA * 50;
        RECT rc; GetClientRect(hWnd, &rc);
        int newPos = g_scrollPos + delta;
        if (newPos < 0) newPos = 0;
        if (newPos + rc.bottom > g_totalContentHeight) newPos = g_totalContentHeight - rc.bottom;
        if (newPos != g_scrollPos) {
            ScrollWindowEx(hWnd, 0, g_scrollPos - newPos, NULL, NULL, NULL, NULL, SW_INVALIDATE | SW_SCROLLCHILDREN);
            g_scrollPos = newPos;
            SetScrollPos(hWnd, SB_VERT, g_scrollPos, TRUE);
            InvalidateRect(hWnd, NULL, TRUE);
            UpdateWindow(hWnd);
        }
        return 0;
    }

    case WM_ERASEBKGND: {
        RECT rc; GetClientRect(hWnd, &rc);
        DrawBackground((HDC)wParam, rc.right - rc.left, rc.bottom - rc.top);
        return TRUE;
    }

    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam; HWND ctrl = (HWND)lParam;
        int cid = GetDlgCtrlID(ctrl);
        SetTextColor(hdc, cid == IDC_STATIC_AUTHOR ? RGB(80, 80, 80) : g_textColor);
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH br = NULL;
        static COLORREF lastBg = 0xFFFFFFFF;
        if (br == NULL || lastBg != g_bgColor) {
            if (br) DeleteObject(br);
            br = CreateSolidBrush(g_bgColor);
            lastBg = g_bgColor;
        }
        SetBkColor(hdc, g_bgColor);
        return (LRESULT)br;
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, g_textColor);
        SetBkColor(hdc, g_bgColor);
        static HBRUSH brEdit = NULL;
        static COLORREF lastBgEdit = 0xFFFFFFFF;
        if (brEdit == NULL || lastBgEdit != g_bgColor) {
            if (brEdit) DeleteObject(brEdit);
            brEdit = CreateSolidBrush(g_bgColor);
            lastBgEdit = g_bgColor;
        }
        return (LRESULT)brEdit;
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, g_textColor);
        SetBkMode(hdc, TRANSPARENT);
        return (LRESULT)GetStockObject(HOLLOW_BRUSH);
    }

    case WM_DESTROY:
        // 释放背景图片对象，但不删除注册表项（持久化）
        if (g_pBgImage) { delete g_pBgImage; g_pBgImage = NULL; }
        GdiplusShutdown(g_gdiplusToken);
        DeleteObject(g_hFontNormal); DeleteObject(g_hFontLog);
        PostQuitMessage(0);
        break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        auto pDPIAware = (BOOL(WINAPI*)(void))GetProcAddress(hUser32, "SetProcessDPIAware");
        if (pDPIAware) pDPIAware();
    }
    srand((unsigned)time(NULL));
    g_hInst = hInstance;

    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    WNDCLASSEXW wc = { sizeof(wc) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = NULL;
    wc.lpszClassName = L"CDCDataGenerator";
    RegisterClassExW(&wc);

    g_hMainWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"CDCDataGenerator", L"测试数据生成器 Future ",
        WS_OVERLAPPEDWINDOW | WS_VSCROLL,
        CW_USEDEFAULT, CW_USEDEFAULT, 1400, 1000, NULL, NULL, hInstance, NULL);
    if (!g_hMainWnd) return 1;

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
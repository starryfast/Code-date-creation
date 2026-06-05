#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <shlobj.h>
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

using namespace std;

HWND g_hMainWnd;
HINSTANCE g_hInst;
HFONT g_hFontNormal, g_hFontBold;

#define IDC_EDIT_START       101
#define IDC_EDIT_END         102
#define IDC_COMBO_TYPE       103
#define IDC_EDIT_RANGE1      104
#define IDC_EDIT_RANGE2      105
#define IDC_EDIT_STRLEN      106
#define IDC_EDIT_COUNT       107
#define IDC_CHECK_WRITE      108
#define IDC_BUTTON_GEN       109
#define IDC_BUTTON_COMPILE   110
#define IDC_EDIT_CPP_PATH    111
#define IDC_BUTTON_BROWSE    112
#define IDC_STATIC_STATUS    118
#define IDC_STATIC_AUTHOR    119
#define IDC_EDIT_VAR_TEXT    120
#define IDC_EDIT_GPP_PATH    121
#define IDC_BUTTON_GPP_BROWSE 122
#define IDC_BUTTON_GPP_AUTO   123
#define IDC_EDIT_OUTDIR      124
#define IDC_BUTTON_OUTDIR_BROWSE 125

struct ExtraVar { wstring name, value; };

int g_baseClientWidth = 950;
int g_baseClientHeight = 780;

wstring g_outputDir;

int randomInt(int min, int max) { return min + rand() % (max - min + 1); }
long long randomBetween(long long a, long long b) {
    if (a > b) swap(a, b);
    unsigned long long range = (unsigned long long)(b - a) + 1;
    unsigned long long r = ((unsigned long long)rand() << 30) | ((unsigned long long)rand() << 15) | (unsigned long long)rand();
    return a + (long long)(r % range);
}
wstring randomString(int length, const wstring& charset) {
    wstring result;
    result.reserve(length);
    for (int i = 0; i < length; ++i) result += charset[rand() % charset.size()];
    return result;
}

wstring GetDesktopPath() {
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_DESKTOP, NULL, SHGFP_TYPE_CURRENT, path)))
        return wstring(path);
    return L".";
}

vector<ExtraVar> ParseExtraVars(HWND hEdit) {
    vector<ExtraVar> result;
    int len = GetWindowTextLengthW(hEdit);
    if (len == 0) return result;
    wchar_t* buf = new wchar_t[len + 2];
    GetDlgItemTextW(GetParent(hEdit), IDC_EDIT_VAR_TEXT, buf, len + 2);
    wstringstream ss(buf);
    wstring line;
    while (getline(ss, line)) {
        if (line.empty()) continue;
        size_t eq = line.find(L'=');
        if (eq == wstring::npos) {
            size_t sp = line.find_first_of(L" \t");
            if (sp != wstring::npos) {
                result.push_back({ line.substr(0, sp), line.substr(sp + 1) });
            }
            else {
                result.push_back({ line, L"" });
            }
        }
        else {
            result.push_back({ line.substr(0, eq), line.substr(eq + 1) });
        }
    }
    delete[] buf;
    return result;
}

bool GenerateInputFiles(int start, int end, int dataType, long long range1, long long range2, int strLen, int countPerFile, bool writeCount, const vector<ExtraVar>& vars) {
    if (g_outputDir.empty()) g_outputDir = GetDesktopPath();
    if (dataType == 2) {
        unsigned long long rangeSize = (range2 >= range1) ? (unsigned long long)(range2 - range1 + 1) : 0;
        if ((unsigned long long)countPerFile > rangeSize) return false;
    }
    const wstring lowerChars = L"abcdefghijklmnopqrstuvwxyz";
    const wstring upperChars = L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const wstring mixedChars = L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    for (int i = start; i <= end; ++i) {
        wchar_t fullPath[MAX_PATH];
        swprintf(fullPath, MAX_PATH, L"%s\\%d.in", g_outputDir.c_str(), i);
        FILE* fp = _wfopen(fullPath, L"w");
        if (!fp) return false;
        for (auto& v : vars) {
            if (v.value.empty()) fwprintf(fp, L"%s\n", v.name.c_str());
            else fwprintf(fp, L"%s %s\n", v.name.c_str(), v.value.c_str());
        }
        if (writeCount) fwprintf(fp, L"%d\n", countPerFile);
        if (dataType <= 2) {
            if (dataType == 1) {
                for (int j = 0; j < countPerFile; ++j) fwprintf(fp, L"%lld ", randomBetween(range1, range2));
            }
            else {
                set<long long> used;
                while (used.size() < (size_t)countPerFile) used.insert(randomBetween(range1, range2));
                for (long long val : used) fwprintf(fp, L"%lld ", val);
            }
        }
        else {
            const wstring* charset = (dataType == 3) ? &lowerChars : (dataType == 4) ? &upperChars : &mixedChars;
            for (int j = 0; j < countPerFile; ++j) fwprintf(fp, L"%s ", randomString(strLen, *charset).c_str());
        }
        fwprintf(fp, L"\n");
        fclose(fp);
    }
    return true;
}

bool FileExists(const wstring& path) {
    return _waccess(path.c_str(), 0) == 0;
}

wstring FindGppInPath() {
    wchar_t* pathEnv = _wgetenv(L"PATH");
    if (!pathEnv) return L"";
    wstringstream ss(pathEnv);
    wstring dir;
    while (getline(ss, dir, L';')) {
        if (dir.empty()) continue;
        if (FileExists(dir + L"\\g++.exe")) return dir + L"\\g++.exe";
    }
    return L"";
}

void SearchGppInDir(const wstring& dir, int depth, vector<wstring>& results) {
    if (depth > 6 || dir.empty()) return;
    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        wstring fullPath = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (_wcsicmp(fd.cFileName, L"Windows") == 0 ||
                _wcsicmp(fd.cFileName, L"System32") == 0 ||
                _wcsicmp(fd.cFileName, L"WinSxS") == 0 ||
                _wcsicmp(fd.cFileName, L"$Recycle.Bin") == 0)
                continue;
            SearchGppInDir(fullPath, depth + 1, results);
        }
        else {
            if (_wcsicmp(fd.cFileName, L"g++.exe") == 0) {
                results.push_back(fullPath);
                break;
            }
        }
    } while (FindNextFileW(hFind, &fd));
    FindClose(hFind);
}

wstring AutoFindGpp() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);
    wstring exeDir = exePath;
    size_t pos = exeDir.find_last_of(L"\\/");
    if (pos != wstring::npos) {
        exeDir = exeDir.substr(0, pos);
        if (FileExists(exeDir + L"\\g++.exe")) return exeDir + L"\\g++.exe";
        wstring tmp = exeDir + L"\\..\\mingw64\\bin\\g++.exe";
        if (FileExists(tmp)) return tmp;
        tmp = exeDir + L"\\..\\..\\mingw64\\bin\\g++.exe";
        if (FileExists(tmp)) return tmp;
    }

    wstring pathGpp = FindGppInPath();
    if (!pathGpp.empty()) return pathGpp;

    const wchar_t* commonPaths[] = {
        L"C:\\MinGW\\bin\\g++.exe", L"C:\\mingw64\\bin\\g++.exe",
        L"D:\\MinGW\\bin\\g++.exe", L"D:\\mingw64\\bin\\g++.exe",
        L"C:\\TDM-GCC-64\\bin\\g++.exe",
        L"C:\\Program Files\\mingw-w64\\mingw64\\bin\\g++.exe",
        L"C:\\Program Files\\CodeBlocks\\MinGW\\bin\\g++.exe",
        L"C:\\Dev-Cpp\\MinGW64\\bin\\g++.exe",
        L"C:\\RedPanda-Cpp\\mingw64\\bin\\g++.exe"
    };
    for (auto& p : commonPaths) if (FileExists(p)) return wstring(p);

    DWORD drives = GetLogicalDrives();
    vector<wstring> results;
    for (int i = 0; i < 26; ++i) {
        if (drives & (1 << i)) {
            wstring root = wstring(1, L'A' + i) + L":\\";
            UINT type = GetDriveTypeW(root.c_str());
            if (type == DRIVE_FIXED || type == DRIVE_REMOVABLE) {
                SearchGppInDir(root, 0, results);
                if (!results.empty()) return results[0];
            }
        }
    }
    return L"";
}

bool CompileAndRunAC(const wstring& gppPath, const wstring& cppPath, int start, int end, HWND hWnd) {
    if (!FileExists(cppPath)) {
        wstring msg = L"找不到源文件：" + cppPath;
        MessageBoxW(hWnd, msg.c_str(), L"编译错误", MB_ICONERROR);
        return false;
    }
    wstring ext = cppPath.size() > 4 ? cppPath.substr(cppPath.size() - 4) : L"";
    if (ext != L".cpp" && ext != L".cxx" && ext != L".cc") {
        MessageBoxW(hWnd, L"请选择 C++ 源文件（.cpp/.cxx/.cc）", L"编译错误", MB_ICONERROR);
        return false;
    }

    if (g_outputDir.empty()) g_outputDir = GetDesktopPath();
    wstring tempExe = g_outputDir + L"\\temp_ac.exe";
    wstring cmdLine = L"\"" + gppPath + L"\" \"" + cppPath + L"\" -o \"" + tempExe + L"\"";

    HANDLE hRead, hWrite;
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
        MessageBoxW(hWnd, L"无法创建管道", L"错误", MB_ICONERROR);
        return false;
    }
    STARTUPINFOW si = { sizeof(STARTUPINFOW) };
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite; si.hStdError = hWrite;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    PROCESS_INFORMATION pi = { 0 };
    wchar_t* cmdBuf = new wchar_t[cmdLine.size() + 1];
    wcscpy(cmdBuf, cmdLine.c_str());
    BOOL success = CreateProcessW(NULL, cmdBuf, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi);
    delete[] cmdBuf;
    CloseHandle(hWrite);
    if (!success) {
        CloseHandle(hRead);
        DWORD err = GetLastError();
        wchar_t msg[256];
        swprintf(msg, 256, L"无法启动编译器，请检查 g++ 路径：\n%s\n错误代码：%lu", gppPath.c_str(), err);
        MessageBoxW(hWnd, msg, L"编译错误", MB_ICONERROR);
        return false;
    }
    wstring errOutput;
    char buffer[256]; DWORD bytesRead;
    while (ReadFile(hRead, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
        buffer[bytesRead] = '\0';
        // 编译器输出可能是多字节，简单追加（此处为简化，保持原样）
        errOutput += wstring(buffer, buffer + bytesRead);
    }
    CloseHandle(hRead);
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
    if (exitCode != 0) {
        if (errOutput.empty()) errOutput = L"（无错误输出）";
        MessageBoxW(hWnd, (L"编译失败，g++ 返回 " + to_wstring(exitCode) + L"：\n" + errOutput).c_str(), L"编译错误", MB_ICONERROR);
        return false;
    }

    for (int i = start; i <= end; ++i) {
        wchar_t inPath[MAX_PATH], outPath[MAX_PATH];
        swprintf(inPath, MAX_PATH, L"%s\\%d.in", g_outputDir.c_str(), i);
        swprintf(outPath, MAX_PATH, L"%s\\%d.out", g_outputDir.c_str(), i);
        // 使用 _wsystem 运行
        wstring runCmd = L"cd /d \"" + g_outputDir + L"\" && \"" + tempExe + L"\" < \"" + inPath + L"\" > \"" + outPath + L"\"";
        _wsystem(runCmd.c_str());
    }

    DeleteFileW(tempExe.c_str());
    return true;
}

#define COLOR_BG_WND      RGB(245, 245, 245)
#define COLOR_BG_CTRL     RGB(255, 255, 255)
#define COLOR_TEXT        RGB(30, 30, 30)
#define COLOR_BTN_BG      RGB(0, 120, 212)
#define COLOR_BTN_TEXT    RGB(255, 255, 255)
#define COLOR_AUTHOR_TEXT RGB(80, 80, 80)

void ResizeControls(HWND hWnd, int cx, int cy) {
    int dx = cx - g_baseClientWidth;
    int dy = cy - g_baseClientHeight;

    HWND hVarEdit = GetDlgItem(hWnd, IDC_EDIT_VAR_TEXT);
    if (hVarEdit) SetWindowPos(hVarEdit, NULL, 20, 215, 500 + dx, max(80, 120 + dy), SWP_NOZORDER);

    HWND hAuthor = GetDlgItem(hWnd, IDC_STATIC_AUTHOR);
    if (hAuthor) SetWindowPos(hAuthor, NULL, 20, 350 + dy, 850 + dx, 20, SWP_NOZORDER);

    HWND hOutDirEdit = GetDlgItem(hWnd, IDC_EDIT_OUTDIR);
    HWND hOutDirBtn = GetDlgItem(hWnd, IDC_BUTTON_OUTDIR_BROWSE);
    if (hOutDirEdit && hOutDirBtn) {
        int outY = 375 + dy;
        int newOutW = 420 + dx;
        SetWindowPos(hOutDirEdit, NULL, 110, outY, newOutW, 25, SWP_NOZORDER);
        SetWindowPos(hOutDirBtn, NULL, 110 + newOutW + 10, outY, 70, 25, SWP_NOZORDER);
    }

    HWND hGenBtn = GetDlgItem(hWnd, IDC_BUTTON_GEN);
    if (hGenBtn) SetWindowPos(hGenBtn, NULL, 20, 415 + dy, 120, 35, SWP_NOZORDER);

    HWND hEditPath = GetDlgItem(hWnd, IDC_EDIT_CPP_PATH);
    HWND hBtnBrowse = GetDlgItem(hWnd, IDC_BUTTON_BROWSE);
    if (hEditPath && hBtnBrowse) {
        int pathY = 465 + dy, newPathW = 420 + dx;
        SetWindowPos(hEditPath, NULL, 20, pathY, newPathW, 25, SWP_NOZORDER);
        SetWindowPos(hBtnBrowse, NULL, 20 + newPathW + 10, pathY, 70, 25, SWP_NOZORDER);
    }

    HWND hCompile = GetDlgItem(hWnd, IDC_BUTTON_COMPILE);
    if (hCompile) SetWindowPos(hCompile, NULL, 20, 515 + dy, 150, 35, SWP_NOZORDER);

    HWND hGppEdit = GetDlgItem(hWnd, IDC_EDIT_GPP_PATH);
    HWND hGppBrowse = GetDlgItem(hWnd, IDC_BUTTON_GPP_BROWSE);
    HWND hGppAuto = GetDlgItem(hWnd, IDC_BUTTON_GPP_AUTO);
    if (hGppEdit && hGppBrowse && hGppAuto) {
        int gppY = 565 + dy;
        int newGppW = 340 + dx;
        SetWindowPos(hGppEdit, NULL, 125, gppY, newGppW, 25, SWP_NOZORDER);
        int newBrowseX = 125 + newGppW + 5;
        SetWindowPos(hGppBrowse, NULL, newBrowseX, gppY, 60, 25, SWP_NOZORDER);
        SetWindowPos(hGppAuto, NULL, newBrowseX + 65, gppY, 70, 25, SWP_NOZORDER);
    }

    HWND hStatus = GetDlgItem(hWnd, IDC_STATIC_STATUS);
    if (hStatus) SetWindowPos(hStatus, NULL, 20, 605 + dy, 520 + dx, 25, SWP_NOZORDER);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        g_hFontNormal = CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
        g_hFontBold = CreateFontW(-16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

        CreateWindowW(L"STATIC", L"起始 ID：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 20, 80, 25, hWnd, NULL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 110, 20, 100, 25, hWnd, (HMENU)IDC_EDIT_START, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"结束 ID：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 240, 20, 80, 25, hWnd, NULL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 330, 20, 100, 25, hWnd, (HMENU)IDC_EDIT_END, g_hInst, NULL);

        CreateWindowW(L"STATIC", L"数据类型：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 60, 80, 25, hWnd, NULL, g_hInst, NULL);
        HWND hCombo = CreateWindowW(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_BORDER, 110, 60, 180, 150, hWnd, (HMENU)IDC_COMBO_TYPE, g_hInst, NULL);
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"整数（随机）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"整数（不重复）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"字符串（小写）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"字符串（大写）");
        SendMessageW(hCombo, CB_ADDSTRING, 0, (LPARAM)L"字符串（混合）");
        SendMessage(hCombo, CB_SETCURSEL, 0, 0);

        CreateWindowW(L"STATIC", L"范围/长度：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 100, 80, 25, hWnd, NULL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 110, 100, 80, 25, hWnd, (HMENU)IDC_EDIT_RANGE1, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"~", WS_CHILD | WS_VISIBLE | SS_CENTER, 195, 100, 20, 25, hWnd, NULL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 220, 100, 80, 25, hWnd, (HMENU)IDC_EDIT_RANGE2, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 110, 100, 190, 25, hWnd, (HMENU)IDC_EDIT_STRLEN, g_hInst, NULL);
        ShowWindow(GetDlgItem(hWnd, IDC_EDIT_STRLEN), SW_HIDE);

        CreateWindowW(L"STATIC", L"每文件个数：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 140, 80, 25, hWnd, NULL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_NUMBER, 110, 140, 100, 25, hWnd, (HMENU)IDC_EDIT_COUNT, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"在文件中写入个数", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 240, 140, 150, 25, hWnd, (HMENU)IDC_CHECK_WRITE, g_hInst, NULL);

        CreateWindowW(L"STATIC", L"额外变量（每行 name=value 或 name value）：", WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 185, 400, 25, hWnd, NULL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
            20, 215, 500, 120, hWnd, (HMENU)IDC_EDIT_VAR_TEXT, g_hInst, NULL);

        CreateWindowW(L"STATIC",
            L"作者：starryfast | Bilibili 同名 | Luogu 2070675 | 源码：https://github.com/starryfast/Code-date-creation",
            WS_CHILD | WS_VISIBLE | SS_LEFT, 20, 350, 860, 20, hWnd, (HMENU)IDC_STATIC_AUTHOR, g_hInst, NULL);

        CreateWindowW(L"STATIC", L"生成位置：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 375, 80, 25, hWnd, NULL, g_hInst, NULL);
        g_outputDir = GetDesktopPath();
        CreateWindowW(L"EDIT", g_outputDir.c_str(), WS_CHILD | WS_VISIBLE | WS_BORDER | ES_READONLY, 110, 375, 420, 25, hWnd, (HMENU)IDC_EDIT_OUTDIR, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 540, 375, 70, 25, hWnd, (HMENU)IDC_BUTTON_OUTDIR_BROWSE, g_hInst, NULL);

        CreateWindowW(L"BUTTON", L"生成 .in 文件", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 415, 120, 35, hWnd, (HMENU)IDC_BUTTON_GEN, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 540, 465, 70, 25, hWnd, (HMENU)IDC_BUTTON_BROWSE, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 20, 465, 420, 25, hWnd, (HMENU)IDC_EDIT_CPP_PATH, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"编译并生成 .out", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 20, 515, 150, 35, hWnd, (HMENU)IDC_BUTTON_COMPILE, g_hInst, NULL);

        CreateWindowW(L"STATIC", L"g++ 编译器路径：", WS_CHILD | WS_VISIBLE | SS_RIGHT, 20, 565, 100, 25, hWnd, NULL, g_hInst, NULL);
        CreateWindowW(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER, 125, 565, 340, 25, hWnd, (HMENU)IDC_EDIT_GPP_PATH, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"浏览...", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 470, 565, 60, 25, hWnd, (HMENU)IDC_BUTTON_GPP_BROWSE, g_hInst, NULL);
        CreateWindowW(L"BUTTON", L"自动检测", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 535, 565, 70, 25, hWnd, (HMENU)IDC_BUTTON_GPP_AUTO, g_hInst, NULL);
        CreateWindowW(L"STATIC", L"就绪", WS_CHILD | WS_VISIBLE | SS_SUNKEN, 20, 605, 520, 25, hWnd, (HMENU)IDC_STATIC_STATUS, g_hInst, NULL);

        SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"正在搜索 g++，请稍候...");
        wstring gppPath = AutoFindGpp();
        if (!gppPath.empty()) {
            SetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, gppPath.c_str());
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"已找到 g++，准备就绪。");
        }
        else {
            SetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, L"未找到，请手动指定。");
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"就绪");
        }

        HWND hChild = GetWindow(hWnd, GW_CHILD);
        while (hChild) {
            SendMessage(hChild, WM_SETFONT, (WPARAM)g_hFontNormal, TRUE);
            hChild = GetNextWindow(hChild, GW_HWNDNEXT);
        }
        RECT rcClient; GetClientRect(hWnd, &rcClient);
        g_baseClientWidth = rcClient.right;
        g_baseClientHeight = rcClient.bottom;
        break;
    }
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == IDC_COMBO_TYPE && HIWORD(wParam) == CBN_SELCHANGE) {
            int sel = SendMessage(GetDlgItem(hWnd, IDC_COMBO_TYPE), CB_GETCURSEL, 0, 0);
            BOOL showRange = (sel == 0 || sel == 1);
            BOOL showStrLen = (sel >= 2);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_RANGE1), showRange ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_RANGE2), showRange ? SW_SHOW : SW_HIDE);
            ShowWindow(GetDlgItem(hWnd, IDC_EDIT_STRLEN), showStrLen ? SW_SHOW : SW_HIDE);
        }
        else if (id == IDC_BUTTON_OUTDIR_BROWSE) {
            BROWSEINFOW bi = { 0 };
            bi.hwndOwner = hWnd;
            bi.lpszTitle = L"选择生成 .in / .out 文件的文件夹";
            bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl) {
                wchar_t path[MAX_PATH];
                if (SHGetPathFromIDListW(pidl, path)) {
                    g_outputDir = path;
                    SetDlgItemTextW(hWnd, IDC_EDIT_OUTDIR, g_outputDir.c_str());
                }
                ILFree(pidl);
            }
        }
        else if (id == IDC_BUTTON_GEN) {
            int start = GetDlgItemInt(hWnd, IDC_EDIT_START, NULL, FALSE);
            int end = GetDlgItemInt(hWnd, IDC_EDIT_END, NULL, FALSE);
            int modeIdx = SendMessage(GetDlgItem(hWnd, IDC_COMBO_TYPE), CB_GETCURSEL, 0, 0);
            int dataType = modeIdx + 1;
            long long range1 = 0, range2 = 0; int strLen = 0;
            if (dataType <= 2) {
                range1 = GetDlgItemInt(hWnd, IDC_EDIT_RANGE1, NULL, TRUE);
                range2 = GetDlgItemInt(hWnd, IDC_EDIT_RANGE2, NULL, TRUE);
            }
            else strLen = GetDlgItemInt(hWnd, IDC_EDIT_STRLEN, NULL, FALSE);
            int countPerFile = GetDlgItemInt(hWnd, IDC_EDIT_COUNT, NULL, FALSE);
            bool writeCount = (SendMessage(GetDlgItem(hWnd, IDC_CHECK_WRITE), BM_GETCHECK, 0, 0) == BST_CHECKED);

            if (start > end) { MessageBoxW(hWnd, L"起始 ID 不能大于结束 ID。", L"错误", MB_ICONERROR); break; }
            if (countPerFile <= 0) { MessageBoxW(hWnd, L"每文件个数必须大于 0。", L"错误", MB_ICONERROR); break; }
            if (dataType == 2) {
                unsigned long long rangeSize = (range2 >= range1) ? (unsigned long long)(range2 - range1 + 1) : 0;
                if ((unsigned long long)countPerFile > rangeSize) {
                    MessageBoxW(hWnd, L"范围内没有足够的不重复数字。", L"错误", MB_ICONERROR);
                    break;
                }
            }
            if (dataType >= 3 && strLen <= 0) {
                MessageBoxW(hWnd, L"字符串长度必须大于 0。", L"错误", MB_ICONERROR);
                break;
            }

            vector<ExtraVar> vars = ParseExtraVars(GetDlgItem(hWnd, IDC_EDIT_VAR_TEXT));
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"正在生成 .in 文件...");
            if (GenerateInputFiles(start, end, dataType, range1, range2, strLen, countPerFile, writeCount, vars)) {
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"所有 .in 文件已生成到指定位置！");
                MessageBoxW(hWnd, L"生成完毕！", L"成功", MB_OK);
            }
            else {
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"生成失败，请检查权限或路径。");
            }
        }
        else if (id == IDC_BUTTON_BROWSE) {
            OPENFILENAMEW ofn = { 0 }; wchar_t file[MAX_PATH] = { 0 };
            ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hWnd;
            ofn.lpstrFilter = L"C++ 源文件\0*.cpp\0所有文件\0*.*\0";
            ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            if (GetOpenFileNameW(&ofn)) SetDlgItemTextW(hWnd, IDC_EDIT_CPP_PATH, file);
        }
        else if (id == IDC_BUTTON_GPP_BROWSE) {
            OPENFILENAMEW ofn = { 0 }; wchar_t file[MAX_PATH] = { 0 };
            ofn.lStructSize = sizeof(ofn); ofn.hwndOwner = hWnd;
            ofn.lpstrFilter = L"可执行文件\0*.exe\0所有文件\0*.*\0";
            ofn.lpstrFile = file; ofn.nMaxFile = MAX_PATH;
            ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
            if (GetOpenFileNameW(&ofn)) SetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, file);
        }
        else if (id == IDC_BUTTON_GPP_AUTO) {
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"正在全盘搜索 g++，请稍候...");
            wstring gppPath = AutoFindGpp();
            if (!gppPath.empty()) {
                SetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, gppPath.c_str());
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"已自动找到 g++。");
            }
            else {
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"未找到 g++，请手动选择。");
                MessageBoxW(hWnd, L"找不到 g++，请点击“浏览...”选择您的 g++.exe。", L"提示", MB_ICONINFORMATION);
            }
        }
        else if (id == IDC_BUTTON_COMPILE) {
            wchar_t cppPath[MAX_PATH]; GetDlgItemTextW(hWnd, IDC_EDIT_CPP_PATH, cppPath, MAX_PATH);
            if (!cppPath[0]) { MessageBoxW(hWnd, L"请先选择一个 C++ 源文件。", L"提示", MB_ICONINFORMATION); break; }
            wchar_t gppPath[MAX_PATH]; GetDlgItemTextW(hWnd, IDC_EDIT_GPP_PATH, gppPath, MAX_PATH);
            if (!gppPath[0] || !FileExists(gppPath)) {
                MessageBoxW(hWnd, L"请设置有效的 g++ 编译器路径。", L"错误", MB_ICONERROR); break;
            }
            int start = GetDlgItemInt(hWnd, IDC_EDIT_START, NULL, FALSE);
            int end = GetDlgItemInt(hWnd, IDC_EDIT_END, NULL, FALSE);
            SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"正在编译并运行，请稍候...");
            if (CompileAndRunAC(gppPath, cppPath, start, end, hWnd)) {
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"所有 .out 文件已生成到指定位置！");
                MessageBoxW(hWnd, L"评测完成！", L"成功", MB_OK);
            }
            else {
                SetDlgItemTextW(hWnd, IDC_STATIC_STATUS, L"编译或运行失败。");
            }
        }
        break;
    }
    case WM_SIZE: {
        ResizeControls(hWnd, LOWORD(lParam), HIWORD(lParam));
        return 0;
    }
    case WM_GETMINMAXINFO: {
        MINMAXINFO* mmi = (MINMAXINFO*)lParam;
        mmi->ptMinTrackSize.x = 950;
        mmi->ptMinTrackSize.y = 700;
        return 0;
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam; HWND hStatic = (HWND)lParam;
        if (GetDlgCtrlID(hStatic) == IDC_STATIC_AUTHOR) {
            SetTextColor(hdc, COLOR_AUTHOR_TEXT); SetBkColor(hdc, COLOR_BG_WND);
            static HBRUSH hBrushAuthor = CreateSolidBrush(COLOR_BG_WND);
            return (LRESULT)hBrushAuthor;
        }
        SetTextColor(hdc, COLOR_TEXT); SetBkColor(hdc, COLOR_BG_CTRL);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    case WM_CTLCOLOREDIT:
    case WM_CTLCOLORLISTBOX: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_TEXT); SetBkColor(hdc, COLOR_BG_CTRL);
        return (LRESULT)GetStockObject(WHITE_BRUSH);
    }
    case WM_CTLCOLORBTN: {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, COLOR_BTN_TEXT); SetBkColor(hdc, COLOR_BTN_BG);
        static HBRUSH hBrushBtn = CreateSolidBrush(COLOR_BTN_BG);
        return (LRESULT)hBrushBtn;
    }
    case WM_ERASEBKGND: {
        RECT rc; GetClientRect(hWnd, &rc);
        HBRUSH hBrush = CreateSolidBrush(COLOR_BG_WND);
        FillRect((HDC)wParam, &rc, hBrush);
        DeleteObject(hBrush);
        return TRUE;
    }
    case WM_DESTROY:
        DeleteObject(g_hFontNormal); DeleteObject(g_hFontBold);
        PostQuitMessage(0); break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow) {
    SetProcessDPIAware();
    srand((unsigned)time(NULL));
    g_hInst = hInstance;
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"CDCDataGenerator";
    RegisterClassExW(&wc);
    g_hMainWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"CDCDataGenerator", L"测试数据生成器 4.5",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1000, 720, NULL, NULL, hInstance, NULL);
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
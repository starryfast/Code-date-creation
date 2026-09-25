// =============================================================
//  CDC DataGen v7.1.0 — C++ 后端（本地 HTTP 服务器）
//  架构：后端 C++ / 前端 HTML
//  本程序监听 127.0.0.1:18888，启动后自动打开浏览器访问。
//  前端通过 HTTP JSON API 调用后端完成真实文件操作：
//    数据生成 / g++ 编译 / ZIP 打包 / 批量重命名 / 大文件 / 系统工具
//  零第三方依赖，winsock 手写，g++ 直接编译。
//  编译（必须链接图标资源，否则 exe 无文件图标）：
//    1) windres icon.rc -O coff -o icon_res.o
//    2) g++ -O2 -std=c++17 cdc_backend.cpp icon_res.o -o cdc_backend.exe -lws2_32 -lole32 -lshell32 -ladvapi32 -lwinhttp
//  （VS：rc icon.rc 生成 icon.res，再
//    cl /EHsc /std:c++17 cdc_backend.cpp icon.res ws2_32.lib ole32.lib shell32.lib advapi32.lib winhttp.lib /Fe:cdc_backend.exe）
//  图标资源见 icon.rc / icon.ico（IDI_APP）。也可直接双击 build.bat 一步出带图标 exe。
// =============================================================
#define _CRT_SECURE_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <process.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <ctime>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <cwchar>
#include <cwctype>
#include <winhttp.h>

// 跨平台文件打开垫片：MSVC 用安全函数，MinGW 用标准 fopen/_wfopen。
// 让同一份源码既能用 VS 编译，也能用旧版 MinGW（无 fopen_s）编译。
static inline FILE* OpenFileS(const char* name, const char* mode) {
    FILE* f = nullptr;
#ifdef _MSC_VER
    if (fopen_s(&f, name, mode) != 0) f = nullptr;
#else
    f = fopen(name, mode);
#endif
    return f;
}
static inline FILE* OpenFileWS(const wchar_t* name, const wchar_t* mode) {
    FILE* f = nullptr;
#ifdef _MSC_VER
    if (_wfopen_s(&f, name, mode) != 0) f = nullptr;
#else
    f = _wfopen(name, mode);
#endif
    return f;
}

// 部分 MinGW 头文件未导出这些常量，手动补定义（MSVC 下已有，#ifndef 保护）
#ifndef SECURITY_FLAG_IGNORE_UNKNOWN_CA
#define SECURITY_FLAG_IGNORE_UNKNOWN_CA        0x00000100
#define SECURITY_FLAG_IGNORE_CERT_DATE_INVALID 0x00002000
#define SECURITY_FLAG_IGNORE_CERT_CN_INVALID   0x00001000
#define SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE  0x00000200
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "winhttp.lib")

using namespace std;

#define PORT 18889
#define APP_VER "7.1.0"
static string g_exeDir;   // exe 所在目录（UTF-8）
static string g_cloudServer; // 云端服务器地址（XOR 编码存放，运行时解码，不在 exe/配置里明文暴露）
static string g_cloudOkPath;  // 上次命中的云端接口完整路径，命中后不再逐个探测候选（避免日志像路径扫描）

// 云端地址以 XOR 编码存放，避免明文出现在二进制/配置中。
// 解码函数运行时还原真实地址；如需临时覆盖，可在 exe 同目录放 cdc_srv.dat（server=...）。
// 旧名 cdc_cloud.cfg 仍兼容读取，但不再推荐：名字里有 cloud 太显眼。
//
// 【开源版】内置云端地址已移除 —— 官方服务器凭据不随开源代码分发。
//   自建服务端后，任选一种方式接入：
//     ① 在 exe 同目录放 cdc_srv.dat，写一行 server=https://你的域名/你的路径
//     ② 或把地址按下面的 XOR 键循环异或，填进 kCloudEnc，并把 kCloudEncLen 改成地址字节数
//   两种都没提供时，云端相关接口返回 cloud_not_configured，本地功能完全不受影响。
//   配置模板见仓库根目录 cdc_srv.dat.example。
static const char kCloudXorKey[] = "cdc_7_1_xor";
static const unsigned char kCloudEnc[] = { 0x00 };  // 开源版占位，长度以 kCloudEncLen 为准
static const size_t kCloudEncLen = 0;               // 0 = 不使用内置地址
static string DecodeCloud() {
    string s;
    size_t n = kCloudEncLen;
    size_t cap = sizeof(kCloudEnc) / sizeof(kCloudEnc[0]);
    size_t kl = sizeof(kCloudXorKey) - 1;
    for (size_t i = 0; i < n && i < cap; i++)
        s += (char)(kCloudEnc[i] ^ (unsigned char)kCloudXorKey[i % kl]);
    return s;
}

// 接口令牌（云端要求的 X-CDC-Token 请求头）——官方客户端凭证。
// 同样 XOR 混淆存放、独用一把钥匙（不复用云端地址的钥匙，一次泄露不牵连两样），
// 避免 `strings cdc_backend.exe` 直接把令牌 grep 出来。
//
// 【开源版】内置令牌已移除（置空后不再发送该请求头，契合"自建服务端默认不校验令牌"）。
//   若你的服务端启用了令牌校验，请把令牌按下面的 XOR 键循环异或后填进 kTokenEnc，
//   并把 kTokenEncLen 改成令牌字节数。注意：两端令牌必须一致，改一端漏一端会全量 401。
static const char kTokenXorKey[] = "cdc_7_1_tok";
static const unsigned char kTokenEnc[] = { 0x00 };  // 开源版占位
static const size_t kTokenEncLen = 0;               // 0 = 不发送令牌
static string DecodeToken() {
    string s;
    size_t n = kTokenEncLen;
    size_t cap = sizeof(kTokenEnc) / sizeof(kTokenEnc[0]);
    size_t kl = sizeof(kTokenXorKey) - 1;
    for (size_t i = 0; i < n && i < cap; i++)
        s += (char)(kTokenEnc[i] ^ (unsigned char)kTokenXorKey[i % kl]);
    return s;
}
// 首次调用时解码并缓存；令牌变了只要改 kTokenEnc 重编即可
static const string& CloudToken() {
    static string t = DecodeToken();
    return t;
}

// 十六进制串 → XOR 解码（与内置 kCloudEnc 同一把钥匙）。非法输入返回空串。
static string XorDecodeHex(const string& hex) {
    auto hv = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    string out;
    size_t kl = sizeof(kCloudXorKey) - 1;
    for (size_t i = 0; i + 1 < hex.size(); i += 2) {
        int hi = hv(hex[i]), lo = hv(hex[i + 1]);
        if (hi < 0 || lo < 0) return "";
        out += (char)((hi * 16 + lo) ^ (unsigned char)kCloudXorKey[(i / 2) % kl]);
    }
    return out;
}

// 从单个配置文件里取 server= ；取到返回 true
static bool LoadCloudCfgFile(const string& path, string& out) {
    FILE* f = OpenFileS(path.c_str(), "r");
    if (!f) return false;
    char line[1024];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        string s = line;
        size_t h = s.find('#');
        if (h != string::npos) s = s.substr(0, h);
        size_t eq = s.find('=');
        if (eq == string::npos) continue;
        // 键名必须正好是 server / s（旧实现用 find("server")，任何含 server 的行都会误命中）
        string k = s.substr(0, eq);
        size_t ka = k.find_first_not_of(" \t\r\n");
        size_t kb = k.find_last_not_of(" \t\r\n");
        if (ka == string::npos) continue;
        k = k.substr(ka, kb - ka + 1);
        if (k != "server" && k != "s") continue;
        string v = s.substr(eq + 1);
        size_t a = v.find_first_not_of(" \t\r\n\"'");
        size_t b = v.find_last_not_of(" \t\r\n\"'");
        if (a == string::npos) continue;
        v = v.substr(a, b - a + 1);
        // 支持混淆写法 x:0A1B2C…（十六进制 XOR）：文件被人翻到也看不出云端地址
        if (v.size() > 2 && (v[0] == 'x' || v[0] == 'X') && v[1] == ':') {
            string d = XorDecodeHex(v.substr(2));
            if (d.empty()) continue;
            v = d;
        }
        while (!v.empty() && (v.back() == '/' || v.back() == '\\')) v.pop_back();
        if (!v.empty()) { out = v; found = true; break; }
    }
    fclose(f);
    return found;
}

// 读取云端服务器地址：
//   1) 默认用内置 XOR 编码地址（始终可用，无需任何配置文件，且不随包暴露明文）
//   2) 若 exe 同目录存在 cdc_srv.dat 的 server=，则以其覆盖（仅开发/调试用，不随发布包）
//      旧文件名 cdc_cloud.cfg 仍静默兼容，方便老装机平滑过渡
static void ReadCloudCfg() {
    g_cloudServer = DecodeCloud();
    string v;
    if (LoadCloudCfgFile(g_exeDir + "\\cdc_srv.dat", v) ||
        LoadCloudCfgFile(g_exeDir + "\\cdc_cloud.cfg", v))
        g_cloudServer = v;
}

// =============================================================
//  UTF-8 / 宽字符 / 本地编码 转换
// =============================================================
static string WstrToUtf8(const wstring& ws) {
    if (ws.empty()) return "";
    int n = WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), nullptr, 0, nullptr, nullptr);
    string s(n, 0);
    WideCharToMultiByte(CP_UTF8, 0, ws.c_str(), (int)ws.size(), &s[0], n, nullptr, nullptr);
    return s;
}
static wstring Utf8ToWstr(const string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    wstring ws(n, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &ws[0], n);
    return ws;
}
static wstring LocalToWstr(const string& s) {
    if (s.empty()) return L"";
    int n = MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), nullptr, 0);
    wstring ws(n, 0);
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), (int)s.size(), &ws[0], n);
    return ws;
}
static string UrlDecode(const string& in) {
    string out;
    out.reserve(in.size());
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == '%' && i + 2 < in.size()) {
            int v = 0;
            if (sscanf(in.c_str() + i + 1, "%2x", &v) == 1) { out.push_back((char)v); i += 2; }
            else out.push_back(in[i]);
        } else if (in[i] == '+') out.push_back(' ');
        else out.push_back(in[i]);
    }
    return out;
}
static string HtmlEscape(const string& s) {
    string out;
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out.push_back(c);
        }
    }
    return out;
}

// JSON 字符串转义（必须转义反斜杠、引号与所有控制字符）。
// 关键点：用 unsigned char 遍历，否则中文 UTF-8 多字节（值 0x80-0xFF）会被
// 当成负数、被误判为控制字符而损坏，导致路径里的反斜杠/中文路径彻底出错。
static string JsonEsc(const string& s) {
    string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += '\\'; out += '"'; break;
            case '\\': out += '\\'; out += '\\'; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            default:
                if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); out += b; }
                else out += (char)c;
        }
    }
    return out;
}

// =============================================================
//  简易 JSON 取值（同前端 JsonValue 语义）
// =============================================================
static string JsonValue(const string& json, const string& key) {
    string q = "\"" + key + "\":\"";
    size_t p = json.find(q);
    if (p != string::npos) {
        p += q.size();
        string out;
        while (p < json.size() && json[p] != '"') {
            if (json[p] == '\\' && p + 1 < json.size()) { out += json[p + 1]; p += 2; }
            else out += json[p++];
        }
        return out;
    }
    q = "\"" + key + "\":";
    p = json.find(q);
    if (p != string::npos) {
        p += q.size();
        size_t e = json.find_first_of(",}\r\n", p);
        if (e == string::npos) e = json.size();
        string v = json.substr(p, e - p);
        while (!v.empty() && (v.front() == ' ' || v.front() == '"')) v.erase(v.begin());
        while (!v.empty() && (v.back() == ' ' || v.back() == '"')) v.pop_back();
        return v;
    }
    return "";
}

// 提取 JSON 字符串字段并【正确反转义】（\" \\ \/ \n \r \t \b \f \uXXXX）。
// 为什么必须单独有它：上面的 JsonValue 是简易取值器，遇到逗号/换行就截断，
// 只能用于路径/短参数；像「生成器代码」这种多行、含逗号的字段会直接被切碎
// （表现为生成器编译出"没有 main"的源文件 → 链接报 undefined reference to WinMain）。
static string JsonStr(const string& json, const string& key) {
    string pat = "\"" + key + "\"";
    size_t p = json.find(pat);
    if (p == string::npos) return "";
    p = json.find(':', p + pat.size());
    if (p == string::npos) return "";
    ++p;
    while (p < json.size() && (json[p] == ' ' || json[p] == '\t' || json[p] == '\r' || json[p] == '\n')) ++p;
    if (p >= json.size() || json[p] != '"') return JsonValue(json, key);   // 数字/布尔：退回简易取值
    ++p;
    string out;
    while (p < json.size()) {
        char c = json[p];
        if (c == '\\') {
            if (p + 1 >= json.size()) break;
            char e = json[p + 1];
            p += 2;
            switch (e) {
                case 'n': out += '\n'; break;
                case 'r': out += '\r'; break;
                case 't': out += '\t'; break;
                case 'b': out += '\b'; break;
                case 'f': out += '\f'; break;
                case '"': out += '"'; break;
                case '\\': out += '\\'; break;
                case '/': out += '/'; break;
                case 'u': {
                    if (p + 4 > json.size()) break;
                    unsigned cp = 0;
                    for (int i = 0; i < 4; ++i) {
                        char h = json[p + i];
                        cp <<= 4;
                        if (h >= '0' && h <= '9') cp |= (unsigned)(h - '0');
                        else if (h >= 'a' && h <= 'f') cp |= (unsigned)(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') cp |= (unsigned)(h - 'A' + 10);
                    }
                    p += 4;
                    if (cp < 0x80) {                       // UTF-8 编码（BMP 足够）
                        out += (char)cp;
                    } else if (cp < 0x800) {
                        out += (char)(0xC0 | (cp >> 6));
                        out += (char)(0x80 | (cp & 0x3F));
                    } else {
                        out += (char)(0xE0 | (cp >> 12));
                        out += (char)(0x80 | ((cp >> 6) & 0x3F));
                        out += (char)(0x80 | (cp & 0x3F));
                    }
                    break;
                }
                default: out += e; break;
            }
        } else if (c == '"') {
            break;                                          // 字符串结束
        } else {
            out += c;
            ++p;
        }
    }
    return out;
}

// =============================================================
//  文件工具
// =============================================================
static bool FileExists(const wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
}
static bool DirExists(const wstring& path) {
    DWORD attr = GetFileAttributesW(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY);
}
static void EnsureDir(const wstring& dir) {
    if (!dir.empty()) CreateDirectoryW(dir.c_str(), nullptr);
}
static wstring GetDesktopPath() {
    wchar_t p[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(nullptr, CSIDL_DESKTOP, nullptr, SHGFP_TYPE_CURRENT, p))) return p;
    return L"C:\\";
}
// 把前端约定的别名目录解析为真实路径（网页模式无法给出绝对路径）
static wstring ResolveOutDir(const wstring& raw) {
    if (raw.empty()) return GetDesktopPath();
    if (raw.size() >= 3 && (raw.substr(0, 3) == L"桌面/" || raw.substr(0, 3) == L"桌面\\")) {
        return GetDesktopPath() + raw.substr(2);
    }
    if (raw.size() >= 2 && raw.substr(0, 2) == L"桌面") {
        return GetDesktopPath() + raw.substr(2);
    }
    return raw;
}
static wstring GetNowString() {
    wchar_t buf[64];
    SYSTEMTIME st; GetLocalTime(&st);
    swprintf(buf, 64, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return buf;
}
static string FileSizeStr(long long sz) {
    char buf[64];
    if (sz < 1024) snprintf(buf, sizeof(buf), "%lld B", sz);
    else if (sz < 1024 * 1024) snprintf(buf, sizeof(buf), "%.1f KB", sz / 1024.0);
    else snprintf(buf, sizeof(buf), "%.2f MB", sz / 1048576.0);
    return buf;
}

// =============================================================
//  数据生成核心
// =============================================================
static wstring DefaultCharset(int type) {
    switch (type) {
        case 2: return L"abcdefghijklmnopqrstuvwxyz";
        case 3: return L"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        case 4: return L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    }
    return L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
}
static long long randomLL(long long lo, long long hi) {
    if (lo > hi) swap(lo, hi);
    long long range = hi - lo + 1;
    if (range <= 0) return lo;
    long long r = 0;
    for (int i = 0; i < 4; ++i) r = (r << 16) | (rand() & 0xFFFF);
    return lo + (r % range);
}
static wstring GenValue(int type, long long r1, long long r2, int slen, const wstring& charset) {
    switch (type) {
        case 0: case 1: return to_wstring(randomLL(r1, r2));
        case 2: case 3: case 4: {
            wstring s; s.reserve(slen);
            int n = (int)charset.size();
            for (int i = 0; i < slen && n > 0; ++i) s += charset[rand() % n];
            return s;
        }
        case 5: {
            double v = (double)randomLL(r1, r2) / 100.0;
            wchar_t buf[64]; swprintf(buf, 64, L"%.2f", v); return buf;
        }
        case 6: {
            SYSTEMTIME st; GetLocalTime(&st);
            st.wYear = (WORD)randomLL(2020, 2025);
            st.wMonth = (WORD)randomLL(1, 12);
            st.wDay = (WORD)randomLL(1, 28);
            st.wHour = (WORD)randomLL(0, 23);
            st.wMinute = (WORD)randomLL(0, 59);
            st.wSecond = (WORD)randomLL(0, 59);
            wchar_t buf[64]; swprintf(buf, 64, L"%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
            return buf;
        }
    }
    return L"0";
}

// 写 UTF-8 文本文件（无 BOM，.in 测试数据用）
static bool WriteUtf8File(const wstring& path, const string& content) {
    FILE* f = OpenFileWS(path.c_str(), L"wb");
    if (!f) return false;
    fwrite(content.data(), 1, content.size(), f);
    fclose(f);
    return true;
}
static string WstrToUtf8Bytes(const wstring& ws) { return WstrToUtf8(ws); }

// 生成接口
static string ApiGenerate(const string& body) {
    int start = max(1, atoi(JsonValue(body, "start").c_str()));
    int endd  = max(start, atoi(JsonValue(body, "end").c_str()));
    int count = max(1, atoi(JsonValue(body, "count").c_str()));
    int type  = atoi(JsonValue(body, "type").c_str());
    long long r1 = _atoi64(JsonValue(body, "r1").c_str());
    long long r2 = _atoi64(JsonValue(body, "r2").c_str());
    int slen = max(1, atoi(JsonValue(body, "strLen").c_str()));
    bool perFile = JsonValue(body, "perFile") == "true";
    bool tsDir   = JsonValue(body, "tsDir") == "true";

    wstring chars = Utf8ToWstr(JsonValue(body, "customCharset"));
    if (chars.empty()) chars = DefaultCharset(type);

    wstring outDir = ResolveOutDir(Utf8ToWstr(JsonValue(body, "outDir")));
    if (tsDir) outDir += L"\\" + GetNowString();
    EnsureDir(outDir);

    // 附加变量（每文件首部，逐行）
    wstring extra = Utf8ToWstr(JsonValue(body, "extra"));
    vector<wstring> extraLines;
    if (!extra.empty()) {
        size_t p = 0;
        while (p < extra.size()) {
            size_t q = extra.find(L'\n', p);
            wstring line = (q == wstring::npos) ? extra.substr(p) : extra.substr(p, q - p);
            while (!line.empty() && (line.back() == L'\r')) line.pop_back();
            if (!line.empty()) extraLines.push_back(line);
            if (q == wstring::npos) break;
            p = q + 1;
        }
    }

    srand((unsigned)time(nullptr));
    int created = 0;
    string filesJson;
    for (int id = start; id <= endd; ++id) {
        wstring path = outDir + L"\\" + to_wstring(id) + L".in";
        string content;
        if (perFile) content += to_string(count) + "\n";
        for (int i = 0; i < count; ++i) content += WstrToUtf8Bytes(GenValue(type, r1, r2, slen, chars)) + "\n";
        for (auto& ln : extraLines) content += WstrToUtf8Bytes(ln) + "\n";
        if (WriteUtf8File(path, content)) {
            created++;
            if (filesJson.size() > 2) filesJson += ",";
            filesJson += "{\"name\":\"" + to_string(id) + ".in\",\"size\":\"" + FileSizeStr((long long)content.size()) + "\"}";
        }
    }
    string res = "{\"ok\":true,\"created\":" + to_string(created) + ",\"dir\":\"" + JsonEsc(WstrToUtf8(outDir)) + "\",\"files\":[" + filesJson + "]}";
    return res;
}

// 大文件生成
static string ApiBigFile(const string& body) {
    int type = atoi(JsonValue(body, "type").c_str());
    long long r1 = _atoi64(JsonValue(body, "r1").c_str());
    long long r2 = _atoi64(JsonValue(body, "r2").c_str());
    int slen = max(1, atoi(JsonValue(body, "strLen").c_str()));
    wstring chars = Utf8ToWstr(JsonValue(body, "customCharset"));
    if (chars.empty()) chars = DefaultCharset(type);

    wstring outDir = ResolveOutDir(Utf8ToWstr(JsonValue(body, "outDir")));
    EnsureDir(outDir);
    wstring path = outDir + L"\\big_" + GetNowString() + L".in";

    FILE* f = OpenFileWS(path.c_str(), L"wb");
    if (!f) return "{\"ok\":false,\"message\":\"无法创建大文件\"}";
    srand((unsigned)time(nullptr));
    for (int i = 0; i < 10000; ++i) {
        string line = WstrToUtf8Bytes(GenValue(type, r1, r2, slen, chars)) + "\n";
        fwrite(line.data(), 1, line.size(), f);
    }
    fclose(f);
    return "{\"ok\":true,\"path\":\"" + JsonEsc(WstrToUtf8(path)) + "\"}";
}

// 批量重命名
static string ApiRename(const string& body) {
    wstring dir = Utf8ToWstr(JsonValue(body, "dir"));
    if (dir.empty()) dir = GetDesktopPath();
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*.in").c_str(), &fd);
    int n = 0;
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            wstring oldp = dir + L"\\" + fd.cFileName;
            wstring newp = oldp + L".txt";
            if (MoveFileW(oldp.c_str(), newp.c_str())) ++n;
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    return "{\"ok\":true,\"renamed\":" + to_string(n) + "}";
}

// 通用写文件（分组生成/手动 .in/导出等）：body 为
// {"outDir":"...","files":[{"name":"a.in","content":"..."}, ...]}
static string ApiWriteFiles(const string& body) {
    wstring outDir = ResolveOutDir(Utf8ToWstr(JsonValue(body, "outDir")));
    EnsureDir(outDir);

    // 解析 files 数组（简易：按 {"name":..,"content":..} 成对提取）
    int created = 0;
    string filesJson;
    size_t pos = 0;
    string b = body;
    // 逐个查找 {"name":
    while ((pos = b.find("\"name\":", pos)) != string::npos) {
        pos += 7;
        while (pos < b.size() && (b[pos] == ' ' || b[pos] == '"' || b[pos] == ':')) pos++;
        string name;
        while (pos < b.size() && b[pos] != '"') {
            if (b[pos] == '\\' && pos + 1 < b.size()) { name += b[pos + 1]; pos += 2; }
            else name += b[pos++];
        }
        pos = b.find("\"content\":", pos);
        if (pos == string::npos) break;
        pos += 10;
        while (pos < b.size() && (b[pos] == ' ' || b[pos] == '"' || b[pos] == ':')) pos++;
        string content;
        while (pos < b.size() && b[pos] != '"') {
            if (b[pos] == '\\' && pos + 1 < b.size()) {
                char nxt = b[pos + 1];
                if (nxt == 'n') content += '\n';
                else if (nxt == 'r') content += '\r';
                else if (nxt == 't') content += '\t';
                else content += nxt;
                pos += 2;
            } else content += b[pos++];
        }
        // 防路径穿越：只用文件名
        wstring wname = Utf8ToWstr(name);
        size_t slash = wname.find_last_of(L"\\/");
        if (slash != wstring::npos) wname = wname.substr(slash + 1);
        wstring path = outDir + L"\\" + wname;
        if (WriteUtf8File(path, content)) {
            created++;
            if (filesJson.size() > 2) filesJson += ",";
            filesJson += "{\"name\":\"" + JsonEsc(name) + "\",\"size\":\"" + FileSizeStr((long long)content.size()) + "\"}";
        }
    }
    return "{\"ok\":true,\"created\":" + to_string(created) + ",\"dir\":\"" + JsonEsc(WstrToUtf8(outDir)) + "\",\"files\":[" + filesJson + "]}";
}

// =============================================================
//  编译运行
// 前向声明：mingw64\bin 目录探测（存在则返回完整路径，否则返回空）
static wstring MingwBinDir();
static void DebugLog(const string& msg);

// =============================================================
static bool RunCommand(const wstring& cmd, const wstring& dir, string& out, DWORD* exitCode = nullptr) {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hRead = nullptr, hWrite = nullptr;
    if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return false;
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    PROCESS_INFORMATION pi = {};
    wstring full = cmd;
    // 注意：不要传自定义环境块！实测（Sep 6 21:14 起）自构环境块会让 CreateProcessW
    // 报 err=87（ERROR_INVALID_PARAMETER），所有编译直接挂掉。
    // g++ 驱动会自己定位 cc1plus/as/ld；生成的 a.exe 是 -static 全静态，无需任何 mingw DLL。
    BOOL ok = CreateProcessW(nullptr, &full[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                             dir.empty() ? nullptr : dir.c_str(), &si, &pi);
    CloseHandle(hWrite);
    if (!ok) {
        DebugLog("  RunCommand CreateProcessW failed err=" + to_string((long long)GetLastError()) + " cmd=" + WstrToUtf8(cmd).substr(0, 260));
        CloseHandle(hRead); return false;
    }
    char buf[4096]; DWORD n; string s;
    while (ReadFile(hRead, buf, sizeof(buf) - 1, &n, nullptr) && n) { buf[n] = 0; s.append(buf, n); }
    CloseHandle(hRead);
    DWORD code = 0;
    WaitForSingleObject(pi.hProcess, INFINITE);
    GetExitCodeProcess(pi.hProcess, &code);
    if (exitCode) *exitCode = code;
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    out = WstrToUtf8(Utf8ToWstr(s)); // 规范化为 UTF-8
    return true;
}

// 探测 exe 同目录下的 mingw64\bin（打包场景：cdc_backend.exe 与 mingw64 同目录）。
// 返回完整路径；不存在返回空串。
// 用途：作为 g++ 子进程的 lpCurrentDirectory —— Windows DLL 搜索顺序中"当前目录"
// 排在 PATH 之前，效果等同把 mingw64\bin 塞进 PATH，但不碰任何环境参数。
static wstring MingwBinDir() {
    if (g_exeDir.empty()) return {};
    wstring mingwBin = Utf8ToWstr(g_exeDir) + L"\\mingw64\\bin";
    DWORD attr = GetFileAttributesW(mingwBin.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) {
        return {};   // 没带 mingw64
    }
    return mingwBin;
}

// 用真实文件句柄重定向 stdin/stdout 运行程序（< in > out 在 CreateProcess 下无效）
static bool RunProcessFile(const wstring& exePath, const wstring& inFile, const wstring& outFile, const wstring& dir) {
    // 句柄必须可继承（bInheritHandle=TRUE），否则子进程拿不到 stdin/stdout
    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    HANDLE hIn = CreateFileW(inFile.c_str(), GENERIC_READ, FILE_SHARE_READ, &sa, OPEN_EXISTING, 0, nullptr);
    HANDLE hOut = CreateFileW(outFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa, CREATE_ALWAYS, 0, nullptr);
    if (hIn == INVALID_HANDLE_VALUE) hIn = nullptr;
    if (hOut == INVALID_HANDLE_VALUE) hOut = nullptr;
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hIn ? hIn : GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hOut ? hOut : GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION pi = {};
    wstring cmd = L"\"" + exePath + L"\"";
    // a.exe 为 -static 全静态产物，无需任何 mingw DLL，不传自定义环境块（见 RunCommand 注释）
    BOOL ok = CreateProcessW(exePath.c_str(), &cmd[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                             dir.empty() ? nullptr : dir.c_str(), &si, &pi);
    if (hIn) CloseHandle(hIn);
    if (hOut) CloseHandle(hOut);
    if (!ok) return false;
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

// 运行程序：可带命令行参数 args，stdout 重定向到 outFile；stdin 用 inFile（为空则接 NUL）。
// 用于「生成器」模式：generator.exe <数据点编号> > <编号>.in
// 返回 true 表示子进程退出码为 0（生成器正常跑完）。
static bool RunProcessTo(const wstring& exePath, const wstring& args,
                         const wstring& inFile, const wstring& outFile, const wstring& dir) {
    SECURITY_ATTRIBUTES sa = { sizeof(sa), nullptr, TRUE };
    wstring inPath = inFile.empty() ? L"NUL" : inFile;   // 无输入时接 NUL，scanf 会干净地 EOF
    HANDLE hIn = CreateFileW(inPath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                             &sa, OPEN_EXISTING, 0, nullptr);
    if (hIn == INVALID_HANDLE_VALUE) hIn = nullptr;
    HANDLE hOut = CreateFileW(outFile.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &sa,
                              CREATE_ALWAYS, 0, nullptr);
    if (hOut == INVALID_HANDLE_VALUE) {
        if (hIn) CloseHandle(hIn);
        return false;
    }
    STARTUPINFOW si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hIn ? hIn : GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = hOut;
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION pi = {};
    wstring cmd = L"\"" + exePath + L"\"";
    if (!args.empty()) { cmd += L" "; cmd += args; }
    // 同 RunCommand：绝不传自定义环境块（会触发 CreateProcessW err=87）
    BOOL ok = CreateProcessW(exePath.c_str(), &cmd[0], nullptr, nullptr, TRUE, CREATE_NO_WINDOW, nullptr,
                             dir.empty() ? nullptr : dir.c_str(), &si, &pi);
    if (hIn) CloseHandle(hIn);
    CloseHandle(hOut);
    if (!ok) return false;
    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return code == 0;
}

// 系统临时目录（末尾自带反斜杠）；失败返回 ".\"
static wstring GetTempDir() {
    wchar_t buf[MAX_PATH + 1] = {0};
    DWORD n = GetTempPathW(MAX_PATH, buf);
    if (n == 0 || n > MAX_PATH) return L".\\";
    return buf;
}

// g++ 路径配置文件：exe 同目录 cdc_gpp.cfg（一行，纯路径）。
// 用户在界面里选过 g++ 路径后自动写入，下次启动优先使用。
static wstring GppCfgPath() {
    if (g_exeDir.empty()) return L"cdc_gpp.cfg";
    return Utf8ToWstr(g_exeDir) + L"\\cdc_gpp.cfg";
}

static string ReadGppCfg() {
    string p = WstrToUtf8(GppCfgPath());
    FILE* f = fopen(p.c_str(), "rb");
    if (!f) return "";
    string s;
    char buf[4096];
    size_t n = fread(buf, 1, sizeof(buf), f);
    s.assign(buf, n);
    fclose(f);
    // 去掉首尾空白/换行/引号
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r' || s.front() == '\n' || s.front() == '"')) s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n' || s.back() == '"')) s.pop_back();
    return s;
}

static bool WriteGppCfg(const string& gpp) {
    return WriteUtf8File(GppCfgPath(), gpp);
}

static string FindGpp() {
    // 0) 用户上次手动选择/检测后保存的路径（cdc_gpp.cfg），最高优先级
    {
        string saved = ReadGppCfg();
        if (!saved.empty() && FileExists(Utf8ToWstr(saved))) return saved;
    }
    // 1) exe 同目录下的 mingw64/bin/g++.exe（打包场景：cdc_backend.exe 与 mingw64 同目录）
    {
        wstring exeDir = Utf8ToWstr(g_exeDir);
        if (!exeDir.empty()) {
            wstring self = exeDir + L"\\mingw64\\bin\\g++.exe";
            if (FileExists(self)) return WstrToUtf8(self);
            // 枚举 exe 目录下所有子目录，找 <子目录>\bin\g++.exe（如 cdc_app\mingw64）
            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW((exeDir + L"\\*").c_str(), &fd);
            if (h != INVALID_HANDLE_VALUE) {
                do {
                    if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                        wstring nm = fd.cFileName;
                        if (nm == L"." || nm == L"..") continue;
                        wstring cand2 = exeDir + L"\\" + nm + L"\\bin\\g++.exe";
                        if (FileExists(cand2)) { FindClose(h); return WstrToUtf8(cand2); }
                    }
                } while (FindNextFileW(h, &fd));
                FindClose(h);
            }
        }
    }
    // 2) 固定盘符候选
    const wchar_t* cand[] = {
        L"D:\\mingw64\\bin\\g++.exe", L"C:\\mingw64\\bin\\g++.exe",
        L"C:\\Program Files\\mingw64\\bin\\g++.exe", L"C:\\Program Files (x86)\\mingw64\\bin\\g++.exe",
        L"D:\\TDM-GCC-64\\bin\\g++.exe", L"C:\\TDM-GCC-64\\bin\\g++.exe",
        L"D:\\msys64\\mingw64\\bin\\g++.exe", L"C:\\msys64\\mingw64\\bin\\g++.exe",
    };
    for (auto p : cand) if (FileExists(p)) return WstrToUtf8(p);
    // 3) PATH 环境变量
    wchar_t pathBuf[32767];
    if (GetEnvironmentVariableW(L"PATH", pathBuf, 32767)) {
        wstring path = pathBuf;
        size_t p = 0;
        while (p < path.size()) {
            size_t q = path.find(L';', p);
            wstring dir = (q == wstring::npos) ? path.substr(p) : path.substr(p, q - p);
            wstring test = dir + L"\\g++.exe";
            if (FileExists(test)) return WstrToUtf8(test);
            if (q == wstring::npos) break;
            p = q + 1;
        }
    }
    return "";
}

// 真正检测 g++ 路径的接口（供前端"自动检测"调用）
static string ApiGppDetect() {
    string gpp = FindGpp();
    if (gpp.empty()) return "{\"ok\":false}";
    return "{\"ok\":true,\"gpp\":\"" + JsonEsc(gpp) + "\"}";
}

// 保存用户选择的 g++ 路径（落盘 cdc_gpp.cfg，下次启动自动回填）
static string ApiSetGpp(const string& body) {
    string gpp = JsonValue(body, "gpp");
    while (!gpp.empty() && (gpp.back() == ' ' || gpp.back() == '\r' || gpp.back() == '\n')) gpp.pop_back();
    while (!gpp.empty() && (gpp.front() == ' ')) gpp.erase(gpp.begin());
    if (gpp.empty()) {
        // 传空 = 清除保存的路径，回到自动检测
        DeleteFileW(GppCfgPath().c_str());
        return "{\"ok\":true,\"cleared\":true}";
    }
    // 校验：必须存在
    if (!FileExists(Utf8ToWstr(gpp))) return "{\"ok\":false,\"message\":\"该路径不存在：" + JsonEsc(gpp) + "\"}";
    if (!WriteGppCfg(gpp)) return "{\"ok\":false,\"message\":\"写入 cdc_gpp.cfg 失败（目录可能只读）\"}";
    DebugLog("  setgpp saved: " + gpp);
    return "{\"ok\":true,\"gpp\":\"" + JsonEsc(gpp) + "\"}";
}

// exe 同目录下的 CDCSCQ 函数库目录（打包后 = {app}\cdc_lib）
static wstring CdcLibDir() {
    if (g_exeDir.empty()) return {};
    wstring d = Utf8ToWstr(g_exeDir) + L"\\cdc_lib";
    DWORD attr = GetFileAttributesW(d.c_str());
    if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY)) return {};
    return d;
}

// 内置生成函数库信息（CDCSCQ）：返回头文件路径 + 示例源码，供前端展示/另存
static string ApiGenLib() {
    wstring dir = CdcLibDir();
    string exeDir = g_exeDir;
    string res = "{";
    res += "\"ok\":true,";
    res += "\"name\":\"CDCSCQ\",";
    res += "\"dir\":\"" + JsonEsc(dir.empty() ? "" : WstrToUtf8(dir)) + "\",";
    res += "\"header\":\"#include <cdcscq.hpp>\",";
    res += "\"available\":" + string(dir.empty() ? "false" : "true") + ",";
    res += "\"gppCfg\":\"" + JsonEsc(ReadGppCfg()) + "\",";

    // 分类说明（与 cdcscq.hpp v2.0 章节对应）
    res += "\"groups\":[";
    res += "{\"t\":\"数据点编号\",\"f\":[\"case_id() 当前数据点编号 1..N\",\"seed_case() 按编号播种（可复现）\",\"CDCSCQ_MAIN {} 稳定取编号的写法\"]},";
    res += "{\"t\":\"随机数与数列\",\"f\":[\"seed() / seed(x) / rnd(l,r) / rndf(l,r) / chance(p)\",\"seq(n,l,r) 可重复 · seq_unique(n,l,r) 互不相同\",\"seq_func(n,l,r,fn) 按解析式 · permutation(n) 排列\",\"vecs(k,dim,l,r,uniq) 多维 · matrix(n,m,l,r) 矩阵\",\"seq_arith/geom/fib/walk/zigzag/binary/pow2\"]},";
    res += "{\"t\":\"★ 极端数据\",\"f\":[\"seq_extreme(n,l,r,kind) 一条命令造边界用例\",\"kind: min/max/zero/same/asc/desc/rev/near/alt/small/pow2/rand\",\"weighted_extreme(edges,l,r,kind) 边权极端化\",\"seq_const(n,v) 全相同\"]},";
    res += "{\"t\":\"字符串 / 段落\",\"f\":[\"str(len[,dict]) · strs(k,len[,dict]) · strs_unique\",\"binary_str(len[,p]) 0/1 串 · palindrome(len) 回文\",\"bracket(pairs) 随机括号 · bracket_valid(pairs) 合法括号\",\"paragraph(lines,words,dict) 段落\",\"DICT_LOWER/UPPER/DIGIT/ALNUM/01/ABC/BRACK\"]},";
    res += "{\"t\":\"随机树\",\"f\":[\"tree(n,kind[,strong]) 统一入口\",\"kind: chain/star/binary/random/prufer/caterpillar/broom/kary:3/balanced:4\",\"tree_prufer(n) 均匀随机树 · tree_broom(n) 扫帚（深+末端度大）\",\"weighted(edges,l,r) 加边权 · out_tree(n,edges[,w]) 标准树输出\"]},";
    res += "{\"t\":\"随机图\",\"f\":[\"graph_random(n,m,directed,simple) · graph_multigraph 重边自环\",\"graph_connected 连通 · graph_dense(n,密度) · graph_tree_plus(n,额外边)\",\"graph_dag(n,m) · graph_dag_layered(n,层数,m) 分层 DAG\",\"graph_complete · graph_complete_bipartite(a,b) · graph_bipartite(a,b,m)\",\"graph_path/graph_cycle/graph_grid(n,m) 链·环·网格\",\"graph_adj · graph_adjw 邻接表/带权邻接表\",\"out_graph(n,edges[,w]) 标准图输出 · out_adj_matrix 邻接矩阵\"]},";
    res += "{\"t\":\"几何 / 询问\",\"f\":[\"polygon_exact(n,l,r) 恰好 n 顶点简单多边形 · polygon_convex 凸多边形\",\"polygon(n,l,r) 凸包多边形 · points(n,l,r,distinct) 点集\",\"polygon_area2/area/perimeter · point_dist\",\"queries(n,l,r) 区间询问 · out_queries / out_pairs\"]},";
    res += "{\"t\":\"输出 / 输入\",\"f\":[\"out_vec / out_lines / out_vec2 / out_matrix / out_double_vec\",\"out_edges / out_edges_weighted / out_points / out_bits\",\"out_tree / out_graph / out_adj_matrix / out_queries\",\"read_int/read_long/read_double/read_str · require(cond,msg)\"]}";
    res += "],";

    // 示例源码（cdc_lib\examples\demo_all.cpp），供前端"一键另存为示例生成器"
    string demo;
    if (!dir.empty()) {
        string dp = WstrToUtf8(dir) + "\\examples\\demo_all.cpp";
        FILE* f = fopen(dp.c_str(), "rb");
        if (f) {
            char buf[8192]; size_t n;
            while ((n = fread(buf, 1, sizeof(buf), f)) > 0) demo.append(buf, n);
            fclose(f);
        }
    }
    res += "\"demo\":\"" + JsonEsc(demo) + "\"";
    res += "}";                                    // ★ 必须有：漏掉它前端 JSON.parse 直接抛错，整个接口等于废掉
    return res;
}

// =============================================================
//  「生成器」模式（exe 内置写码 → 编译 → 批量出 .in）
// -----------------------------------------------------------------------------
//  用户完全不用碰磁盘：在界面里写 C++ 生成器代码（可用 CDCSCQ 库），
//  一键完成「编译 → 对 i=start..end 逐个运行 → stdout 存成 <i>.in」。
//  临时源文件/可执行文件落在 %TEMP%\cdc_gen\，绝不污染用户目录。
//  说明：.out 仍由「自编译」模式用 AC 代码跑 .in 得到，流程不变。
// =============================================================
static string ApiGenRun(const string& body) {
    string gpp = JsonValue(body, "gpp");
    string code = JsonStr(body, "code");            // 多行代码必须用 JsonStr（JsonValue 会在逗号处截断）
    string mode = JsonValue(body, "mode");              // 空/gen = 生成；check = 只编译检查
    int start = atoi(JsonValue(body, "start").c_str());
    int endd  = atoi(JsonValue(body, "end").c_str());
    if (code.find_first_not_of(" \t\r\n") == string::npos)
        return "{\"ok\":false,\"output\":\"生成器代码为空：请先在编辑器里写代码（或点「载入示例」）\"}";

    auto normSlash = [](wstring p) { for (auto& c : p) if (c == L'/') c = L'\\'; return p; };
    wstring wsGpp = normSlash(Utf8ToWstr(gpp));
    if (wsGpp.empty() || !FileExists(wsGpp)) {
        gpp = FindGpp();
        wsGpp = Utf8ToWstr(gpp);
        if (wsGpp.empty())
            return "{\"ok\":false,\"output\":\"未找到 g++.exe：请到「自编译」页点「自动检测」或手动指定路径\"}";
    }

    // 数据存放目录（.in 落点）：只接受绝对路径，避免把文件写错地方。
    // 「仅检查代码」模式不需要目录，允许留空。
    wstring runDir;
    {
        wstring wsData = Utf8ToWstr(JsonValue(body, "dataDir"));
        while (!wsData.empty() && (wsData.back() == L' ' || wsData.back() == L'\\' || wsData.back() == L'/'))
            wsData.pop_back();
        wsData = normSlash(wsData);
        bool isAbs = (wsData.size() >= 3 && wsData[1] == L':' && (wsData[2] == L'\\' || wsData[2] == L'/'))
                  || (wsData.size() >= 2 && wsData[0] == L'\\' && wsData[1] == L'\\');
        if (!wsData.empty() && isAbs) runDir = wsData;
        else if (mode != "check")
            return "{\"ok\":false,\"output\":\"请先选择「数据存放目录」（需为绝对路径，可点「📁 时间戳目录」自动建）\"}";
    }
    if (!runDir.empty()) EnsureDir(runDir);

    // 临时工作目录（不落在用户目录，避免桌面被搞乱）
    wstring work = GetTempDir() + L"cdc_gen";
    EnsureDir(work);
    wstring src = work + L"\\generator.cpp";
    wstring gen = work + L"\\generator.exe";
    if (!WriteUtf8File(src, code))
        return "{\"ok\":false,\"output\":\"写入临时源文件失败：" + JsonEsc(WstrToUtf8(work)) + "\"}";

    wstring libDir = CdcLibDir();
    wstring incOpt;
    if (!libDir.empty()) incOpt = L" -I\"" + libDir + L"\"";

    wstring compileCmd = L"\"" + wsGpp + L"\" -O2 -std=c++23 -static -static-libgcc -static-libstdc++"
                       + incOpt + L" \"" + src + L"\" -o \"" + gen + L"\"";

    DeleteFileW(gen.c_str());
    wstring compileDir = MingwBinDir();
    if (compileDir.empty()) compileDir = work;

    string cout_;
    string output = ">>> 编译生成器：g++ -O2 -std=c++23 -static\n";
    if (libDir.empty()) output += "（提示：未找到 cdc_lib，生成器代码不能 #include <cdcscq.hpp>）\n";
    DWORD code_ = 0;
    if (!RunCommand(compileCmd, compileDir, cout_, &code_)) {
        output += "编译进程启动失败（g++ 无法启动）\n";
        return "{\"ok\":false,\"output\":\"" + JsonEsc(output) + "\"}";
    }
    // 静默失败（退出码非 0、零输出、无产物）：安全软件对新 exe 的瞬时拦截 → 等 600ms 重试一次
    if (!FileExists(gen) && cout_.empty()) {
        Sleep(600);
        DeleteFileW(gen.c_str());
        cout_.clear();
        code_ = 0;
        RunCommand(compileCmd, compileDir, cout_, &code_);
    }
    if (!FileExists(gen)) {
        output += cout_ + "\n=== 生成器编译失败 ===\n";
        output += "g++ 退出码：" + to_string((long long)code_) + "\n";
        if (cout_.empty())
            output += "（g++ 未输出任何信息：多半被安全软件瞬时拦截，已自动重试一次仍失败。）\n";
        return "{\"ok\":false,\"output\":\"" + JsonEsc(output) + "\"}";
    }
    if (!cout_.empty()) output += cout_ + "\n";
    output += "✔ 生成器编译成功\n";

    if (mode == "check") {
        output += "已通过编译检查，未生成数据。";
        return "{\"ok\":true,\"checked\":true,\"output\":\"" + JsonEsc(output) + "\"}";
    }

    if (start <= 0) start = 1;
    if (endd < start) endd = start;
    if (endd - start > 999) endd = start + 999;      // 防御：单次最多 1000 个数据点

    int okCnt = 0, failCnt = 0;
    string filesJson;
    output += ">>> 生成数据 " + to_string(start) + " – " + to_string(endd) + "\n";
    for (int id = start; id <= endd; ++id) {
        wstring inFile = runDir + L"\\" + to_wstring(id) + L".in";
        // argv[1] = 数据点编号（代码里用 case_id() 取）
        if (RunProcessTo(gen, to_wstring(id), L"", inFile, runDir)) {
            ++okCnt;
            long long sz = 0;
            WIN32_FILE_ATTRIBUTE_DATA fad;
            if (GetFileAttributesExW(inFile.c_str(), GetFileExInfoStandard, &fad))
                sz = ((long long)fad.nFileSizeHigh << 32) | (long long)fad.nFileSizeLow;
            output += to_string(id) + ".in  " + FileSizeStr(sz) + "\n";
            if (filesJson.size() > 2) filesJson += ",";
            filesJson += "{\"name\":\"" + to_string(id) + ".in\",\"size\":\"" + FileSizeStr(sz) + "\"}";
        } else {
            ++failCnt;
            DeleteFileW(inFile.c_str());             // 运行失败：清掉半截输出
            output += to_string(id) + ".in 运行失败（生成器退出码非 0）\n";
        }
    }
    output += "=== 完成：成功 " + to_string(okCnt) + " 个，失败 " + to_string(failCnt) + " 个 ===\n";
    output += "目录：" + WstrToUtf8(runDir) + "\n";
    output += "下一步：去「自编译」页，用你的 AC 代码对这些 .in 生成 .out。";

    string res = "{";
    res += "\"ok\":" + string(okCnt > 0 ? "true" : "false") + ",";
    res += "\"okCount\":" + to_string(okCnt) + ",";
    res += "\"failCount\":" + to_string(failCnt) + ",";
    res += "\"dir\":\"" + JsonEsc(WstrToUtf8(runDir)) + "\",";
    res += "\"files\":[" + filesJson + "],";
    res += "\"output\":\"" + JsonEsc(output) + "\"";
    res += "}";
    return res;
}

// 调试日志：默认【关闭】——防止 backend_debug.log 只增不减、无限膨胀占空间。
// 需要排障时：设环境变量 CDC_DEBUG=1 再启动本程序即可重新落盘。
// 即便开启，单文件超过 2MB 也会自动重开（滚动），杜绝无限增长。
static void DebugLog(const string& msg) {
    static const bool s_enabled = []() {
        char buf[8] = {0};
        DWORD n = GetEnvironmentVariableA("CDC_DEBUG", buf, (DWORD)sizeof(buf));
        return n > 0 && buf[0] == '1';
    }();
    if (!s_enabled) return;   // 关闭：不建文件、不写盘
    string p = g_exeDir.empty() ? "backend_debug.log" : g_exeDir + "\\backend_debug.log";
    const long long MAXSZ = 2LL * 1024 * 1024;   // 2MB 上限
    long long sz = 0;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (GetFileAttributesExW(Utf8ToWstr(p).c_str(), GetFileExInfoStandard, &fad)) {
        sz = ((long long)fad.nFileSizeHigh << 32) | (long long)fad.nFileSizeLow;
    }
    FILE* f = fopen(p.c_str(), sz > MAXSZ ? "w" : "a");   // 超限则清空重开
    if (f) { fprintf(f, "%s\n", msg.c_str()); fclose(f); }
}

static string ApiCompile(const string& body) {
    DebugLog("ApiCompile start");
    string gpp = JsonValue(body, "gpp");
    string source = JsonValue(body, "source");
    string mode = JsonValue(body, "mode");       // batch | single
    int start = atoi(JsonValue(body, "start").c_str());
    int endd  = atoi(JsonValue(body, "end").c_str());
    DebugLog("  params: " + mode + " " + source);

    wstring wsSource = Utf8ToWstr(source);
    wstring wsGpp = Utf8ToWstr(gpp);
    // 路径规范化：正斜杠统一为反斜杠（CreateProcessW 的 lpCurrentDirectory 不接受 '/'）
    auto normSlash = [](wstring p) { for (auto& c : p) if (c == L'/') c = L'\\'; return p; };
    wsSource = normSlash(wsSource);
    wsGpp = normSlash(wsGpp);

    if (wsGpp.empty() || !FileExists(wsGpp)) {
        gpp = FindGpp();
        wsGpp = Utf8ToWstr(gpp);
        if (wsGpp.empty()) return "{\"ok\":false,\"output\":\"未找到 g++.exe，请在设置中指定路径\"}";
    }
    DebugLog("  gpp ok");
    if (wsSource.empty() || !FileExists(wsSource)) return "{\"ok\":false,\"output\":\"源文件不存在\"}";

    size_t pos = wsSource.find_last_of(L"\\/");
    wstring srcDir = (pos == wstring::npos) ? L"." : wsSource.substr(0, pos);
    wstring exe = srcDir + L"\\a.exe";

    // 数据目录（.in / .out 所在）：默认与源 .cpp 同目录；前端可传 dataDir 指定
    wstring runDir = srcDir;
    wstring wsData = Utf8ToWstr(JsonValue(body, "dataDir"));
    while (!wsData.empty() && (wsData.back() == L' ' || wsData.back() == L'\\' || wsData.back() == L'/')) wsData.pop_back();
    wsData = normSlash(wsData);
    // 防御：只接受绝对路径（盘符 X:\ 或 UNC \\），否则忽略，避免"桌面/xxx"这类半吊子路径把文件写错地方
    bool isAbs = (wsData.size() >= 3 && wsData[1] == L':' && (wsData[2] == L'\\' || wsData[2] == L'/'))
              || (wsData.size() >= 2 && wsData[0] == L'\\' && wsData[1] == L'\\');
    if (!wsData.empty() && isAbs) { runDir = wsData; EnsureDir(runDir); }
    else if (!wsData.empty()) DebugLog("  dataDir ignored (not absolute): " + WstrToUtf8(wsData));
    DebugLog("  srcDir=" + WstrToUtf8(srcDir) + " runDir=" + WstrToUtf8(runDir));

    // 内置函数库 CDCSCQ：若 exe 同目录存在 cdc_lib，则自动加入头文件搜索路径，
    // 用户写生成器时直接 #include <cdcscq.hpp> 即可，无需手动配 include。
    wstring libDir = CdcLibDir();
    wstring incOpt;
    if (!libDir.empty()) {
        incOpt = L" -I\"" + libDir + L"\"";
        DebugLog("  cdc_lib include: " + WstrToUtf8(libDir));
    } else {
        DebugLog("  cdc_lib not found (skip -I)");
    }
    wstring compileCmd = L"\"" + wsGpp + L"\" -O2 -std=c++23 -static -static-libgcc -static-libstdc++" + incOpt
                       + L" \"" + wsSource + L"\" -o \"" + exe + L"\"";
    DebugLog("  compileCmd built");
    DebugLog("  compileCmd: " + WstrToUtf8(compileCmd));

    string cout;
    string output;
    output += ">>> 编译：g++ -O2 -std=c++23 -static\n";

    // 编译前清理旧 exe：若被占用（程序还在运行），提前给出明确提示
    if (FileExists(exe)) {
        if (!DeleteFileW(exe.c_str())) {
            DWORD del = GetLastError();
            if (del == ERROR_ACCESS_DENIED || del == ERROR_SHARING_VIOLATION || del == ERROR_USER_MAPPED_FILE) {
                output += "输出文件被占用，无法覆盖写入：\"" + WstrToUtf8(exe) + "\"\n";
                output += "请先关闭正在运行的该程序窗口（a.exe / as.exe），再重新编译。\n";
                DebugLog("  exe locked, delete failed err=" + to_string((long long)del));
                return "{\"ok\":false,\"output\":\"" + JsonEsc(output) + "\"}";
            }
        }
    }

    // g++ 子进程工作目录：优先 mingw64\bin（Windows DLL 搜索含"当前目录"，排在 PATH 前），
    // 保证编译器子程序（cc1plus/as/ld）的 DLL 依赖可命中；否则退回源文件目录。
    // 编译命令里源文件与输出均为绝对路径，工作目录不影响文件定位。
    wstring compileDir = MingwBinDir();
    if (compileDir.empty()) compileDir = srcDir;

    DWORD code = 0;
    if (!RunCommand(compileCmd, compileDir, cout, &code)) {
        output += "编译进程启动失败（g++ 无法启动）\n";
        return "{\"ok\":false,\"output\":\"" + JsonEsc(output) + "\"}";
    }
    DebugLog("  RunCommand compile done, len=" + to_string(cout.size()) + " exit=" + to_string((long long)code));
    // 静默失败（退出码非 0 且零输出、无产物）：多见于安全软件（腾讯电脑管家等）对
    // 新生成 a.exe 的瞬时锁定/云查杀。等 600ms 自动重试一次，重试前清掉可能残留的旧 a.exe。
    if (!FileExists(exe) && cout.empty()) {
        DebugLog("  silent compile failure, retrying once after 600ms");
        Sleep(600);
        DeleteFileW(exe.c_str());
        cout.clear();
        code = 0;
        RunCommand(compileCmd, compileDir, cout, &code);
        DebugLog("  retry done, len=" + to_string(cout.size()) + " exit=" + to_string((long long)code));
    }
    if (!FileExists(exe)) {
        output += cout + "\n=== 编译失败 ===\n";
        output += "g++ 退出码：" + to_string((long long)code) + "\n";
        if (cout.empty()) {
            output += "（g++ 未输出任何信息：a.exe 多半被安全软件瞬时拦截或被占用，已自动重试一次仍失败。）\n";
            output += "建议：① 关掉仍在运行的 a.exe 程序窗口；② 在腾讯电脑管家「信任区」添加本软件目录（含 mingw64）与存放数据的目录；③ 再试。\n";
        }
        DebugLog("  compile FAILED: no exe, code=" + to_string((long long)code));
        return "{\"ok\":false,\"output\":\"" + JsonEsc(output) + "\"}";
    }
    output += "编译成功：a.exe\n";
    DebugLog("  exe exists");

    if (mode == "batch") {
        if (start <= 0) start = 1;
        if (endd < start) endd = start;
        output += ">>> 批量运行 " + to_string(start) + " – " + to_string(endd) + "\n";
        for (int id = start; id <= endd; ++id) {
            wstring inFile = runDir + L"\\" + to_wstring(id) + L".in";
            wstring outFile = runDir + L"\\" + to_wstring(id) + L".out";
            if (!FileExists(inFile)) {
                WriteUtf8File(inFile, "1\n0\n");
            }
            DebugLog("  run id=" + to_string(id));
            if (RunProcessFile(exe, inFile, outFile, runDir)) {
                output += to_string(id) + ".in -> " + to_string(id) + ".out\n";
            } else {
                output += to_string(id) + ".in 运行失败\n";
            }
        }
        output += "=== 批量运行完成 ===";
    } else {
        wstring inFile = runDir + L"\\" + to_wstring(start) + L".in";
        wstring outFile = runDir + L"\\" + to_wstring(start) + L".out";
        if (!FileExists(inFile)) WriteUtf8File(inFile, "1\n0\n");
        if (RunProcessFile(exe, inFile, outFile, runDir)) {
            output += "运行完成：" + WstrToUtf8(outFile);
        } else {
            output += "运行失败：" + WstrToUtf8(inFile);
        }
    }

    return "{\"ok\":true,\"output\":\"" + JsonEsc(output) + "\"}";
}

// =============================================================
//  ZIP 打包 / 工具
// =============================================================
static string ApiZip(const string& body) {
    string src = JsonValue(body, "src");
    string dst = JsonValue(body, "dst");
    if (src.empty() || dst.empty()) return "{\"ok\":false,\"message\":\"请选择源目录和输出 ZIP\"}";
    if (dst.find(".zip") == string::npos) dst += ".zip";
    string ps = "powershell -NoProfile -Command \"Compress-Archive -Path '" + src + "' -DestinationPath '" + dst + "' -Force\"";
    string out;
    bool ok = RunCommand(Utf8ToWstr(ps), L"", out);
    if (ok) return "{\"ok\":true,\"path\":\"" + JsonEsc(dst) + "\"}";
    return "{\"ok\":false,\"message\":\"打包失败：" + JsonEsc(out) + "\"}";
}

// 保存为 ZIP：弹"另存为"对话框选位置，可只打包 dir 下的指定文件列表（files 用 | 分隔文件名）
static string ApiSaveZip(const string& body) {
    string dir = JsonValue(body, "dir");
    string filesRaw = JsonValue(body, "files");
    string defName = JsonValue(body, "defName");
    if (defName.empty()) defName = "数据点.zip";
    if (defName.find(".zip") == string::npos) defName += ".zip";

    wchar_t p[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    wstring filter = L"ZIP 压缩包 (*.zip)\0*.zip\0所有文件\0*.*\0\0";
    wstring defW = Utf8ToWstr(defName);
    wcscpy_s(p, defW.c_str());
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = p;
    ofn.nMaxFile = MAX_PATH;
    wstring initDir = Utf8ToWstr(dir);
    if (!initDir.empty()) ofn.lpstrInitialDir = initDir.c_str();
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    if (!GetSaveFileNameW(&ofn)) return "{\"ok\":true,\"cancel\":true}";  // 用户取消
    wstring dst = p;

    // 组装要打包的条目
    wstring ps = L"powershell -NoProfile -Command \"Compress-Archive -Path ";
    vector<wstring> items;
    wstring wsDir = Utf8ToWstr(dir);
    string fs = filesRaw;
    size_t q0 = 0;
    bool hasFiles = false;
    while (q0 < fs.size()) {
        size_t q1 = fs.find('|', q0);
        string name = (q1 == string::npos) ? fs.substr(q0) : fs.substr(q0, q1 - q0);
        if (!name.empty()) { hasFiles = true; items.push_back(wsDir + L"\\" + Utf8ToWstr(name)); }
        if (q1 == string::npos) break;
        q0 = q1 + 1;
    }
    if (!hasFiles) {
        if (dir.empty()) return "{\"ok\":false,\"message\":\"缺少源目录\"}";
        items.push_back(wsDir + L"\\*");
    }
    for (size_t i = 0; i < items.size(); ++i) {
        if (i) ps += L",";
        ps += L"'" + items[i] + L"'";
    }
    ps += L" -DestinationPath '" + dst + L"' -Force\"";
    string out;
    bool ok = RunCommand(ps, L"", out);
    if (ok) return "{\"ok\":true,\"path\":\"" + JsonEsc(WstrToUtf8(dst)) + "\"}";
    return "{\"ok\":false,\"message\":\"打包失败：" + JsonEsc(out) + "\"}";
}

static string ApiTool(const string& body) {
    string cmd = JsonValue(body, "cmd");
    if (cmd.empty()) return "{\"ok\":false,\"message\":\"工具命令为空\"}";
    // 系统命令或 URL
    HINSTANCE hr = ShellExecuteW(nullptr, L"open", Utf8ToWstr(cmd).c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if ((intptr_t)hr > 32) return "{\"ok\":true}";
    return "{\"ok\":false,\"message\":\"启动失败，可能命令无效\"}";
}

static string ApiInfo() {
    string gpp = FindGpp();
    string res = "{";
    res += "\"ok\":true,";
    res += "\"ver\":\"" + string(APP_VER) + "\",";
    res += "\"backend\":\"cpp\",";
    res += "\"cloudReady\":true,";   // 仅告知前端云端可用，不暴露真实地址
    res += "\"gpp\":\"" + gpp + "\",";
    res += "\"gppSaved\":" + string(ReadGppCfg().empty() ? "false" : "true") + ",";
    res += "\"libReady\":" + string(CdcLibDir().empty() ? "false" : "true") + ",";
    res += "\"libName\":\"CDCSCQ\",";
    res += "\"desktop\":\"" + JsonEsc(WstrToUtf8(GetDesktopPath())) + "\",";
    res += "\"exeDir\":\"" + JsonEsc(g_exeDir) + "\",";
    res += "\"os\":\"Windows\"";
    res += "}";
    return res;
}

// 时间戳目录：返回真实桌面绝对路径 + 时间戳子目录（已创建），供"📁 时间戳目录"使用
static string ApiMakeTsDir() {
    wstring base = GetDesktopPath();
    wstring dir = base + L"\\" + GetNowString();
    EnsureDir(dir);
    DebugLog("  maketsdir -> " + WstrToUtf8(dir));
    return "{\"ok\":true,\"dir\":\"" + JsonEsc(WstrToUtf8(dir)) + "\"}";
}

// 浏览对话框：mode=file|folder
static string ApiBrowse(const string& body) {
    string mode = JsonValue(body, "mode");
    if (mode == "folder") {
        BROWSEINFOW bi = {};
        bi.lpszTitle = L"选择目录";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (!pidl) return "{\"ok\":true,\"path\":\"\"}";
        wchar_t p[MAX_PATH];
        if (SHGetPathFromIDListW(pidl, p)) { CoTaskMemFree(pidl); return "{\"ok\":true,\"path\":\"" + JsonEsc(WstrToUtf8(p)) + "\"}"; }
        CoTaskMemFree(pidl);
        return "{\"ok\":true,\"path\":\"\"}";
    }
    // file
    wchar_t p[MAX_PATH] = {};
    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    wstring filter = L"所有文件\0*.*\0\0";
    ofn.lpstrFilter = filter.c_str();
    ofn.lpstrFile = p;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_HIDEREADONLY;
    if (GetOpenFileNameW(&ofn)) return "{\"ok\":true,\"path\":\"" + JsonEsc(WstrToUtf8(p)) + "\"}";
    return "{\"ok\":true,\"path\":\"\"}";
}

// =============================================================
//  HTTP 服务器
// =============================================================
// =============================================================
//  云端代理（接管原原生客户端 exe 的私信/账号/AI 同步职责）
//  前端只跟本地 127.0.0.1 通信，由本后端用 WinHTTP 转发到
//  内置（或 cdc_srv.dat 覆盖）的云端服务器，彻底绕开浏览器跨域（CORS）。
// =============================================================

struct HttpReq {
    string method, path, body, headers;
};

// 大小写不敏感取请求头字段值
static string GetHeaderVal(const string& headers, const string& name) {
    string key = name + ":";
    size_t i = 0;
    while (i + key.size() <= headers.size()) {
        bool match = true;
        for (size_t j = 0; j < key.size(); ++j) {
            char a = headers[i + j], b = key[j];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) { match = false; break; }
        }
        if (match) {
            size_t vstart = i + key.size();
            size_t vend = headers.find("\r\n", vstart);
            string v = (vend == string::npos) ? headers.substr(vstart) : headers.substr(vstart, vend - vstart);
            size_t a = v.find_first_not_of(" \t");
            size_t b = v.find_last_not_of(" \t\r\n");
            if (a == string::npos) return "";
            return v.substr(a, b - a + 1);
        }
        i++;
    }
    return "";
}

// 用 WinHTTP 把请求转发到云端服务器（支持 https + 忽略证书错误）
// 向指定 host/path 发一次 POST，返回响应体；outStatus 带回 HTTP 状态码（0=失败）
static string HttpPostTo(const string& host, int port, bool https, const string& fullPath,
                         const string& body, const string& contentType, DWORD& outStatus) {
    outStatus = 0;
    HINTERNET hSess = WinHttpOpen(L"CDCDataGen/7.1.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return "";
    HINTERNET hConn = WinHttpConnect(hSess, Utf8ToWstr(host).c_str(), (INTERNET_PORT)port, 0);
    if (!hConn) { WinHttpCloseHandle(hSess); return ""; }
    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"POST", Utf8ToWstr(fullPath).c_str(), nullptr,
                                        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return ""; }

    // 忽略证书错误（自签/免费证书常见），保证能连上
    DWORD dwOpt = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
                | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    WinHttpSetOption(hReq, WINHTTP_OPTION_SECURITY_FLAGS, &dwOpt, sizeof(dwOpt));

    // 官方客户端凭证：X-CDC-Token 放请求头（不塞进 query，免得进服务器访问日志或 Referer）
    wstring hdr = Utf8ToWstr("Content-Type: " + contentType + "\r\nX-CDC-Token: " + CloudToken() + "\r\n");
    if (!WinHttpSendRequest(hReq, hdr.c_str(), (DWORD)-1L, (LPVOID)body.data(), (DWORD)body.size(), (DWORD)body.size(), 0)) {
        WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return "";
    }
    if (!WinHttpReceiveResponse(hReq, nullptr)) {
        WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return "";
    }
    DWORD status = 0, slen = sizeof(status);
    if (!WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, nullptr, &status, &slen, nullptr))
        status = 0;
    outStatus = status;
    string resp; DWORD avail = 0;
    while (WinHttpQueryDataAvailable(hReq, &avail) && avail) {
        string buf(avail, 0); DWORD rd = 0;
        if (!WinHttpReadData(hReq, &buf[0], avail, &rd) || rd == 0) break;
        resp.append(buf.c_str(), (size_t)rd);
    }
    WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess);
    return resp;
}

static bool LooksLikeJson(const string& s) {
    size_t i = 0;
    while (i < s.size() && (s[i] == ' ' || s[i] == '\r' || s[i] == '\n' || s[i] == '\t')) i++;
    return i < s.size() && (s[i] == '{' || s[i] == '[');
}

// 日志脱敏①：主机名半掩成 s***.example.com（保留首字母与域名后缀，够排障又不可直接复用）
static string MaskHost(const string& h) {
    if (h.empty()) return "(none)";
    size_t dot = h.find('.');
    if (dot == string::npos || dot == 0) return h.substr(0, 1) + "***";
    return h.substr(0, 1) + "***" + h.substr(dot);
}
// 日志脱敏②：压掉换行等不可打印字符（响应正文可能含私信内容，绝不整段落盘）
static string Sanitize(const string& s) {
    string o; o.reserve(s.size());
    for (char c : s) {
        unsigned char u = (unsigned char)c;
        o += (c == '\r' || c == '\n' || c == '\t') ? ' ' : (u < 32 ? '.' : c);
    }
    return o;
}

static string CloudProxyForward(const string& targetPath, const string& extraQuery,
                                const string& body, const string& contentType) {
    if (g_cloudServer.empty())
        return "{\"ok\":false,\"err\":\"cloud_not_configured\",\"message\":\"云端服务器未配置（内置地址为空）\"}";

    // 解析 scheme / host / 基础路径 / port
    // g_cloudServer 形如 https://host[:port][/basepath]，基础路径需拼到请求里
    string s = g_cloudServer;
    bool https = true;
    size_t sp = s.find("://");
    if (sp != string::npos) { string scheme = s.substr(0, sp); s = s.substr(sp + 3); https = (scheme == "https"); }
    // 分离 host 与 基础路径（形如 host/cdc_server）
    size_t slash = s.find('/');
    string host = s, basePath = "";
    if (slash != string::npos) { host = s.substr(0, slash); basePath = s.substr(slash); }
    int port = https ? 443 : 80;
    size_t colon = host.find(':');
    if (colon != string::npos) { port = atoi(host.substr(colon + 1).c_str()); host = host.substr(0, colon); }
    // 规范化基础路径：去掉末尾多余斜杠，保留前导 "/"
    while (basePath.size() > 1 && (basePath.back() == '/' || basePath.back() == '\\')) basePath.pop_back();

    string path = targetPath.empty() ? "/api/activate" : targetPath;
    if (path[0] != '/') path = "/" + path;

    // 候选路径（按命中概率排序，**第一个就是内置默认后台的真身**）。
    // 旧实现把 8 个候选每次都试一遍：正常一次云端调用要打 6 次 HTTPS，
    // 404/405 混在一起看就像在扫描路径，既慢又难看。现在：
    //   ① 子目录部署优先（内置默认 https://host/cdc_server → /cdc_server/api/activate/，第 1 次即命中）
    //   ② 目录写法 api/activate/ 命中率最高，排第一；不带尾斜杠的排第二（有些服务器会 405）
    //   ③ 只有「根目录部署」时才补 .php / index.php 文件写法，兜底老 MySQL 后台
    vector<string> tmpl;
    tmpl.push_back(path + "/");          // /api/activate/   ← 命中率最高，排第一
    tmpl.push_back(path);                // /api/activate    ← 部分服务器拒收 POST，兜底
    if (basePath.empty()) {              // 仅根目录部署才需要文件写法兜底
        tmpl.push_back(path + "/index.php");
        tmpl.push_back(path + ".php");
    }
    vector<string> bases;
    if (!basePath.empty()) bases.push_back(basePath); // /cdc_server（新版 PHP 后台）优先
    bases.push_back(string(""));                      // 根目录（原版 MySQL 后台）兜底
    vector<string> cands;
    for (auto& base : bases) {
        for (auto& t : tmpl) {
            string c = base + t;
            bool dup = false;
            for (auto& u : cands) if (u == c) { dup = true; break; }
            if (!dup) cands.push_back(c);
        }
    }

    // 已经命中过的路径直接打，不再逐个探测：正常情况每次云端调用只发 1 次 HTTPS
    if (!g_cloudOkPath.empty()) {
        DWORD st0 = 0;
        string r0 = HttpPostTo(host, port, https, g_cloudOkPath + extraQuery, body, contentType, st0);
        if (st0 != 0 && LooksLikeJson(r0)) {
            DebugLog("CloudProxy host=" + MaskHost(host) + " path=" + g_cloudOkPath
                     + " status=" + to_string(st0) + " cached=1");
            return r0;
        }
        g_cloudOkPath.clear();   // 云端换路径了，重新学习
    }

    string lastResp; DWORD lastStatus = 0;
    for (size_t i = 0; i < cands.size(); i++) {
        string fp = cands[i] + extraQuery;
        DWORD st = 0;
        string r = HttpPostTo(host, port, https, fp, body, contentType, st);
        bool js = LooksLikeJson(r);
        // 日志脱敏：主机名半掩、响应正文不落盘（私信内容属于用户隐私），
        // 只留「路径 + 状态码 + 长度 + 是否 JSON」，排障足够。
        string note = "CloudProxy https=" + string(https ? "1" : "0") + " host=" + MaskHost(host)
                    + ":" + to_string(port) + " path=" + fp + " status=" + to_string(st)
                    + " len=" + to_string(r.size()) + " json=" + (js ? "1" : "0");
        if (!js && !r.empty()) note += " head=" + Sanitize(r.substr(0, 80));  // 非 JSON 才留一小段（多为 404/405 错误页）
        DebugLog(note);
        if (st == 0) { lastResp = r; lastStatus = 0; break; }        // 连接层失败，停止
        if (st == 404 || st == 405) { lastStatus = st; continue; }   // 路径不存在/拒收 POST，试下一个
        lastResp = r; lastStatus = st;
        if (js) { g_cloudOkPath = cands[i]; break; }   // 命中 JSON：记住路径，以后一步到位
        // 非 JSON 的 2xx/5xx：记录后继续试下一个候选
    }
    if (lastResp.empty() && lastStatus == 0)
        return "{\"ok\":false,\"err\":\"winhttp_no_response\",\"message\":\"云端无响应（网络不可达或地址错误）\"}";
    if (!LooksLikeJson(lastResp))
        return "{\"ok\":false,\"err\":\"cloud_nonjson\",\"status\":" + to_string(lastStatus)
             + ",\"message\":\"云端返回非 JSON（HTTP " + to_string(lastStatus) + "），多为未部署 PHP / 路径不对 / 404 / 405\"}";
    return lastResp;
}

// /api/cloud 路由：解析 sub / chat，转发到云端，原样回传响应
static string ApiCloud(const HttpReq& req) {
    string path = req.path, query;
    size_t q = path.find('?');
    if (q != string::npos) { query = path.substr(q + 1); path = path.substr(0, q); }
    string sub = "/api/activate";
    bool chat = false;
    size_t p = 0;
    while (p < query.size()) {
        size_t e = query.find('&', p);
        string kv = (e == string::npos) ? query.substr(p) : query.substr(p, e - p);
        size_t eq = kv.find('=');
        string k = (eq == string::npos) ? kv : kv.substr(0, eq);
        string v = (eq == string::npos) ? "" : kv.substr(eq + 1);
        k = UrlDecode(k); v = UrlDecode(v);
        if (k == "sub" && !v.empty()) sub = v;
        else if (k == "chat") chat = (v == "1" || v == "true");
        p = (e == string::npos) ? query.size() : e + 1;
    }
    string extraQuery = chat ? "?chat=1" : "";
    string ct = GetHeaderVal(req.headers, "Content-Type");
    if (ct.empty()) ct = "application/x-www-form-urlencoded";
    return CloudProxyForward(sub, extraQuery, req.body, ct);
}

// =============================================================
//  自动更新
//  链路：打开即检查 update.json → 有新版立即弹窗 → 后台线程下载安装包
//        → 按"最新时间戳"在下载目录里找到安装包 → 自动运行它
//        → 若本机已安装，安装包以 /KEEPCFG=1 启动（保留用户配置）
//  设计取舍：
//   * 只把「版本 / 说明 / 包大小 / 是否已安装」给前端，**不把云端主机与下载直链
//     交给页面**——沿用既有设计，前端永远不出现云端主机名，免得进浏览器
//     历史与代理日志。下载由后端自己按清单里的 url 完成。
//   * update.json 是公开可读的（PHP 侧刻意不封 json 扩展名）。
//   * 下载跑在独立线程，主服务不阻塞；进度靠 /api/updstatus 轮询。
//   * 找安装包用「文件名特征 + 最近 72 小时内 mtime 最新」双保险，
//     服务器给安装包改名也照样认得出（这就是"按时间戳推断"的兜底）。
// =============================================================
struct CdcUrl { bool https = true; string host; int port = 443; string path = "/"; };

// 浏览器的"下载"目录：老 shell API 没有 CSIDL_DOWNLOADS，用 %USERPROFILE%\Downloads
// （拿不到就退回桌面，保证"按时间戳找安装包"至少有一个可信目录）
static wstring GetDownloadPath() {
    wchar_t up[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"USERPROFILE", up, MAX_PATH)) {
        wstring d = wstring(up) + L"\\Downloads";
        if (DirExists(d)) return d;
    }
    return GetDesktopPath();
}
// 取路径里的文件名（UTF-8 视角，只认反斜杠与正斜杠）
static string BaseNameUtf8(const string& p) {
    size_t s = p.find_last_of("\\/");
    return (s == string::npos) ? p : p.substr(s + 1);
}

static bool SplitUrl(const string& url, CdcUrl& u) {
    string s = url;
    size_t sp = s.find("://");
    if (sp != string::npos) {
        string scheme = s.substr(0, sp);
        for (size_t i = 0; i < scheme.size(); ++i) {
            char c = scheme[i];
            scheme[i] = (char)((c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c);
        }
        u.https = (scheme != "http");
        s = s.substr(sp + 3);
    }
    size_t slash = s.find('/');
    string hostport = (slash == string::npos) ? s : s.substr(0, slash);
    u.path = (slash == string::npos) ? "/" : s.substr(slash);   // 保留 ?query
    size_t colon = hostport.find(':');
    if (colon != string::npos) {
        u.host = hostport.substr(0, colon);
        u.port = atoi(hostport.c_str() + colon + 1);
    } else {
        u.host = hostport;
        u.port = u.https ? 443 : 80;
    }
    if (u.port <= 0) u.port = u.https ? 443 : 80;
    return !u.host.empty() && !u.path.empty();
}

// 从下载直链推断落地文件名；不合法就退回默认名（Inno 输出名就叫这个）
static string LowerAscii(const string& s) {
    string o = s;
    for (size_t i = 0; i < o.size(); ++i) {
        char c = o[i];
        o[i] = (char)((c >= 'A' && c <= 'Z') ? c - 'A' + 'a' : c);
    }
    return o;
}
// 该 URL 是否指向"一个安装包文件"（而不是首页/目录页）。
// 【2026-09-23 事故】后台「发布更新」里下载网址误填成了项目首页（不带安装包扩展名），
//   旧逻辑会把域名抠出来当文件名（域名.exe），再把首页 HTML 存成 exe，
//   最后"按文件名特征找安装包"自然一个也找不到 → 表现为"下载完不自动打开"。
static bool LooksLikeInstallerUrl(const string& url) {
    string base = url;
    size_t q = base.find('?'); if (q != string::npos) base = base.substr(0, q);
    q = base.find('#');        if (q != string::npos) base = base.substr(0, q);
    size_t sl = base.find_last_of('/');
    string name = LowerAscii((sl == string::npos) ? base : base.substr(sl + 1));
    const char* exts[] = { ".exe", ".msi", ".zip", ".7z", ".rar" };
    for (const char* e : exts) {
        size_t el = strlen(e);
        if (name.size() > el && name.compare(name.size() - el, el, e) == 0) return true;
    }
    if (name.find("setup") != string::npos && name.find('.') != string::npos) return true;
    return false;
}

static string UrlFileName(const string& url) {
    string base = url;
    size_t q = base.find('?'); if (q != string::npos) base = base.substr(0, q);
    size_t sl = base.find_last_of('/');
    string name = (sl == string::npos) ? base : base.substr(sl + 1);
    string out;
    for (char c : name) {
        if (strchr("\\/:*?\"<>|", c)) continue;
        out += c;
    }
    // 只有「确实带着安装包扩展名」才敢用它的文件名；否则用标准名，
    // 免得把域名当成文件名拼出「域名.exe」
    string low = LowerAscii(out);
    const char* exts[] = { ".exe", ".msi", ".zip", ".7z", ".rar" };
    bool hasExt = false;
    for (const char* e : exts) {
        size_t el = strlen(e);
        if (low.size() > el && low.compare(low.size() - el, el, e) == 0) hasExt = true;
    }
    if (!hasExt) return "CodeDateCreation_Setup.exe";
    return out;
}

// 版本号逐段数值比较（1.10 > 1.9；段数不同按 0 补齐）
static vector<long long> VerNums(const string& v) {
    vector<long long> out;
    long long cur = 0; bool has = false;
    for (char c : v) {
        if (c >= '0' && c <= '9') { cur = cur * 10 + (c - '0'); has = true; }
        else { if (has) out.push_back(cur); cur = 0; has = false; }
    }
    if (has) out.push_back(cur);
    return out;
}
static int VerCmp(const string& a, const string& b) {
    vector<long long> x = VerNums(a), y = VerNums(b);
    size_t n = x.size() > y.size() ? x.size() : y.size();
    for (size_t i = 0; i < n; ++i) {
        long long xa = (i < x.size()) ? x[i] : 0;
        long long ya = (i < y.size()) ? y[i] : 0;
        if (xa != ya) return xa > ya ? 1 : -1;
    }
    return 0;
}

// 更新信息清单的地址：优先 exe 同目录 cdc_update.cfg 的第一行有效行，
// 否则用内置云端地址 + /update.json（发布给用户时走这条，不需任何配置文件）。
// fromCfg 出参：这个地址是不是"用户手动指定的" —— 是的话就不再走云端动态接口，
// 完全尊重指定源（自测/内网部署要能锁死数据来源，否则真数据会把测试干扰掉）。
static string UpdateManifestUrl(bool* fromCfg = nullptr) {
    if (fromCfg) *fromCfg = false;
    string f = g_exeDir + "\\cdc_update.cfg";
    FILE* fp = OpenFileS(f.c_str(), "rb");
    if (fp) {
        string all; char buf[4096]; size_t n;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) all.append(buf, n);
        fclose(fp);
        size_t p = 0;
        while (p < all.size()) {
            size_t e = all.find('\n', p);
            string line = (e == string::npos) ? all.substr(p) : all.substr(p, e - p);
            p = (e == string::npos) ? all.size() : e + 1;
            size_t a = line.find_first_not_of(" \t\r\n");
            if (a == string::npos) continue;
            if (line[a] == '#') continue;
            size_t b = line.find_last_not_of(" \t\r\n");
            string v = line.substr(a, b - a + 1);
            if (v.rfind("http", 0) == 0) {
                if (fromCfg) *fromCfg = true;
                return v;
            }
        }
    }
    if (!g_cloudServer.empty()) return g_cloudServer + "/update.json";
    return "";
}

// 本地更新清单兜底：exe 同目录（或同级 cdc_server）的 update.json。
// 用途：① 云端清单还没上线时也能把更新链路跑通；② 内网/离线部署。
// 优先级低于云端——只有云端取不到（404 / 连不上 / 不是 JSON）才会用到。
static string ReadLocalUpdateManifest() {
    const char* cands[] = { "\\update.json", "\\cdc_server\\update.json" };
    for (const char* c : cands) {
        string p = g_exeDir + c;
        FILE* fp = OpenFileS(p.c_str(), "rb");
        if (!fp) continue;
        string all; char buf[4096]; size_t n;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) all.append(buf, n);
        fclose(fp);
        if (LooksLikeJson(all)) return all;
    }
    return "";
}

// 通用 HTTP GET：取清单走内存；下安装包传 fOut 落盘并按块回报进度
static bool WinHttpGetUrl(const string& url, DWORD& outStatus, string* memOut,
                          FILE* fOut, std::atomic<long long>* gotPtr,
                          long long* outTotal = nullptr) {
    outStatus = 0;
    CdcUrl u;
    if (!SplitUrl(url, u)) return false;
    HINTERNET hSess = WinHttpOpen(L"CDCDataGen/7.1.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return false;
    WinHttpSetTimeouts(hSess, 10000, 10000, 20000, 60000);
    HINTERNET hConn = WinHttpConnect(hSess, Utf8ToWstr(u.host).c_str(), (INTERNET_PORT)u.port, 0);
    if (!hConn) { WinHttpCloseHandle(hSess); return false; }
    DWORD flags = u.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"GET", Utf8ToWstr(u.path).c_str(), nullptr,
                                        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return false; }
    // 与云代理一致：忽略证书错误，免费证书/自签也能更新
    DWORD dwOpt = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
                | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    WinHttpSetOption(hReq, WINHTTP_OPTION_SECURITY_FLAGS, &dwOpt, sizeof(dwOpt));
    // 重定向自动跟随（安装包常放在 CDN 或二级路径）
    DWORD redir = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hReq, WINHTTP_OPTION_REDIRECT_POLICY, &redir, sizeof(redir));

    if (!WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, nullptr, 0, 0, 0) ||
        !WinHttpReceiveResponse(hReq, nullptr)) {
        WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess);
        return false;
    }
    DWORD status = 0, slen = sizeof(status);
    if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            nullptr, &status, &slen, nullptr))
        outStatus = status;
    // Content-Length → 供进度条用（拿不到就只显示"已下载多少"）
    if (outTotal) {
        *outTotal = 0;
        wchar_t cl[64] = {};
        DWORD cln = sizeof(cl);
        if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CONTENT_LENGTH, nullptr, cl, &cln, nullptr)) {
            long long t = wcstoll(cl, nullptr, 10);
            if (t > 0) *outTotal = t;
        }
    }
    long long got = 0;
    DWORD avail = 0;
    while (WinHttpQueryDataAvailable(hReq, &avail) && avail) {
        string buf(avail, 0);
        DWORD rd = 0;
        if (!WinHttpReadData(hReq, &buf[0], avail, &rd) || rd == 0) break;
        if (fOut) fwrite(buf.data(), 1, (size_t)rd, fOut);
        if (memOut) memOut->append(buf.data(), (size_t)rd);
        got += rd;
        if (gotPtr) gotPtr->store(got);
        if (got > (long long)2 * 1024 * 1024 * 1024) break;   // 2GB 护栏
    }
    WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess);
    if (gotPtr) gotPtr->store(got);
    return true;
}

// 探测一个 URL 是否存在、多大（只取 1 字节：Range: bytes=0-0）。
// 用 GET+Range 而不是 HEAD —— 不少面板/CDN 会直接拒掉 HEAD（405）。
static DWORD WinHttpProbeUrl(const string& url, long long& outLen) {
    outLen = 0;
    CdcUrl u;
    if (!SplitUrl(url, u)) return 0;
    HINTERNET hSess = WinHttpOpen(L"CDCDataGen/7.1.0", WINHTTP_ACCESS_TYPE_NO_PROXY,
                                  WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSess) return 0;
    WinHttpSetTimeouts(hSess, 8000, 8000, 8000, 15000);
    HINTERNET hConn = WinHttpConnect(hSess, Utf8ToWstr(u.host).c_str(), (INTERNET_PORT)u.port, 0);
    if (!hConn) { WinHttpCloseHandle(hSess); return 0; }
    DWORD flags = u.https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET hReq = WinHttpOpenRequest(hConn, L"GET", Utf8ToWstr(u.path).c_str(), nullptr,
                                        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!hReq) { WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess); return 0; }
    DWORD dwOpt = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID
                | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
    WinHttpSetOption(hReq, WINHTTP_OPTION_SECURITY_FLAGS, &dwOpt, sizeof(dwOpt));
    DWORD redir = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hReq, WINHTTP_OPTION_REDIRECT_POLICY, &redir, sizeof(redir));
    DWORD st = 0;
    wstring rng = L"Range: bytes=0-0";
    if (WinHttpSendRequest(hReq, rng.c_str(), (DWORD)-1L, nullptr, 0, 0, 0) &&
        WinHttpReceiveResponse(hReq, nullptr)) {
        DWORD sl = sizeof(st);
        WinHttpQueryHeaders(hReq, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                            nullptr, &st, &sl, nullptr);
        // 206 时正文只有 1 字节，总大小在 Content-Range: bytes 0-0/47000000 里
        wchar_t cr[160] = {};
        DWORD crn = sizeof(cr);
        if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CUSTOM, L"Content-Range", cr, &crn, nullptr)) {
            wstring w(cr);
            size_t s = w.find(L'/');
            if (s != wstring::npos) {
                long long t = wcstoll(w.c_str() + s + 1, nullptr, 10);
                if (t > 0) outLen = t;
            }
        }
        if (outLen == 0) {
            wchar_t cl[64] = {};
            DWORD cln = sizeof(cl);
            if (WinHttpQueryHeaders(hReq, WINHTTP_QUERY_CONTENT_LENGTH, nullptr, cl, &cln, nullptr)) {
                long long t = wcstoll(cl, nullptr, 10);
                if (t > 0) outLen = t;
            }
        }
    }
    // 不读正文直接关句柄：Range 只请求了 1 字节，服务器若忽略 Range 也只会白传一点
    WinHttpCloseHandle(hReq); WinHttpCloseHandle(hConn); WinHttpCloseHandle(hSess);
    return st;
}

// 把"清单里的下载网址"解析成真正能下载的安装包直链。
// 正常情况原样返回；只有当网址不像安装包（例如只填了官网首页）时，
// 才按约定去几个候选位置探一探 —— 至少 1MB 才认，避免探到 404 页/小图标。
static string ResolveInstallerUrl(const string& rawUrl, const string& ver) {
    if (rawUrl.empty()) return rawUrl;
    if (LooksLikeInstallerUrl(rawUrl)) return rawUrl;
    string base = rawUrl;
    while (!base.empty() && base.back() == '/') base.pop_back();
    vector<string> cands;
    cands.push_back(base + "/CodeDateCreation" + ver + "_Setup.exe");
    cands.push_back(base + "/dist/CodeDateCreation" + ver + "_Setup.exe");
    if (!g_cloudServer.empty()) {
        string cs = g_cloudServer;
        while (!cs.empty() && cs.back() == '/') cs.pop_back();
        cands.push_back(cs + "/dist/CodeDateCreation" + ver + "_Setup.exe");
        cands.push_back(cs + "/CodeDateCreation" + ver + "_Setup.exe");
    }
    DebugLog("  update: 下载网址不像安装包直链（" + Sanitize(rawUrl.substr(0, 80)) + "），开始探测候选");
    for (const string& c : cands) {
        long long len = 0;
        DWORD st = WinHttpProbeUrl(c, len);
        DebugLog("  update: 候选 " + c + " → HTTP " + to_string(st) + " len=" + to_string(len));
        if ((st == 200 || st == 206) && len > 1024 * 1024) return c;
    }
    return rawUrl;   // 都没探到：保持原样，由下载环节给出明确报错
}

// 下载进度用的全局量（原子，跨线程读写安全）
static std::atomic<long long> g_updGot{0};
static std::atomic<long long> g_updTotal{0};

struct UpdateState {
    std::mutex mu;
    string state = "idle";        // idle | downloading | done | error
    string err;
    string fileW;                 // 下载落地的完整路径（UTF-8）
    bool checked = false;
    bool hasUpdate = false;
    string url;                   // 安装包直链：只留在后端，不下发给前端
    string newVer, notice, notes, force, sha256;
    long long size = 0;
};
static UpdateState g_upd;

// ---------- 是否已安装（注册表优先，目录兜底）----------
static string ReadRegStr(HKEY hk, const wchar_t* sub, const wchar_t* val) {
    HKEY h;
    if (RegOpenKeyExW(hk, sub, 0, KEY_READ | KEY_WOW64_64KEY, &h) != ERROR_SUCCESS) {
        if (RegOpenKeyExW(hk, sub, 0, KEY_READ, &h) != ERROR_SUCCESS) return "";
    }
    wchar_t buf[1024] = {};
    DWORD sz = sizeof(buf), type = 0;
    LONG r = RegQueryValueExW(h, val, nullptr, &type, (LPBYTE)buf, &sz);
    RegCloseKey(h);
    if (r != ERROR_SUCCESS) return "";
    return WstrToUtf8(buf);
}
// 判断一个卸载项是否"真的还装着"。
// 【2026-09-23 事故】本机 `HKCU\...\Uninstall\CDC4 数据生成器_is1` 是 CDC4 v4.0.0
// 卸载后残留的空壳（安装目录 %LOCALAPPDATA%\Programs\CDC4 早已不存在、unins000.exe
// 也不在了），但只看 DisplayName 就会把它当成"已安装"，于是：
//   /api/installed 返回 installed=true / version=4.0.0  → 前端弹"检测到已安装 v4.0.0"，
//   更新的"是否保留配置"也问错了对象。
// 规则：InstallLocation 目录存在，或 UninstallString 里的卸载器存在，才算真装着。
static bool UninstallEntryAlive(HKEY root, const wstring& keyPath) {
    string loc = ReadRegStr(root, keyPath.c_str(), L"InstallLocation");
    if (!loc.empty()) {
        size_t b = loc.find_last_not_of("\\/ \t");
        if (b != string::npos) loc = loc.substr(0, b + 1);
        if (DirExists(Utf8ToWstr(loc))) return true;
    }
    string un = ReadRegStr(root, keyPath.c_str(), L"UninstallString");
    if (!un.empty()) {
        size_t a = un.find_first_not_of(" \t");
        if (a != string::npos) {
            string rest = un.substr(a), exe;
            if (rest[0] == '"') {                       // "C:\...\unins000.exe" /SILENT
                size_t e = rest.find('"', 1);
                exe = (e == string::npos) ? rest.substr(1) : rest.substr(1, e - 1);
            } else {                                    // C:\...\unins000.exe /SILENT
                size_t sp = rest.find(' ');
                exe = (sp == string::npos) ? rest : rest.substr(0, sp);
            }
            if (!exe.empty() && FileExists(Utf8ToWstr(exe))) return true;
        }
    }
    return false;
}

static bool ScanUninstallRoot(HKEY root, string& ver, string& dir) {
    const wchar_t* sub = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall";
    HKEY hk;
    if (RegOpenKeyExW(root, sub, 0, KEY_READ | KEY_WOW64_64KEY, &hk) != ERROR_SUCCESS) {
        if (RegOpenKeyExW(root, sub, 0, KEY_READ, &hk) != ERROR_SUCCESS) return false;
    }
    bool found = false;
    for (DWORD i = 0; !found; ++i) {
        wchar_t name[512] = {};
        DWORD nlen = 512;
        if (RegEnumKeyExW(hk, i, name, &nlen, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
        wstring ws = wstring(sub) + L"\\" + name;
        string dn = ReadRegStr(root, ws.c_str(), L"DisplayName");
        bool hit = false;
        // ① 固定 AppId 前缀（iss 里的 AppId）
        if (wcsncmp(name, L"{B3A7E91F", 9) == 0) hit = true;
        // ② 展示名匹配（改过 AppName 也认得）：含 CDC 且含"数据生成器"/"DataGen"
        if (!hit && dn.find("CDC") != string::npos &&
            (dn.find("数据生成器") != string::npos || dn.find("DataGen") != string::npos)) hit = true;
        if (!hit) continue;
        // 命中名字还不够——必须是"真的还装着"的那个（跳过卸载残留）
        if (!UninstallEntryAlive(root, ws)) continue;
        ver = ReadRegStr(root, ws.c_str(), L"DisplayVersion");
        dir = ReadRegStr(root, ws.c_str(), L"InstallLocation");
        found = true;
    }
    RegCloseKey(hk);
    return found;
}
static bool DetectInstalled(string& ver, string& dir, string& exePath, string& source) {
    ver.clear(); dir.clear(); exePath.clear(); source.clear();
    if (ScanUninstallRoot(HKEY_LOCAL_MACHINE, ver, dir)) {
        source = "registry";
    } else if (ScanUninstallRoot(HKEY_CURRENT_USER, ver, dir)) {
        source = "registry-user";
    }
    // 目录兜底：iss 的 DefaultDirName={autopf}\CDC，主程序改名 CDC.exe
    wchar_t pf[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"ProgramFiles", pf, MAX_PATH)) {
        wstring cand = wstring(pf) + L"\\CDC\\CDC.exe";
        if (FileExists(cand)) {
            if (exePath.empty()) exePath = WstrToUtf8(cand);
            if (source.empty()) source = "dir";
            if (dir.empty()) dir = WstrToUtf8(wstring(pf) + L"\\CDC");
        }
    }
    if (exePath.empty() && !dir.empty()) {
        wstring cand = Utf8ToWstr(dir) + L"\\CDC.exe";
        if (FileExists(cand)) exePath = WstrToUtf8(cand);
    }
    return !source.empty();
}

// ---------- 按时间戳找安装包 ----------
// 匹配规则（不区分大小写、不看目录只看文件名）：
//   扩展名 .exe、且不是卸载器 unins*、且（含 codedatecreation）或（含 cdc 且含 setup）
static bool NameLooksLikeInstaller(const wstring& name) {
    wstring n = name;
    for (size_t i = 0; i < n.size(); ++i) n[i] = (wchar_t)towlower(n[i]);
    if (n.size() < 5 || n.substr(n.size() - 4) != L".exe") return false;
    if (n.rfind(L"unins", 0) == 0) return false;
    bool hasCdcName = (n.find(L"codedatecreation") != wstring::npos);
    bool hasCdcSetup = (n.find(L"cdc") != wstring::npos && n.find(L"setup") != wstring::npos);
    return hasCdcName || hasCdcSetup;
}
// 在候选目录里取「最近 windowHours 小时内、mtime 最新」的安装包
static wstring FindNewestInstaller(int windowHours = 72) {
    vector<wstring> dirs;
    wchar_t tmp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tmp);
    dirs.push_back(wstring(tmp) + L"cdc_update");          // 我们自己下载到这里
    dirs.push_back(GetDownloadPath());                      // 浏览器手动下载
    dirs.push_back(GetDesktopPath());
    wchar_t up[MAX_PATH] = {};
    if (GetEnvironmentVariableW(L"USERPROFILE", up, MAX_PATH))
        dirs.push_back(wstring(up) + L"\\Downloads");

    ULARGE_INTEGER now; FILETIME ftNow; GetSystemTimeAsFileTime(&ftNow);
    now.LowPart = ftNow.dwLowDateTime; now.HighPart = ftNow.dwHighDateTime;
    unsigned long long cutoff = now.QuadPart - (unsigned long long)windowHours * 3600ULL * 10000000ULL;

    wstring best; unsigned long long bestTime = 0;
    for (const wstring& d : dirs) {
        if (d.empty() || !DirExists(d)) continue;
        wstring pat = d + L"\\*";
        WIN32_FIND_DATAW fd = {};
        HANDLE h = FindFirstFileW(pat.c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            if (!NameLooksLikeInstaller(fd.cFileName)) continue;
            ULARGE_INTEGER m;
            m.LowPart = fd.ftLastWriteTime.dwLowDateTime;
            m.HighPart = fd.ftLastWriteTime.dwHighDateTime;
            if (m.QuadPart < cutoff) continue;
            if (m.QuadPart > bestTime) { bestTime = m.QuadPart; best = d + L"\\" + fd.cFileName; }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    return best;
}

// ---------- 后台下载线程 ----------
static unsigned __stdcall UpdateDownloadThread(void*) {
    string url, ver;
    {
        std::lock_guard<std::mutex> lk(g_upd.mu);
        url = g_upd.url;
        ver = g_upd.newVer;
    }
    if (url.empty()) {
        std::lock_guard<std::mutex> lk(g_upd.mu);
        g_upd.state = "error"; g_upd.err = "清单里没有下载地址";
        return 0;
    }
    // 清单里的网址可能只填了官网/目录 → 先解析成真正的安装包直链再下
    url = ResolveInstallerUrl(url, ver);
    if (!LooksLikeInstallerUrl(url)) {
        std::lock_guard<std::mutex> lk(g_upd.mu);
        g_upd.state = "error";
        g_upd.err = "下载网址没有指向安装包，且未能自动找到。请在云端后台「发布更新」里"
                    "把下载网址填成安装包直链（以 .exe 结尾）";
        DebugLog("  update: 网址解析失败 " + Sanitize(url.substr(0, 90)));
        return 0;
    }
    wchar_t tmp[MAX_PATH] = {};
    GetTempPathW(MAX_PATH, tmp);
    wstring dir = wstring(tmp) + L"cdc_update";
    EnsureDir(dir);
    wstring dst = dir + L"\\" + Utf8ToWstr(UrlFileName(url));

    g_updGot.store(0);
    g_updTotal.store(0);
    // 同目录先清掉旧包，避免"按时间戳取最新"时选中上一次的旧文件
    DeleteFileW(dst.c_str());

    FILE* f = OpenFileWS(dst.c_str(), L"wb");
    if (!f) {
        std::lock_guard<std::mutex> lk(g_upd.mu);
        g_upd.state = "error"; g_upd.err = "无法写入下载目录";
        return 0;
    }
    DebugLog("  update: 开始下载 " + WstrToUtf8(dst));
    DWORD st = 0;
    long long total = 0;
    bool ok = WinHttpGetUrl(url, st, nullptr, f, &g_updGot, &total);
    fclose(f);
    if (total > 0) g_updTotal.store(total);

    std::lock_guard<std::mutex> lk(g_upd.mu);
    long long got = g_updGot.load();
    if (ok && got > 0 && (st == 200 || st == 206)) {
        // 校验落盘的是不是真的可执行文件。
        // 【为什么必须校验】后台"下载网址"若填成官网，会下载到一个 HTML 页面；
        // 不校验就会当成"下载成功"，然后"按文件名找安装包"永远找不到，
        // 用户只看到"下载完了但没反应"，查不出原因。
        bool isPE = false;
        FILE* cf = OpenFileWS(dst.c_str(), L"rb");
        if (cf) {
            char mz[2] = {};
            size_t rd = fread(mz, 1, 2, cf);
            fclose(cf);
            isPE = (rd == 2 && mz[0] == 'M' && mz[1] == 'Z');
        }
        if (!isPE) {
            g_upd.state = "error";
            g_upd.err = "下载到的不是安装包（像是网页）。请在云端后台「发布更新」里"
                        "把下载网址改成安装包直链（以 .exe 结尾）";
            DeleteFileW(dst.c_str());
            DebugLog("  update: 落盘文件非 PE，已删除");
        } else {
            g_upd.state = "done";
            g_upd.fileW = WstrToUtf8(dst);
            g_upd.err.clear();
            if (g_updTotal.load() <= 0) g_updTotal.store(got);
            DebugLog("  update: 下载完成 " + to_string(got) + " B");
        }
    } else {
        g_upd.state = "error";
        g_upd.err = ok ? ("下载失败，HTTP " + to_string(st)) : "网络连接失败";
        DeleteFileW(dst.c_str());
        DebugLog("  update: " + g_upd.err);
    }
    return 0;
}

// ---------- 对外接口 ----------
static string ApiUpdateCheck() {
    string instVer, instDir, instExe, instSrc;
    bool installed = DetectInstalled(instVer, instDir, instExe, instSrc);

    string res = "{";
    res += "\"ok\":true,";
    res += "\"current\":\"" + string(APP_VER) + "\",";
    res += "\"site\":\"https://cdc.mjhcsp.xyz\",";      // 官网：打不开更新时的兜底入口
    res += "\"installed\":" + string(installed ? "true" : "false") + ",";
    res += "\"installedVersion\":\"" + JsonEsc(instVer) + "\",";
    res += "\"installDir\":\"" + JsonEsc(instDir) + "\",";

    // ---------- 取更新清单：三级回退 ----------
    // 【为什么要有第①级】后台管理页的「发布更新」只写数据库 updates 表，
    //   并不会生成 update.json 文件。客户端以前只读那个静态文件（没上传就 404），
    //   于是「后台发布成功、客户端收不到」。op=version 是读数据库的，必须走它。
    // ① 云端动态接口 op=version —— 后台发布后【立刻】生效，无需传任何文件（主通道）
    // ② 云端静态 update.json —— 老流程（手动上传文件），保留兼容
    // ③ exe 同目录 update.json —— 离线 / 内网 / 云端全挂时的兜底
    string ver, url, notice, notes, force, sha, source;
    long long size = 0;
    string failMsg = "未配置更新地址";

    // cdc_update.cfg 指定了更新源（自测 / 内网 / 私有部署）→ 只认它，不碰云端动态接口。
    // 发布给用户的安装包里没有这个文件，所以线上永远是"动态接口优先"。
    bool customSrc = false;
    string manifestUrl = UpdateManifestUrl(&customSrc);

    if (!customSrc && !g_cloudServer.empty()) {
        string r = CloudProxyForward("/api/activate", "", "op=version",
                                     "application/x-www-form-urlencoded");
        if (LooksLikeJson(r)) {
            string v0 = JsonStr(r, "version");
            if (!v0.empty()) {
                ver = v0;
                url = JsonStr(r, "url");
                notice = JsonStr(r, "notice");
                notes = JsonStr(r, "notes");
                sha = JsonStr(r, "sha256");
                force = JsonValue(r, "force");
                size = atoll(JsonValue(r, "size").c_str());
                source = "cloud-api";
                DebugLog("  update: 动态接口命中 version=" + ver);
            } else {
                failMsg = "云端未返回版本号";
            }
        } else {
            failMsg = "云端接口无响应";
        }
    }

    if (source.empty()) {
        if (!manifestUrl.empty()) {            // 复用上面已解析好的地址（避免重复读 cfg）
            DWORD st = 0;
            string body2;
            if (WinHttpGetUrl(manifestUrl, st, &body2, nullptr, nullptr) && LooksLikeJson(body2)) {
                string v0 = JsonStr(body2, "version");
                if (!v0.empty()) {
                    ver = v0;
                    url = JsonStr(body2, "url");
                    notice = JsonStr(body2, "notice");
                    notes = JsonStr(body2, "notes");
                    sha = JsonStr(body2, "sha256");
                    force = JsonValue(body2, "force");
                    size = atoll(JsonValue(body2, "size").c_str());
                    source = "cloud-json";
                    DebugLog("  update: 静态清单命中 version=" + ver);
                }
            } else if (st != 0) {
                failMsg = "更新信息不可用（HTTP " + to_string(st) + "）";
            }
        }
    }

    if (source.empty()) {
        string local = ReadLocalUpdateManifest();
        if (!local.empty() && LooksLikeJson(local)) {
            string v0 = JsonStr(local, "version");
            if (!v0.empty()) {
                ver = v0;
                url = JsonStr(local, "url");
                notice = JsonStr(local, "notice");
                notes = JsonStr(local, "notes");
                sha = JsonStr(local, "sha256");
                force = JsonValue(local, "force");
                size = atoll(JsonValue(local, "size").c_str());
                source = "local";
                DebugLog("  update: 本地清单兜底 version=" + ver);
            }
        }
    }

    if (source.empty()) {
        res += "\"hasUpdate\":false,\"manifest\":false,\"source\":\"none\",";
        res += "\"message\":\"" + JsonEsc(failMsg) + "\"}";
        std::lock_guard<std::mutex> lk(g_upd.mu);
        g_upd.checked = true; g_upd.hasUpdate = false;
        return res;
    }
    bool has = (!ver.empty() && VerCmp(ver, APP_VER) > 0);

    {
        std::lock_guard<std::mutex> lk(g_upd.mu);
        g_upd.checked = true;
        g_upd.hasUpdate = has;
        g_upd.url = url;              // 只有后端知道直链，不下发给页面
        g_upd.newVer = ver; g_upd.notice = notice; g_upd.notes = notes;
        g_upd.force = force; g_upd.sha256 = sha; g_upd.size = size;
        if (!has) { g_upd.state = "idle"; g_upd.fileW.clear(); g_upd.err.clear(); }
    }
    if (has) g_updTotal.store(size);
    DebugLog("  update: 检查完成 current=" + string(APP_VER) + " latest=" + ver +
             " hasUpdate=" + string(has ? "1" : "0"));

    res += "\"manifest\":true,";
    res += "\"source\":\"" + JsonEsc(source) + "\",";   // cloud-api / cloud-json / local（排障用）
    res += "\"hasUpdate\":" + string(has ? "true" : "false") + ",";
    res += "\"version\":\"" + JsonEsc(ver) + "\",";
    res += "\"notice\":\"" + JsonEsc(notice) + "\",";
    res += "\"notes\":\"" + JsonEsc(notes) + "\",";
    res += "\"force\":" + string((force == "true" || force == "1") ? "true" : "false") + ",";
    res += "\"size\":" + to_string(size);
    res += "}";
    return res;
}

static string ApiUpdateDownload() {
    {
        std::lock_guard<std::mutex> lk(g_upd.mu);
        if (g_upd.state == "downloading")
            return "{\"ok\":true,\"state\":\"downloading\"}";
        if (!g_upd.checked) return "{\"ok\":false,\"err\":\"请先检查更新\"}";
        if (g_upd.url.empty()) return "{\"ok\":false,\"err\":\"更新信息里没有下载地址\"}";
        // 已有下好的同一版本就直接复用，不重复下载
        if (g_upd.state == "done" && !g_upd.fileW.empty() && FileExists(Utf8ToWstr(g_upd.fileW)))
            return "{\"ok\":true,\"state\":\"done\",\"reused\":true}";
        g_upd.state = "downloading";
        g_upd.err.clear();
    }
    g_updGot.store(0);
    _beginthreadex(nullptr, 0, UpdateDownloadThread, nullptr, 0, nullptr);
    return "{\"ok\":true,\"state\":\"downloading\"}";
}

static string ApiUpdateStatus() {
    std::lock_guard<std::mutex> lk(g_upd.mu);
    long long got = g_updGot.load(), total = g_updTotal.load();
    if (total <= 0) total = g_upd.size;
    int pct = (total > 0) ? (int)(got * 100 / total) : 0;
    if (pct > 100) pct = 100;
    string res = "{";
    res += "\"ok\":true,";
    res += "\"state\":\"" + JsonEsc(g_upd.state) + "\",";
    res += "\"got\":" + to_string(got) + ",";
    res += "\"total\":" + to_string(total) + ",";
    res += "\"percent\":" + to_string(pct) + ",";
    res += "\"hasUpdate\":" + string(g_upd.hasUpdate ? "true" : "false") + ",";
    res += "\"version\":\"" + JsonEsc(g_upd.newVer) + "\",";
    res += "\"file\":\"" + JsonEsc(g_upd.fileW) + "\",";
    res += "\"name\":\"" + JsonEsc(g_upd.fileW.empty() ? "" : BaseNameUtf8(g_upd.fileW)) + "\",";
    res += "\"err\":\"" + JsonEsc(g_upd.err) + "\"";
    res += "}";
    return res;
}

// 打开安装包：优先用刚下载好的那个；没有就"按时间戳"在下载目录里找最新的那个。
// keepCfg=true 时给安装包传 /KEEPCFG=1 —— iss 侧据此保留用户配置与数据。
static string ApiUpdateLaunch(const string& body) {
    bool keep = (JsonValue(body, "keepCfg") == "true" || JsonValue(body, "keepCfg") == "1");
    string given = JsonStr(body, "path");      // 可选：指定某个具体的安装包

    wstring target;
    if (!given.empty() && FileExists(Utf8ToWstr(given))) target = Utf8ToWstr(given);
    if (target.empty()) {
        std::lock_guard<std::mutex> lk(g_upd.mu);
        if (!g_upd.fileW.empty() && FileExists(Utf8ToWstr(g_upd.fileW)))
            target = Utf8ToWstr(g_upd.fileW);
    }
    if (target.empty()) target = FindNewestInstaller();
    if (target.empty())
        return "{\"ok\":false,\"err\":\"no_installer\",\"message\":\"没找到安装包，请到下载目录里手动运行\"}";

    wstring params = keep ? L"/KEEPCFG=1" : L"";
    HINSTANCE hr = ShellExecuteW(nullptr, L"open", target.c_str(), params.c_str(), nullptr, SW_SHOWNORMAL);
    if ((intptr_t)hr <= 32)
        return "{\"ok\":false,\"err\":\"launch_failed\",\"message\":\"无法启动安装包（可能被安全软件拦截）\"}";
    DebugLog("  update: 已启动安装包 " + WstrToUtf8(target) + (keep ? "  /KEEPCFG=1" : ""));

    string res = "{";
    res += "\"ok\":true,";
    res += "\"path\":\"" + JsonEsc(WstrToUtf8(target)) + "\",";
    res += "\"name\":\"" + JsonEsc(BaseNameUtf8(WstrToUtf8(target))) + "\",";
    res += "\"keepCfg\":" + string(keep ? "true" : "false");
    res += "}";
    return res;
}

// 本机是否已安装（前端在弹「替换并保留配置」之前调它）
static string ApiInstalled() {
    string v, d, e, s;
    bool inst = DetectInstalled(v, d, e, s);
    string res = "{";
    res += "\"ok\":true,";
    res += "\"installed\":" + string(inst ? "true" : "false") + ",";
    res += "\"version\":\"" + JsonEsc(v) + "\",";
    res += "\"dir\":\"" + JsonEsc(d) + "\",";
    res += "\"exe\":\"" + JsonEsc(e) + "\",";
    res += "\"source\":\"" + JsonEsc(s) + "\"";
    res += "}";
    return res;
}

static string ServeFile(const string& path) {
    FILE* f = OpenFileS(path.c_str(), "rb");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    string s((size_t)sz, 0);
    if (sz > 0) fread(&s[0], 1, sz, f);
    fclose(f);
    return s;
}

// 从 exe 内嵌资源读取数据（前端已编译进 exe，运行时无需磁盘文件）
static string ReadRes(const char* name) {
    HMODULE h = GetModuleHandleW(nullptr);
    HRSRC hr = FindResourceA(h, name, RT_RCDATA);
    if (!hr) return "";
    HGLOBAL hg = LoadResource(h, hr);
    if (!hg) return "";
    DWORD sz = SizeofResource(h, hr);
    const char* p = (const char*)LockResource(hg);
    return (p && sz) ? string(p, (size_t)sz) : string();
}

// 前端路由：优先取内嵌资源，取不到再回退磁盘（便于开发期调试）
static string ServeFrontend(const string& path) {
    const char* res = nullptr;
    if (path == "/" || path == "/index.html")        res = "IDR_INDEX";
    else if (path == "/cpp_bridge.js")               res = "IDR_BRIDGE";
    else if (path == "/app.css")                     res = "IDR_CSS";
    if (res) {
        string s = ReadRes(res);
        if (!s.empty()) return s;
    }
    if (path.size() > 1) {
        string s = ServeFile(g_exeDir + path);
        if (!s.empty()) return s;
    }
    return "";
}

static string ContentTypeOf(const string& path) {
    if (path.size() > 5 && path.substr(path.size() - 5) == ".html") return "text/html; charset=utf-8";
    if (path.size() > 4 && path.substr(path.size() - 4) == ".css")  return "text/css; charset=utf-8";
    if (path.size() > 3 && path.substr(path.size() - 3) == ".js")   return "application/javascript; charset=utf-8";
    if (path.size() > 4 && path.substr(path.size() - 4) == ".png")  return "image/png";
    if (path.size() > 4 && path.substr(path.size() - 4) == ".ico")  return "image/x-icon";
    return "application/octet-stream";
}

static string HandleRequest(const HttpReq& req) {
    // 路由
    if (req.path == "/api/ping") {
        return "{\"ok\":true,\"ver\":\"" + string(APP_VER) + "\",\"backend\":\"cpp\",\"time\":" + to_string((long long)time(nullptr)) + "}";
    }
    if (req.path == "/api/info") return ApiInfo();
    if (req.path == "/api/maketsdir") return ApiMakeTsDir();
    if (req.path == "/api/gppdetect") return ApiGppDetect();
    if (req.path == "/api/setgpp") return ApiSetGpp(req.body);
    if (req.path == "/api/genlib") return ApiGenLib();
    if (req.path == "/api/genrun") return ApiGenRun(req.body);
    if (req.path == "/api/generate") return ApiGenerate(req.body);
    if (req.path == "/api/writefiles") return ApiWriteFiles(req.body);
    if (req.path == "/api/bigfile") return ApiBigFile(req.body);
    if (req.path == "/api/rename") return ApiRename(req.body);
    if (req.path == "/api/compile") return ApiCompile(req.body);
    if (req.path == "/api/zip") return ApiZip(req.body);
    if (req.path == "/api/savezip") return ApiSaveZip(req.body);
    if (req.path == "/api/tool") return ApiTool(req.body);
    if (req.path == "/api/browse") return ApiBrowse(req.body);
    // 自动更新：检查清单 / 后台下载 / 下载进度 / 打开安装包 / 是否已安装
    if (req.path == "/api/update")     return ApiUpdateCheck();
    if (req.path == "/api/updatedl")   return ApiUpdateDownload();
    if (req.path == "/api/updstatus")  return ApiUpdateStatus();
    if (req.path == "/api/updlaunch")  return ApiUpdateLaunch(req.body);
    if (req.path == "/api/installed")  return ApiInstalled();
    // 云端代理：前端私信/账号/AI 统一走这里，由本后端转发到内置（或 cdc_srv.dat）的服务器
    if (req.path.rfind("/api/cloud", 0) == 0) return ApiCloud(req);

    // 前端静态资源：优先取 exe 内嵌资源，取不到再回退磁盘
    if (req.path == "/" || req.path == "/index.html" ||
        req.path == "/cpp_bridge.js" || req.path == "/app.css") {
        string content = ServeFrontend(req.path);
        if (content.empty()) {
            content = "<!DOCTYPE html><html><head><meta charset=\"utf-8\"><title>CDC</title></head>"
                      "<body style=\"background:#0a0e1a;color:#e8edf5;font-family:sans-serif;display:grid;place-items:center;height:100vh\">"
                      "<div><h2>CDC 后端已启动</h2><p>未找到内嵌前端资源，请用 build.bat 重新生成 cdc_backend.exe。</p></div></body></html>";
        }
        return content;
    }

    // 其他静态资源（从 exe 目录读取，作为兜底）
    if (req.path.size() > 1) {
        string p = g_exeDir + req.path;
        string content = ServeFile(p);
        if (!content.empty()) return content;
    }
    return "{\"ok\":false,\"error\":\"not found\"}";
}

static void SendResponse(SOCKET sock, const string& body, const string& contentType, int status = 200) {
    string stext = (status == 200 ? "OK" : (status == 204 ? "No Content" : "Not Found"));
    string head = "HTTP/1.1 " + to_string(status) + " " + stext + "\r\n"
                  "Content-Type: " + contentType + "\r\n"
                  "Content-Length: " + to_string(body.size()) + "\r\n"
                  "Connection: close\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                  "Access-Control-Allow-Headers: Content-Type\r\n"
                  "Cache-Control: no-cache\r\n"
                  "\r\n";
    string full = head + body;
    send(sock, full.data(), (int)full.size(), 0);
}

// CORS 预检（浏览器对 application/json 的 POST 会先发 OPTIONS）
static void SendPreflight(SOCKET sock) {
    string head = "HTTP/1.1 204 No Content\r\n"
                  "Access-Control-Allow-Origin: *\r\n"
                  "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                  "Access-Control-Allow-Headers: Content-Type\r\n"
                  "Access-Control-Max-Age: 86400\r\n"
                  "Connection: close\r\n"
                  "\r\n";
    send(sock, head.data(), (int)head.size(), 0);
}

static unsigned __stdcall ClientThread(void* arg) {
    SOCKET sock = (SOCKET)(intptr_t)arg;
    char buf[16384];
    string raw;
    int n = 0;
    // 读请求头 + body（简化：一次性读入）
    while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
        raw.append(buf, n);
        // 检查是否包含完整 body（Content-Length）
        size_t hdrEnd = raw.find("\r\n\r\n");
        if (hdrEnd != string::npos) {
            string headers = raw.substr(0, hdrEnd);
            size_t clPos = headers.find("Content-Length:");
            int cl = 0;
            if (clPos != string::npos) {
                cl = atoi(headers.c_str() + clPos + 15);
            }
            size_t bodyStart = hdrEnd + 4;
            if (raw.size() - bodyStart >= (size_t)cl) break;
        } else if (raw.size() > 64 * 1024) break;
    }

    // 解析请求行（方法 + 路径）
    string method = "GET", path = "/";
    size_t lineEnd = raw.find("\r\n");
    if (lineEnd != string::npos) {
        string line = raw.substr(0, lineEnd);
        size_t sp1 = line.find(' ');
        size_t sp2 = line.find(' ', sp1 + 1);
        if (sp1 != string::npos && sp2 != string::npos) {
            method = line.substr(0, sp1);
            path = UrlDecode(line.substr(sp1 + 1, sp2 - sp1 - 1));
        }
    }

    // CORS 预检：浏览器对 application/json 的 POST 会先发 OPTIONS，必须应答
    if (method == "OPTIONS") {
        SendPreflight(sock);
        shutdown(sock, SD_BOTH);
        closesocket(sock);
        return 0;
    }

    HttpReq req;
    req.method = method;
    req.path = path;
    size_t hdrEnd0 = raw.find("\r\n\r\n");
    req.headers = (hdrEnd0 != string::npos) ? raw.substr(0, hdrEnd0) : "";
    size_t hdrEnd = raw.find("\r\n\r\n");
    if (hdrEnd != string::npos && raw.size() > hdrEnd + 4) {
        req.body = raw.substr(hdrEnd + 4);
    }

    // 防止单个请求异常拖垮整个服务进程
    string content;
    try {
        content = HandleRequest(req);
    } catch (...) {
        content = "{\"ok\":false,\"error\":\"server_internal\"}";
    }

    string ct = "application/json; charset=utf-8";
    if (req.path == "/" || req.path == "/index.html") ct = "text/html; charset=utf-8";
    else if (req.path.size() > 1 && req.path[0] == '/' && req.path.find("/api/") != 0) ct = ContentTypeOf(req.path);

    SendResponse(sock, content, ct);
    shutdown(sock, SD_BOTH);
    closesocket(sock);
    return 0;
}

static string GetExeDir() {
    wchar_t p[MAX_PATH];
    GetModuleFileNameW(nullptr, p, MAX_PATH);
    wstring s(p);
    size_t pos = s.find_last_of(L"\\/");
    if (pos != string::npos) s = s.substr(0, pos);
    return WstrToUtf8(s);
}

// 探测 127.0.0.1:PORT 是否已被监听（另一实例在跑？）
static bool IsPortListening() {
    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);
    if (s == INVALID_SOCKET) return false;
    sockaddr_in a = {};
    a.sin_family = AF_INET;
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    a.sin_port = htons(PORT);
    bool busy = (connect(s, (sockaddr*)&a, sizeof(a)) == 0);
    closesocket(s);
    return busy;
}

// 查找 Chrome / Edge，用于 --app 独立窗口（不跳浏览器标签页）
static wstring FindBrowserExe() {
    // 1. 注册表 App Paths（Chrome / Edge）
    const wchar_t* keys[] = {
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe",
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\msedge.exe",
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\App Paths\\chrome.exe",
        L"SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\App Paths\\msedge.exe",
    };
    for (auto k : keys) {
        HKEY hk = nullptr;
        if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, k, 0, KEY_READ, &hk) == ERROR_SUCCESS) {
            wchar_t buf[MAX_PATH] = {};
            DWORD sz = sizeof(buf), type = 0;
            if (RegQueryValueExW(hk, nullptr, nullptr, &type, (LPBYTE)buf, &sz) == ERROR_SUCCESS && buf[0]) {
                RegCloseKey(hk);
                if (FileExists(buf)) return buf;
            }
            RegCloseKey(hk);
        }
    }
    // 2. 常见安装路径（Chrome 优先，--app 行为更稳定；Edge 兜底）
    const wchar_t* cand[] = {
        L"C:\\Program Files\\Google\\Chrome\\Application\\chrome.exe",
        L"C:\\Program Files (x86)\\Google\\Chrome\\Application\\chrome.exe",
        L"C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe",
        L"C:\\Program Files\\Microsoft\\Edge\\Application\\msedge.exe",
    };
    for (auto p : cand) if (FileExists(p)) return p;
    return L"";
}

// 打开独立应用窗口（--app 模式，无地址栏/标签页）
static void OpenAppWindow() {
    wstring url = L"http://127.0.0.1:" + to_wstring(PORT) + L"/";
    wstring browser = FindBrowserExe();
    if (!browser.empty()) {
        wstring args = L"--app=" + url + L" --window-size=1320,880 --window-position=center --disable-features=Translate";
        ShellExecuteW(nullptr, L"open", browser.c_str(), args.c_str(), nullptr, SW_SHOWNORMAL);
    } else {
        // 兜底：默认浏览器打开
        ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

// =============================================================
//  入口
// =============================================================
// exe 编成 GUI 子系统（-mwindows）：双击不再弹控制台黑框，直接后台运行。
// 需要看启动日志 / 排障时：设环境变量 CDC_CONSOLE=1 再运行，会临时调出一个控制台。
// 注意：GUI 子系统下没有任何控制台，printf 输出会被丢弃（这是预期行为）。
static void AttachConsoleIfRequested() {
    wchar_t buf[16] = {};
    if (GetEnvironmentVariableW(L"CDC_CONSOLE", buf, 16) == 0) return;
    if (!AttachConsole(ATTACH_PARENT_PROCESS)) AllocConsole();
    bool ok1 = (freopen("CONOUT$", "w", stdout) != nullptr);
    bool ok2 = (freopen("CONOUT$", "w", stderr) != nullptr);
    bool ok3 = (freopen("CONIN$",  "r", stdin)  != nullptr);
    if (ok1 || ok2 || ok3) SetConsoleOutputCP(CP_UTF8);
    SetConsoleTitleW(L"CDC DataGen 后端 · 调试控制台（CDC_CONSOLE=1）");
}

int main() {
    AttachConsoleIfRequested();
    g_exeDir = GetExeDir();
    ReadCloudCfg();   // 读取内置 / cdc_srv.dat（旧名 cdc_cloud.cfg 兼容）的云端服务器地址

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        MessageBoxA(nullptr, "WSAStartup 失败", "CDC 后端", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 单实例：端口已被监听 → 复用现有后端，只开窗口，本进程直接退出
    if (IsPortListening()) {
        OpenAppWindow();
        WSACleanup();
        return 0;
    }

    SOCKET ls = socket(AF_INET, SOCK_STREAM, 0);
    if (ls == INVALID_SOCKET) { WSACleanup(); return 1; }

    int opt = 1;
    setsockopt(ls, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);   // 仅本机
    addr.sin_port = htons(PORT);
    if (bind(ls, (sockaddr*)&addr, sizeof(addr)) != 0) {
        MessageBoxA(nullptr, "端口被占用，无法启动服务。", "CDC 后端", MB_OK | MB_ICONWARNING);
        closesocket(ls);
        WSACleanup();
        return 1;
    }
    listen(ls, 8);

    // 打开独立应用窗口（不跳浏览器标签页）
    OpenAppWindow();

    // 控制台提示
    printf("CDC DataGen v%s C++ 后端已启动\n", APP_VER);
    printf("独立窗口：--app 模式 @ http://127.0.0.1:%d/\n", PORT);
    printf("按 Ctrl+C 退出。\n\n");

    while (true) {
        SOCKET cs = accept(ls, nullptr, nullptr);
        if (cs == INVALID_SOCKET) break;
        _beginthreadex(nullptr, 0, ClientThread, (void*)(intptr_t)cs, 0, nullptr);
    }

    closesocket(ls);
    WSACleanup();
    return 0;
}

// =============================================================
//  CDC Fantasy Enhancer  cdcfantasy.dll  —  v8.0.0「幻想」
// -------------------------------------------------------------
//  定位：可选的外挂式增强动态库。缺失时主程序 cdc_backend.exe
//        完全不受影响（后端 LoadLibrary 失败即静默降级）。
//  职责：20 项数据校验 / 统计 / 数论 / 图论增强计算，纯本地、零网络。
//
//  对外 ABI（全部 __cdecl，全部 char* 缓冲区由调用方分配并释放，
//  DLL 内部不把堆指针交出去，避免跨 CRT 释放）：
//    CDC_FX_LIST()                 -> 返回静态 JSON 字符串（能力清单）
//    CDC_FX_CALL(req, out, cap)    -> 把结果 JSON 写入 out，返回字节数
//                                   out 为调用方提供的缓冲区，绝不分配/释放
//    CDC_FX_ABI()                  -> 返回 ABI 版本字符串
//
//  请求格式：{"op":"<能力名>","a":[...]}   结果：{"ok":true,"op":"..","v":..}
//
//  构建：g++ -O2 -std=c++17 -shared -static -o cdcfantasy.dll cdcfantasy.cpp
// =============================================================
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cstdint>
#include <cmath>

// -------------------------------------------------------------
//  基础设施：字符串 / JSON
// -------------------------------------------------------------
static std::string JEsc(const std::string& s) {
    std::string o;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
        case '"':  o += "\\\""; break;
        case '\\': o += "\\\\"; break;
        case '\n': o += "\\n";  break;
        case '\r': o += "\\r";  break;
        case '\t': o += "\\t";  break;
        default:
            if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; }
            else o += (char)c;
        }
    }
    return o;
}

static std::string Arr(const std::vector<long long>& v) {
    std::string o = "[";
    for (size_t i = 0; i < v.size(); i++) { if (i) o += ","; o += std::to_string(v[i]); }
    return o + "]";
}

static std::string ArrD(const std::vector<double>& v) {
    std::string o = "[";
    char b[40];
    for (size_t i = 0; i < v.size(); i++) {
        if (i) o += ",";
        snprintf(b, sizeof b, "%.10g", v[i]);
        o += b;
    }
    return o + "]";
}

// 极简 JSON 取值：本 DLL 只需要读 "op" 和 "a"（整型数组），刻意不引第三方库。
static std::string JGetStr(const std::string& j, const char* key) {
    std::string pat = std::string("\"") + key + "\"";
    size_t p = j.find(pat);
    if (p == std::string::npos) return "";
    p = j.find(':', p + pat.size());
    if (p == std::string::npos) return "";
    p++;
    while (p < j.size() && (j[p] == ' ' || j[p] == '\t' || j[p] == '\r' || j[p] == '\n')) p++;
    if (p >= j.size() || j[p] != '"') return "";
    p++;
    std::string out;
    while (p < j.size() && j[p] != '"') {
        if (j[p] == '\\' && p + 1 < j.size()) {
            p++;
            if (j[p] == 'n') out += '\n';
            else if (j[p] == 't') out += '\t';
            else out += j[p];
        } else out += j[p];
        p++;
    }
    return out;
}

static bool JGetArr(const std::string& j, const char* key, std::vector<long long>& out) {
    std::string pat = std::string("\"") + key + "\"";
    size_t p = j.find(pat);
    if (p == std::string::npos) return false;
    p = j.find(':', p + pat.size());
    if (p == std::string::npos) return false;
    p = j.find('[', p);
    if (p == std::string::npos) return false;
    int depth = 0;
    size_t start = p;
    for (; p < j.size(); p++) {
        if (j[p] == '[') depth++;
        else if (j[p] == ']') { depth--; if (!depth) break; }
    }
    if (p >= j.size()) return false;
    std::string sub = j.substr(start, p - start + 1);
    out.clear();
    size_t i = 1;
    while (i < sub.size()) {
        while (i < sub.size() && (sub[i] == ',' || sub[i] == '[' || sub[i] == ' '
               || sub[i] == '\t' || sub[i] == '\r' || sub[i] == '\n')) i++;
        if (i >= sub.size() || sub[i] == ']') break;
        bool neg = false;
        if (sub[i] == '-') { neg = true; i++; }
        long long val = 0;
        bool got = false;
        while (i < sub.size() && sub[i] >= '0' && sub[i] <= '9') {
            val = val * 10 + (sub[i] - '0');
            i++; got = true;
        }
        if (got) out.push_back(neg ? -val : val);
        while (i < sub.size() && sub[i] != ',' && sub[i] != ']') i++;
    }
    return true;
}

// -------------------------------------------------------------
//  01 FNV-1a 哈希（64 位变体，FNV-1a 对输入顺序敏感）
//  a[0] = 字符串，其余忽略；返回无符号 64 位（JSON 按字符串回传避免精度丢失）
// -------------------------------------------------------------
static std::string fx01_fnv1a(const std::vector<long long>& a) {
    std::string s = a.empty() ? std::string() : std::to_string(a[0]);
    uint64_t h = 1469598103934665603ULL;
    for (size_t i = 0; i < s.size(); i++) {
        h ^= (unsigned char)s[i];
        h *= 1099511628211ULL;
    }
    char b[32];
    snprintf(b, sizeof b, "%llu", (unsigned long long)h);
    return std::string("\"") + b + "\"";
}

// -------------------------------------------------------------
//  02 MurmurHash3 x86_32
// -------------------------------------------------------------
static uint32_t rotl32(uint32_t x, int8_t r) { return (x << r) | (x >> (32 - r)); }

static std::string fx02_murmur3(const std::vector<long long>& a) {
    std::string s = a.empty() ? std::string() : std::to_string(a[0]);
    const uint32_t c1 = 0xcc9e2d51u, c2 = 0x1b873593u;
    uint32_t h = 0;                        // 标准实现 h1 = seed，本 DLL 固定 seed=0
                                        // 自检基准：空串 -> 0
    size_t nblocks = s.size() / 4;
    const uint8_t* p = (const uint8_t*)s.data();
    for (size_t i = 0; i < nblocks; i++) {
        uint32_t k;
        memcpy(&k, p + i * 4, 4);
        k *= c1; k = rotl32(k, 15); k *= c2;
        h ^= k; h = rotl32(h, 13); h = h * 5 + 0xe6546b64u;
    }
    const uint8_t* tail = p + nblocks * 4;
    uint32_t k1 = 0;
    switch (s.size() & 3) {
    case 3: k1 ^= tail[2] << 16; [[fallthrough]];
    case 2: k1 ^= tail[1] << 8;  [[fallthrough]];
    case 1: k1 ^= tail[0];
             k1 *= c1; k1 = rotl32(k1, 15); k1 *= c2; h ^= k1;
    }
    h ^= (uint32_t)s.size();
    h ^= h >> 16; h *= 0x85ebca6bu; h ^= h >> 13; h *= 0xc2b2ae35u; h ^= h >> 16;
    char b[32];
    snprintf(b, sizeof b, "%u", h);
    return std::string(b);
}

// -------------------------------------------------------------
//  03 前缀和：返回 n+1 个值，pre[0]=0
// -------------------------------------------------------------
static std::string fx03_prefix(const std::vector<long long>& a) {
    std::vector<long long> r;
    r.reserve(a.size() + 1);
    r.push_back(0);
    long long s = 0;
    for (size_t i = 0; i < a.size(); i++) { s += a[i]; r.push_back(s); }
    return Arr(r);
}

// -------------------------------------------------------------
//  04 最长严格上升子序列长度（O(n log n)，patience sorting）
// -------------------------------------------------------------
static int lisLen(const std::vector<long long>& a, bool increasing) {
    std::vector<long long> tail;
    for (size_t i = 0; i < a.size(); i++) {
        // increasing=true 时找第一个 >= x 的位置（严格上升用 lower_bound）
        // increasing=false 时把它视作 -x，转化为同一个严格上升问题
        long long x = increasing ? a[i] : -a[i];
        std::vector<long long>::iterator it =
            lower_bound(tail.begin(), tail.end(), x);
        if (it == tail.end()) tail.push_back(x);
        else *it = x;
    }
    return (int)tail.size();
}
static std::string fx04_lis(const std::vector<long long>& a) { return std::to_string(lisLen(a, true)); }
static std::string fx05_lds(const std::vector<long long>& a) { return std::to_string(lisLen(a, false)); }

// -------------------------------------------------------------
//  06 逆序对计数（归并排序，O(n log n)）
// -------------------------------------------------------------
static long long mergeCount(std::vector<long long>& v, std::vector<long long>& tmp, size_t l, size_t r) {
    if (r - l <= 1) return 0;
    size_t m = l + (r - l) / 2;
    long long c = mergeCount(v, tmp, l, m) + mergeCount(v, tmp, m, r);
    size_t i = l, j = m, k = l;
    while (i < m && j < r) {
        if (v[i] <= v[j]) tmp[k++] = v[i++];
        else { tmp[k++] = v[j++]; c += (long long)(m - i); }   // v[i..m) 全都 > v[j]
    }
    while (i < m) tmp[k++] = v[i++];
    while (j < r) tmp[k++] = v[j++];
    for (size_t t = l; t < r; t++) v[t] = tmp[t];
    return c;
}
static std::string fx06_inversions(const std::vector<long long>& a) {
    std::vector<long long> v = a, tmp(a.size());
    char b[32];
    snprintf(b, sizeof b, "%lld", mergeCount(v, tmp, 0, v.size()));
    return b;
}

// -------------------------------------------------------------
//  07 中位数（偶数个取中间两数均值，结果为 double）
// -------------------------------------------------------------
static double medianOf(std::vector<long long> v) {
    if (v.empty()) return 0;
    std::sort(v.begin(), v.end());
    size_t n = v.size();
    if (n & 1) return (double)v[n / 2];
    return ((double)v[n / 2 - 1] + (double)v[n / 2]) / 2.0;
}
static std::string fx07_median(const std::vector<long long>& a) {
    char b[40]; snprintf(b, sizeof b, "%.10g", medianOf(a)); return b;
}

// -------------------------------------------------------------
//  08 中位绝对偏差 MAD：median(|x - median(x)|)
// -------------------------------------------------------------
static std::string fx08_mad(const std::vector<long long>& a) {
    double m = medianOf(a);
    std::vector<long long> dev;
    dev.reserve(a.size());
    for (size_t i = 0; i < a.size(); i++) dev.push_back((long long)llround(fabs((double)a[i] - m)));
    char b[40]; snprintf(b, sizeof b, "%.10g", medianOf(dev)); return b;
}

// -------------------------------------------------------------
//  09 四分位距 IQR = Q3 - Q1
//    分位用「线性插值」法（与 numpy 默认一致），便于对拍。
// -------------------------------------------------------------
static double quantileSorted(const std::vector<long long>& s, double q) {
    if (s.empty()) return 0;
    if (s.size() == 1) return (double)s[0];
    double pos = q * (double)(s.size() - 1);
    size_t lo = (size_t)floor(pos);
    double frac = pos - (double)lo;
    if (lo + 1 >= s.size()) return (double)s.back();
    return (double)s[lo] + frac * ((double)s[lo + 1] - (double)s[lo]);
}
static std::string fx09_iqr(const std::vector<long long>& a) {
    std::vector<long long> s = a;
    std::sort(s.begin(), s.end());
    char b[40];
    snprintf(b, sizeof b, "%.10g", quantileSorted(s, 0.75) - quantileSorted(s, 0.25));
    return b;
}

// -------------------------------------------------------------
//  10 IQR 离群点：下界 Q1-1.5IQR，上界 Q3+1.5IQR，返回下标数组
// -------------------------------------------------------------
static std::string fx10_outlier_iqr(const std::vector<long long>& a) {
    std::vector<long long> s = a;
    std::sort(s.begin(), s.end());
    double q1 = quantileSorted(s, 0.25), q3 = quantileSorted(s, 0.75);
    double iqr = q3 - q1;
    double lo = q1 - 1.5 * iqr, hi = q3 + 1.5 * iqr;
    std::vector<long long> idx;
    for (size_t i = 0; i < a.size(); i++)
        if ((double)a[i] < lo || (double)a[i] > hi) idx.push_back((long long)i);
    return Arr(idx);
}

// -------------------------------------------------------------
//  11 Z-score 离群点：|x - mean| / std > 3（std 用总体标准差）
// -------------------------------------------------------------
static std::string fx11_outlier_z(const std::vector<long long>& a) {
    if (a.empty()) return "[]";
    double sum = 0;
    for (size_t i = 0; i < a.size(); i++) sum += (double)a[i];
    double mean = sum / (double)a.size();
    double var = 0;
    for (size_t i = 0; i < a.size(); i++) var += ((double)a[i] - mean) * ((double)a[i] - mean);
    var /= (double)a.size();
    double sd = sqrt(var);
    std::vector<long long> idx;
    if (sd > 0) {
        for (size_t i = 0; i < a.size(); i++)
            if (fabs(((double)a[i] - mean) / sd) > 3.0) idx.push_back((long long)i);
    }
    return Arr(idx);
}

// -------------------------------------------------------------
//  12 皮尔逊相关系数：输入需为偶数长度，前一半为 x、后一半为 y
// -------------------------------------------------------------
static std::string fx12_pearson(const std::vector<long long>& a) {
    size_t n = a.size() / 2;
    if (n < 2) return "null";
    double sx = 0, sy = 0;
    for (size_t i = 0; i < n; i++) { sx += (double)a[i]; sy += (double)a[i + n]; }
    double mx = sx / n, my = sy / n;
    double sxy = 0, sxx = 0, syy = 0;
    for (size_t i = 0; i < n; i++) {
        double dx = (double)a[i] - mx, dy = (double)a[i + n] - my;
        sxy += dx * dy; sxx += dx * dx; syy += dy * dy;
    }
    char b[40];
    if (sxx <= 0 || syy <= 0) { snprintf(b, sizeof b, "null"); return b; }
    snprintf(b, sizeof b, "%.10g", sxy / sqrt(sxx * syy));
    return b;
}

// -------------------------------------------------------------
//  13 最小二乘斜率：y = kx + b 的 k，x = 下标 0..n-1
// -------------------------------------------------------------
static std::string fx13_slope(const std::vector<long long>& a) {
    size_t n = a.size();
    if (n < 2) return "null";
    double sx = 0, sy = 0, sxy = 0, sxx = 0;
    for (size_t i = 0; i < n; i++) {
        double x = (double)i, y = (double)a[i];
        sx += x; sy += y; sxy += x * y; sxx += x * x;
    }
    double dn = (double)n;
    double den = dn * sxx - sx * sx;
    char b[40];
    if (den == 0) { snprintf(b, sizeof b, "null"); return b; }
    snprintf(b, sizeof b, "%.10g", (dn * sxy - sx * sy) / den);
    return b;
}

// -------------------------------------------------------------
//  14 gcd / lcm：a[0]=x, a[1]=y
// -------------------------------------------------------------
static std::string fx14_gcdlcm(const std::vector<long long>& a) {
    if (a.size() < 2) return "null";
    long long x = a[0], y = a[1];
    long long g = x < 0 ? -x : x, h = y < 0 ? -y : y;
    while (h) { long long t = g % h; g = h; h = t; }
    long long l = g ? (x / g) * y : 0;
    char b[80];
    snprintf(b, sizeof b, "{\"gcd\":%lld,\"lcm\":%lld}", g, l);
    return b;
}

// -------------------------------------------------------------
//  15 质因数分解：a[0]=n（n>=2），返回 [[p,e],...] 按 p 升序
// -------------------------------------------------------------
static std::string fx15_factor(const std::vector<long long>& a) {
    if (a.empty()) return "[]";
    long long n = a[0];
    if (n < 2) return "[]";
    std::string out = "[";
    bool firstPair = true;
    for (long long p = 2; p * p <= n; p += (p == 2 ? 1 : 2)) {
        if (n % p) continue;
        int e = 0;
        while (n % p == 0) { n /= p; e++; }
        if (!firstPair) out += ",";
        firstPair = false;
        out += "[" + std::to_string(p) + "," + std::to_string(e) + "]";
    }
    if (n > 1) {
        if (!firstPair) out += ",";
        out += "[" + std::to_string(n) + ",1]";
    }
    return out + "]";
}

// -------------------------------------------------------------
//  16 Miller-Rabin 素性判定（确定性底数集，对 64 位内取值无误判）
// -------------------------------------------------------------
static uint64_t mulmod64(uint64_t a, uint64_t b, uint64_t m) {
    return (uint64_t)((unsigned __int128)a * b % m);
}
static uint64_t powmod64(uint64_t a, uint64_t e, uint64_t m) {
    uint64_t r = 1 % m;
    a %= m;
    while (e) {
        if (e & 1) r = mulmod64(r, a, m);
        a = mulmod64(a, a, m);
        e >>= 1;
    }
    return r;
}
static bool isPrime64(uint64_t n) {
    if (n < 2) return false;
    for (uint64_t p : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) {
        if (n % p == 0) return n == p;
    }
    uint64_t d = n - 1, s = 0;
    while (!(d & 1)) { d >>= 1; s++; }
    for (uint64_t a : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37}) {
        uint64_t x = powmod64(a, d, n);
        if (x == 1 || x == n - 1) continue;
        bool ok = false;
        for (uint64_t r = 1; r < s; r++) {
            x = mulmod64(x, x, n);
            if (x == n - 1) { ok = true; break; }
        }
        if (!ok) return false;
    }
    return true;
}
static std::string fx16_isprime(const std::vector<long long>& a) {
    if (a.empty()) return "false";
    uint64_t n = (uint64_t)a[0];
    return isPrime64(n) ? "true" : "false";
}

// -------------------------------------------------------------
//  17 模快速幂：a[0]=base, a[1]=exp, a[2]=mod -> base^exp mod mod
// -------------------------------------------------------------
static std::string fx17_modpow(const std::vector<long long>& a) {
    if (a.size() < 3) return "null";
    uint64_t m = (uint64_t)a[2];
    if (m == 0) return "null";
    char b[40];
    snprintf(b, sizeof b, "%llu",
             (unsigned long long)powmod64((uint64_t)a[0], (uint64_t)a[1], m));
    return b;
}

// -------------------------------------------------------------
//  18 Kruskal 最小生成树
//  输入 a = [n, m, u1,v1,w1, u2,v2,w2, ...]（1-based 点编号）
//  输出 {"w":总权重, "cnt":用边数, "ok":是否连通}
// -------------------------------------------------------------
struct DSU {
    std::vector<int> p, r;
    DSU(int n) : p(n + 1), r(n + 1, 0) { for (int i = 0; i <= n; i++) p[i] = i; }
    int find(int x) { return p[x] == x ? x : p[x] = find(p[x]); }
    bool unite(int x, int y) {
        x = find(x); y = find(y);
        if (x == y) return false;
        if (r[x] < r[y]) std::swap(x, y);
        p[y] = x;
        if (r[x] == r[y]) r[x]++;
        return true;
    }
};
struct Edge { int u, v; long long w; };

static std::string fx18_mst(const std::vector<long long>& a) {
    if (a.size() < 2) return "{\"w\":0,\"cnt\":0,\"ok\":false}";
    int n = (int)a[0];
    size_t m = (size_t)a[1];
    std::vector<Edge> es;
    for (size_t i = 0; i < m && 2 + i * 3 + 2 < a.size(); i++) {
        Edge e;
        e.u = (int)a[2 + i * 3];
        e.v = (int)a[2 + i * 3 + 1];
        e.w = a[2 + i * 3 + 2];
        es.push_back(e);
    }
    std::sort(es.begin(), es.end(), [](const Edge& x, const Edge& y) { return x.w < y.w; });
    DSU d(n);
    long long total = 0;
    int used = 0;
    for (size_t i = 0; i < es.size(); i++) {
        if (d.unite(es[i].u, es[i].v)) { total += es[i].w; used++; }
    }
    char b[96];
    snprintf(b, sizeof b, "{\"w\":%lld,\"cnt\":%d,\"ok\":%s}",
             total, used, (n > 0 && used == n - 1) ? "true" : "false");
    return b;
}

// -------------------------------------------------------------
//  19 Dijkstra 最短路（无向图）
//  输入 a = [n, m, u1,v1,w1, ...]，返回从 1 号点出发的距离数组
//  不可达用 -1 表示；n 个点
// -------------------------------------------------------------
static std::string fx19_dijkstra(const std::vector<long long>& a) {
    if (a.size() < 2) return "[]";
    int n = (int)a[0];
    size_t m = (size_t)a[1];
    std::vector<std::vector<std::pair<int, long long>>> g(n + 1);
    for (size_t i = 0; i < m && 2 + i * 3 + 2 < a.size(); i++) {
        int u = (int)a[2 + i * 3], v = (int)a[2 + i * 3 + 1];
        long long w = a[2 + i * 3 + 2];
        if (u < 1 || v < 1 || u > n || v > n) continue;
        g[u].push_back(std::make_pair(v, w));
        g[v].push_back(std::make_pair(u, w));
    }
    const long long INF = LLONG_MAX / 4;
    std::vector<long long> dist(n + 1, INF);
    std::vector<char> done(n + 1, 0);
    dist[1] = 0;
    for (int it = 0; it < n; it++) {
        int u = -1;
        for (int i = 1; i <= n; i++)
            if (!done[i] && (u == -1 || dist[i] < dist[u])) u = i;
        if (u == -1 || dist[u] == INF) break;
        done[u] = 1;
        for (size_t i = 0; i < g[u].size(); i++) {
            int v = g[u][i].first;
            long long nd = dist[u] + g[u][i].second;
            if (nd < dist[v]) dist[v] = nd;
        }
    }
    std::string o = "[";
    for (int i = 1; i <= n; i++) {
        if (i > 1) o += ",";
        o += (dist[i] == INF ? std::string("-1") : std::to_string(dist[i]));
    }
    return o + "]";
}

// -------------------------------------------------------------
//  20 游程编码 RLE：连续相同值折叠为 [值, 次数]
// -------------------------------------------------------------
static std::string fx20_rle(const std::vector<long long>& a) {
    std::string o = "[";
    size_t i = 0;
    bool first = true;
    while (i < a.size()) {
        size_t j = i;
        while (j < a.size() && a[j] == a[i]) j++;
        if (!first) o += ",";
        first = false;
        o += "[" + std::to_string(a[i]) + "," + std::to_string((long long)(j - i)) + "]";
        i = j;
    }
    return o + "]";
}

// -------------------------------------------------------------
//  能力清单（前端「幻想增强」页面直接读这份 JSON 渲染卡片）
// -------------------------------------------------------------
struct FxCap { const char* op; const char* name; const char* kind; const char* desc; const char* in; };

static const FxCap kCaps[] = {
    {"fnv1a",       "FNV-1a 哈希",       "哈希", "64 位 FNV-1a 摘要，用于快速比对数据串是否完全一致", "a[0]=字符串（数字形式）"},
    {"murmur3",     "MurmurHash3",       "哈希", "x86_32 变体，分布性优于 FNV，适合做随机数据自检", "a[0]=字符串（数字形式）"},
    {"prefix",      "前缀和",            "序列", "返回 n+1 个值，首项为 0，用于快速求任意区间和", "a=整数数组"},
    {"lis",         "最长上升子序列",      "序列", "O(n log n) patience sorting，严格上升", "a=整数数组"},
    {"lds",         "最长下降子序列",      "序列", "O(n log n) patience sorting，严格下降", "a=整数数组"},
    {"inversions",  "逆序对计数",         "序列", "归并排序计数，O(n log n)，衡量序列有序程度", "a=整数数组"},
    {"median",      "中位数",            "统计", "偶数个取中间两数均值，与 numpy 默认口径一致", "a=整数数组"},
    {"mad",         "中位绝对偏差",        "统计", "median(|x - median|)，对离群点比标准差更稳健", "a=整数数组"},
    {"iqr",         "四分位距IQR",        "统计", "Q3 - Q1，线性插值分位，离群检测的基准量", "a=整数数组"},
    {"outlier_iqr", "IQR 离群点",         "统计", "超出 [Q1-1.5IQR, Q3+1.5IQR] 的元素下标", "a=整数数组"},
    {"outlier_z",   "Z-score 离群点",     "统计", "|x-mean|/std > 3 的元素下标（总体标准差）", "a=整数数组"},
    {"pearson",     "皮尔逊相关系数",      "统计", "前后两半分别为 x、y 的线性相关强度，范围 [-1,1]", "a=偶数长度，前半 x 后半 y"},
    {"slope",       "最小二乘斜率",        "统计", "以元素下标为 x 求拟合直线斜率 k，n<2 返回 null", "a=整数数组"},
    {"gcdlcm",      "gcd / lcm",        "数论", "最大公约数与最小公倍数同时给出", "a[0]=x, a[1]=y"},
    {"factor",      "质因数分解",         "数论", "试除分解，返回 [[质数,指数],...]，n<2 返回空", "a[0]=n"},
    {"isprime",     "Miller-Rabin 素性",   "数论", "确定性底数集，64 位内取值无概率误判", "a[0]=n"},
    {"modpow",      "模快速幂",           "数论", "base^exp mod mod，O(log exp)", "a[0]=base, a[1]=exp, a[2]=mod"},
    {"mst",         "Kruskal 最小生成树",   "图论", "按权重升序并查集合并，返回总权重与连通性", "a=[n,m,u1,v1,w1,...]（1-based）"},
    {"dijkstra",    "Dijkstra 最短路",    "图论", "无向图 1 号源点最短距离，不可达为 -1", "a=[n,m,u1,v1,w1,...]（1-based）"},
    {"rle",         "游程编码 RLE",       "编码", "连续相同值折叠为 [值,次数]，便于压缩与校验", "a=整数数组"},
};
static const int kCapN = (int)(sizeof(kCaps) / sizeof(kCaps[0]));

static std::string buildListJson() {
    std::string o = "[";
    for (int i = 0; i < kCapN; i++) {
        if (i) o += ",";
        char no[8];
        snprintf(no, sizeof no, "%02d", i + 1);
        o += "{\"no\":\"" + std::string(no) + "\",\"op\":\"" + std::string(kCaps[i].op) + "\""
           + ",\"name\":\"" + JEsc(kCaps[i].name) + "\""
           + ",\"kind\":\"" + JEsc(kCaps[i].kind) + "\""
           + ",\"desc\":\"" + JEsc(kCaps[i].desc) + "\""
           + ",\"in\":\"" + JEsc(kCaps[i].in) + "\"}";
    }
    return o + "]";
}

// -------------------------------------------------------------
//  分发
// -------------------------------------------------------------
// 注意：body 内部字段（不含最外层花括号），最后由 CDC_FX_CALL 用 {} 包成一个完整 JSON 对象，
// 后端才能把它原样嵌进 /api/fx 的 "result" 字段。曾经漏掉外层 {}，
// 结果后端拼出 "result":"ok":true... 这种坏 JSON（对拍只比 v 值，没暴露出来）。
static bool RunFx(const std::string& op, const std::vector<long long>& a, std::string& out) {
    struct B { const char* op; std::string (*fn)(const std::vector<long long>&); };
    static const B tbl[] = {
        {"fnv1a", fx01_fnv1a}, {"murmur3", fx02_murmur3}, {"prefix", fx03_prefix},
        {"lis", fx04_lis}, {"lds", fx05_lds}, {"inversions", fx06_inversions},
        {"median", fx07_median}, {"mad", fx08_mad}, {"iqr", fx09_iqr},
        {"outlier_iqr", fx10_outlier_iqr}, {"outlier_z", fx11_outlier_z},
        {"pearson", fx12_pearson}, {"slope", fx13_slope}, {"gcdlcm", fx14_gcdlcm},
        {"factor", fx15_factor}, {"isprime", fx16_isprime}, {"modpow", fx17_modpow},
        {"mst", fx18_mst}, {"dijkstra", fx19_dijkstra}, {"rle", fx20_rle},
    };
    for (const B& b : tbl)
        if (op == b.op) { out = b.fn(a); return true; }
    return false;
}

// -------------------------------------------------------------
//  导出 ABI
// -------------------------------------------------------------
extern "C" __declspec(dllexport) const char* __cdecl CDC_FX_ABI(void) { return "8.0.0-fantasy/1"; }

extern "C" __declspec(dllexport) const char* __cdecl CDC_FX_LIST(void) {
    static std::string s = buildListJson();
    return s.c_str();
}

extern "C" __declspec(dllexport) int __cdecl CDC_FX_CALL(const char* req, char* out, int cap) {
    if (!req || !out || cap <= 0) return -1;
    std::string r(req);
    std::string op = JGetStr(r, "op");
    std::vector<long long> a;
    JGetArr(r, "a", a);

    std::string body;
    if (op.empty()) {
        body = "{\"ok\":false,\"error\":\"缺少 op\"}";
    } else {
        std::string v;
        if (!RunFx(op, a, v))
            body = "{\"ok\":false,\"error\":\"未知能力:" + JEsc(op) + "\"}";
        else
            body = "{\"ok\":true,\"op\":\"" + JEsc(op) + "\",\"v\":" + v + "}";
    }
    if (body.size() + 1 > (size_t)cap) return -2;   // 缓冲区不够，调用方按需加大重试
    memcpy(out, body.c_str(), body.size());
    out[body.size()] = '\0';
    return (int)body.size();
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
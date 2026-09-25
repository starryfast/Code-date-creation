// =============================================================================
//  CDCSCQ  —— Code date creation 内置数据生成函数库（C++23 单头文件）
//  cdc_lib / cdcscq.hpp                                    Version 2.0（超详细版）
// -----------------------------------------------------------------------------
//  CDCSCQ = Code Date Creation 的 C++ 数据生成库（命名致敬洛谷 CYaRon）。
//  纯 C++ 单头文件实现，零外部依赖，专为 CDC 的「生成器」模式设计：
//
//      #include <cdcscq.hpp>          // 编译命令自动带 -I<cdc_lib 目录>
//      using namespace cdcscq;
//
//      int main() {
//          seed();                  // 时间播种（或 seed_case() 按数据点固定）
//          int n = 10 * case_id();  // case_id() = 当前数据点编号 1..N
//          out_vec(seq(n, 1, 1000000000));
//          out_tree(n, tree(n, "random"));
//      }
//
//  ── 三条铁律 ────────────────────────────────────────────────────────────────
//   1. 数据写 stdout（cout / printf），CDC 会把它存成 <编号>.in
//   2. 顶点编号一律 1..n；边用 out_edges / out_tree / out_graph 输出
//   3. 不用管 include 路径，编译器自动挂载 cdc_lib
//
//  ── 章节索引 ────────────────────────────────────────────────────────────────
//   §1 数据点编号   §2 随机引擎      §3 数列 / 向量 / 极端数据
//   §4 字符串       §5 树            §6 图
//   §7 几何         §8 询问 / 矩阵   §9 输出工具     §10 输入工具
// =============================================================================
#ifndef CDC_CDCSCQ_HPP
#define CDC_CDCSCQ_HPP

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <iostream>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace cdcscq {

// =============================================================================
//  §1  数据点编号（配合 CDC「生成器」模式：每个 .in 用不同规模/形态）
// =============================================================================
//  CDC 逐个运行生成器时会传入 argv[1] = 数据点编号（1,2,3,…）。
//  case_id() 直接取到这个编号，让你按点号造递增规模的数据。
inline int& _case_store() { static int v = 0; return v; }

// 显式绑定（跨平台稳妥写法：int main(int argc,char**argv){ bind_args(argc,argv); ... }）
inline void bind_args(int argc, char** argv) {
    _case_store() = (argc > 1 && argv && argv[1]) ? std::atoi(argv[1]) : 0;
}

// 当前数据点编号；取不到时返回 1（保证第一次运行就有合理默认值）
inline int case_id() {
    if (_case_store() > 0) return _case_store();
#if defined(_WIN32) && (defined(__MINGW32__) || defined(__MINGW64__) || defined(_MSC_VER))
    if (__argc > 1) {
        int v = std::atoi(__argv[1]);
        if (v > 0) return v;
    }
#endif
    return 1;
}

// =============================================================================
//  §2  随机引擎
// =============================================================================
inline std::mt19937_64& rng() {
    static std::mt19937_64 engine(
        (uint64_t)std::chrono::steady_clock::now().time_since_epoch().count());
    return engine;
}

// 指定种子（做可复现数据）
inline void seed(uint64_t s) { rng().seed(s); }

// 用系统时间重新播种
inline void seed() {
    rng().seed((uint64_t)std::chrono::steady_clock::now().time_since_epoch().count());
}

// 按「数据点编号」播种：同一个点的数据可复现，不同点各不相同。
// 强烈推荐生成器第一行就调用它，避免每次生成结果都不一样、无法复现。
inline void seed_case(uint64_t base = 20260915u) {
    rng().seed(base * 1000003ull + (uint64_t)case_id());
}

// 闭区间 [l, r] 均匀随机整数
inline long long rnd(long long l, long long r) {
    if (l > r) std::swap(l, r);
    std::uniform_int_distribution<long long> d(l, r);
    return d(rng());
}

// [l, r] 均匀随机实数
inline double rndf(double l, double r) {
    if (l > r) std::swap(l, r);
    std::uniform_real_distribution<double> d(l, r);
    return d(rng());
}

// 以概率 p 返回 true（p 取 0~1）
inline bool chance(double p) { return rndf(0.0, 1.0) < p; }

// 原地洗牌
template <class T>
inline void shuffle(std::vector<T>& v) { std::shuffle(v.begin(), v.end(), rng()); }

// 返回 [0, n-1] 的下标（n<=0 时返回 0）
inline int pick_index(int n) { return n <= 0 ? 0 : (int)rnd(0, n - 1); }

// 从容器里随机取一个元素（空容器返回默认值）
template <class T>
inline T pick(const std::vector<T>& v) {
    if (v.empty()) return T{};
    return v[(size_t)pick_index((int)v.size())];
}

// 随机奇数 / 偶数 / 2 的幂 / 非零数
inline long long rnd_odd(long long l, long long r) {
    if (l > r) std::swap(l, r);
    if (l % 2 == 0) ++l;
    if (r % 2 == 0) --r;
    if (l > r) return l;                       // 区间内无奇数
    return l + 2 * rnd(0, (r - l) / 2);
}
inline long long rnd_even(long long l, long long r) {
    if (l > r) std::swap(l, r);
    if (l % 2 != 0) ++l;
    if (r % 2 != 0) --r;
    if (l > r) return l;
    return l + 2 * rnd(0, (r - l) / 2);
}
inline long long rnd_pow2(int max_exp) { return 1LL << (int)rnd(0, max_exp < 0 ? 0 : max_exp); }
inline long long rnd_nonzero(long long l, long long r) {
    for (int i = 0; i < 64; ++i) { long long x = rnd(l, r); if (x != 0) return x; }
    return (l > 0 || r < 0) ? rnd(l, r) : 1;
}

// =============================================================================
//  §3  数列 / 向量 / 极端数据
// =============================================================================
//  可重复：n 个 [lo, hi] 的整数
inline std::vector<long long> seq(int n, long long lo, long long hi) {
    std::vector<long long> v;
    v.reserve(n > 0 ? n : 0);
    for (int i = 0; i < n; ++i) v.push_back(rnd(lo, hi));
    return v;
}

//  互不相同：n 个 [lo, hi] 的整数（要求 n <= hi-lo+1）
inline std::vector<long long> seq_unique(int n, long long lo, long long hi) {
    long long span = hi - lo + 1;
    if (n <= 0) return {};
    if (n > span) n = (int)span;
    std::vector<long long> v;
    if (span <= 4000000) {                     // 小区间：整体洗牌
        v.resize((size_t)span);
        std::iota(v.begin(), v.end(), lo);
        std::shuffle(v.begin(), v.end(), rng());
        v.resize((size_t)n);
    } else {                                   // 大区间：哈希去重
        std::unordered_set<long long> seen;
        seen.reserve((size_t)n * 2);
        while ((int)v.size() < n) {
            long long x = rnd(lo, hi);
            if (seen.insert(x).second) v.push_back(x);
        }
    }
    return v;
}

//  由解析式生成：f(1..n)，结果夹到 [lo, hi]
template <class F>
inline std::vector<long long> seq_func(int n, long long lo, long long hi, F f) {
    std::vector<long long> v;
    v.reserve(n > 0 ? n : 0);
    for (int i = 1; i <= n; ++i) {
        long long x = (long long)f((long long)i);
        if (x < lo) x = lo;
        if (x > hi) x = hi;
        v.push_back(x);
    }
    return v;
}

//  排列 1..n 的随机打乱
inline std::vector<int> permutation(int n) {
    std::vector<int> v(n > 0 ? (size_t)n : 0u);
    std::iota(v.begin(), v.end(), 1);
    std::shuffle(v.begin(), v.end(), rng());
    return v;
}

//  多维向量：n 个 d 维点，每维 [lo, hi]；distinct=true 时各点互不相同
inline std::vector<std::vector<long long>> vecs(int n, int d, long long lo, long long hi,
                                                bool distinct = false) {
    std::vector<std::vector<long long>> vs;
    vs.reserve(n > 0 ? n : 0);
    std::unordered_set<std::string> seen;
    for (int i = 0; i < n; ++i) {
        std::vector<long long> p((size_t)(d > 0 ? d : 1));
        for (int j = 0; j < d; ++j) p[(size_t)j] = rnd(lo, hi);
        if (distinct) {
            std::string key;
            for (auto x : p) { key += std::to_string(x); key.push_back(','); }
            if (!seen.insert(key).second) { --i; continue; }
        }
        vs.push_back(std::move(p));
    }
    return vs;
}

//  矩阵：n 行 m 列，元素 [lo, hi]
inline std::vector<std::vector<long long>> matrix(int n, int m, long long lo, long long hi) {
    std::vector<std::vector<long long>> a;
    a.reserve(n > 0 ? n : 0);
    for (int i = 0; i < n; ++i) {
        std::vector<long long> row((size_t)(m > 0 ? m : 1));
        for (int j = 0; j < m; ++j) row[(size_t)j] = rnd(lo, hi);
        a.push_back(std::move(row));
    }
    return a;
}

// ---- 有序 / 单调 / 特殊形态数列 -------------------------------------------
//  全相同（v）
inline std::vector<long long> seq_const(int n, long long v) {
    return std::vector<long long>(n > 0 ? (size_t)n : 0u, v);
}
//  等差：a0, a0+d, a0+2d, …
inline std::vector<long long> seq_arith(int n, long long a0, long long d) {
    std::vector<long long> v;
    v.reserve(n > 0 ? n : 0);
    for (int i = 0; i < n; ++i) v.push_back(a0 + (long long)i * d);
    return v;
}
//  等比：a0, a0*q, …（溢出时用饱和值补齐，保证长度仍是 n）
inline std::vector<long long> seq_geom(int n, long long a0, long long q) {
    const long long LIM = 9223372036854775807LL;
    std::vector<long long> v;
    v.reserve(n > 0 ? n : 0);
    long long cur = a0;
    for (int i = 0; i < n; ++i) {
        v.push_back(cur);
        __int128 nxt = (__int128)cur * (__int128)q;
        if (nxt > (__int128)LIM) cur = LIM;
        else if (nxt < -(__int128)LIM) cur = -LIM;
        else cur = (long long)nxt;
    }
    return v;
}
//  斐波那契（可取模，mod<=0 表示不取模）
inline std::vector<long long> seq_fib(int n, long long mod = 0) {
    std::vector<long long> v;
    v.reserve(n > 0 ? n : 0);
    long long a = 1, b = 1;
    for (int i = 0; i < n; ++i) {
        long long x = (i < 2) ? 1 : (a + b);
        if (mod > 0) x %= mod;
        v.push_back(x);
        a = b; b = x;
    }
    return v;
}
//  随机游走：相邻两项差值 <= step，整体夹在 [lo, hi]
inline std::vector<long long> seq_walk(int n, long long lo, long long hi, long long step = -1) {
    if (step < 0) step = std::max(1LL, (hi - lo) / 10);
    std::vector<long long> v;
    v.reserve(n > 0 ? n : 0);
    long long cur = rnd(lo, hi);
    for (int i = 0; i < n; ++i) {
        v.push_back(cur);
        cur += rnd(-step, step);
        if (cur < lo) cur = lo + (lo - cur);
        if (cur > hi) cur = hi - (cur - hi);
        if (cur < lo) cur = lo;
        if (cur > hi) cur = hi;
    }
    return v;
}
//  锯齿形：小-大-小-大交替（排序算法的经典坑）
inline std::vector<long long> seq_zigzag(int n, long long lo, long long hi) {
    std::vector<long long> v;
    v.reserve(n > 0 ? n : 0);
    long long mid = (lo + hi) / 2;
    for (int i = 0; i < n; ++i) v.push_back(i % 2 ? rnd(mid, hi) : rnd(lo, mid));
    return v;
}
//  0/1 数列
inline std::vector<int> seq_binary(int n, double p1 = 0.5) {
    std::vector<int> v;
    v.reserve(n > 0 ? n : 0);
    for (int i = 0; i < n; ++i) v.push_back(chance(p1) ? 1 : 0);
    return v;
}
//  2 的幂数列
inline std::vector<long long> seq_pow2(int n, int max_exp = 30) {
    std::vector<long long> v;
    v.reserve(n > 0 ? n : 0);
    for (int i = 0; i < n; ++i) v.push_back(rnd_pow2(max_exp));
    return v;
}

// ---- ★ 极端数据专用：一条命令造出边界用例 --------------------------------
//  kind 取值（不区分大小写不敏感，直接写字符串）：
//    "min"   全下界          "max"   全上界
//    "zero"  全 0            "same"  全为同一个随机值
//    "asc"   非降序          "desc"  非升序
//    "near"  近似有序(少量交换)   "rev"  完全逆序（严格降序）
//    "alt"   下界/上界交替    "small" 只在小范围里取（大量重复）
//    "pow2"  取值贴近 2 的幂  "rand"  普通随机
inline std::vector<long long> seq_extreme(int n, long long lo, long long hi,
                                          const std::string& kind = "rand") {
    if (n < 0) n = 0;
    std::string k = kind;
    for (auto& c : k) c = (char)std::tolower((unsigned char)c);
    if (k == "min")  return seq_const(n, lo);
    if (k == "max")  return seq_const(n, hi);
    if (k == "zero") return seq_const(n, 0);
    if (k == "same") return seq_const(n, rnd(lo, hi));
    if (k == "pow2") return seq_pow2(n, 30);
    if (k == "alt") {
        std::vector<long long> v; v.reserve((size_t)n);
        for (int i = 0; i < n; ++i) v.push_back(i % 2 ? hi : lo);
        return v;
    }
    if (k == "small") {
        long long cap = std::min(hi, lo + 3);
        return seq(n, lo, cap);
    }
    if (k == "asc") {
        std::vector<long long> v = seq(n, lo, hi);
        std::sort(v.begin(), v.end());
        return v;
    }
    if (k == "desc") {
        std::vector<long long> v = seq(n, lo, hi);
        std::sort(v.begin(), v.end(), std::greater<long long>());
        return v;
    }
    if (k == "rev") {                       // 严格逆序：先取 n 个互异再降序
        std::vector<long long> v = seq_unique(n, lo, hi);
        std::sort(v.begin(), v.end(), std::greater<long long>());
        return v;
    }
    if (k == "near") {
        std::vector<long long> v = seq(n, lo, hi);
        std::sort(v.begin(), v.end());
        int sw = std::max(1, n / 20);
        for (int i = 0; i < sw && n > 1; ++i) {
            int a = pick_index(n), b = pick_index(n);
            std::swap(v[(size_t)a], v[(size_t)b]);
        }
        return v;
    }
    return seq(n, lo, hi);
}

// ---- 容器小工具 -----------------------------------------------------------
template <class T>
inline std::vector<T> sorted_copy(std::vector<T> v, bool asc = true) {
    std::sort(v.begin(), v.end());
    if (!asc) std::reverse(v.begin(), v.end());
    return v;
}
template <class T>
inline std::vector<T> reversed_copy(std::vector<T> v) {
    std::reverse(v.begin(), v.end());
    return v;
}

// =============================================================================
//  §4  字符串 / 段落
// =============================================================================
const std::string DICT_LOWER = "abcdefghijklmnopqrstuvwxyz";
const std::string DICT_UPPER = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string DICT_DIGIT = "0123456789";
const std::string DICT_ALNUM = DICT_LOWER + DICT_UPPER + DICT_DIGIT;
const std::string DICT_01    = "01";
const std::string DICT_ABC   = "abc";
const std::string DICT_BRACK = "()[]{}";

//  长度 len 的随机串（从 dict 取字符）
inline std::string str(int len, const std::string& dict = DICT_LOWER) {
    if (dict.empty() || len <= 0) return "";
    std::string s;
    s.reserve((size_t)len);
    for (int i = 0; i < len; ++i) s.push_back(dict[(size_t)rnd(0, (long long)dict.size() - 1)]);
    return s;
}
//  n 个随机串
inline std::vector<std::string> strs(int n, int len, const std::string& dict = DICT_LOWER) {
    std::vector<std::string> v;
    v.reserve(n > 0 ? n : 0);
    for (int i = 0; i < n; ++i) v.push_back(str(len, dict));
    return v;
}
//  等长随机串，但两两不同（构造"没有重复串"的用例）
inline std::vector<std::string> strs_unique(int n, int len,
                                            const std::string& dict = DICT_LOWER) {
    std::vector<std::string> v;
    std::unordered_set<std::string> seen;
    for (int i = 0; i < n; ++i) {
        for (int attempt = 0; attempt < 64; ++attempt) {
            std::string s = str(len, dict);
            if (seen.insert(s).second) { v.push_back(s); break; }
        }
    }
    return v;
}
//  全相同字符的串
inline std::string str_const(int len, char ch = 'a') {
    return std::string(len > 0 ? (size_t)len : 0u, ch);
}
//  0/1 串
inline std::string binary_str(int len, double p1 = 0.5) {
    std::string s;
    s.reserve((size_t)(len > 0 ? len : 0));
    for (int i = 0; i < len; ++i) s.push_back(chance(p1) ? '1' : '0');
    return s;
}
//  回文串
inline std::string palindrome(int len, const std::string& dict = DICT_LOWER) {
    if (len <= 0 || dict.empty()) return "";
    int half = (len + 1) / 2;
    std::string h = str(half, dict);
    std::string s = h;
    s.resize((size_t)len);
    for (int i = 0; i < len; ++i) s[(size_t)i] = h[(size_t)std::min(i, len - 1 - i)];
    return s;
}
//  随机括号序列（合法与非法都有可能）
inline std::string bracket(int pairs) { return str(pairs * 2, DICT_BRACK); }
//  合法括号序列（先造平衡结构再打乱配对类型）
inline std::string bracket_valid(int pairs) {
    std::string s;
    int depth = 0, rem_open = pairs;
    while ((int)s.size() < pairs * 2) {
        bool can_open = rem_open > 0;
        bool can_close = depth > 0;
        if (can_open && (!can_close || chance(0.5))) { s.push_back('('); ++depth; --rem_open; }
        else { s.push_back(')'); --depth; }
    }
    // 把 '('/')' 随机替换成 [] {}，保持结构合法
    for (auto& c : s) {
        if (c == '(') { int t = pick_index(3); c = "([{"[t]; }
        else { int t = pick_index(3); c = ")]}"[t]; }
    }
    return s;
}
//  段落：用字典 words 拼出 lines 行，每行 words_per_line 个词
inline std::vector<std::string> paragraph(int lines, int words_per_line,
                                          const std::vector<std::string>& words) {
    std::vector<std::string> v;
    if (words.empty()) return v;
    for (int i = 0; i < lines; ++i) {
        std::string line;
        for (int j = 0; j < words_per_line; ++j) {
            if (j) line.push_back(' ');
            line += words[(size_t)pick_index((int)words.size())];
        }
        v.push_back(std::move(line));
    }
    return v;
}

// =============================================================================
//  §5  树（n 个点，编号 1..n；每个函数返回 n-1 条边）
// =============================================================================
struct Edge {
    int u = 0, v = 0;
    long long w = 1;
};

//  链：1-2-…-n（深度最大，考验递归/树剖）
inline std::vector<Edge> tree_chain(int n) {
    std::vector<Edge> es;
    for (int i = 2; i <= n; ++i) es.push_back({i - 1, i, 1});
    return es;
}
//  菊花 / 星形：1 连所有点（度数最大，考验邻接表）
inline std::vector<Edge> tree_flower(int n) {
    std::vector<Edge> es;
    for (int i = 2; i <= n; ++i) es.push_back({1, i, 1});
    return es;
}
inline std::vector<Edge> tree_star(int n) { return tree_flower(n); }
//  完全二叉树：i 的父亲是 i/2
inline std::vector<Edge> tree_binary(int n) {
    std::vector<Edge> es;
    for (int i = 2; i <= n; ++i) es.push_back({i / 2, i, 1});
    return es;
}
//  k 叉树：i 的父亲是 (i-2)/k+1
inline std::vector<Edge> tree_kary(int n, int k) {
    if (k < 1) k = 1;
    std::vector<Edge> es;
    for (int i = 2; i <= n; ++i) es.push_back({(i - 2) / k + 1, i, 1});
    return es;
}
//  平衡 k 叉树（同 kary，语义化命名）
inline std::vector<Edge> tree_balanced(int n, int k = 2) { return tree_kary(n, k); }
//  随机树：strong=true 父节点取 [1,i-1]（更深更随机）；false 偏邻近（更扁平）
inline std::vector<Edge> tree_random(int n, bool strong = true) {
    std::vector<Edge> es;
    long long window = std::max(1, n / 8);
    for (int i = 2; i <= n; ++i) {
        int p;
        if (strong) p = (int)rnd(1, i - 1);
        else {
            long long lo = std::max(1LL, (long long)i - window);
            p = (int)rnd(lo, i - 1);
        }
        es.push_back({p, i, 1});
    }
    return es;
}
//  均匀随机树（Prüfer 序列法）：所有 n^(n-2) 棵带标号树等概率
inline std::vector<Edge> tree_prufer(int n) {
    std::vector<Edge> es;
    if (n <= 1) return es;
    if (n == 2) { es.push_back({1, 2, 1}); return es; }
    std::vector<int> seq((size_t)(n - 2));
    for (auto& x : seq) x = (int)rnd(1, n);
    std::vector<int> deg((size_t)n + 1, 1);
    for (int x : seq) ++deg[(size_t)x];
    std::set<int> leaves;
    for (int i = 1; i <= n; ++i) if (deg[(size_t)i] == 1) leaves.insert(i);
    for (int x : seq) {
        int leaf = *leaves.begin();
        leaves.erase(leaves.begin());
        es.push_back({leaf, x, 1});
        if (--deg[(size_t)x] == 1) leaves.insert(x);
        deg[(size_t)leaf] = 0;
    }
    int u = *leaves.begin(); leaves.erase(leaves.begin());
    int v = *leaves.begin();
    es.push_back({u, v, 1});
    return es;
}
//  毛毛虫：主干链 + 每个主干节点挂若干叶子
inline std::vector<Edge> tree_caterpillar(int n) {
    std::vector<Edge> es;
    if (n <= 1) return es;
    int spine = std::max(2, n / 2);
    for (int i = 2; i <= spine; ++i) es.push_back({i - 1, i, 1});
    for (int i = spine + 1; i <= n; ++i) es.push_back({(int)rnd(1, spine), i, 1});
    return es;
}
//  扫帚：一条长链，链尾挂一大堆叶子（深度大 + 末端度大）
inline std::vector<Edge> tree_broom(int n) {
    std::vector<Edge> es;
    if (n <= 1) return es;
    int handle = std::max(2, n / 3);
    for (int i = 2; i <= handle; ++i) es.push_back({i - 1, i, 1});
    for (int i = handle + 1; i <= n; ++i) es.push_back({handle, i, 1});
    return es;
}
//  统一入口：kind = chain | star | flower | binary | random | prufer |
//                     caterpillar | broom | kary:3 | balanced:4
inline std::vector<Edge> tree(int n, const std::string& kind = "random", bool strong = true) {
    std::string k = kind;
    for (auto& c : k) c = (char)std::tolower((unsigned char)c);
    if (k == "chain") return tree_chain(n);
    if (k == "star" || k == "flower") return tree_flower(n);
    if (k == "binary") return tree_binary(n);
    if (k == "prufer") return tree_prufer(n);
    if (k == "caterpillar") return tree_caterpillar(n);
    if (k == "broom") return tree_broom(n);
    if (k.rfind("kary", 0) == 0 || k.rfind("balanced", 0) == 0) {
        int kk = 2;
        size_t c = k.find(':');
        if (c != std::string::npos) kk = std::atoi(k.c_str() + c + 1);
        return tree_kary(n, kk);
    }
    return tree_random(n, strong);
}

// =============================================================================
//  §6  图（n 个点，编号 1..n）
// =============================================================================
//  随机图：n 点 m 边；directed 有向；simple=true 无自环无重边
inline std::vector<Edge> graph_random(int n, int m, bool directed = false, bool simple = true) {
    std::vector<Edge> es;
    if (n < 1 || m <= 0) return es;
    long long cap = directed ? (long long)n * (n - 1) : (long long)n * (n - 1) / 2;
    long long limit = simple ? std::min<long long>(m, cap) : m;
    std::unordered_set<std::string> seen;
    if (simple) seen.reserve((size_t)limit * 2);
    for (long long i = 0; i < limit; ++i) {
        for (int attempt = 0; attempt < 64; ++attempt) {
            int u = (int)rnd(1, n), v = (int)rnd(1, n);
            if (u == v) continue;
            if (directed) {
                if (simple) {
                    std::string k = std::to_string(u) + ">" + std::to_string(v);
                    if (!seen.insert(k).second) continue;
                }
            } else {
                int a = std::min(u, v), b = std::max(u, v);
                if (simple) {
                    std::string k = std::to_string(a) + "-" + std::to_string(b);
                    if (!seen.insert(k).second) continue;
                }
                u = a; v = b;
            }
            es.push_back({u, v, 1});
            break;
        }
    }
    return es;
}
//  非简单随机图：允许重边与自环（考验算法的鲁棒性）
inline std::vector<Edge> graph_multigraph(int n, int m, bool directed = false,
                                          bool self_loop = true) {
    std::vector<Edge> es;
    if (n < 1 || m <= 0) return es;
    for (int i = 0; i < m; ++i) {
        int u = (int)rnd(1, n), v = (int)rnd(1, n);
        if (!self_loop && u == v) { --i; continue; }
        if (!directed && u > v) std::swap(u, v);
        es.push_back({u, v, 1});
    }
    return es;
}
//  稠密图：边数 = density * 最大边数（density 取 0~1）
inline std::vector<Edge> graph_dense(int n, double density, bool directed = false) {
    long long cap = directed ? (long long)n * (n - 1) : (long long)n * (n - 1) / 2;
    return graph_random(n, (int)(cap * density), directed, true);
}
//  连通图：先随机树保证连通，再补边到 m 条
inline std::vector<Edge> graph_connected(int n, int m, bool directed = false) {
    std::vector<Edge> es = tree_random(n, true);
    if (directed) for (auto& e : es) if (chance(0.5)) std::swap(e.u, e.v);
    std::unordered_set<std::string> seen;
    for (auto& e : es) {
        int a = std::min(e.u, e.v), b = std::max(e.u, e.v);
        seen.insert(std::to_string(a) + "-" + std::to_string(b));
    }
    long long cap = (long long)n * (n - 1) / 2;
    long long need = std::min<long long>(m, cap) - (long long)es.size();
    for (long long i = 0; i < need; ++i) {
        for (int attempt = 0; attempt < 64; ++attempt) {
            int u = (int)rnd(1, n), v = (int)rnd(1, n);
            if (u == v) continue;
            int a = std::min(u, v), b = std::max(u, v);
            std::string k = std::to_string(a) + "-" + std::to_string(b);
            if (!seen.insert(k).second) continue;
            bool flip = directed && chance(0.5);
            es.push_back({flip ? b : a, flip ? a : b, 1});
            break;
        }
    }
    return es;
}
//  有向无环图：随机拓扑序后只从前往后连边
inline std::vector<Edge> graph_dag(int n, int m) {
    std::vector<Edge> es;
    if (n < 1 || m <= 0) return es;
    std::vector<int> ord = permutation(n);
    long long cap = (long long)n * (n - 1) / 2;
    long long limit = std::min<long long>(m, cap);
    for (long long i = 0; i < limit; ++i) {
        int a = (int)rnd(0, n - 2), b = (int)rnd(a + 1, n - 1);
        es.push_back({ord[(size_t)a], ord[(size_t)b], 1});
    }
    return es;
}
//  分层 DAG：把点分成 layers 层，边只从第 k 层指向第 k+1 层（经典最短路/DP 图）
inline std::vector<Edge> graph_dag_layered(int n, int layers, int m) {
    std::vector<Edge> es;
    if (n < 2 || layers < 2) return es;
    std::vector<int> level((size_t)n + 1, 0);
    for (int i = 1; i <= n; ++i) level[(size_t)i] = (int)rnd(0, layers - 1);
    for (int i = 0; i < m; ++i) {
        for (int attempt = 0; attempt < 64; ++attempt) {
            int u = (int)rnd(1, n), v = (int)rnd(1, n);
            if (u == v) continue;
            if (level[(size_t)u] >= level[(size_t)v]) continue;
            es.push_back({u, v, 1});
            break;
        }
    }
    return es;
}
//  完全图
inline std::vector<Edge> graph_complete(int n, bool directed = false) {
    std::vector<Edge> es;
    for (int u = 1; u <= n; ++u)
        for (int v = u + 1; v <= n; ++v) {
            es.push_back({u, v, 1});
            if (directed) es.push_back({v, u, 1});
        }
    return es;
}
//  二分图：左 1..n1，右 n1+1..n1+n2，m 条边
inline std::vector<Edge> graph_bipartite(int n1, int n2, int m) {
    std::vector<Edge> es;
    long long cap = (long long)n1 * n2;
    long long limit = std::min<long long>(m, cap);
    std::unordered_set<std::string> seen;
    for (long long i = 0; i < limit; ++i) {
        for (int attempt = 0; attempt < 64; ++attempt) {
            int u = (int)rnd(1, n1);
            int v = (int)rnd(n1 + 1, n1 + n2);
            std::string k = std::to_string(u) + "-" + std::to_string(v);
            if (!seen.insert(k).second) continue;
            es.push_back({u, v, 1});
            break;
        }
    }
    return es;
}
//  完全二分图 K(n1,n2)
inline std::vector<Edge> graph_complete_bipartite(int n1, int n2) {
    std::vector<Edge> es;
    for (int u = 1; u <= n1; ++u)
        for (int v = 1; v <= n2; ++v) es.push_back({u, n1 + v, 1});
    return es;
}
//  链图 1-2-…-n
inline std::vector<Edge> graph_path(int n) { return tree_chain(n); }
//  环：1-2-…-n-1
inline std::vector<Edge> graph_cycle(int n) {
    std::vector<Edge> es = tree_chain(n);
    if (n >= 3) es.push_back({n, 1, 1});
    return es;
}
//  网格图：n 行 m 列，点 (i,j) 编号 (i-1)*m+j，四连通
inline std::vector<Edge> graph_grid(int n, int m) {
    std::vector<Edge> es;
    auto id = [m](int i, int j) { return (i - 1) * m + j; };
    for (int i = 1; i <= n; ++i)
        for (int j = 1; j <= m; ++j) {
            if (j < m) es.push_back({id(i, j), id(i, j + 1), 1});
            if (i < n) es.push_back({id(i, j), id(i + 1, j), 1});
        }
    return es;
}
//  树 + 少量额外边（连通、有环，最贴近真实测试数据）
inline std::vector<Edge> graph_tree_plus(int n, int extra) {
    std::vector<Edge> es = tree_random(n, true);
    std::unordered_set<std::string> seen;
    for (auto& e : es) {
        int a = std::min(e.u, e.v), b = std::max(e.u, e.v);
        seen.insert(std::to_string(a) + "-" + std::to_string(b));
    }
    for (int i = 0; i < extra; ++i) {
        for (int attempt = 0; attempt < 64; ++attempt) {
            int u = (int)rnd(1, n), v = (int)rnd(1, n);
            if (u == v) continue;
            int a = std::min(u, v), b = std::max(u, v);
            if (!seen.insert(std::to_string(a) + "-" + std::to_string(b)).second) continue;
            es.push_back({a, b, 1});
            break;
        }
    }
    return es;
}
//  给边随机赋权 [lo, hi]
inline std::vector<Edge> weighted(std::vector<Edge> es, long long lo, long long hi) {
    for (auto& e : es) e.w = rnd(lo, hi);
    return es;
}
//  边权极端化：全下界 / 全上界 / 全相同（考验溢出）
inline std::vector<Edge> weighted_extreme(std::vector<Edge> es, long long lo, long long hi,
                                          const std::string& kind = "rand") {
    std::string k = kind;
    for (auto& c : k) c = (char)std::tolower((unsigned char)c);
    for (auto& e : es) {
        if (k == "min") e.w = lo;
        else if (k == "max") e.w = hi;
        else if (k == "same") e.w = (es.empty() ? lo : es[0].w);
        else e.w = rnd(lo, hi);
    }
    if (k == "same" && !es.empty()) { long long w = rnd(lo, hi); for (auto& e : es) e.w = w; }
    return es;
}
//  编号平移（把 1..n 变成 l..r 之类的自定义编号）
inline std::vector<Edge> edge_shift(std::vector<Edge> es, int delta) {
    for (auto& e : es) { e.u += delta; e.v += delta; }
    return es;
}
//  邻接表（方便自己写算法）
inline std::vector<std::vector<int>> graph_adj(int n, const std::vector<Edge>& es,
                                               bool directed = false) {
    std::vector<std::vector<int>> g((size_t)(n > 0 ? n : 0) + 1);
    for (auto& e : es) {
        if (e.u >= 1 && e.u <= n && e.v >= 1 && e.v <= n) g[(size_t)e.u].push_back(e.v);
        if (!directed && e.v >= 1 && e.v <= n && e.u >= 1 && e.u <= n)
            g[(size_t)e.v].push_back(e.u);
    }
    return g;
}
//  带权邻接表：g[u] = {(v, w), …}
inline std::vector<std::vector<std::pair<int, long long>>> graph_adjw(
    int n, const std::vector<Edge>& es, bool directed = false) {
    std::vector<std::vector<std::pair<int, long long>>> g((size_t)(n > 0 ? n : 0) + 1);
    for (auto& e : es) {
        if (e.u >= 1 && e.u <= n && e.v >= 1 && e.v <= n) g[(size_t)e.u].push_back({e.v, e.w});
        if (!directed && e.v >= 1 && e.v <= n && e.u >= 1 && e.u <= n)
            g[(size_t)e.v].push_back({e.u, e.w});
    }
    return g;
}

// =============================================================================
//  §7  几何
// =============================================================================
struct Point {
    long long x = 0, y = 0;
};

//  恰好 n 个顶点的简单多边形（星形：按极角排序 + 半径随机 → 一定不自交）
inline std::vector<Point> polygon_exact(int n, long long lo, long long hi) {
    if (n < 3) n = 3;
    long long width = hi - lo;
    if (width < 2) width = 2;
    double R = (double)width / 2.0;
    double cx = (double)(lo + hi) / 2.0, cy = (double)(lo + hi) / 2.0;
    double step = 6.28318530717958647692 / n;
    std::vector<Point> pts;
    for (int i = 0; i < n; ++i) {
        double ang = step * i + rndf(-0.4, 0.4) * step;   // 角度保持严格递增
        double rr = R * rndf(0.35, 1.0);
        pts.push_back({(long long)std::llround(cx + rr * std::cos(ang)),
                       (long long)std::llround(cy + rr * std::sin(ang))});
    }
    return pts;
}

//  凸多边形：恰好 n 个顶点的正 n 边形（随机旋转 + 整点），保证凸性
inline std::vector<Point> polygon_convex(int n, long long lo, long long hi) {
    if (n < 3) n = 3;
    long long width = hi - lo;
    if (width < 2) width = 2;
    double R = (double)width / 2.0;
    double cx = (double)(lo + hi) / 2.0, cy = (double)(lo + hi) / 2.0;
    double step = 6.28318530717958647692 / n;
    double rot = rndf(0.0, 1.0) * step;
    std::vector<Point> pts;
    for (int i = 0; i < n; ++i) {
        double ang = rot + step * i;
        pts.push_back({(long long)std::llround(cx + R * std::cos(ang)),
                       (long long)std::llround(cy + R * std::sin(ang))});
    }
    return pts;
}

//  随机简单多边形（取凸包，顶点数可能少于 n——保证不自交）
inline std::vector<Point> polygon(int n, long long lo, long long hi) {
    if (n < 3) n = 3;
    long long width = hi - lo;
    if (width < 2) width = 2;
    double R = (double)width / 2.0;
    double center = (double)(lo + hi) / 2.0;
    std::vector<Point> pts;
    pts.reserve((size_t)n);
    double step = 6.28318530717958647692 / n;
    for (int i = 0; i < n; ++i) {
        double ang = step * i + rndf(-0.55, 0.55) * step;
        double rr = R * rndf(0.55, 1.0);
        pts.push_back({(long long)std::llround(rr * std::cos(ang)) + (long long)center,
                       (long long)std::llround(rr * std::sin(ang)) + (long long)center});
    }
    std::sort(pts.begin(), pts.end(), [](const Point& a, const Point& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });
    pts.erase(std::unique(pts.begin(), pts.end(), [](const Point& a, const Point& b) {
        return a.x == b.x && a.y == b.y;
    }), pts.end());
    auto cross = [](const Point& O, const Point& A, const Point& B) {
        return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
    };
    std::vector<Point> hull;
    for (auto& p : pts) {
        while (hull.size() >= 2 && cross(hull[hull.size() - 2], hull.back(), p) <= 0) hull.pop_back();
        hull.push_back(p);
    }
    size_t lower = hull.size();
    for (int i = (int)pts.size() - 2; i >= 0; --i) {
        while (hull.size() > lower && cross(hull[hull.size() - 2], hull.back(), pts[(size_t)i]) <= 0)
            hull.pop_back();
        hull.push_back(pts[(size_t)i]);
    }
    if (hull.size() > 1) hull.pop_back();
    return hull;
}

//  随机点集（可重合 / 互不相同）
inline std::vector<Point> points(int n, long long lo, long long hi, bool distinct = false) {
    std::vector<Point> ps;
    ps.reserve(n > 0 ? n : 0);
    std::unordered_set<std::string> seen;
    for (int i = 0; i < n; ++i) {
        long long x = rnd(lo, hi), y = rnd(lo, hi);
        if (distinct) {
            std::string k = std::to_string(x) + "," + std::to_string(y);
            if (!seen.insert(k).second) { --i; continue; }
        }
        ps.push_back({x, y});
    }
    return ps;
}

//  面积的两倍（整数运算，避免浮点误差；可直接当答案输出）
inline long long polygon_area2(const std::vector<Point>& p) {
    if (p.size() < 3) return 0;
    long long s = 0;
    for (size_t i = 0; i < p.size(); ++i) {
        const Point& a = p[i];
        const Point& b = p[(i + 1) % p.size()];
        s += a.x * b.y - a.y * b.x;
    }
    return s < 0 ? -s : s;
}
inline double polygon_area(const std::vector<Point>& p) { return polygon_area2(p) / 2.0; }
inline double polygon_perimeter(const std::vector<Point>& p) {
    double s = 0;
    for (size_t i = 0; i < p.size(); ++i) {
        const Point& a = p[i];
        const Point& b = p[(i + 1) % p.size()];
        double dx = (double)(a.x - b.x), dy = (double)(a.y - b.y);
        s += std::sqrt(dx * dx + dy * dy);
    }
    return s;
}
//  两点距离（浮点）
inline double point_dist(const Point& a, const Point& b) {
    double dx = (double)(a.x - b.x), dy = (double)(a.y - b.y);
    return std::sqrt(dx * dx + dy * dy);
}

// =============================================================================
//  §8  询问 / 区间
// =============================================================================
struct Query {
    int l = 0, r = 0;
};

//  生成 n 个区间询问，端点落在 [lo, hi]，保证 l <= r
inline std::vector<Query> queries(int n, int lo, int hi, bool l_le_r = true) {
    std::vector<Query> v;
    v.reserve(n > 0 ? n : 0);
    for (int i = 0; i < n; ++i) {
        int a = (int)rnd(lo, hi), b = (int)rnd(lo, hi);
        if (l_le_r && a > b) std::swap(a, b);
        v.push_back({a, b});
    }
    return v;
}

// =============================================================================
//  §9  输出工具（全部直接写 stdout，CDC 会存成 <编号>.in）
// =============================================================================
//  一行，用 sep 分隔
template <class T>
inline void out_vec(const std::vector<T>& v, char sep = ' ') {
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) std::cout << sep;
        std::cout << v[i];
    }
    std::cout << '\n';
}
//  每行一个
template <class T>
inline void out_lines(const std::vector<T>& v) {
    for (auto& x : v) std::cout << x << '\n';
}
//  二维：每行一个向量（矩阵）
template <class T>
inline void out_vec2(const std::vector<std::vector<T>>& v) {
    for (auto& row : v) out_vec(row);
}
template <class T>
inline void out_matrix(const std::vector<std::vector<T>>& v) { out_vec2(v); }
//  浮点向量，固定小数位
inline void out_double_vec(const std::vector<double>& v, int prec = 6, char sep = ' ') {
    std::cout.precision(prec);
    std::cout << std::fixed;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i) std::cout << sep;
        std::cout << v[i];
    }
    std::cout << '\n';
    std::cout.unsetf(std::ios::floatfield);
}
//  边表：每行 "u v"
inline void out_edges(const std::vector<Edge>& es) {
    for (auto& e : es) std::cout << e.u << ' ' << e.v << '\n';
}
//  带权边表：每行 "u v w"
inline void out_edges_weighted(const std::vector<Edge>& es) {
    for (auto& e : es) std::cout << e.u << ' ' << e.v << ' ' << e.w << '\n';
}
//  点集：每行 "x y"
inline void out_points(const std::vector<Point>& ps) {
    for (auto& p : ps) std::cout << p.x << ' ' << p.y << '\n';
}
//  区间询问：每行 "l r"
inline void out_queries(const std::vector<Query>& qs) {
    for (auto& q : qs) std::cout << q.l << ' ' << q.r << '\n';
}
//  点对：每行 "a b"
template <class A, class B>
inline void out_pairs(const std::vector<std::pair<A, B>>& ps) {
    for (auto& p : ps) std::cout << p.first << ' ' << p.second << '\n';
}
//  0/1 向量（按位输出，无分隔）
inline void out_bits(const std::vector<int>& v) {
    for (int x : v) std::cout << (x ? 1 : 0);
    std::cout << '\n';
}
//  ★ 树：先输出 n，再输出 n-1 条边（最常用的树题格式）
inline void out_tree(int n, const std::vector<Edge>& es, bool w = false) {
    std::cout << n << '\n';
    if (w) out_edges_weighted(es); else out_edges(es);
}
//  ★ 图：先输出 "n m"，再输出 m 条边
inline void out_graph(int n, const std::vector<Edge>& es, bool w = false) {
    std::cout << n << ' ' << es.size() << '\n';
    if (w) out_edges_weighted(es); else out_edges(es);
}
//  带权图的邻接矩阵（n 行，每行 n 个数）
inline void out_adj_matrix(int n, const std::vector<Edge>& es, long long none = 0,
                           bool directed = false) {
    std::vector<std::vector<long long>> a((size_t)(n > 0 ? n : 0),
                                          std::vector<long long>((size_t)(n > 0 ? n : 0), none));
    for (auto& e : es) {
        if (e.u >= 1 && e.u <= n && e.v >= 1 && e.v <= n) a[(size_t)(e.u - 1)][(size_t)(e.v - 1)] = e.w;
        if (!directed && e.u >= 1 && e.u <= n && e.v >= 1 && e.v <= n)
            a[(size_t)(e.v - 1)][(size_t)(e.u - 1)] = e.w;
    }
    out_vec2(a);
}

// =============================================================================
//  §10  输入工具（生成器需要读参数时用；数据点编号优先用 case_id()）
// =============================================================================
inline int read_int() { int x = 0; if (std::scanf("%d", &x) != 1) x = 0; return x; }
inline long long read_long() { long long x = 0; if (std::scanf("%lld", &x) != 1) x = 0; return x; }
inline double read_double() { double x = 0; if (std::scanf("%lf", &x) != 1) x = 0; return x; }
inline std::string read_str() { char buf[4096]; if (std::scanf("%4095s", buf) != 1) return ""; return buf; }

//  断言（生成器自查用）：条件不满足就报错退出，避免造出非法数据
inline void require(bool cond, const std::string& msg = "CDCSCQ data check failed") {
    if (!cond) {
        std::fprintf(stderr, "[CDCSCQ] %s\n", msg.c_str());
        std::exit(1);
    }
}

}  // namespace cdcscq

// 兼容别名：习惯写 cdc:: 的代码同样可用
namespace cdc = cdcscq;

// 需要跨平台稳定拿到数据点编号时，可以这样写主函数：
//     CDCSCQ_MAIN { seed_case(); int n = 10 * case_id(); ... }
#define CDCSCQ_MAIN int main(int argc, char** argv) { ::cdcscq::bind_args(argc, argv);

#endif  // CDC_CDCSCQ_HPP

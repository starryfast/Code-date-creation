// =============================================================================
//  CDCSCQ 全功能示例生成器 —— 在 CDC「数据生成 → 🧩 生成器代码」里可直接运行
// -----------------------------------------------------------------------------
//  演示：数列 / 极端数据 / 字符串 / 树 / 图 / 几何 / 询问 / 矩阵 全部分类。
//  想拆开用，就把对应几行留下、其余删掉即可。
//
//  说明：数据点编号 case_id() 在批量生成时依次为 1,2,3,…，
//        这里用它来让规模随编号阶梯增长（1→小数据，越大→越大数据）。
// =============================================================================
#include <cdcscq.hpp>

using namespace cdcscq;

int main() {
    // 按数据点编号播种：同一点可复现，不用每次结果都变
    seed_case();

    int id = case_id();          // 当前数据点编号（1,2,3,…）
    int n  = 10 * id;            // 规模阶梯：10, 20, 30, …
    long long LIM = 1000000000;  // 1e9

    // ---------- 1. 数列 / 向量 ----------
    // out_vec(seq(n, 1, LIM));                     // 随机可重复
    // out_vec(seq_unique(n, 1, LIM));              // 互不相同
    // out_vec(seq_extreme(n, 1, LIM, "min"));      // 全下界（极端）
    // out_vec(seq_extreme(n, 1, LIM, "max"));      // 全上界（极端）
    // out_vec(seq_extreme(n, 1, LIM, "asc"));      // 非降序
    // out_vec(seq_extreme(n, 1, LIM, "rev"));      // 严格逆序（极端）
    // out_vec(seq_extreme(n, 1, LIM, "alt"));      // 上下界交替
    // out_vec(seq_arith(n, 1, 7));                 // 等差数列
    // out_vec(seq_func(n, 1, LIM, [](long long i){ return i * i; }));  // 解析式
    // out_vec(permutation(n));                     // 1..n 的排列

    // ---------- 2. 极端数据专章 ----------
    out_vec(seq_extreme(n, 1, LIM, "asc"));    // ← 演示：非降序数列

    // ---------- 3. 字符串 ----------
    // cout << str(n, DICT_LOWER) << '\n';          // 随机小写串
    // cout << binary_str(n) << '\n';               // 0/1 串
    // cout << palindrome(n) << '\n';               // 回文串
    // cout << bracket_valid(n / 2) << '\n';        // 合法括号序列
    // out_lines(strs(5, n, DICT_ALNUM));           // 多个随机串

    // ---------- 4. 树 ----------
    // out_tree(n, tree(n, "chain"));        // 链（深度最大，极端）
    // out_tree(n, tree(n, "star"));         // 菊花（度数最大，极端）
    // out_tree(n, tree(n, "binary"));       // 完全二叉树
    // out_tree(n, tree(n, "prufer"));       // 均匀随机树
    // out_tree(n, tree(n, "kary:3"));       // 3 叉树
    // out_tree(n, weighted(tree(n, "random"), 1, LIM), true);   // 带权随机树

    // ---------- 5. 图 ----------
    // out_graph(n, graph_random(n, 2 * n, false, true));        // 无向简单图
    // out_graph(n, graph_tree_plus(n, n / 2));                  // 树 + 额外边（有环）
    // out_graph(n, graph_dag(n, 2 * n));                        // DAG
    // out_graph(n, graph_dag_layered(n, 5, 2 * n));             // 分层 DAG
    // out_graph(n, graph_complete(min(n, 200)));                // 完全图（小心爆内存）
    // out_graph(n, graph_cycle(n));                             // 环
    // out_graph(n * n, graph_grid(n, n));                       // n×n 网格图
    // out_graph(n1 + n2, graph_complete_bipartite(n1, n2));     // 完全二分图
    // out_graph(n, weighted(graph_random(n, 2 * n), 1, LIM), true);  // 带权图

    // ---------- 6. 几何 ----------
    // auto p = polygon_convex(n, -LIM, LIM);    // 恰好 n 个顶点的凸多边形
    // cout << p.size() << '\n'; out_points(p);
    // cout << polygon_area2(p) << '\n';         // 面积×2，可直接当答案
    // out_points(points(n, -LIM, LIM, true));   // 互不相同的点集

    // ---------- 7. 询问 / 矩阵 ----------
    // out_queries(queries(n, 1, n));            // n 个区间询问
    // out_matrix(n, n, 1, LIM);                 // n×n 矩阵

    return 0;
}

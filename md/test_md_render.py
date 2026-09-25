# -*- coding: utf-8 -*-
"""
无头 Chrome 实测 Markdown + LaTeX 渲染（AI 回复气泡）。

做法：把 index.html 复制一份，在 </body> 前注入一段探针脚本，
      探针在 DOMContentLoaded 后调用 window.cdcRenderMd() 渲染一份"刁钻样本"
      （标题 / 列表 / 表格 / 行内与块级公式 / 代码块 / 引用 / 链接 / 裸链接），
      把结果写进 #__probe_out 与 document.title，再 --dump-dom 取回解析。

用法：
    python cdc_app/md/test_md_render.py

退出码 0 = 全部通过。
"""
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile

import html as html_mod

HERE = os.path.dirname(os.path.abspath(__file__))
APP = os.path.dirname(HERE)
INDEX = os.path.join(APP, 'index.html')

CHROME_CANDIDATES = [
    r'C:\Program Files\Google\Chrome\Application\chrome.exe',
    r'C:\Program Files (x86)\Google\Chrome\Application\chrome.exe',
    r'C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe',
    r'C:\Program Files\Microsoft\Edge\Application\msedge.exe',
]

# 探针用的 Markdown 样本：每项都对应一个断言
SAMPLE = r"""# 标题一
## 标题二

普通段落带 **粗体**、*斜体*、`行内代码`、~~删除线~~。

- 列表项 A
- 列表项 B
  - 嵌套项

1. 有序一
2. 有序二

> 引用块第一行
> 引用块第二行

| 名称 | 值 | 说明 |
|---|---|---|
| a | 1 | 第一行 |
| b | 2 | 第二行 |

行内公式 $E = mc^2$ 与 $a^2+b^2=c^2$ 应渲染成 KaTeX。

块级公式：

$$
\int_{-\infty}^{\infty} e^{-x^2}\,dx = \sqrt{\pi}
$$

下一种定界符：\(\alpha + \beta = \gamma\)

```cpp
for (int i = 0; i < n; ++i) {
    sum += i * i;   // <b>这里不能变成加粗</b>
}
```

---

自动链接 https://example.org 与 [带文字的链接](https://example.com)。
"""

# (断言名, 期望出现在渲染结果里的片段)
ASSERTIONS = [
    ('标题 h1',        '<h1>标题一</h1>'),
    ('标题 h2',        '<h2>标题二</h2>'),
    ('粗体',           '<strong>粗体</strong>'),
    ('斜体',           '<em>斜体</em>'),
    ('删除线',         '<s>删除线</s>'),
    ('行内代码',       '<code>行内代码</code>'),
    ('无序列表',       '<ul>'),
    ('有序列表',       '<ol>'),
    ('引用块',         '<blockquote>'),
    ('表格',           '<table>'),
    ('表格包裹层',     'class="md-table"'),
    ('分隔线',         '<hr>'),
    ('代码块 pre',     '<pre>'),
    ('语言 class',     'language-cpp'),
    ('KaTeX 行内',     'class="katex"'),
    ('KaTeX 块级',     'katex-display'),
    ('块级公式渲染',   '\\int'),          # KaTeX 会把 \int 保留在 MathML/HTML 里
    ('brackets 定界符', '\\alpha'),
    ('自动链接',       'href="https://example.org"'),
    ('md 链接',        'href="https://example.com"'),
    ('新窗口打开',     'target="_blank"'),
    ('代码里的 HTML 已转义', '&lt;b&gt;这里不能变成加粗&lt;/b&gt;'),
]


def find_chrome():
    for c in CHROME_CANDIDATES:
        if os.path.exists(c):
            return c
    return None


def build_probe(html):
    probe_js = """
<script id="__probe">
(function () {
  function report(payload) {
    try {
      document.title = 'PROBE:' + JSON.stringify(payload);
      var pre = document.createElement('pre');
      pre.id = '__probe_out';
      pre.textContent = JSON.stringify(payload);
      document.body.appendChild(pre);
    } catch (e) { document.title = 'PROBE-ERR:' + e.message; }
  }
  function run() {
    var out = { info: null, html: '', err: null };
    try {
      out.info = (typeof window.cdcMdInfo === 'function') ? window.cdcMdInfo() : { ready: false, reason: 'cdcMdInfo 不存在' };
      if (typeof window.cdcRenderMd === 'function') out.html = window.cdcRenderMd(SAMPLE);
      else out.err = 'cdcRenderMd 不存在';
    } catch (e) { out.err = String(e && e.stack || e); }

    // ---- 集成：真的走一遍 addBubble + mdToHtml，取计算样式 ----
    try {
      if (typeof window.addBubble === 'function' && typeof window.mdToHtml === 'function') {
        var bub = window.addBubble('bot', 'x');
        bub.innerHTML = window.mdToHtml(SAMPLE);
        var tbl = bub.querySelector('.md-table');
        out.bubble = {
          cls: bub.className,
          whiteSpace: getComputedStyle(bub).whiteSpace,
          hasKatex: !!bub.querySelector('.katex'),
          katexCount: bub.querySelectorAll('.katex').length,
          hasTable: !!bub.querySelector('table'),
          hasPre: !!bub.querySelector('pre'),
          hasH1: !!bub.querySelector('h1'),
          tableOverflowX: tbl ? getComputedStyle(tbl).overflowX : null,
          height: Math.round(bub.getBoundingClientRect().height),
          textLength: bub.textContent.length
        };
        bub.remove();
      } else {
        out.bubble = { missing: 'addBubble / mdToHtml 不是全局函数' };
      }
    } catch (e) { out.bubbleErr = String(e && e.stack || e); }

    // ---- 安全：原始 HTML 必须被转义，不能真注入 ----
    try {
      out.xss = window.cdcRenderMd('<script>alert(1)<\\/script><img src=x onerror=alert(2)>');
    } catch (e) { out.xss = 'ERR ' + e.message; }

    report(out);
  }
  if (document.readyState === 'complete' || document.readyState === 'interactive') setTimeout(run, 60);
  else document.addEventListener('DOMContentLoaded', function () { setTimeout(run, 60); });
})();
</script>
"""
    sample_js = '<script>var SAMPLE = %s;</script>' % json.dumps(SAMPLE, ensure_ascii=False)
    k = html.rfind('</body>')
    if k < 0:
        raise SystemExit('index.html 里找不到 </body>')
    return html[:k] + sample_js + probe_js + html[k:]


def main():
    chrome = find_chrome()
    if not chrome:
        print('!! 找不到 Chrome / Edge，无法做渲染实测')
        return 1
    if not os.path.exists(INDEX):
        print('!! 找不到 %s' % INDEX)
        return 1

    html = open(INDEX, encoding='utf-8', errors='replace').read()
    tmp = os.path.join(tempfile.gettempdir(), 'cdc_mdtest')
    shutil.rmtree(tmp, ignore_errors=True)
    os.makedirs(tmp)
    page = os.path.join(tmp, 'probe.html')
    with open(page, 'w', encoding='utf-8', newline='') as f:
        f.write(build_probe(html))

    print('浏览器: %s' % chrome)
    print('探针页: %s (%.0f KB)' % (page, os.path.getsize(page) / 1024))
    cmd = [chrome, '--headless=new', '--disable-gpu', '--no-sandbox', '--no-first-run',
           '--disable-extensions', '--virtual-time-budget=15000', '--dump-dom',
           '--enable-logging=stderr', '--v=0',
           'file:///' + page.replace('\\', '/')]
    r = subprocess.run(cmd, capture_output=True, text=True, encoding='utf-8', errors='replace', timeout=180)
    dom = r.stdout or ''
    err = r.stderr or ''

    print('dump-dom 长度: %d 字符' % len(dom))

    # 控制台报错 / 未捕获异常
    noise = [l for l in err.splitlines()
             if re.search(r'\b(Uncaught|SyntaxError|ReferenceError|TypeError|is not a function|SEVERE|ERROR:)', l)
             and '__probe' not in l]
    print()
    print('=== 浏览器控制台（过滤后 %d 行）===' % len(noise))
    for l in noise[:20]:
        print('  ' + l.strip()[:200])

    m = re.search(r'<pre id="__probe_out">(.*?)</pre>', dom, re.S)
    if not m:
        print()
        print('!! 探针没有输出结果（页面可能报错）')
        tail = dom[-1500:]
        print('--- DOM 尾部 ---')
        print(tail)
        print('--- stderr 尾部 ---')
        print(err[-2500:])
        return 1

    payload = json.loads(m.group(1))
    info = payload.get('info') or {}
    print()
    print('=== 渲染器自检 cdcMdInfo() ===')
    for k in ('ready', 'renderer', 'markdownIt', 'katex', 'texmath', 'reason'):
        v = info.get(k)
        print('  %-11s = %s' % (k, v if v != '' else '(空)'))
    if payload.get('err'):
        print('!! 渲染抛异常: %s' % payload['err'])

    # 探针把 HTML 塞进 <pre>.textContent，dump-dom 序列化时会把 & < > 转义一层；
    # 这里还原一次，才能拿真正的渲染 HTML 做断言（否则所有含尖括号的断言都会假失败）。
    rendered = html_mod.unescape(payload.get('html') or '')
    print()
    print('=== 渲染结果 (%.1f KB) 断言 ===' % (len(rendered) / 1024))
    failed = []
    for name, needle in ASSERTIONS:
        ok = needle in rendered
        if not ok:
            failed.append((name, needle))
        print('  %s  %s' % ('PASS' if ok else 'FAIL', name))

    bubb = payload.get('bubble') or {}
    print()
    print('=== 集成：AI 气泡（addBubble + mdToHtml）===')
    if payload.get('bubbleErr'):
        print('  !! 异常: %s' % payload['bubbleErr'])
    if bubb.get('missing'):
        print('  !! %s' % bubb['missing'])
    for k in ('cls', 'whiteSpace', 'hasKatex', 'katexCount', 'hasTable', 'hasPre',
              'hasH1', 'tableOverflowX', 'textLength'):
        if k in bubb:
            print('  %-15s = %s' % (k, bubb[k]))
    if 'height' in bubb:
        print('  %-15s = %s  （AI 页此刻 display:none，高度恒为 0，不代表渲染失败）' % ('height', bubb['height']))
    bubble_fail = []
    if bubb.get('missing'):
        bubble_fail.append('addBubble/mdToHtml 不可用')
    else:
        if bubb.get('whiteSpace') != 'normal':
            bubble_fail.append('气泡 white-space 不是 normal（= %s）' % bubb.get('whiteSpace'))
        if not bubb.get('hasKatex'):
            bubble_fail.append('气泡内没有 KaTeX 节点')
        if not bubb.get('hasTable'):
            bubble_fail.append('气泡内没有表格')
        if bubb.get('tableOverflowX') != 'auto':
            bubble_fail.append('表格未横向滚动（overflow-x = %s）' % bubb.get('tableOverflowX'))
    print('  判定:', 'PASS' if not bubble_fail else 'FAIL -> ' + '; '.join(bubble_fail))

    xss = html_mod.unescape(payload.get('xss') or '')
    print()
    print('=== 安全：原始 HTML 必须被转义 ===')
    xss_ok = ('&lt;script&gt;' in xss) and ('<script' not in xss) and ('<img' not in xss)
    print('  渲染出 <script 标签:', '<script' in xss, '（必须为 False）')
    print('  渲染出 <img 标签  :', '<img' in xss, '（必须为 False）')
    print('  渲染出 &lt;script&gt;:', '&lt;script&gt;' in xss, '（必须为 True）')
    print('  判定:', 'PASS' if xss_ok else 'FAIL')

    print()
    if info.get('ready') and not failed and not payload.get('err') and not bubble_fail and xss_ok:
        print('全部通过 ✓  markdown-it + KaTeX + texmath 工作正常，气泡样式与转义均正确')
        rc = 0
    else:
        if failed:
            print('未通过的断言:')
            for n, nd in failed:
                print('   - %s  (期望含 %r)' % (n, nd[:70]))
        rc = 1
    shutil.rmtree(tmp, ignore_errors=True)
    return rc


if __name__ == '__main__':
    sys.exit(main())

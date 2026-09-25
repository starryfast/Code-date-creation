# -*- coding: utf-8 -*-
"""
CDC AI 助手 —— 拉取 Markdown + LaTeX 渲染所需的开源库（全部 MIT）。

用法：  python cdc_app/md/fetch_vendor.py
产物：  cdc_app/md/vendor/  （js / css / fonts）

依赖：
  markdown-it          14.1.0    MIT   Markdown 解析
  katex                0.16.22   MIT   LaTeX 渲染
  markdown-it-texmath  1.0.0    MIT   把 $..$ / $$..$$ 接给 KaTeX

说明：registry.npmjs.org 在本机不通，统一走 jsdelivr CDN（实测 ~1s/文件）。
"""
import os
import re
import ssl
import sys
import time
import urllib.request
from concurrent.futures import ThreadPoolExecutor

VERSIONS = {
    'markdown-it': '14.1.0',
    'katex': '0.16.22',
    'markdown-it-texmath': '1.0.0',
}
CDN = 'https://cdn.jsdelivr.net/npm'
HERE = os.path.dirname(os.path.abspath(__file__))
VENDOR = os.path.join(HERE, 'vendor')
FONTS = os.path.join(VENDOR, 'fonts')

_CTX = ssl.create_default_context()


def font_ok(path):
    """woff2 必须以 wOF2 开头；半截文件（被中断的下载）会被这里拦下。"""
    try:
        with open(path, 'rb') as f:
            return f.read(4) == b'wOF2'
    except OSError:
        return False


def fetch(url, timeout=90):
    req = urllib.request.Request(url, headers={'User-Agent': 'Mozilla/5.0'})
    return urllib.request.urlopen(req, timeout=timeout, context=_CTX).read()


def main():
    os.makedirs(FONTS, exist_ok=True)

    files = [
        ('markdown-it.min.js', f'{CDN}/markdown-it@{VERSIONS["markdown-it"]}/dist/markdown-it.min.js'),
        ('katex.min.js',       f'{CDN}/katex@{VERSIONS["katex"]}/dist/katex.min.js'),
        ('katex.min.css',      f'{CDN}/katex@{VERSIONS["katex"]}/dist/katex.min.css'),
        ('texmath.min.js',     f'{CDN}/markdown-it-texmath@{VERSIONS["markdown-it-texmath"]}/texmath.min.js'),
    ]
    for name, url in files:
        path = os.path.join(VENDOR, name)
        if os.path.exists(path) and os.path.getsize(path) > 0:
            print('skip  %-22s %10d' % (name, os.path.getsize(path)), flush=True)
            continue
        data = fetch(url)
        open(path, 'wb').write(data)
        print('ok    %-22s %10d' % (name, len(data)), flush=True)

    # CSS 里引用的字体：只取 woff2（Chrome/Edge/Firefox 全支持，且最省体积）
    css = open(os.path.join(VENDOR, 'katex.min.css'), encoding='utf-8', errors='replace').read()
    refs = sorted({r.strip('\'"') for r in re.findall(r'url\(([^)]+)\)', css)
                   if r.strip('\'"').endswith('.woff2')})
    print('\n需要 %d 个 woff2 字体，并行下载…' % len(refs), flush=True)

    def one(rel):
        name = os.path.basename(rel)
        dest = os.path.join(FONTS, name)
        if os.path.exists(dest) and font_ok(dest):
            return name, os.path.getsize(dest), 'skip'
        last = ''
        # 并发下偶发 SSL 握手失败（实测 20 个里挂 1 个），必须重试
        for attempt in range(3):
            try:
                data = fetch(f'{CDN}/katex@{VERSIONS["katex"]}/dist/{rel}')
                if data[:4] != b'wOF2':
                    last = '返回的不是 woff2（%d 字节）' % len(data)
                    time.sleep(0.8)
                    continue
                open(dest, 'wb').write(data)
                return name, len(data), 'ok' if attempt == 0 else 'ok(重试%d)' % attempt
            except Exception as e:                                # noqa: BLE001
                last = '%s: %s' % (type(e).__name__, str(e)[:50])
                time.sleep(0.8)
        return name, 0, last

    total = 0
    with ThreadPoolExecutor(max_workers=8) as pool:
        for name, size, status in pool.map(one, refs):
            total += size
            print('  %-45s %8d  %s' % (name, size, status), flush=True)

    print('\n字体合计 %d 字节 -> base64 后约 %.0f KB' % (total, total * 4 / 3 / 1024))

    # 自检：库文件 + 字体全都要在且完好。以前只查 4 个 js/css，
    # 结果字体挂了一个也照样打印"全部就绪"——现在补齐。
    problems = []
    for f in ('markdown-it.min.js', 'katex.min.js', 'katex.min.css', 'texmath.min.js'):
        p = os.path.join(VENDOR, f)
        if not os.path.exists(p) or os.path.getsize(p) == 0:
            problems.append('缺失 ' + f)
    bad_fonts = [n for n in sorted(os.listdir(FONTS)) if not font_ok(os.path.join(FONTS, n))]
    if bad_fonts:
        problems.append('字体损坏: ' + ', '.join(bad_fonts))
    if len(os.listdir(FONTS)) < len(refs):
        problems.append('字体数量不足: %d/%d' % (len(os.listdir(FONTS)), len(refs)))

    if problems:
        print('!! 未就绪（重跑本脚本即可续传）:')
        for p in problems:
            print('   - ' + p)
        return 1
    print('全部就绪 ✓ （%d 个库文件 + %d 个 woff2 字体）' % (4, len(os.listdir(FONTS))))
    return 0


if __name__ == '__main__':
    sys.exit(main())

# -*- coding: utf-8 -*-
"""
把 Markdown + LaTeX 渲染栈内联进 cdc_app/index.html。

用法：
    python cdc_app/md/build_md_assets.py

做的事：
  1. 把 md/vendor/fonts/*.woff2 以 base64 塞进 katex.min.css（去掉 woff/ttf 回退，省一半体积）；
  2. 拼成「<style> + 4 个 <script>」，用 CDC-MD-VENDOR 标记包裹，注入 index.html 的 </head> 之前；
  3. 幂等——重复跑只会替换标记之间的内容，不会越插越多。

为什么内联而不是外链 CDN / 独立资源：
  exe 内嵌前端要在「网页模式 / file:// 直开 / Qt 模式」下都可用，
  外链 CDN 一断网就白屏；独立 RC 资源又要改 C++ 路由。内联最稳、零 C++ 改动。

改完 index.html 记得重跑 windres + g++（见 skills/cdc-backend-dev）。
"""
import base64
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
APP = os.path.dirname(HERE)                       # cdc_app/
VENDOR = os.path.join(HERE, 'vendor')
FONTS = os.path.join(VENDOR, 'fonts')
INDEX = os.path.join(APP, 'index.html')
BACKUP = os.path.join(HERE, '_index_before_md.html')

START = '<!-- ==== CDC-MD-VENDOR-START （由 md/build_md_assets.py 自动生成，勿手改） ==== -->'
END = '<!-- ==== CDC-MD-VENDOR-END ==== -->'


def read(path, mode='r'):
    if mode == 'rb':
        with open(path, 'rb') as f:
            return f.read()
    with open(path, encoding='utf-8', errors='replace') as f:
        return f.read()


def die(msg):
    print('!! ' + msg)
    sys.exit(1)


def build_css():
    """katex.min.css + 字体 base64 内联 + 我们自己的样式"""
    css_path = os.path.join(VENDOR, 'katex.min.css')
    css = read(css_path)

    # 1) 干掉 woff / ttf 回退（Chrome/Edge/Firefox 全支持 woff2，能省一大半体积）
    css, n_drop = re.subn(
        r',\s*url\(\s*["\']?fonts/[^)]*\.(?:woff|ttf)["\']?\s*\)\s*format\(["\'][\w-]+["\']\)',
        '', css)

    # 2) woff2 -> data URI
    used = []

    def repl(m):
        name = os.path.basename(m.group(1))
        path = os.path.join(FONTS, name)
        if not os.path.exists(path):
            die('缺少字体文件: %s' % name)
        raw = read(path, 'rb')
        if raw[:4] != b'wOF2':
            die('字体文件损坏（不是 wOF2）: %s  —— 删掉后重跑 md/fetch_vendor.py' % name)
        used.append((name, len(raw)))
        return 'url(data:font/woff2;base64,%s)' % base64.b64encode(raw).decode('ascii')

    css, n_font = re.subn(r'url\(\s*["\']?fonts/([^)"\']+\.woff2)["\']?\s*\)', repl, css)

    leftover = re.findall(r'url\(\s*["\']?fonts/', css)
    if leftover:
        die('CSS 里仍残留 %d 处 fonts/ 外链引用（替换正则没覆盖到）' % len(leftover))

    own = read(os.path.join(HERE, 'cdc_md_style.css'))
    total_font = sum(s for _, s in used)
    print('  CSS : katex.min.css 去 %d 条 woff/ttf 回退, 内联 %d 个 woff2 (%.0f KB 原始 -> %.0f KB base64)'
          % (n_drop, n_font, total_font / 1024, total_font * 4 / 3 / 1024))
    if n_font != 20:
        print('  ~ 提示：内联了 %d 个 woff2（KaTeX 0.16.22 预期 20 个）' % n_font)
    return css + '\n\n' + own, total_font


def build_js():
    glue = read(os.path.join(HERE, 'cdc_md_render.js'))
    return glue


def wrap_script(tag_id, body):
    if '</script' in body.lower():
        die('<script id="%s"> 内容里含 </script（会截断 HTML）' % tag_id)
    return '<script id="%s">\n%s\n</script>' % (tag_id, body.rstrip())


def main():
    for f in ('markdown-it.min.js', 'katex.min.js', 'katex.min.css', 'texmath.min.js'):
        if not os.path.exists(os.path.join(VENDOR, f)):
            die('缺少 %s —— 先跑: python cdc_app/md/fetch_vendor.py' % f)
    if not os.path.exists(INDEX):
        die('找不到 %s' % INDEX)

    html = read(INDEX)
    if not os.path.exists(BACKUP):
        shutil.copy2(INDEX, BACKUP)
        print('  备份: %s (%.0f KB)' % (os.path.relpath(BACKUP, APP), os.path.getsize(BACKUP) / 1024))

    print('  组装中…')
    css, font_bytes = build_css()
    js_glue = build_js()

    css_block = '<style id="cdc-md-vendor-css">\n%s\n</style>' % css.rstrip()
    if '</style' in css_block.lower()[:-8]:
        die('CSS 内容里含 </style（会截断 HTML）')

    parts = [
        START,
        css_block,
        wrap_script('cdc-md-1-markdown-it', read(os.path.join(VENDOR, 'markdown-it.min.js'))),
        wrap_script('cdc-md-2-katex', read(os.path.join(VENDOR, 'katex.min.js'))),
        wrap_script('cdc-md-3-texmath', read(os.path.join(VENDOR, 'texmath.min.js'))),
        wrap_script('cdc-md-4-glue', js_glue),
        END,
    ]
    block = '\n'.join(parts)

    if START in html and END in html:
        i = html.index(START)
        j = html.index(END) + len(END)
        new = html[:i] + block + html[j:]
        action = '替换已有内联块'
    else:
        k = html.find('</head>')
        if k < 0:
            die('index.html 里找不到 </head>，无法定位注入点')
        new = html[:k] + block + '\n' + html[k:]
        action = '首次注入（</head> 之前）'

    with open(INDEX, 'w', encoding='utf-8', newline='') as f:
        f.write(new)

    print('  %s' % action)
    print('  index.html: %.0f KB -> %.0f KB   (内联体 %.0f KB, 其中字体 %.0f KB)'
          % (len(html.encode('utf-8')) / 1024, len(new.encode('utf-8')) / 1024,
             len(block.encode('utf-8')) / 1024, font_bytes * 4 / 3 / 1024))

    # 自检：4 个库 + 胶水都在，且顺序正确
    for tid in ('cdc-md-1-markdown-it', 'cdc-md-2-katex', 'cdc-md-3-texmath', 'cdc-md-4-glue'):
        if new.count('id="%s"' % tid) != 1:
            die('注入后自检失败：%s 出现次数 != 1' % tid)
    for key in ('markdownit', 'katex', 'texmath', 'cdcRenderMd'):
        if key not in new:
            die('注入后自检失败：缺少 %s' % key)
    print('  自检通过 ✓')
    return 0


if __name__ == '__main__':
    print('== 内联 Markdown + LaTeX 渲染栈进 index.html ==')
    sys.exit(main())

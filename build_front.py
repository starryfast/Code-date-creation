# -*- coding: utf-8 -*-
"""构建 CDC 前端：ui/cdc_ui.html、ui/cdc_ui_open.html，并生成仓库根目录的 index.html

用法：在本目录执行  python build_front.py
说明：
  * 全部使用相对路径（以本文件所在目录为根），不含任何本机绝对路径。
  * 产物 index.html 会被 cdc.rc 内嵌进 cdc_backend.exe，改动前端后必须重跑本脚本，
    再重新编译资源（windres）才会生效。
"""
import re, os

ROOT = os.path.dirname(os.path.abspath(__file__))   # 仓库根目录
UI = os.path.join(ROOT, 'ui')                       # 前端上游源目录
APP = ROOT                                          # index.html 输出到仓库根目录

def read(p):
    return open(p, encoding='utf-8').read()

def build_single(src_html, css_path, js_path, extra_js_path=None):
    html = read(src_html)
    css = read(css_path)
    js = read(js_path)

    # 提取原内联脚本（星空 + tab）
    inline = re.findall(r'<script(?![^>]*src=)[^>]*>(.*?)</script>', html, re.S)
    extra = '\n'.join(inline)

    html = re.sub(r'<link rel="stylesheet" href="[^"]*"\s*/?>',
                  lambda m: '<style>\n' + css + '\n</style>', html, count=1)
    html = re.sub(r'<script src="app\.js"></script>',
                  lambda m: '<script>\n' + js + '\n</script>', html, count=1)
    html = re.sub(r'<!-- ============ 星空背景 \+ 子标签切换 ============ -->\s*<script>.*?</script>\s*',
                  '', html, flags=re.S)
    html = html.replace('</body>', '<script>\n' + extra + '\n</script>\n</body>')

    if extra_js_path:
        bridge = read(extra_js_path)
        html = html.replace('</body>', '<script>\n' + bridge + '\n</script>\n</body>')
    return html

# 1) ui/cdc_ui.html（单文件，登录遮罩默认显示）
single = build_single(os.path.join(UI, 'index.html'), os.path.join(UI, 'style.css'), os.path.join(UI, 'app.js'))
open(os.path.join(UI, 'cdc_ui.html'), 'w', encoding='utf-8').write(single)
print('cdc_ui.html      %.0f KB' % (len(single.encode('utf-8'))/1024))

# 2) ui/cdc_ui_open.html（跳过登录 + 纯 CSS 星空兜底）
fallback_css = '''
<style id="fallback-stars">
#starfield { display: none !important; }
body {
  background:
    radial-gradient(1.5px 1.5px at 20% 30%, rgba(255,255,255,.9), transparent 50%),
    radial-gradient(1px 1px at 40% 70%, rgba(168,192,255,.7), transparent 50%),
    radial-gradient(2px 2px at 60% 20%, rgba(255,255,255,.8), transparent 50%),
    radial-gradient(1px 1px at 80% 50%, rgba(200,210,255,.6), transparent 50%),
    radial-gradient(1.5px 1.5px at 15% 80%, rgba(255,255,255,.7), transparent 50%),
    radial-gradient(1px 1px at 50% 15%, rgba(168,192,255,.8), transparent 50%),
    radial-gradient(1.2px 1.2px at 75% 85%, rgba(255,255,255,.7), transparent 50%),
    radial-gradient(1px 1px at 30% 50%, rgba(180,200,255,.5), transparent 50%),
    radial-gradient(900px 620px at 12% -8%,  rgba(63, 43, 150, 0.32), transparent 62%),
    radial-gradient(760px 520px at 92% 8%,   rgba(168, 192, 255, 0.16), transparent 60%),
    radial-gradient(900px 700px at 55% 112%, rgba(109, 91, 176, 0.20), transparent 62%),
    linear-gradient(160deg, #080c1a 0%, #0c1228 55%, #060913 100%) !important;
  background-attachment: fixed !important;
  background-size: 100% 100% !important;
}
</style>
'''
skip_js = '''
<script>
(function(){
  function go(){
    var ov = document.getElementById('overlay');
    if (ov) { ov.classList.remove('show'); ov.style.display = 'none'; }
    var t = document.getElementById('toast');
    if (t) t.style.opacity = '0';
  }
  if (document.readyState === 'loading') document.addEventListener('DOMContentLoaded', go);
  else go();
  setTimeout(go, 200);
  setTimeout(go, 800);
})();
</script>
'''
open_html = single.replace('</head>', fallback_css + '</head>', 1).replace('</body>', skip_js + '</body>', 1)
open(os.path.join(UI, 'cdc_ui_open.html'), 'w', encoding='utf-8').write(open_html)
print('cdc_ui_open.html %.0f KB' % (len(open_html.encode('utf-8'))/1024))

# 3) 仓库根目录 index.html（后端返回的页面：跳过登录 + 桥接层）
bridge = read(os.path.join(APP, 'cpp_bridge.js'))
final = open_html.replace('</body>', '<script>\n' + bridge + '\n</script>\n</body>', 1)
open(os.path.join(APP, 'index.html'), 'w', encoding='utf-8').write(final)
print('index.html %.0f KB' % (len(final.encode('utf-8'))/1024))
print('全部构建完成')

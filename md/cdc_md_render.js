/*!
 * CDC 内置 AI 助手 —— Markdown + LaTeX 渲染（胶水层）
 *
 * 依赖（同目录 vendor/，全部 MIT，全部内联进 index.html，离线可用）：
 *   markdown-it 14.1.0           Markdown 解析（GFM 表格默认开启）
 *   KaTeX 0.16.22               LaTeX 渲染（字体已 base64 内联）
 *   markdown-it-texmath 1.0.0  把 $..$ / $$..$$ / \(..\) / \[..\] 交给 KaTeX
 *
 * 本文件由 md/build_md_assets.py 打包注入 index.html，请改这里、不要改内联副本。
 */
(function () {
  'use strict';

  function esc(s) {
    return String(s == null ? '' : s)
      .replace(/&/g, '&amp;')
      .replace(/</g, '&lt;')
      .replace(/>/g, '&gt;')
      .replace(/"/g, '&quot;')
      .replace(/'/g, '&#39;');
  }

  var md = null;
  var why = '';

  try {
    if (typeof markdownit !== 'function') {
      why = 'markdown-it 未加载';
    } else if (typeof katex === 'undefined') {
      why = 'katex 未加载';
    } else {
      md = markdownit({
        html: false,       // AI 输出里的原始 HTML 一律转义，杜绝注入
        xhtmlOut: false,
        breaks: true,      // 单个换行即换行（聊天场景更符合直觉）
        linkify: true,     // 裸链接自动成链
        typographer: false
      });

      // ---- LaTeX ----
      if (typeof texmath === 'function') {
        var katexOpts = {
          throwOnError: false,               // 公式写错不炸整条消息，就地标红
          errorColor: '#ff7b72',
          strict: false,
          trust: false,
          macros: {
            '\\RR': '\\mathbb{R}',
            '\\NN': '\\mathbb{N}',
            '\\ZZ': '\\mathbb{Z}',
            '\\QQ': '\\mathbb{Q}'
          }
        };
        try {
          // 同时认 $..$ / $$..$$（dollars）和 \(..\) / \[..\]（brackets）
          md.use(texmath, { engine: katex, delimiters: ['dollars', 'brackets'], katexOptions: katexOpts });
        } catch (e1) {
          md.use(texmath, { engine: katex, delimiters: 'dollars', katexOptions: katexOpts });
        }
      } else {
        why = 'texmath 未加载（公式将按原文显示）';
      }

      // ---- 表格外包一层容器：窄气泡里横向滚动，不再撑破布局 ----
      var tOpen = md.renderer.rules.table_open;
      var tClose = md.renderer.rules.table_close;
      md.renderer.rules.table_open = function (tokens, idx, opts, env, self) {
        return '<div class="md-table">' +
          (tOpen ? tOpen(tokens, idx, opts, env, self) : self.renderToken(tokens, idx, opts));
      };
      md.renderer.rules.table_close = function (tokens, idx, opts, env, self) {
        return '</div>' +
          (tClose ? tClose(tokens, idx, opts, env, self) : self.renderToken(tokens, idx, opts));
      };

      // ---- 外链一律新窗口打开 ----
      var lOpen = md.renderer.rules.link_open || function (tokens, idx, opts, env, self) {
        return self.renderToken(tokens, idx, opts);
      };
      md.renderer.rules.link_open = function (tokens, idx, opts, env, self) {
        tokens[idx].attrSet('target', '_blank');
        tokens[idx].attrSet('rel', 'noopener noreferrer');
        return lOpen(tokens, idx, opts, env, self);
      };
    }
  } catch (e) {
    why = '初始化异常: ' + ((e && e.message) || e);
    md = null;
  }

  if (!md) {
    try { console.warn('[cdc-ai] Markdown 渲染器不可用：' + why); } catch (e2) {}
  }

  /** 渲染 Markdown（含 LaTeX）→ HTML 字符串 */
  window.cdcRenderMd = function (src) {
    var text = String(src == null ? '' : src);
    if (!md) return '<pre class="md-fallback">' + esc(text) + '</pre>';
    try {
      return md.render(text);
    } catch (e) {
      try { console.warn('[cdc-ai] Markdown 渲染失败，回退纯文本', e); } catch (e2) {}
      return '<pre class="md-fallback">' + esc(text) + '</pre>';
    }
  };

  /** 自检信息：设置页 / 排障用 */
  window.cdcMdInfo = function () {
    return {
      ready: !!md,
      markdownIt: typeof markdownit === 'function',
      katex: typeof katex !== 'undefined' ? (katex.version || '已加载') : false,
      texmath: typeof texmath === 'function',
      renderer: 'markdown-it + KaTeX + texmath',
      reason: why
    };
  };
})();

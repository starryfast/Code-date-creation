/* =============================================================
   CDC 前端 ←→ C++ 后端 桥接层
   探测本地 C++ 后端（127.0.0.1:18889），在线则：
   1) 把 MODE 切到 'qt'（复用前端已有的本地操作分支）
   2) 用 Object.assign 覆盖 webBridge 的本地操作 → 改调 C++ HTTP API
   离线则保持纯网页模式（localStorage + 浏览器下载）
   ============================================================= */
(function () {
  'use strict';
  var BASE = 'http://127.0.0.1:18889';
  var started = false;

  function post(path, data) {
    return fetch(BASE + path, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(data || {}),
    }).then(function (r) { return r.json(); });
  }

  function enableCpp(info) {
    if (started) return;
    started = true;

    MODE = 'qt';                    // 复用前端 qt 分支（本地文件/编译/工具）
    window.CPP_BACKEND = true;
    window.CPP_BASE = BASE;

    // 预填 g++ 路径
    if (info && info.gpp) {
      try { var g1 = document.getElementById('cmpGpp'); if (g1) g1.value = info.gpp; } catch (e) {}
      try { var g2 = document.getElementById('setGpp'); if (g2) g2.value = info.gpp; } catch (e) {}
    }

    /* ---- 覆盖 webBridge 的本地操作 → C++ 后端 ---- */
    Object.assign(webBridge, {
      gen: async function (params) {
        var body = {
          start: Number(params.startId), end: Number(params.endId),
          count: Math.max(1, Number(params.count) || 1), type: Number(params.type) || 0,
          r1: Number(params.range1), r2: Number(params.range2),
          strLen: Math.max(1, Number(params.strLen) || 8),
          customCharset: params.customCharset || '',
          perFile: !!params.writeCount, extra: params.extraVars || '',
          tsDir: false, outDir: params.outDir || ''
        };
        try {
          var j = await post('/api/generate', body);
          if (j.ok) {
            var names = (j.files || []).map(function (f) { return f.name; });
            LS.set('genfiles', names);
            return { ok: true, message: '已生成 ' + j.created + ' 个 .in 文件 → ' + j.dir, files: names };
          }
          return { ok: false, message: j.message || '生成失败' };
        } catch (e) { return { ok: false, message: 'C++ 后端连接失败：' + e.message }; }
      },

      genGroup: async function (groups, count) {
        // 用网页逻辑组装内容，再交给 C++ 后端写真实文件
        var files = [];
        for (var g = 1; g <= Number(count); g++) {
          var lines = [];
          groups.forEach(function (grp) {
            var row = grp.fields.map(function (f) {
              var t = Number(f.type), r1 = Number(f.r1), r2 = Number(f.r2), len = Math.max(1, Number(f.r1) || 1);
              if (t === 0 || t === 1) return String(Math.floor(r1 + Math.random() * (r2 - r1 + 1)));
              var lower = 'abcdefghijklmnopqrstuvwxyz', upper = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
              var cs = [lower, upper, lower + upper + '0123456789'][Math.min(4, t) - 2] || lower + upper + '0123456789';
              var s = ''; for (var i = 0; i < len; i++) s += cs[Math.floor(Math.random() * cs.length)];
              return s;
            });
            lines.push(row.join(' '));
          });
          files.push({ name: 'group_' + g + '.in', content: lines.join('\n') + '\n' });
        }
        try {
          var j = await post('/api/writefiles', { outDir: '', files: files });
          var names = (j.files || []).map(function (f) { return f.name; });
          LS.set('genfiles', names);
          return { ok: true, message: '已生成 ' + j.created + ' 个分组文件 → ' + j.dir, files: names };
        } catch (e) { return { ok: false, message: 'C++ 后端连接失败：' + e.message }; }
      },

      renameAll: async function () {
        try {
          var dir = document.getElementById('genOutDir') ? document.getElementById('genOutDir').value : '';
          var j = await post('/api/rename', { dir: dir });
          return { ok: true, message: '已重命名 ' + (j.renamed || 0) + ' 个文件 *.in → *.in.txt' };
        } catch (e) { return { ok: false, message: e.message }; }
      },

      bigFile: async function () {
        var body = {
          type: Number(document.getElementById('genType').value) || 0,
          r1: Number(document.getElementById('genR1').value),
          r2: Number(document.getElementById('genR2').value),
          strLen: Math.max(1, Number(document.getElementById('genStrLen').value) || 8),
          customCharset: document.getElementById('genCharset').value || '',
          outDir: document.getElementById('genOutDir').value || ''
        };
        try {
          var j = await post('/api/bigfile', body);
          return { ok: true, message: '大文件已生成：' + j.path };
        } catch (e) { return { ok: false, message: e.message }; }
      },

      changeExt: async function () {
        try {
          var dir = document.getElementById('genOutDir') ? document.getElementById('genOutDir').value : '';
          var j = await post('/api/rename', { dir: dir });
          return { ok: true, message: '已改名 ' + (j.renamed || 0) + ' 个文件' };
        } catch (e) { return { ok: false, message: e.message }; }
      },

      compileBatch: async function (cpp, gpp, start, end) {
        try {
          var j = await post('/api/compile', { gpp: gpp, source: cpp, mode: 'batch', start: Number(start) || 1, end: Number(end) || 10 });
          return { ok: j.ok, output: j.output || j.message || '' };
        } catch (e) { return { ok: false, output: 'C++ 后端连接失败：' + e.message }; }
      },

      compileSingle: async function (cpp, gpp) {
        try {
          var j = await post('/api/compile', { gpp: gpp, source: cpp, mode: 'single', start: 1, end: 1 });
          return { ok: j.ok, output: j.output || j.message || '' };
        } catch (e) { return { ok: false, output: 'C++ 后端连接失败：' + e.message }; }
      },

      autoGpp: async function () { return (info && info.gpp) || ''; },
      getGpp: async function () { return (info && info.gpp) || ''; },
      setGpp: async function () {},

      packZip: async function (name, src) {
        var dst = src ? src + '.zip' : (name || 'archive') + '.zip';
        if (name && src) dst = (name.indexOf('.zip') >= 0 ? name : name + '.zip');
        try {
          var j = await post('/api/zip', { src: src, dst: dst });
          return { ok: j.ok, message: j.ok ? ('打包完成：' + j.path) : (j.message || '打包失败') };
        } catch (e) { return { ok: false, message: e.message }; }
      },

      runTool: async function (cmd) {
        try {
          var j = await post('/api/tool', { cmd: cmd });
          return j.ok;
        } catch (e) { return false; }
      },

      browseFile: async function () {
        try { var j = await post('/api/browse', { mode: 'file' }); return j.path || ''; } catch (e) { return ''; }
      },
      browseFolder: async function () {
        try { var j = await post('/api/browse', { mode: 'folder' }); return j.path || ''; } catch (e) { return ''; }
      },

      manSave: async function (name, content) {
        try {
          var j = await post('/api/writefiles', { outDir: '', files: [{ name: (name || 'example') + '.in', content: content }] });
          return { ok: j.ok, message: '已保存 ' + (name || 'example') + '.in → ' + j.dir };
        } catch (e) { return { ok: false, message: e.message }; }
      },

      saveTextFile: async function (defaultName, content) {
        try {
          var j = await post('/api/writefiles', { outDir: '', files: [{ name: defaultName || 'file.txt', content: content }] });
          return { ok: j.ok, message: '已保存到 ' + j.dir };
        } catch (e) { return { ok: false, message: e.message }; }
      },

      getInfo: async function () {
        return { user: '', isAdmin: false, server: '', gpp: (info && info.gpp) || '', outDir: '后端本地目录', theme: 'default', version: '7.1.0', mode: 'cpp', hasKey: false };
      },
    });

    /* ---- UI 提示 ---- */
    try {
      var mc = document.getElementById('modeChip');
      if (mc) mc.textContent = 'C++ 后端 · ' + (info.ver || '7.1.0');
      var td = document.getElementById('toolsDesc');
      if (td) td.textContent = '系统工具由 C++ 后端直接启动（calc/notepad/浏览器/系统命令…）';
      var ab = document.getElementById('aboutBackend');
      if (ab) ab.textContent = 'C++ 本地后端（' + BASE + '）';
    } catch (e) {}

    // 刷新日志提示
    try {
      var gl = document.getElementById('genLog');
      if (gl && !gl.textContent) gl.textContent = 'CDC v7.1.0 · C++ 后端模式（真实文件操作）\n';
    } catch (e) {}
  }

  /* ---- 探测后端 ---- */
  function ping() {
    var ctrl = null;
    try { ctrl = new AbortController(); } catch (e) {}
    var opt = { method: 'GET' };
    if (ctrl) { opt.signal = ctrl.signal; setTimeout(function () { try { ctrl.abort(); } catch (e) {} }, 1000); }
    fetch(BASE + '/api/ping', opt)
      .then(function (r) { return r.json(); })
      .then(function (d) {
        if (d && d.backend === 'cpp') {
          return fetch(BASE + '/api/info', opt).then(function (r) { return r.json(); }).catch(function () { return d; });
        }
        return null;
      })
      .then(function (info) { if (info) enableCpp(info); })
      .catch(function () { /* 后端未启动 → 保持网页模式 */ });
  }

  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', ping);
  } else {
    ping();
  }
})();

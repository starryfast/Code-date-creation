/* ============================================================
   CDC 数据生成器 · UI 前端逻辑（v7.1.0 全功能版）
   - Qt 桌面模式：通过 QWebChannel 调用 C++ Bridge（原生文件/编译/工具）
   - 网页模式：localStorage 本地数据 + 云端 API（服务器可配）
   两套模式共用同一套 api.* 接口，页面逻辑无需区分。
   ============================================================ */
'use strict';

/* ---------------- 基础工具 ---------------- */
const $  = (s) => document.querySelector(s);
const $$ = (s) => Array.from(document.querySelectorAll(s));
const esc = (s) => String(s == null ? '' : s).replace(/[&<>"']/g, (c) => ({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
function uid() { return Date.now().toString(36) + Math.random().toString(36).slice(2, 8); }

let toastTimer = 0;
function toast(msg, type) {
  const t = $('#toast');
  t.textContent = msg;
  t.className = 'toast show' + (type ? ' ' + type : '');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.remove('show'), 2200);
}
function setStatus(msg) { const s = $('#pageTitle'); if (s) s.textContent = msg; }

/* ---------------- 工具清单（单一数据源） ---------------- */
const TOOLS = [
  { label: '百度搜索',     icon: '🔍', kind: 'url',      cmd: 'https://www.baidu.com' },
  { label: '打开网址',     icon: '🌐', kind: 'prompt-url', cmd: '' },
  { label: '密码生成',     icon: '🔑', kind: 'calc',     calc: 'password' },
  { label: '计算器',       icon: '🧮', kind: 'tool',     cmd: 'calc' },
  { label: '记事本',       icon: '📝', kind: 'tool',     cmd: 'notepad' },
  { label: '命令提示符',   icon: '❯',  kind: 'tool',     cmd: 'cmd' },
  { label: 'PowerShell',   icon: '🟦', kind: 'tool',     cmd: 'powershell' },
  { label: '文件加密',     icon: '🔒', kind: 'tool',     cmd: 'cipher' },
  { label: '截图工具',     icon: '✂',  kind: 'tool',     cmd: 'snippingtool' },
  { label: '屏幕键盘',     icon: '⌨',  kind: 'tool',     cmd: 'osk' },
  { label: 'IP 查询',      icon: '🌍', kind: 'tool',     cmd: 'cmd /c start "" ipconfig' },
  { label: 'MD5 校验',     icon: '＃',  kind: 'calc',     calc: 'md5' },
  { label: 'Base64',       icon: '🔡', kind: 'calc',     calc: 'base64' },
  { label: 'JSON 格式化',  icon: '🧾', kind: 'calc',     calc: 'json' },
  { label: '二维码',       icon: '🔳', kind: 'prompt-qr', cmd: '' },
  { label: '天气查询',     icon: '⛅', kind: 'url',      cmd: 'https://www.weather.com.cn' },
  { label: '单位换算',     icon: '📐', kind: 'url',      cmd: 'https://www.google.com/search?q=unit+converter' },
  { label: '定时关机',     icon: '⏻',  kind: 'prompt-shutdown', cmd: '' },
  { label: '倒计时',       icon: '⏱',  kind: 'prompt-countdown', cmd: '' },
  { label: '清理临时文件', icon: '🧹', kind: 'tool',     cmd: 'cleanmgr' },
  { label: '任务管理器',   icon: '📊', kind: 'tool',     cmd: 'taskmgr' },
  { label: '注册表编辑器', icon: '🗃',  kind: 'tool',     cmd: 'regedit' },
  { label: '服务',         icon: '⚙',  kind: 'tool',     cmd: 'services.msc' },
  { label: '设备管理器',   icon: '🖥',  kind: 'tool',     cmd: 'devmgmt.msc' },
  { label: '磁盘管理',     icon: '💽', kind: 'tool',     cmd: 'diskmgmt.msc' },
  { label: '控制面板',     icon: '🎛', kind: 'tool',     cmd: 'control' },
  { label: '系统配置',     icon: '🛠',  kind: 'tool',     cmd: 'msconfig' },
  { label: '资源管理器',   icon: '📁', kind: 'tool',     cmd: 'explorer' },
  { label: '远程桌面',     icon: '🖧', kind: 'tool',     cmd: 'mstsc' },
  { label: '字符映射表',   icon: '🔣', kind: 'tool',     cmd: 'charmap' },
  { label: '系统信息',     icon: 'ℹ',  kind: 'tool',     cmd: 'msinfo32' },
  { label: '性能监视器',   icon: '📈', kind: 'tool',     cmd: 'perfmon' },
  { label: '资源监视器',   icon: '📉', kind: 'tool',     cmd: 'resmon' },
  { label: '防火墙',       icon: '🛡', kind: 'tool',     cmd: 'firewall.cpl' },
  { label: '网络连接',     icon: '🌐', kind: 'tool',     cmd: 'ncpa.cpl' },
  { label: '取色器',       icon: '🎨', kind: 'tool',     cmd: 'colorcpl' },
];

/* ---------------- 主题清单 ---------------- */
const THEMES = [
  { name: 'default', label: '默认蓝',  sw: 'linear-gradient(135deg,#3887ff,#7db4ff)' },
  { name: 'dark',    label: '暗夜灰',  sw: 'linear-gradient(135deg,#2b2f38,#15181f)' },
  { name: 'green',   label: '护眼绿',  sw: 'linear-gradient(135deg,#2e9e5b,#7bd6a0)' },
  { name: 'cream',   label: '柔和米',  sw: 'linear-gradient(135deg,#c9913f,#e8c98f)' },
  { name: 'blue',    label: '深蓝科',  sw: 'linear-gradient(135deg,#1f6fff,#6fa8ff)' },
  { name: 'pink',    label: '粉彩樱',  sw: 'linear-gradient(135deg,#e8659b,#ffb3d4)' },
  { name: 'gold',    label: '黑金奢',  sw: 'linear-gradient(135deg,#d4af37,#8a6f1f)' },
  { name: 'grey',    label: '极简灰',  sw: 'linear-gradient(135deg,#7a8696,#b6c0cf)' },
];

/* ============================================================
   运行模式检测 + 统一 api 层
   ============================================================ */
let MODE = (typeof qt !== 'undefined' && qt.webChannelTransport) ? 'qt' : 'web';
let qtBridge = null;              // QWebChannel bridge（Qt 模式）
let api = null;                   // 统一接口（Promise 风格）

/* ---------------- 网页模式后端 ---------------- */
const LS = {
  get(k, d) { try { const v = localStorage.getItem('cdc_' + k); return v == null ? d : JSON.parse(v); } catch (e) { return d; } },
  set(k, v) { try { localStorage.setItem('cdc_' + k, JSON.stringify(v)); } catch (e) {} },
  del(k) { try { localStorage.removeItem('cdc_' + k); } catch (e) {} },
};

function webDeviceId() {
  const raw = navigator.userAgent + '|' + navigator.hardwareConcurrency + '|' + navigator.language + '|' + screen.width + 'x' + screen.height;
  let h = 5381;
  for (let i = 0; i < raw.length; i++) h = ((h << 5) + h) + raw.charCodeAt(i);
  return (h >>> 0).toString(16).toUpperCase().padStart(8, '0');
}

/* 云端 HTTP（网页模式）：与桌面版 cdc_cloud 接口完全一致 */
async function cloudForm(server, fields) {
  const body = new URLSearchParams(fields);
  const r = await fetch((server || '').replace(/\/+$/, '') + '/api/activate', {
    method: 'POST',
    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
    body: body.toString(),
  });
  return r.json().catch(() => ({}));
}
async function cloudJson(server, path, data) {
  const r = await fetch((server || '').replace(/\/+$/, '') + path, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(data || {}),
  });
  return r.json().catch(() => ({}));
}

const webBridge = {
  mode: 'web',

  /* ---- 账号 ---- */
  async login(user, pass) {
    const server = LS.get('server', '');
    if (server) {
      try {
        const r = await cloudForm(server, { op: 'login', user, pass });
        if (r.ok && String(r.ok) !== '0') {
          LS.set('session', { user, isAdmin: String(r.admin) === '1' || String(r.admin) === 'true' });
          return { ok: true, user, isAdmin: LS.get('session').isAdmin, message: '登录成功（云端）' };
        }
        return { ok: false, message: r.err === 'nouser' ? '账号不存在' : (r.err === 'badpass' ? '密码错误' : '登录失败（云端）') };
      } catch (e) { return { ok: false, message: '无法连接云端服务器：' + e.message }; }
    }
    const users = LS.get('users', {});
    const u = users[user];
    if (!u) return { ok: false, message: '账号不存在，请先注册' };
    if (u.pass !== pass) return { ok: false, message: '密码错误' };
    LS.set('session', { user, isAdmin: !!u.isAdmin });
    return { ok: true, user, isAdmin: !!u.isAdmin, message: '登录成功（本地）' };
  },
  async register(user, pass) {
    if (!user || !pass) return { ok: false, message: '请输入用户名和密码' };
    const server = LS.get('server', '');
    if (server) {
      try {
        const r = await cloudForm(server, { op: 'register', user, pass });
        if (r.ok && String(r.ok) !== '0') return { ok: true, message: '注册成功（云端）' };
        return { ok: false, message: r.err === 'exists' ? '用户名已存在' : '注册失败（云端）' };
      } catch (e) { return { ok: false, message: '无法连接云端服务器：' + e.message }; }
    }
    const users = LS.get('users', {});
    if (users[user]) return { ok: false, message: '用户名已存在' };
    users[user] = { pass, isAdmin: false, created: Date.now() };
    LS.set('users', users);
    return { ok: true, message: '注册成功（本地）' };
  },
  async logout() { LS.del('session'); return { ok: true }; },
  async renameUser(name) {
    const s = LS.get('session', null); if (!s) return { ok: false, message: '未登录' };
    const users = LS.get('users', {});
    if (users[name] && name !== s.user) return { ok: false, message: '用户名已存在' };
    if (users[s.user]) { users[name] = users[s.user]; delete users[s.user]; LS.set('users', users); }
    LS.set('session', { user: name, isAdmin: s.isAdmin });
    return { ok: true, message: '已修改为 ' + name };
  },
  async activate(code) {
    const server = LS.get('server', '');
    if (server) {
      try {
        const r = await cloudJson(server, '/api/activate', { code, device: webDeviceId(), user: (LS.get('session') || {}).user || '', app: 'cdc-exceed', ver: '7.1.0' });
        if (r.ok) {
          const add = r.add || 15;
          const q = LS.get('quota', { paidLeft: 0 });
          q.paidLeft += add; LS.set('quota', q);
          return { ok: true, message: '激活成功！本次 +' + add + ' 次', add };
        }
        const reason = { used_out: '该激活码已在 3 台电脑上使用，达到上限', invalid: '激活码无效或不存在', db_error: '服务器数据库错误' }[r.reason] || '激活失败';
        return { ok: false, message: reason };
      } catch (e) { return { ok: false, message: '无法连接激活服务器：' + e.message }; }
    }
    return { ok: false, message: '网页模式未配置云端服务器，激活码仅桌面版/云端可用' };
  },
  async getInfo() {
    const s = LS.get('session', null);
    return {
      user: s ? s.user : '', isAdmin: s ? !!s.isAdmin : false,
      server: LS.get('server', ''), gpp: '', outDir: '桌面', theme: LS.get('theme', 'default'),
      version: '7.1.0', mode: 'web', hasKey: !!LS.get('ai', {}).key,
    };
  },

  /* ---- 私信 ---- */
  async getUsers() {
    const server = LS.get('server', '');
    if (server) { try { const r = await cloudForm(server, { op: 'msg_users' }); return Array.isArray(r.users) ? r.users : []; } catch (e) { return []; } }
    const users = LS.get('users', {});
    const me = (LS.get('session') || {}).user;
    return Object.keys(users).filter((u) => u !== me);
  },
  async sendMsg(to, content) {
    const s = LS.get('session', null); if (!s) return { ok: false, message: '请先登录' };
    const server = LS.get('server', '');
    if (server) {
      try { const r = await cloudForm(server, { op: 'msg_send', from: s.user, to, content }); return r.ok ? { ok: true, message: '已发送' } : { ok: false, message: '发送失败（云端）' }; }
      catch (e) { return { ok: false, message: '无法连接云端服务器：' + e.message }; }
    }
    const msgs = LS.get('msgs', []);
    msgs.push({ from: s.user, to, content, time: Date.now(), read: false });
    LS.set('msgs', msgs);
    return { ok: true, message: '已发送' };
  },
  async getConv(peer) {
    const s = LS.get('session', null); if (!s) return [];
    const server = LS.get('server', '');
    if (server) {
      try {
        const r = await cloudForm(server, { op: 'msg_conv', user: s.user, peer });
        return (Array.isArray(r.msgs) ? r.msgs : []).map((m) => ({ from: m.from, content: m.content, time: m.time || 0 }));
      } catch (e) { return []; }
    }
    return LS.get('msgs', []).filter((m) => (m.from === s.user && m.to === peer) || (m.from === peer && m.to === s.user));
  },
  async unread() {
    const s = LS.get('session', null); if (!s) return 0;
    const server = LS.get('server', '');
    if (server) { try { const r = await cloudForm(server, { op: 'msg_unread', user: s.user }); return Number(r.count) || 0; } catch (e) { return 0; } }
    return LS.get('msgs', []).filter((m) => m.to === s.user && !m.read).length;
  },
  async markRead(peer) {
    const s = LS.get('session', null); if (!s) return;
    const server = LS.get('server', '');
    if (server) { try { await cloudForm(server, { op: 'msg_read', from: peer, to: s.user }); } catch (e) {} return; }
    const msgs = LS.get('msgs', []);
    msgs.forEach((m) => { if (m.to === s.user && m.from === peer) m.read = true; });
    LS.set('msgs', msgs);
  },
  async getNotifs() {
    return LS.get('notifs', []).sort((a, b) => b.time - a.time);
  },
  async postNotif(title, content) {
    const s = LS.get('session', null);
    if (!s || !s.isAdmin) return { ok: false, message: '仅管理员可发布公告' };
    const notifs = LS.get('notifs', []);
    notifs.push({ id: uid(), title, content, createdBy: s.user, time: Date.now() });
    LS.set('notifs', notifs);
    return { ok: true, message: '已发布' };
  },
  async delNotif(id) {
    LS.set('notifs', LS.get('notifs', []).filter((n) => n.id !== id));
    return { ok: true };
  },

  /* ---- AI ---- */
  async aiChat(text) {
    const server = LS.get('server', '');
    const ai = LS.get('ai', {});
    if (ai.key) {
      try {
        const body = { model: ai.model || 'qwen-plus', messages: [
          { role: 'system', content: '你是 CDC exceed 测试数据生成器的内置 AI 助手，回答请用 Markdown 格式。' },
          { role: 'user', content: text },
        ], stream: false };
        const r = await fetch((ai.base || 'https://dashscope.aliyuncs.com/compatible-mode/v1') + '/chat/completions', {
          method: 'POST', headers: { 'Content-Type': 'application/json', 'Authorization': 'Bearer ' + ai.key }, body: JSON.stringify(body),
        });
        const j = await r.json().catch(() => ({}));
        const reply = (j.choices && j.choices[0] && j.choices[0].message && j.choices[0].message.content) || (j.error && (j.error.message || JSON.stringify(j.error))) || '（空回复）';
        return { ok: true, reply, left: -1, mode: 'key' };
      } catch (e) { return { ok: false, reply: '⚠️ 请求失败：' + e.message + '（若为 CORS 错误，请使用 Qt 桌面版或配置云端服务器）', mode: 'key' }; }
    }
    if (server) {
      try {
        const s = LS.get('session', null);
        const payload = { model: 'qwen-plus', messages: [
          { role: 'system', content: '你是 CDC exceed 测试数据生成器的内置 AI 助手，回答请用 Markdown 格式。' },
          { role: 'user', content: text },
        ], stream: false };
        const r = await cloudJson(server, '/api/activate?chat=1', { device: webDeviceId(), model: 'qwen-plus', user: s ? s.user : '', payload });
        if (r.ok) return { ok: true, reply: r.reply || '（空回复）', left: Number(r.left) || 0, mode: 'server' };
        const reason = { no_server_key: '服务器未配置 AI 密钥，请在「设置」填写自己的 API Key', quota: '服务器 AI 额度已用完，请激活码购买或在设置填写 Key', upstream_error: '服务器调用模型失败', network_error: '服务器无法连接模型 API', api_error: '模型 API 返回错误：' + (r.detail || ''), bad_gateway: '服务器返回异常：' + (r.detail || '') }[r.reason] || ('服务器错误：' + (r.reason || 'unknown'));
        return { ok: false, reply: '⚠️ ' + reason, mode: 'server' };
      } catch (e) { return { ok: false, reply: '⚠️ 无法连接云端服务器：' + e.message, mode: 'server' }; }
    }
    return { ok: false, reply: '⚠️ 未配置 API Key 且未连接服务器。请在「设置 → 云端服务器与 AI 配置」填写 Key 或服务器地址。', mode: 'none' };
  },
  async aiQuota() {
    const server = LS.get('server', '');
    const q = LS.get('quota', { paidLeft: 0 });
    if (server) {
      try {
        const s = LS.get('session', null);
        const r = await cloudForm(server, { op: 'quota', device: webDeviceId(), user: s ? s.user : '' });
        return { left: Number(r.left) || 0, granted: Number(r.granted) || 0, mode: 'server' };
      } catch (e) { return { left: 0, granted: 0, mode: 'server' }; }
    }
    return { left: q.paidLeft, granted: 0, mode: LS.get('ai', {}).key ? 'key' : 'none' };
  },

  /* ---- 生成（网页模式：JS 生成 + 预览 + 下载） ---- */
  async gen(params) {
    const files = [];
    try {
      for (let id = Number(params.startId); id <= Number(params.endId); id++) {
        let lines = [];
        const vars = (params.extraVars || '').split('\n').map((l) => l.trim()).filter(Boolean);
        vars.forEach((l) => { const eq = l.indexOf('='); lines.push(eq >= 0 ? l : l); });
        if (params.writeCount) lines.push(String(params.count));
        const r1 = Number(params.range1), r2 = Number(params.range2);
        const len = Math.max(1, Number(params.strLen) || 8);
        const cnt = Math.max(1, Number(params.count) || 1);
        const cs = params.customCharset || 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
        const lower = 'abcdefghijklmnopqrstuvwxyz', upper = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
        const charset = params.customCharset ? cs : [2, 3, 4].includes(Number(params.type)) ? [lower, upper, lower + upper + '0123456789'][Number(params.type) - 2] : lower + upper + '0123456789';
        const rs = (n) => { let s = ''; for (let i = 0; i < n; i++) s += charset[Math.floor(Math.random() * charset.length)]; return s; };
        const unique = new Set();
        const data = [];
        for (let i = 0; i < cnt; i++) {
          const t = Number(params.type);
          if (t === 0 || t === 5) data.push(t === 5 ? (r1 + Math.random() * (r2 - r1)).toFixed(6) : String(Math.floor(r1 + Math.random() * (r2 - r1))));
          else if (t === 1) { let v; do { v = Math.floor(r1 + Math.random() * (r2 - r1 + 1)); } while (unique.has(v) && unique.size < (r2 - r1 + 1)); unique.add(v); data.push(String(v)); }
          else if (t === 6) { const d = new Date(2000 + Math.floor(Math.random() * 31), Math.floor(Math.random() * 12), 1 + Math.floor(Math.random() * 28), Math.floor(Math.random() * 24), Math.floor(Math.random() * 60), Math.floor(Math.random() * 60)); data.push(d.getFullYear() + '-' + String(d.getMonth() + 1).padStart(2, '0') + '-' + String(d.getDate()).padStart(2, '0') + ' ' + String(d.getHours()).padStart(2, '0') + ':' + String(d.getMinutes()).padStart(2, '0') + ':' + String(d.getSeconds()).padStart(2, '0')); }
          else data.push(rs(len));
        }
        if (params.writeCount === false && !vars.length) { /* 无首行 */ } 
        lines = lines.concat([data.join(' '), '']);
        files.push({ name: id + '.in', content: lines.join('\n') });
      }
      LS.set('genfiles', LS.get('genfiles', []).concat(files.map((f) => f.name)));
      const cache = LS.get('genPreviewCache', {});
      files.forEach((f) => { cache[f.name] = f.content; });
      LS.set('genPreviewCache', cache);
      return { ok: true, message: '已生成 ' + files.length + ' 个文件（网页预览/下载）', files: files.map((f) => f.name), preview: files };
    } catch (e) { return { ok: false, message: '生成失败：' + e.message }; }
  },
  async genGroup(groups, count) {
    const files = [];
    for (let g = 1; g <= Number(count); g++) {
      const lines = [];
      groups.forEach((grp) => {
        const row = grp.fields.map((f) => {
          const t = Number(f.type), r1 = Number(f.r1), r2 = Number(f.r2), len = Math.max(1, Number(f.r1) || 1);
          if (t === 0) return String(Math.floor(r1 + Math.random() * (r2 - r1 + 1)));
          if (t === 1) return String(Math.floor(r1 + Math.random() * (r2 - r1 + 1)));
          const lower = 'abcdefghijklmnopqrstuvwxyz', upper = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ';
          const cs = [lower, upper, lower + upper + '0123456789'][Math.min(4, t) - 2] || lower + upper + '0123456789';
          let s = ''; for (let i = 0; i < len; i++) s += cs[Math.floor(Math.random() * cs.length)];
          return s;
        });
        lines.push(row.join(' '));
      });
      files.push({ name: 'group_' + g + '.in', content: lines.join('\n') + '\n' });
    }
    LS.set('genfiles', LS.get('genfiles', []).concat(files.map((f) => f.name)));
    return { ok: true, message: '已生成 ' + files.length + ' 个分组文件', files: files.map((f) => f.name), preview: files };
  },
  async preview() {
    const names = LS.get('genfiles', []);
    const stored = LS.get('genPreviewCache', {});
    return names.slice(-20).map((n) => ({ name: n, content: stored[n] || '（内容不可用）' }));
  },
  async genFiles() { return LS.get('genfiles', []); },
  async clearGen() { LS.set('genfiles', []); LS.set('genPreviewCache', {}); return { ok: true }; },
  async exportCsv() {
    const names = LS.get('genfiles', []);
    const csv = '文件名,状态\n' + names.map((n) => n + ',已生成').join('\n');
    downloadText('gen_export.csv', csv);
    return { ok: true, message: '已导出 ' + names.length + ' 条' };
  },
  async stats() { return { inCount: LS.get('genfiles', []).length, outCount: 0 }; },
  async renameAll() { return { ok: false, message: '网页模式不支持批量重命名（Qt 桌面版可用）' }; },
  async bigFile() { return { ok: false, message: '网页模式不支持写大文件（Qt 桌面版可用）' }; },
  async changeExt() { return { ok: false, message: '网页模式不支持改扩展名（Qt 桌面版可用）' }; },
  async makeTsDir() {
    const d = new Date();
    const ts = [d.getFullYear(), String(d.getMonth() + 1).padStart(2, '0'), String(d.getDate()).padStart(2, '0')].join('') + '_' + [String(d.getHours()).padStart(2, '0'), String(d.getMinutes()).padStart(2, '0'), String(d.getSeconds()).padStart(2, '0')].join('');
    return '桌面/' + ts;
  },
  async makeTsDir() {
    const d = new Date();
    const ts = [d.getFullYear(), String(d.getMonth() + 1).padStart(2, '0'), String(d.getDate()).padStart(2, '0')].join('') + '_' + [String(d.getHours()).padStart(2, '0'), String(d.getMinutes()).padStart(2, '0'), String(d.getSeconds()).padStart(2, '0')].join('');
    return '桌面/' + ts;
  },
  async manSave(name, content) {
    downloadText((name || 'example') + '.in', content);
    LS.set('genfiles', LS.get('genfiles', []).concat((name || 'example') + '.in'));
    return { ok: true, message: '已下载 ' + name + '.in' };
  },
  /* 通用文本文件读写（网页模式：下载 / 打开） */
  async saveTextFile(defaultName, content) {
    downloadText(defaultName || 'file.txt', content);
    return { ok: true, message: '已下载 ' + defaultName };
  },
  async openTextFile() {
    return new Promise((resolve) => {
      const input = document.createElement('input'); input.type = 'file';
      input.onchange = () => {
        const f = input.files[0]; if (!f) return resolve({ ok: false, message: '未选择文件' });
        const rd = new FileReader();
        rd.onload = () => resolve({ ok: true, message: '已读取 ' + f.name, content: String(rd.result) });
        rd.readAsText(f);
      };
      input.click();
    });
  },

  /* ---- 编译 / 打包 / 工具（网页模式提示） ---- */
  async compileBatch() { return { ok: false, output: '网页模式不支持本地编译，请使用 Qt 桌面版' }; },
  async compileSingle() { return { ok: false, output: '网页模式不支持本地编译，请使用 Qt 桌面版' }; },
  async autoGpp() { return ''; },
  async packZip() { return { ok: false, message: '网页模式不支持本地打包，请使用 Qt 桌面版' }; },
  async runTool() { return false; },
  async openUrl(url) { window.open(url, '_blank'); return true; },
  async browseFile() { return ''; },
  async browseFolder() { return ''; },

  /* ---- 设置 ---- */
  async setTheme(t) { LS.set('theme', t); },
  async getTheme() { return LS.get('theme', 'default'); },
  async setGpp() {},
  async getGpp() { return ''; },
  async setOutDir() {},
  async getOutDir() { return '桌面'; },
  async setServer(u) { LS.set('server', (u || '').trim().replace(/\/+$/, '')); },
  async getServer() { return LS.get('server', ''); },
  async setAiConf(key, model, base) { LS.set('ai', { key: (key || '').trim(), model: (model || 'qwen-plus').trim(), base: (base || 'https://dashscope.aliyuncs.com/compatible-mode/v1').trim() }); },
  async getAiConf() { return LS.get('ai', {}); },
  async setBgImage() {},
  async clearBgImage() {},
  async checkUpdate() {
    const server = LS.get('server', '');
    if (server) {
      try { const r = await cloudForm(server, { op: 'version' }); if (r.version && r.version !== '7.1.0') return { hasUpdate: true, version: r.version, url: r.url || '', notice: r.notice || '' }; } catch (e) {}
    }
    return { hasUpdate: false, version: '7.1.0' };
  },
};

/* ---------------- Qt 模式包装 ---------------- */
function qtCall(method, args) {
  return new Promise((resolve, reject) => {
    try {
      if (!qtBridge || typeof qtBridge[method] !== 'function') return reject(new Error('桥接方法不可用：' + method));
      const r = qtBridge[method].apply(qtBridge, args || []);
      resolve(r);
    } catch (e) { reject(e); }
  });
}

/* ---------------- 统一初始化 ---------------- */
function buildApi() {
  if (MODE === 'qt') {
    api = new Proxy({}, {
      get(t, prop) {
        if (typeof prop !== 'string') return undefined;
        if (prop in t) return t[prop];          // 显式覆盖（如 register 别名）
        return (...args) => qtCall(prop, args);
      },
    });
    // QWebChannel 暴露的 C++ 方法名是 reg（register 是保留字）
    api.register = (u, p) => qtCall('reg', [u, p]);
  } else {
    api = new Proxy(webBridge, {
      get(t, prop) { if (typeof prop !== 'string') return undefined; const v = t[prop]; return typeof v === 'function' ? v.bind(t) : v; },
    });
  }
}

/* ============================================================
   通用小工具（纯 JS 计算）
   ============================================================ */
function md5(text) {
  const s = String(text || '');
  const rotl = (x, c) => (x << c) | (x >>> (32 - c));
  const byte2hex = [];
  for (let i = 0; i < 256; i++) { let h = i.toString(16); if (h.length === 1) h = '0' + h; byte2hex[i] = h; }
  const u = unescape(encodeURIComponent(s));
  const b = []; for (let i = 0; i < u.length; i++) b.push(u.charCodeAt(i));
  b.push(0x80);
  while (b.length % 64 !== 56) b.push(0);
  const bits = u.length * 8; for (let i = 0; i < 8; i++) b.push((bits >>> (i * 8)) & 0xFF);
  let a0 = 0x67452301, b0 = 0xefcdab89, c0 = 0x98badcfe, d0 = 0x10325476;
  const K = [0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391];
  const S = [7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,5,9,14,20,5,9,14,20,5,9,14,20,5,9,14,20,4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21];
  for (let i = 0; i < b.length; i += 64) {
    const M = []; for (let j = 0; j < 16; j++) { M[j] = b[i + j * 4] | (b[i + j * 4 + 1] << 8) | (b[i + j * 4 + 2] << 16) | (b[i + j * 4 + 3] << 24); }
    let A = a0, B = b0, C = c0, D = d0;
    for (let j = 0; j < 64; j++) {
      let F, g;
      if (j < 16) { F = (B & C) | (~B & D); g = j; }
      else if (j < 32) { F = (D & B) | (~D & C); g = (5 * j + 1) % 16; }
      else if (j < 48) { F = B ^ C ^ D; g = (3 * j + 5) % 16; }
      else { F = C ^ (B | ~D); g = (7 * j) % 16; }
      const tmp = D; D = C; C = B;
      const sum = (A + F + K[j] + M[g]) | 0;
      B = (B + rotl(sum, S[j])) | 0; A = tmp;
    }
    a0 = (a0 + A) | 0; b0 = (b0 + B) | 0; c0 = (c0 + C) | 0; d0 = (d0 + D) | 0;
  }
  const hex = (n) => { const h = (n >>> 0).toString(16); return '00000000'.slice(h.length) + h; };
  return hex(a0) + hex(b0) + hex(c0) + hex(d0);
}
function b64Encode(s) { try { return btoa(unescape(encodeURIComponent(s))); } catch (e) { return ''; } }
function b64Decode(s) { try { return decodeURIComponent(escape(atob(String(s).replace(/\s/g, '')))); } catch (e) { return '（解码失败：Base64 内容无效）'; } }
function jsonFormat(s) { try { return JSON.stringify(JSON.parse(s), null, 2); } catch (e) { return 'JSON 解析失败：' + e.message; } }
function genPassword(len) {
  const cs = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+';
  let s = ''; for (let i = 0; i < (len || 16); i++) s += cs[Math.floor(Math.random() * cs.length)];
  return s;
}
function downloadText(name, content) {
  const blob = new Blob([content], { type: 'text/plain;charset=utf-8' });
  const a = document.createElement('a');
  a.href = URL.createObjectURL(blob); a.download = name;
  document.body.appendChild(a); a.click();
  setTimeout(() => { URL.revokeObjectURL(a.href); a.remove(); }, 400);
}

/* Markdown 轻量渲染（AI 回复用） */
function mdToHtml(md) {
  let s = esc(md || '');
  s = s.replace(/```(\w*)\n([\s\S]*?)```/g, (m, lang, code) => '<pre class="code"><code>' + code.trim() + '</code></pre>');
  s = s.replace(/`([^`]+)`/g, '<code>$1</code>');
  s = s.replace(/^### (.*)$/gm, '<h4>$1</h4>');
  s = s.replace(/^## (.*)$/gm, '<h4>$1</h4>');
  s = s.replace(/^# (.*)$/gm, '<h4>$1</h4>');
  s = s.replace(/\*\*([^*]+)\*\*/g, '<b>$1</b>');
  s = s.replace(/\*([^*]+)\*/g, '<i>$1</i>');
  s = s.replace(/^[-*] (.*)$/gm, '• $1');
  s = s.replace(/^\d+\. (.*)$/gm, '<div class="li">$1</div>');
  s = s.replace(/\n/g, '<br>');
  return s;
}

/* ============================================================
   页面逻辑
   ============================================================ */
const state = { user: '', isAdmin: false, server: '', gpp: '', theme: 'default', mode: MODE, peer: '' };

function setModeChip() {
  $('#modeChip').textContent = MODE === 'qt' ? '⚡ Qt 桌面' : '🌐 网页';
  $('#aboutBackend').textContent = MODE === 'qt' ? 'C++ Bridge（Qt + WebChannel）' : 'Web 模式（localStorage + 云端 API）';
  $('#aboutMode').textContent = MODE === 'qt' ? 'Qt 桌面版（全功能）' : '网页版（编译/系统工具需 Qt 桌面版）';
  if (MODE === 'web') $('#toolsDesc').textContent = '系统工具需 Qt 桌面版；网页版提供计算类与在线工具。';
}

function switchPage(page) {
  $$('.nav-item').forEach((b) => b.classList.toggle('active', b.dataset.page === page));
  $$('.page').forEach((p) => p.classList.toggle('active', p.dataset.page === page));
  const titles = { gen: '数据生成', compile: '自编译', zip: '打包工具', tools: '实用工具', account: '账号中心', msg: '私信', ai: 'AI 助手', settings: '设置' };
  $('#pageTitle').textContent = titles[page] || '';
  if (page === 'msg') refreshContacts();
  if (page === 'ai') refreshAiQuota();
  if (page === 'account') refreshAccountPage();
  if (page === 'settings') refreshSettings();
}

/* ---------- 主题 / 毛玻璃 ---------- */
function applyTheme(theme) {
  const t = THEMES.some((x) => x.name === theme) ? theme : 'default';
  document.documentElement.dataset.theme = t;
  $$('.theme-chip').forEach((c) => c.classList.toggle('active', c.dataset.theme === t));
}
function setFrost(on) {
  document.body.classList.toggle('frosted', on);
  $('#setFrosted').checked = on;
  try { localStorage.setItem('cdc_frost', on ? '1' : '0'); } catch (e) {}
}

/* ---------- 登录 ---------- */
let authMode = 'login';
function setAuthMode(m) {
  authMode = m;
  $('#tabLogin').classList.toggle('on', m === 'login');
  $('#tabReg').classList.toggle('on', m === 'register');
  $('#btnAuth').textContent = m === 'login' ? '登录' : '注册';
  $('#authHint').textContent = '';
}
async function doAuth() {
  const user = $('#authUser').value.trim(), pass = $('#authPass').value;
  if (!user || !pass) { $('#authHint').textContent = '请输入用户名和密码'; return; }
  const btn = $('#btnAuth'); btn.disabled = true;
  try {
    if (authMode === 'login') {
      const r = await api.login(user, pass);
      if (r.ok) { afterLogin(r); } else { $('#authHint').textContent = r.message || '登录失败'; }
    } else {
      const r = await api.register(user, pass);
      if (r.ok) { toast(r.message, 'ok'); setAuthMode('login'); $('#authPass').value = ''; }
      else $('#authHint').textContent = r.message || '注册失败';
    }
  } catch (e) { $('#authHint').textContent = e.message; }
  btn.disabled = false;
}
function afterLogin(r) {
  state.user = r.user || r.message || state.user;
  state.isAdmin = !!r.isAdmin;
  $('#overlay').classList.remove('show');
  toast('欢迎，' + state.user, 'ok');
  applyUserUI(); refreshAccountPage(); refreshUnread();
}
function applyUserUI() {
  const u = state.user || '未登录';
  $('#footUser').textContent = u;
  $('#chipName').textContent = u;
  $('#footRole').textContent = state.isAdmin ? '管理员' : (state.user ? '已登录' : '点击右上角登录');
  $('#chipAv').textContent = (state.user || '?').slice(0, 1).toUpperCase();
  if (state.user) {
    $('#accLoginCard').style.display = 'none';
    $('#accInfoCard').style.display = '';
    $('#accNotifCard').style.display = state.isAdmin ? '' : 'none';
    $('#accName').textContent = state.user;
    $('#accRole').textContent = state.isAdmin ? '管理员' : '普通用户';
  } else {
    $('#accLoginCard').style.display = '';
    $('#accInfoCard').style.display = 'none';
    $('#accNotifCard').style.display = 'none';
  }
  if (state.isAdmin) $('#notifList') && renderNotifs();
}

/* ---------- 私信 ---------- */
async function refreshContacts() {
  if (!state.user) { $('#contactList').innerHTML = '<div class="muted center">请先登录</div>'; return; }
  let users = [];
  try { users = await api.getUsers(); } catch (e) {}
  const box = $('#contactList'); box.innerHTML = '';
  const peers = [...new Set([...users, ...(LS.get('recentPeers', []))])];
  peers.sort();
  if (!peers.length) box.innerHTML = '<div class="muted center">暂无其他用户，输入用户名添加</div>';
  peers.forEach((p) => {
    const d = document.createElement('div');
    d.className = 'contact' + (p === state.peer ? ' active' : '');
    d.innerHTML = '<span class="dot"></span>' + esc(p);
    d.onclick = () => { state.peer = p; selectPeer(p); };
    box.appendChild(d);
  });
}
async function selectPeer(peer) {
  state.peer = peer;
  $('#msgPeerTitle').textContent = '与 ' + peer + ' 的对话';
  $$('.contact').forEach((c) => c.classList.toggle('active', c.textContent === peer));
  const msgs = await api.getConv(peer).catch(() => []);
  const log = $('#msgLog'); log.innerHTML = '';
  if (!msgs.length) log.innerHTML = '<div class="muted center">还没有消息</div>';
  msgs.forEach((m) => {
    const me = m.from === state.user;
    const d = document.createElement('div');
    d.className = 'bubble ' + (me ? 'me' : 'bot');
    d.textContent = m.content;
    log.appendChild(d);
  });
  log.scrollTop = log.scrollHeight;
  api.markRead(peer).catch(() => {});
  refreshUnread();
}
async function sendMsg() {
  const txt = $('#msgText').value.trim();
  if (!state.peer) { toast('请先选择联系人', 'warn'); return; }
  if (!txt) return;
  const r = await api.sendMsg(state.peer, txt).catch(() => ({ ok: false, message: '发送失败' }));
  if (r.ok) {
    $('#msgText').value = '';
    const log = $('#msgLog');
    const d = document.createElement('div'); d.className = 'bubble me'; d.textContent = txt;
    log.appendChild(d); log.scrollTop = log.scrollHeight;
  } else toast(r.message || '发送失败', 'err');
}
async function refreshUnread() {
  if (!state.user) { $('#navUnread').style.display = 'none'; return; }
  try {
    const n = await api.unread();
    $('#navUnread').style.display = n > 0 ? '' : 'none';
    $('#navUnread').textContent = n > 99 ? '99+' : n;
  } catch (e) {}
}

/* ---------- AI ---------- */
let aiWaiting = false;
async function sendAi() {
  const txt = $('#aiInput').value.trim();
  if (!txt || aiWaiting) return;
  aiWaiting = true;
  addBubble('me', txt);
  $('#aiInput').value = '';
  const wait = addBubble('bot', '思考中…');
  const r = await api.aiChat(txt).catch((e) => ({ ok: false, reply: '⚠️ ' + e.message }));
  wait.innerHTML = r.ok ? mdToHtml(r.reply) : mdToHtml(r.reply || '请求失败');
  $('#aiLog').scrollTop = $('#aiLog').scrollHeight;
  aiWaiting = false;
  if (r.left != null) $('#aiQuota').textContent = r.left < 0 ? '自备 Key 无限' : '剩余 ' + r.left + ' 次';
  const m = await api.aiQuota().catch(() => ({}));
  if (m.mode) $('#aiMode').textContent = { server: '服务器模式', key: '自备 Key', none: '未配置' }[m.mode] || m.mode;
}
function addBubble(who, text) {
  const log = $('#aiLog');
  const b = document.createElement('div');
  b.className = 'bubble ' + who;
  b.textContent = text;
  log.appendChild(b);
  log.scrollTop = log.scrollHeight;
  return b;
}
async function refreshAiQuota() {
  const q = await api.aiQuota().catch(() => ({}));
  $('#aiQuota').textContent = q.mode === 'key' ? '自备 Key · 无限' : (q.left == null ? '剩余 -- 次' : '剩余 ' + q.left + ' 次');
  $('#aiMode').textContent = { server: '服务器模式', key: '自备 Key', none: '未配置' }[q.mode] || '--';
}

/* ---------- 数据生成 ---------- */
function toggleGenFields() {
  const t = $('#genType').value;
  const isStr = ['2', '3', '4'].includes(t);
  $('#labRange1').style.display = isStr ? 'none' : '';
  $('#labRange2').style.display = isStr ? 'none' : '';
  $('#labStrLen').style.display = isStr ? '' : 'none';
  if (isStr) { $('#labRange1').style.display = 'none'; $('#labRange2').style.display = 'none'; $('#labStrLen').style.display = ''; }
}
function genLogLine(msg) { $('#genLog').textContent += msg + '\n'; $('#genLog').scrollTop = $('#genLog').scrollHeight; }
async function runGen() {
  const params = {
    startId: Number($('#genStart').value), endId: Number($('#genEnd').value),
    type: Number($('#genType').value), range1: Number($('#genR1').value), range2: Number($('#genR2').value),
    strLen: Number($('#genStrLen').value), count: Number($('#genCount').value),
    writeCount: $('#genWriteCnt').checked, extraVars: $('#genExtra').value,
    customCharset: $('#genCharset').value.trim(), outDir: $('#genOutDir').value,
  };
  genLogLine('▶ 生成 ' + params.startId + ' ~ ' + params.endId + '（类型 ' + params.type + '）…');
  const btn = $('#genRun'); btn.disabled = true;
  try {
    const r = await api.gen(params);
    genLogLine((r.ok ? '✔ ' : '✘ ') + (r.message || ''));
    if (r.ok && r.preview) showGenFiles(r.preview);
    else if (r.ok) renderGenFileNames(r.files || []);
    if (!r.ok) toast(r.message, 'err');
  } catch (e) { genLogLine('✘ ' + e.message); toast(e.message, 'err'); }
  btn.disabled = false;
}
function showGenFiles(files) {
  const box = $('#genFiles');
  box.innerHTML = '<div class="gen-files-title">已生成 ' + files.length + ' 个文件（网页模式可下载）：</div>' +
    files.map((f) => '<div class="gen-file"><span>' + esc(f.name) + '</span><button class="btn btn-sm" data-dl="' + esc(f.name) + '">下载</button></div>').join('');
  box.querySelectorAll('[data-dl]').forEach((b) => {
    b.onclick = () => {
      const f = files.find((x) => x.name === b.dataset.dl);
      if (f) downloadText(f.name, f.content);
    };
  });
  box.style.display = '';
}
function renderGenFileNames(names) {
  const box = $('#genFiles');
  box.innerHTML = '<div class="gen-files-title">已生成 ' + names.length + ' 个文件：</div>' + names.map((n) => '<div class="gen-file"><span>' + esc(n) + '</span></div>').join('');
  box.style.display = '';
}
async function previewGen() {
  const r = await api.preview().catch(() => ({ ok: false }));
  const box = $('#genPreview'); box.style.display = 'block';
  const files = r.preview || r || [];
  if (!files.length) { box.innerHTML = '<div class="muted">还没有生成结果</div>'; return; }
  box.innerHTML = files.map((f) => '<div class="pf"><div class="fn">' + esc(f.name) + '</div><pre>' + esc((f.content || '').slice(0, 2000)) + '</pre></div>').join('');
}

function genCfgIni() {
  return '[Settings]\n' +
    'Start=' + $('#genStart').value + '\n' +
    'End=' + $('#genEnd').value + '\n' +
    'Count=' + $('#genCount').value + '\n' +
    'Range1=' + $('#genR1').value + '\n' +
    'Range2=' + $('#genR2').value + '\n' +
    'StrLen=' + $('#genStrLen').value + '\n' +
    'Type=' + $('#genType').value + '\n';
}
function applyCfgIni(txt) {
  const g = (k) => { const m = String(txt || '').match(new RegExp('^' + k + '=(.*)$', 'm')); return m ? m[1].trim() : null; };
  if (g('start')) $('#genStart').value = g('start');
  if (g('end')) $('#genEnd').value = g('end');
  if (g('count')) $('#genCount').value = g('count');
  if (g('range1')) $('#genR1').value = g('range1');
  if (g('range2')) $('#genR2').value = g('range2');
  if (g('strlen')) $('#genStrLen').value = g('strlen');
  if (g('type')) $('#genType').value = g('type');
}
async function saveGenCfg() {
  const r = await api.saveTextFile('cdc_gen_cfg.ini', genCfgIni()).catch((e) => ({ ok: false, message: e.message }));
  toast(r.message || (r.ok ? '已保存' : '保存失败'), r.ok ? 'ok' : 'err');
}
async function loadGenCfg() {
  const r = await api.openTextFile().catch((e) => ({ ok: false, message: e.message }));
  if (!r.ok) { toast(r.message || '已取消', 'warn'); return; }
  applyCfgIni(r.content);
  toast('配置已加载', 'ok');
}

/* ---------- 分组生成 ---------- */
let groups = [];
function loadGroups() { groups = LS.get('groups', []); if (!groups.length) { groups = [{ fields: [{ name: '字段1', type: 2, r1: 1, r2: 10 }, { name: '字段2', type: 0, r1: 1, r2: 999999 }] }]; } }
function renderGroups() {
  const box = $('#groupBox'); box.innerHTML = '';
  if (!groups.length) { box.innerHTML = '<div class="muted">还没有分组，点「+ 添加组」创建</div>'; return; }
  groups.forEach((grp, gi) => {
    const g = document.createElement('div');
    g.className = 'group-card';
    let html = '<div class="group-head">组 ' + (gi + 1) + ' <button class="btn btn-sm" data-grm="' + gi + '">- 删除组</button></div>';
    grp.fields.forEach((f, fi) => {
      html += '<div class="group-field">' +
        '<input type="text" value="' + esc(f.name) + '" data-gf="' + gi + ',' + fi + ',name" placeholder="字段名">' +
        '<select data-gf="' + gi + ',' + fi + ',type">' +
        ['整数(随机)', '整数(不重复)', '小写字母', '大写字母', '混合字母'].map((o, i) => '<option value="' + i + '"' + (Number(f.type) === i ? ' selected' : '') + '>' + o + '</option>').join('') +
        '</select>' +
        '<input type="number" value="' + esc(f.r1) + '" data-gf="' + gi + ',' + fi + ',r1" title="范围起/字符串长度">' +
        '<input type="number" value="' + esc(f.r2) + '" data-gf="' + gi + ',' + fi + ',r2" title="范围止">' +
        '<button class="btn btn-sm danger" data-gfr="' + gi + ',' + fi + '">×</button></div>';
    });
    html += '<div class="group-foot"><button class="btn btn-sm" data-gfa="' + gi + '">+ 字段</button></div>';
    g.innerHTML = html;
    box.appendChild(g);
  });
  box.querySelectorAll('[data-gf]').forEach((el) => {
    el.onchange = () => {
      const [gi, fi, k] = el.dataset.gf.split(',');
      groups[Number(gi)].fields[Number(fi)][k] = el.value;
      saveGroups();
    };
  });
  box.querySelectorAll('[data-grm]').forEach((b) => { b.onclick = () => { groups.splice(Number(b.dataset.grm), 1); saveGroups(); renderGroups(); }; });
  box.querySelectorAll('[data-gfr]').forEach((b) => { b.onclick = () => { const [gi, fi] = b.dataset.gfr.split(','); groups[Number(gi)].fields.splice(Number(fi), 1); saveGroups(); renderGroups(); }; });
  box.querySelectorAll('[data-gfa]').forEach((b) => { b.onclick = () => { const gi = Number(b.dataset.gfa); groups[gi].fields.push({ name: 'F' + (groups[gi].fields.length + 1), type: 0, r1: 1, r2: 100 }); saveGroups(); renderGroups(); }; });
}
function saveGroups() { LS.set('groups', groups); }
async function runGroups() {
  if (!groups.length) { toast('请先添加分组', 'warn'); return; }
  const count = Number($('#grpCount').value) || 1;
  const r = await api.genGroup(groups, count).catch((e) => ({ ok: false, message: e.message }));
  $('#grpLog').textContent += (r.ok ? '✔ ' : '✘ ') + (r.message || '') + '\n';
  if (r.ok && r.preview) showGenFiles(r.preview);
  toast(r.ok ? '分组生成完成' : '分组生成失败', r.ok ? 'ok' : 'err');
}

/* ---------- 工具 ---------- */
function buildTools() {
  const grid = $('#toolGrid'); grid.innerHTML = '';
  TOOLS.forEach((t) => {
    const b = document.createElement('button');
    b.className = 'tool-btn';
    b.innerHTML = '<span class="ti">' + t.icon + '</span><span>' + t.label + '</span>';
    b.onclick = () => onTool(t);
    grid.appendChild(b);
  });
}
function openModal(title, html) {
  $('#modalTitle').textContent = title;
  $('#modalBody').innerHTML = html;
  $('#modal').classList.add('show');
}
function onTool(t) {
  if (t.kind === 'tool') {
    if (MODE === 'qt') { api.runTool(t.cmd).then((ok) => toast(ok ? '已启动：' + t.label : '启动失败：' + t.label, ok ? 'ok' : 'err')); }
    else toast('「' + t.label + '」为系统工具，需 Qt 桌面版', 'warn');
  } else if (t.kind === 'url') {
    window.open(t.cmd, '_blank');
  } else if (t.kind === 'prompt-url') {
    const u = prompt('输入网址：'); if (u) window.open(/^https?:/.test(u) ? u : 'https://' + u, '_blank');
  } else if (t.kind === 'prompt-qr') {
    const v = prompt('二维码内容：'); if (v) openModal('二维码', '<img src="https://api.qrserver.com/v1/create-qr-code/?size=240x240&data=' + encodeURIComponent(v) + '" style="display:block;margin:0 auto">');
  } else if (t.kind === 'prompt-shutdown') {
    const s = prompt('定时关机（秒，0=取消）：'); if (s == null) return;
    if (MODE === 'qt') { const n = Number(s); api.runTool(n > 0 ? 'shutdown -s -t ' + n : 'shutdown -a').then(() => toast(n > 0 ? '已设置 ' + n + ' 秒后关机' : '已取消定时关机', 'ok')); }
    else toast('定时关机需 Qt 桌面版', 'warn');
  } else if (t.kind === 'prompt-countdown') {
    const s = Number(prompt('倒计时秒数：')); if (!s || s <= 0) return;
    if (MODE === 'qt') { api.runTool('cmd /c timeout /t ' + s + ' && msg * 时间到！').then(() => toast('倒计时 ' + s + ' 秒已启动', 'ok')); }
    else toast('倒计时需 Qt 桌面版', 'warn');
  } else if (t.kind === 'calc') {
    if (t.calc === 'password') openModal('密码生成', '<div class="kv"><span><b>长度</b></span><span><input type="number" id="pwLen" value="16" min="4" max="64"></span></div><button class="btn primary" id="pwGo">生成</button><pre class="log" id="pwOut" style="margin-top:10px"></pre>');
    if (t.calc === 'md5') openModal('MD5 校验', '<textarea id="md5In" class="editor" style="min-height:80px" placeholder="输入文本…"></textarea><button class="btn primary" id="md5Go">计算 MD5</button><pre class="log" id="md5Out" style="margin-top:10px"></pre>');
    if (t.calc === 'base64') openModal('Base64 编解码', '<textarea id="b64In" class="editor" style="min-height:80px" placeholder="输入文本…"></textarea><div class="tool-row"><button class="btn primary" id="b64Enc">编码</button><button class="btn" id="b64Dec">解码</button></div><pre class="log" id="b64Out" style="margin-top:10px"></pre>');
    if (t.calc === 'json') openModal('JSON 格式化', '<textarea id="jsonIn" class="editor" style="min-height:140px" placeholder="粘贴 JSON…"></textarea><button class="btn primary" id="jsonGo">格式化</button><pre class="log" id="jsonOut" style="margin-top:10px"></pre>');
    wireCalcModal(t.calc);
  }
}
function wireCalcModal(kind) {
  const go = (btnId, outId, fn) => {
    const b = $(btnId); if (!b) return;
    b.onclick = () => { const v = fn(); $(outId).textContent = v; };
  };
  if (kind === 'password') go('#pwGo', '#pwOut', () => genPassword(Number($('#pwLen').value) || 16));
  if (kind === 'md5') go('#md5Go', '#md5Out', () => md5($('#md5In').value));
  if (kind === 'base64') { go('#b64Enc', '#b64Out', () => b64Encode($('#b64In').value)); go('#b64Dec', '#b64Out', () => b64Decode($('#b64In').value)); }
  if (kind === 'json') go('#jsonGo', '#jsonOut', () => jsonFormat($('#jsonIn').value));
}

/* ---------- GitHub 代理 ---------- */
function ghBuild(pre, raw) {
  let r = (raw || '').trim();
  if (!r) r = 'https://github.com';
  if (!/github\.(com|usercontent\.com|io)/.test(r)) return '';
  let p = (pre || 'https://ghproxy.net/').trim();
  if (!/\/$/.test(p)) p += '/';
  return p + r;
}
function ghAction(copyOnly) {
  const u = ghBuild($('#ghPre').value, $('#ghUrl').value);
  if (!u) { toast('请输入有效的 GitHub 链接', 'err'); return; }
  if (copyOnly) { navigator.clipboard.writeText(u).then(() => toast('已复制加速链接', 'ok')).catch(() => toast('复制失败', 'err')); return; }
  window.open(u, '_blank');
}

/* ---------- 账号中心 ---------- */
async function refreshAccountPage() {
  applyUserUI();
  if (state.user) {
    const q = await api.aiQuota().catch(() => ({}));
    $('#accQuota').textContent = q.mode === 'key' ? '自备 Key 无限' : ((q.left == null ? '--' : q.left) + ' 次');
    renderNotifs();
  }
}
async function renderNotifs() {
  const notifs = await api.getNotifs().catch(() => []);
  const box = $('#notifList'); box.innerHTML = '';
  if (!notifs.length) { box.innerHTML = '<div class="muted">暂无公告</div>'; return; }
  notifs.forEach((n) => {
    const d = document.createElement('div');
    d.className = 'notif-item';
    d.innerHTML = '<div class="nt"><b>' + esc(n.title) + '</b><small>' + new Date(Number(n.time)).toLocaleString() + '</small></div>' +
      '<div class="nc">' + esc(n.content) + ' <span class="muted">（' + esc(n.createdBy || '') + '）</span></div>' +
      (state.isAdmin ? '<button class="btn btn-sm danger" data-ndel="' + esc(n.id) + '">删除</button>' : '');
    box.appendChild(d);
  });
  box.querySelectorAll('[data-ndel]').forEach((b) => {
    b.onclick = async () => { await api.delNotif(b.dataset.ndel).catch(() => {}); renderNotifs(); };
  });
}

/* ---------- 设置 ---------- */
async function refreshSettings() {
  try {
    const info = await api.getInfo();
    $('#setGpp').value = info.gpp || '';
    $('#setOutDir').value = info.outDir || '';
    $('#setServer').value = info.server || '';
    const ai = await api.getAiConf().catch(() => ({}));
    $('#setAiKey').value = ai.key || '';
    $('#setAiModel').value = ai.model || 'qwen-plus';
    $('#setAiBase').value = ai.base || 'https://dashscope.aliyuncs.com/compatible-mode/v1';
    applyTheme(info.theme || 'default');
  } catch (e) {}
}

/* ---------- 更新检查 ---------- */
async function checkUpdate() {
  const r = await api.checkUpdate().catch(() => ({ hasUpdate: false }));
  const box = $('#updBox');
  if (r.hasUpdate) {
    box.innerHTML = '<span class="chip green">发现新版本 ' + esc(r.version) + '</span> ' +
      (r.url ? '<button class="btn btn-sm" id="updGo">前往下载</button>' : '') + '<br><span class="muted">' + esc(r.notice || '') + '</span>';
    const go = $('#updGo'); if (go) go.onclick = () => { api.openUrl(r.url).catch(() => window.open(r.url, '_blank')); };
    toast('发现新版本 ' + r.version, 'ok');
  } else {
    box.innerHTML = '<span class="chip green">已是最新版本（v7.1.0）</span>';
    toast('已是最新版本', 'ok');
  }
}

/* ============================================================
   事件绑定
   ============================================================ */
function bind() {
  // 导航
  $$('.nav-item').forEach((b) => { b.onclick = () => switchPage(b.dataset.page); });

  // 顶部栏
  $('#btnTheme').onclick = () => {
    const next = document.documentElement.dataset.theme === 'dark' ? 'default' : 'dark';
    applyTheme(next); api.setTheme(next).catch(() => {});
  };
  $('#btnFrost').onclick = () => setFrost(!document.body.classList.contains('frosted'));
  $('#userChip').onclick = () => {
    if (state.user) { if (confirm('退出当前账号？')) { api.logout().then(() => { state.user = ''; state.isAdmin = false; applyUserUI(); $('#overlay').classList.add('show'); toast('已退出', 'ok'); }); } }
    else switchPage('account');
  };

  // 登录遮罩
  $('#tabLogin').onclick = () => setAuthMode('login');
  $('#tabReg').onclick = () => setAuthMode('register');
  $('#btnAuth').onclick = doAuth;
  $('#authPass').addEventListener('keydown', (e) => { if (e.key === 'Enter') doAuth(); });
  $('#authSkip').onclick = () => { $('#overlay').classList.remove('show'); toast('已跳过登录，多数功能可用', 'warn'); };

  // 数据生成
  $('#genType').addEventListener('change', toggleGenFields);
  $('#genBrowse').onclick = async () => { if (MODE === 'qt') { const d = await api.browseFolder('选择输出目录'); if (d) $('#genOutDir').value = d; } else toast('网页模式输出到桌面', 'warn'); };
  $('#genTsDir').onclick = async () => { const d = await api.makeTsDir(); $('#genOutDir').value = d; toast('已设置时间戳目录：' + d, 'ok'); };
  $('#genRun').onclick = runGen;
  $('#genPreviewBtn').onclick = previewGen;
  $('#genClear').onclick = async () => { await api.clearGen(); $('#genFiles').style.display = 'none'; $('#genPreview').style.display = 'none'; $('#genLog').textContent = ''; toast('已清空', 'ok'); };
  $('#genExportCsv').onclick = async () => { const r = await api.exportCsv(); toast(r.message || '已导出', 'ok'); };
  $('#genStats').onclick = async () => { const s = await api.stats(); openModal('统计', '<div class="kv"><span><b>.in 文件</b></span><span>' + s.inCount + '</span></div><div class="kv"><span><b>.out 文件</b></span><span>' + s.outCount + '</span></div>'); };
  $('#genRename').onclick = async () => { const r = await api.renameAll(); toast(r.message || '完成', r.ok ? 'ok' : 'warn'); };
  $('#genBig').onclick = async () => { const r = await api.bigFile(); toast(r.message || '完成', r.ok ? 'ok' : 'warn'); };
  $('#genView').onclick = async () => { if (MODE === 'qt') api.runTool('notepad'); else toast('网页模式请用「下载」按钮查看文件', 'warn'); };
  $('#genChangeExt').onclick = async () => { const r = await api.changeExt(); toast(r.message || '完成', r.ok ? 'ok' : 'warn'); };
  $('#genSaveCfg').onclick = saveGenCfg;
  $('#genLoadCfg').onclick = loadGenCfg;

  // 分组
  $('#grpAdd').onclick = () => { groups.push({ fields: [{ name: '字段1', type: 0, r1: 1, r2: 100 }] }); saveGroups(); renderGroups(); };
  $('#grpRun').onclick = runGroups;
  $('#grpSave').onclick = () => { saveGroups(); toast('分组配置已保存', 'ok'); };

  // 手动
  $('#manSave').onclick = async () => {
    const name = ($('#manName').value.trim() || 'example').replace(/\.in$/i, '');
    const content = $('#manContent').value;
    if (!content.trim()) { toast('内容为空', 'warn'); return; }
    if (MODE === 'qt') {
      const r = await api.manSave(name, content);
      toast(r.message || '已保存', r.ok ? 'ok' : 'err');
    } else {
      const r = await api.manSave(name, content);
      toast(r.message || '已下载', 'ok');
    }
  };

  // 自编译
  $('#cmpCppBrowse').onclick = async () => { if (MODE === 'qt') { const f = await api.browseFile('选择 .cpp'); if (f) $('#cmpCpp').value = f; } else toast('需 Qt 桌面版', 'warn'); };
  $('#cmpGppBrowse').onclick = async () => { if (MODE === 'qt') { const f = await api.browseFile('选择 g++.exe'); if (f) { $('#cmpGpp').value = f; $('#setGpp').value = f; api.setGpp(f); } } else toast('需 Qt 桌面版', 'warn'); };
  $('#cmpGppAuto').onclick = async () => { if (MODE === 'qt') { const g = await api.autoGpp(); if (g) { $('#cmpGpp').value = g; $('#setGpp').value = g; toast('找到 g++：' + g, 'ok'); } else toast('未找到 g++', 'err'); } else toast('需 Qt 桌面版', 'warn'); };
  $('#cmpBatch').onclick = async () => {
    if (MODE !== 'qt') { toast('编译需 Qt 桌面版', 'warn'); return; }
    $('#cmpLog').textContent += '▶ 批量编译运行 ' + $('#cmpStart').value + ' ~ ' + $('#cmpEnd').value + '…\n';
    const r = await api.compileBatch($('#cmpCpp').value.trim(), $('#cmpGpp').value.trim(), Number($('#cmpStart').value), Number($('#cmpEnd').value));
    $('#cmpLog').textContent += (r.ok ? '✔ ' : '✘ ') + (r.output || '') + '\n';
  };
  $('#cmpSingle').onclick = async () => {
    if (MODE !== 'qt') { toast('编译需 Qt 桌面版', 'warn'); return; }
    $('#cmpLog').textContent += '▶ 单文件编译运行…\n';
    const r = await api.compileSingle($('#cmpCpp').value.trim(), $('#cmpGpp').value.trim());
    $('#cmpLog').textContent += (r.ok ? '✔ ' : '✘ ') + (r.output || '') + '\n';
  };

  // 打包
  $('#zipBrowse').onclick = async () => { if (MODE === 'qt') { const d = await api.browseFolder('选择源目录'); if (d) $('#zipSrc').value = d; } else toast('需 Qt 桌面版', 'warn'); };
  $('#zipRun').onclick = async () => {
    if (MODE !== 'qt') { toast('打包需 Qt 桌面版', 'warn'); return; }
    const r = await api.packZip($('#zipName').value, $('#zipSrc').value);
    $('#zipLog').textContent += (r.ok ? '✔ ' : '✘ ') + (r.message || r.output || '') + '\n';
    toast(r.ok ? '打包完成' : '打包失败', r.ok ? 'ok' : 'err');
  };

  // 工具
  buildTools();
  $('#ghOpen').onclick = () => ghAction(false);
  $('#ghHome').onclick = () => { $('#ghUrl').value = ''; ghAction(false); };
  $('#ghCopy').onclick = () => ghAction(true);

  // 账号
  $('#accLogin').onclick = async () => {
    const u = $('#accUser').value.trim(), p = $('#accPass').value;
    if (!u || !p) { $('#accLog').textContent = '请输入用户名和密码'; return; }
    const r = await api.login(u, p);
    $('#accLog').textContent = r.message || (r.ok ? '登录成功' : '登录失败');
    if (r.ok) afterLogin(r);
  };
  $('#accReg').onclick = async () => {
    const u = $('#accUser').value.trim(), p = $('#accPass').value;
    if (!u || !p) { $('#accLog').textContent = '请输入用户名和密码'; return; }
    const r = await api.register(u, p);
    $('#accLog').textContent = r.message || '';
    if (r.ok) toast(r.message, 'ok');
  };
  $('#accLogout').onclick = async () => { await api.logout(); state.user = ''; state.isAdmin = false; applyUserUI(); toast('已退出', 'ok'); };
  $('#accRename').onclick = async () => {
    const n = prompt('新用户名：'); if (!n) return;
    const r = await api.renameUser(n.trim());
    if (r.ok) { state.user = n.trim(); applyUserUI(); toast(r.message, 'ok'); } else toast(r.message, 'err');
  };
  $('#accActivate').onclick = () => {
    openModal('激活码', '<input type="text" id="payCode" placeholder="输入激活码" style="width:100%"><button class="btn primary" id="payGo" style="margin-top:10px">激活（+15 次 AI 通话）</button><pre class="log" id="payOut" style="margin-top:10px"></pre>');
    $('#payGo').onclick = async () => {
      const code = $('#payCode').value.trim();
      if (!code) return;
      $('#payOut').textContent = '正在验证…';
      const r = await api.activate(code);
      $('#payOut').textContent = r.message || '';
      refreshAccountPage();
    };
  };
  $('#notifPost').onclick = async () => {
    const t = $('#notifTitle').value.trim(), c = $('#notifContent').value.trim();
    if (!t) { toast('请输入标题', 'warn'); return; }
    const r = await api.postNotif(t, c);
    if (r.ok) { $('#notifTitle').value = ''; $('#notifContent').value = ''; renderNotifs(); toast('已发布', 'ok'); }
    else toast(r.message, 'err');
  };
  $('#accUpdate').onclick = checkUpdate;

  // 私信
  $('#msgAddPeer').onclick = () => {
    const p = $('#msgSearch').value.trim();
    if (!p) return;
    const rec = LS.get('recentPeers', []);
    if (!rec.includes(p)) { rec.push(p); LS.set('recentPeers', rec.slice(-20)); }
    state.peer = p; selectPeer(p); refreshContacts();
  };
  $('#msgSearch').addEventListener('keydown', (e) => { if (e.key === 'Enter') $('#msgAddPeer').click(); });
  $('#msgSend').onclick = sendMsg;
  $('#msgText').addEventListener('keydown', (e) => { if (e.key === 'Enter') sendMsg(); });

  // AI
  $('#aiSend').onclick = sendAi;
  $('#aiInput').addEventListener('keydown', (e) => { if (e.key === 'Enter' && !e.shiftKey) { e.preventDefault(); sendAi(); } });
  $('#aiClear').onclick = () => { $('#aiLog').innerHTML = ''; toast('已清空对话', 'ok'); };

  // 设置
  $('#setFrosted').onchange = (e) => setFrost(e.target.checked);
  $('#setGppBrowse').onclick = async () => { if (MODE === 'qt') { const f = await api.browseFile('选择 g++.exe'); if (f) { $('#setGpp').value = f; api.setGpp(f); } } else toast('需 Qt 桌面版', 'warn'); };
  $('#setGppAuto').onclick = async () => { if (MODE === 'qt') { const g = await api.autoGpp(); if (g) { $('#setGpp').value = g; api.setGpp(g); toast('找到 g++：' + g, 'ok'); } else toast('未找到 g++', 'err'); } else toast('需 Qt 桌面版', 'warn'); };
  $('#setGpp').addEventListener('change', () => api.setGpp($('#setGpp').value).catch(() => {}));
  $('#setOutDirBrowse').onclick = async () => { if (MODE === 'qt') { const d = await api.browseFolder('选择输出目录'); if (d) { $('#setOutDir').value = d; api.setOutDir(d); } } else toast('网页模式输出到桌面', 'warn'); };
  $('#setServerSave').onclick = async () => { await api.setServer($('#setServer').value); state.server = $('#setServer').value.trim(); toast('服务器已保存', 'ok'); refreshSettings(); };
  $('#setAiSave').onclick = async () => {
    await api.setAiConf($('#setAiKey').value, $('#setAiModel').value, $('#setAiBase').value);
    toast('AI 配置已保存', 'ok');
  };
  $('#setCfgReset').onclick = async () => {
    await api.setServer(''); await api.setAiConf('', 'qwen-plus', 'https://dashscope.aliyuncs.com/compatible-mode/v1');
    $('#setServer').value = ''; $('#setAiKey').value = ''; $('#setAiModel').value = 'qwen-plus'; $('#setAiBase').value = 'https://dashscope.aliyuncs.com/compatible-mode/v1';
    toast('已恢复默认', 'ok');
  };
  $('#setBgPick').onclick = async () => { if (MODE === 'qt') { const f = await api.browseFile('选择背景图片'); if (f) { api.setBgImage(f); toast('背景已设置', 'ok'); } } else toast('背景图片需 Qt 桌面版', 'warn'); };
  $('#setBgClear').onclick = () => api.clearBgImage().then(() => toast('背景已清除', 'ok')).catch(() => toast('需 Qt 桌面版', 'warn'));
  $('#btnAbout').onclick = () => {
    openModal('关于 CDC 测试数据生成器',
      '<div class="kv"><span><b>版本</b></span><span>v7.1.0</span></div>' +
      '<div class="kv"><span><b>作者</b></span><span>starryfast · Bilibili 同名</span></div>' +
      '<div class="kv"><span><b>界面</b></span><span>HTML / CSS / JS</span></div>' +
      '<div class="kv"><span><b>后端</b></span><span>' + (MODE === 'qt' ? 'C++ Bridge（Qt）' : 'Web 模式（localStorage + 云端）') + '</span></div>' +
      '<div class="kv"><span><b>功能</b></span><span>数据生成 / 自编译 / 打包 / 工具 / 账号 / 私信 / AI</span></div>');
  };

  // 主题网格
  const grid = $('#themeGrid'); grid.innerHTML = '';
  THEMES.forEach((th) => {
    const c = document.createElement('div');
    c.className = 'theme-chip'; c.dataset.theme = th.name;
    c.innerHTML = '<div class="theme-swatch" style="background:' + th.sw + '"></div>' + th.label;
    c.onclick = () => { applyTheme(th.name); api.setTheme(th.name).catch(() => {}); toast('主题：' + th.label, 'ok'); };
    grid.appendChild(c);
  });

  // 弹窗
  $('#modalClose').onclick = () => $('#modal').classList.remove('show');
  $('#modal').addEventListener('click', (e) => { if (e.target.id === 'modal') $('#modal').classList.remove('show'); });

  // 生成日志初始提示
  genLogLine('CDC 数据生成器 v7.1.0 · ' + (MODE === 'qt' ? 'Qt 桌面模式（全功能）' : '网页模式'));
}

/* ============================================================
   启动
   ============================================================ */
window.addEventListener('load', () => {
  buildApi();
  setModeChip();

  if (MODE === 'qt') {
    new QWebChannel(qt.webChannelTransport, (ch) => {
      qtBridge = ch.objects.bridge;
      // 信号
      if (qtBridge.logUpdated && qtBridge.logUpdated.connect) qtBridge.logUpdated.connect((m) => genLogLine(m));
      if (qtBridge.statusUpdated && qtBridge.statusUpdated.connect) qtBridge.statusUpdated.connect((m) => {});
      init();
    });
  } else {
    init();
  }
});

async function init() {
  // 主题恢复
  try {
    const theme = MODE === 'qt' ? (await api.getTheme()) : LS.get('theme', 'default');
    applyTheme(theme || 'default');
  } catch (e) {}
  try { const f = LS.get('frost', false); if (f) setFrost(true); } catch (e) {}

  bind();
  loadGroups(); renderGroups();
  toggleGenFields();

  // 登录态恢复
  try {
    const info = await api.getInfo();
    state.user = info.user || ''; state.isAdmin = !!info.isAdmin;
    state.server = info.server || ''; state.gpp = info.gpp || '';
    applyUserUI();
    if (!state.user) { $('#overlay').classList.add('show'); toast('请先登录（也可跳过，多数功能可用）', 'warn'); }
  } catch (e) {
    $('#overlay').classList.add('show');
  }
  refreshUnread();
  refreshAiQuota();
}

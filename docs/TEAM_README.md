# Code date creation 团队官网

PHP 8.1 + MySQL。白渐变落地页 + 液态玻璃卡片 + 反馈系统（自带后台）。

## 部署：上传即用

**不需要数据库、不需要导 SQL、不需要改密码。** 把文件传到网站根目录就能跑。

唯一要确认的：`data` 目录可写（宝塔/虚拟主机一般是 755 或 777）。

反馈存在 `data/feedback.json`，后台 `/feedback_admin.php` 直接读它。

### 怎么看反馈

浏览器打开 `你的域名/feedback_admin.php`，输入 `inc/config.php` 里 `admin.pass` 的口令。

### 备份（重要）

文件存储没有数据库副本，**`data/feedback.json` 就是全部数据**。两种备份方式：

| 方式 | 怎么做 |
|---|---|
| 后台导出 | 登录后台点「导出 CSV」/「导出 JSON」，存到本地 |
| 直接拷贝 | 把 `data/feedback.json` 下载或复制走 |

**服务器迁移**时只要把 `data/feedback.json` 一起拷过去，数据就跟着走了。

### 以后想改用 MySQL

1. `inc/config.php` 里把 `site.file_feedback` 改成 `false`
2. 填上 `db` 段的 `host` / `name` / `user` / `pass`
3. 导入 `data/cdc_site.sql`

三种存储方式的自动选择顺序（`file_feedback = false` 时）：
**MySQL → SQLite → JSON 文件**，哪级可用就用哪级。

## 目录

```
index.php            落地页
cdccmd.php           CDCCMD 单独介绍页
feedback.php         反馈表单
feedback_admin.php   反馈后台（只读 + 标记已处理）
api/feedback.php     JSON 接口
inc/config.php       ★ 运行配置（部署改这里）
inc/lib.php          公共库（存储层 / CSRF / 限流 / 转义）
inc/data.php         内容唯一数据源（改文案只动这里）
data/feedback.json   ★ 反馈数据（文件存储模式下就是全部）
data/.htaccess       禁止直接下载 data 里的文件
data/cdc_site.sql    改用 MySQL 时才需要导入
assets/site.css      样式
assets/site.js       交互
assets/img/          hero 大图 / 头像 / favicon
打包/打包.py          打包脚本
```

## 安全

- 反馈字段白名单校验 + PDO 预处理（注入串当纯文本入库）
- CSRF 令牌 + 同 IP 限流 + 蜜罐字段
- 所有输出 `h()` 转义
- 后台口令 `hash_equals` 比对，不回显、不进日志

## 验收

```bash
python tools/selftest.py      # 全站自检
```

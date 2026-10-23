# CDC 云端后台 · 详细部署步骤

本文档一步一步教你把 `cdc_server/` 这套 PHP 后台部署上线，并让 C++ 客户端连上来。

涉及的文件（本地开发目录）：
```
cdc_server/
├─ config.php            全局配置（改这里切换数据库 / 密码）
├─ db.php                数据库层 + AI 代理函数
├─ admin.php             管理员后台界面
├─ index.php             入口
├─ api/activate/index.php 客户端云端接口（/api/activate）
├─ mysql_schema.sql      MySQL 建表脚本（用 MySQL 才需要）
├─ .htaccess             Apache 路由 + 保护敏感文件
├─ web.config            IIS 路由 + 保护敏感文件
├─ data/                 运行时生成，SQLite 数据库放这里
└─ cpp_demo/             生成 .out 的示例（与部署无关，可删）
```

---

## 一、服务器环境要求

| 组件 | 要求 |
|------|------|
| Web 服务器 | Apache / Nginx / IIS 均可 |
| PHP | ≥ 7.4，需启用扩展 `pdo_sqlite`（SQLite 模式）或 `pdo_mysql`（MySQL 模式） |
| 数据库 | 默认 SQLite（不用装）；可选 MySQL 5.7+/8.0 |
| PHP 设置 | `file_uploads=On`，`allow_url_fopen=On` 或已装 `curl`（AI 服务器代理要用） |
| 权限 | `data/` 目录可写；用 AI 服务器代理时 `server_key.cfg` 可写 |

> 多数虚拟主机 / 宝塔 / LNMP 默认都带 `pdo_sqlite` 和 `curl`，开箱即用。

---

## 二、上传源码到服务器（两种放法）

### 放法 A：整文件夹放到子目录（推荐，最省事）
把**整个 `cdc_server/` 文件夹**上传到网站根目录，例如：

| 环境 | 目标路径 |
|------|----------|
| 虚拟主机 | `public_html/cdc_server/` |
| Apache | `/var/www/html/cdc_server/` |
| Nginx | `/usr/share/nginx/html/cdc_server/` |
| IIS | `C:\inetpub\wwwroot\cdc_server\` |

上传后：
- 后台地址：`https://你的域名/cdc_server/admin.php`
- 客户端接口：`https://你的域名/cdc_server/api/activate`

### 放法 B：内容直接放根目录（接口更干净）
把 `cdc_server/` **里面的文件**直接丢到网站根目录（不套子目录）：
- 后台：`https://你的域名/admin.php`
- 客户端接口：`https://你的域名/api/activate`

> ⚠️ 选哪种都行，只要后面客户端 `cdc_srv.dat` 的 `server=` 和这里保持一致。

---

## 三、方案 A —— SQLite 部署（默认，零配置，推荐）

1. 用 FTP / 面板把 `cdc_server/` 传上去（见第二节）。
2. 确认 `config.php` 里是默认：
   ```php
   define('DB_TYPE', 'sqlite');        // 不用改
   define('DB_PATH', __DIR__ . '/data/cdc.db');  // 不用改
   ```
3. 给 `data/` 目录设置可写权限：
   - 虚拟主机 / 面板：文件管理器里把 `data` 设为 `755`，若不行再 `777`。
   - 命令行（Linux）：`chmod -R 755 cdc_server/data`
4. 浏览器访问一次后台 `https://你的域名/cdc_server/admin.php`，程序会自动建库 `data/cdc.db`（首次访问可能慢 1~2 秒，属正常）。
5. 完成。无需导入任何 SQL。

---

## 四、方案 B —— MySQL 部署（数据量大 / 要远程管理时）

1. 在服务器上新建数据库与账号（以 MySQL 命令行或 phpMyAdmin 为例）：
   ```sql
   CREATE DATABASE cdc_server CHARACTER SET utf8mb4;
   CREATE USER 'cdc_user'@'localhost' IDENTIFIED BY 'cdc_pass';
   GRANT ALL PRIVILEGES ON cdc_server.* TO 'cdc_user'@'localhost';
   FLUSH PRIVILEGES;
   ```
2. 导入表结构（任选一种）：
   - 命令行：`mysql -u cdc_user -p cdc_server < cdc_server/mysql_schema.sql`
   - phpMyAdmin：选 `cdc_server` 库 → 导入 → 选 `mysql_schema.sql`
3. 编辑 `config.php`，把数据库切换过去：
   ```php
   define('DB_TYPE', 'mysql');
   define('MYSQL_HOST', 'localhost');
   define('MYSQL_PORT', '3306');
   define('MYSQL_DB',   'cdc_server');
   define('MYSQL_USER', 'cdc_user');
   define('MYSQL_PASS', 'cdc_pass');   // 改成你刚设的密码
   ```
4. 访问一次后台确认能打开即可（MySQL 表已由 SQL 建好，不用再建）。

---

## 五、设置文件权限（关键，漏了会报错）

**SQLite 模式（最常见）：**
- `cdc_server/data/` → 可写（755，不行再 777）
- `cdc_server/server_key.cfg` → 可写（后台保存 AI 设置时会写它；755 即可）

**IIS 注意：** 默认无 `.htaccess` 生效，但 `web.config` 已写好保护规则（禁止下载 `.db/.cfg/.log/.exe/.dll`）。

**Nginx 注意：** `.htaccess` 不生效，需在站点配置里手动加保护（见第七节）。

---

## 六、配置 C++ 客户端连上来

客户端靠两个 cfg 文件找服务器，都在程序运行目录（或 `%APPDATA%\CDCExceed\`）。

### 1）云端地址（必填）
编辑 `cdc_srv.dat`，把 `server=` 改成你的后台域名：
```
# 子目录部署：
server=https://你的域名/cdc_server
# 根目录部署：
server=https://你的域名
```
> 接口路径 `/api/activate` 程序自动拼，**不用手填**。改完重开客户端即可注册/登录/收发私信。

### 2）AI 语言大模型（选填，两种填法）
优先级：**本机本地密钥 > 服务器代理**。

**填法① 用户本机填（推荐给高级用户，用自己的额度）**
文件 `%APPDATA%\CDCExceed\cdc_ai.cfg`（首次运行自动生成空模板）：
```
api_key=sk-你的模型Key
model=qwen-plus
base_url=https://dashscope.aliyuncs.com/compatible-mode/v1
```
- `api_key`：模型厂商密钥（阿里云百炼 / DeepSeek / OpenAI 等）
- `model`：`qwen-plus`、`deepseek-chat`、`gpt-4o-mini`…
- `base_url`：OpenAI 兼容地址，**末尾不要带 `/chat/completions`**（程序自动补）

**填法② 你在服务器统一填（推荐集中管理，所有人共用你的额度）**
1. 浏览器登录后台 `admin.php`
2. 找到「**AI 语言大模型（服务器代理设置）**」卡片
3. 填 **API Key / 模型名 / base_url** → 保存
4. 之后**本机没填 `cdc_ai.cfg` 的客户端**会自动把对话转给服务器代理；本机填了的仍用自己的密钥，互不影响。

> 服务器代理要求 PHP 开启了 `curl` 或 `allow_url_fopen`。密钥保存在 `cdc_server/server_key.cfg`，已被禁止浏览器下载。

---

## 七、Nginx 额外保护（没有就跳过）

Apache / IIS 已自带保护，Nginx 需在站点 `server {}` 块加：
```nginx
location ~* \.(db|cfg|log|exe|dll)$ { deny all; }
location ~* ^/cdc_server/data/ { deny all; }
location ~* ^/cdc_server/server_key\.cfg$ { deny all; }
# 美化 /api/activate（可选）
location /api/activate { try_files /api/activate/index.php /api/activate/index.php; }
```

---

## 八、登录后台并使用

1. 打开 `https://你的域名/cdc_server/admin.php`
2. 输入后台密码：**`config.php` 里的 `ADMIN_PASS`**（可在 `config.php` 的 `ADMIN_PASS` 改）
3. 登录后可：
   - 看 **用户数量 / 已封禁人数 / 私信条数 / API 绑定设备数 / 激活码 / 已发布更新** 总览
   - **私信内容**：按用户名过滤查看
   - **发布软件更新**：填版本名字 + 下载网址 + 说明 → 保存（客户端「检查更新」自动拉最新一条）
   - **批量生成 AI 对话激活码**：一次生成 N 个，可复制
   - **封禁 / 解封用户**
   - **AI 模型设置**：填服务器代理密钥（见第六节）
   - **设备绑定**：客户端调用 `bind_device` 自动记录，也可手动加

---

## 九、上线前安全检查清单

- [ ] `data/` 目录不可通过浏览器直接列目录（Apache 设 `Options -Indexes`，Nginx 默认不可列）
- [ ] `.db` / `.cfg` 文件无法被下载（Apache 看 `.htaccess`，IIS 看 `web.config`，Nginx 看第七节）
- [ ] 后台密码已从默认值 改成强密码（`config.php` → `ADMIN_PASS`）
- [ ] 后台 `admin.php` 建议用 HTTPS 访问（防止密码被窃听）
- [ ] MySQL 模式：数据库账号只用 `localhost`，不用 `%` 开放远程
- [ ] 若公开暴露，建议给 `admin.php` 加一层 Basic Auth 或 IP 白名单
- [ ] **删掉 web 根目录里的 `README.md` / `DEPLOY.md` / `DEPLOY-HOSTING.md` / `mysql_schema.sql`**（文档和建表 SQL 不该放在能被下载的位置；面板 / Nginx 下 `.htaccess` 不生效，**删文件才是万能解**，删完清一次 CDN 缓存）
- [ ] `config.php` 的 `CDC_API_TOKEN` 已换成自己生成的随机串，并同步重编客户端
- [ ] 自测：不带令牌 `POST /api/activate/`（`op=msg_users`）应返回 **401**；带正确令牌应返回 200
- [ ] 需要最严格时把 `CDC_TOKEN_MODE` 改成 `strict`（默认 `grace` 只拦高敏操作）
- [ ] 定期看一眼 `data/api_deny.log`，有异常来源 IP 说明有人在探接口

---

## 十、常见故障排查

| 现象 | 原因 / 解决 |
|------|------|
| 打开后台空白 / 500 | PHP 版本过低或缺少 `pdo_sqlite`；看服务器错误日志，升级 PHP 或装扩展 |
| 提示 `could not open database` | `data/` 没写权限 → 设为 755/777 |
| 客户端注册无反应 | `cdc_srv.dat` 的 `server=` 填错，或服务器接口 404（确认路径含 `/api/activate`） |
| AI 对话走服务器代理报错 | PHP 未开 `curl` / `allow_url_fopen`；或后台没填 AI 设置 |
| 保存 AI 设置失败 | `server_key.cfg` 不可写 → 755 |
| MySQL 模式连不上 | 检查 `MYSQL_*` 常量、`cdc_server` 库是否已导入 `mysql_schema.sql` |
| 访问 `server_key.cfg` 能下载 | 保护规则没生效（Nginx 需手动加，见第七节） |

---

## 十一、目录 / 接口速查

| 项 | 值 |
|----|----|
| 后台地址 | `https://你的域名/cdc_server/admin.php`（子目录）或 `https://你的域名/admin.php`（根目录） |
| 客户端接口 | `https://你的域名/cdc_server/api/activate`（子目录）或 `https://你的域名/api/activate` |
| AI 代理接口 | 同上，自动加 `?chat=1`（客户端内部使用，不用手填） |
| 客户端云端配置 | `cdc_srv.dat` 的 `server=` 一行 |
| 客户端本地 AI 密钥 | `%APPDATA%\CDCExceed\cdc_ai.cfg` |
| 服务器 AI 密钥 | 后台「AI 模型设置」卡片 → 存 `cdc_server/server_key.cfg` |
| 后台密码 | `config.php` 的 `ADMIN_PASS`（改 `config.php` 的 `ADMIN_PASS`） |
| 数据库 | 默认 `data/cdc.db`（SQLite）；MySQL 用 `mysql_schema.sql` 建表 |

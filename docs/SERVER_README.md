# CDC 云端管理后台（PHP）

根据 `WindowsProject1.cpp` 的云端账号/私信契约实现的 **PHP 管理后台**。
它同时承担两个角色：

1. **客户端云端接口** `/api/activate` —— 与 C++ 程序的 `Cloud_*` 函数完全兼容，
   让 `cdc_srv.dat` 里 `server=https://cdc.mjhcsp.xyz` 的程序能注册/登录/私信。
2. **管理员面板** `admin.php` —— 展示用户数量、私信内容、API 绑定设备数，
   批量生成 AI 对话激活码，封禁/解封用户。后台密码：见 `config.php` 的 `ADMIN_PASS`（部署后请立刻改成强密码）。

---

## 一、部署

把整个 `cdc_server/` 目录放进网站根目录（例如 `cdc.mjhcsp.xyz` 的根）。
访问：

- 管理后台： `https://cdc.mjhcsp.xyz/admin.php`  （密码见 `config.php` 的 `ADMIN_PASS`）
- 客户端接口： `https://cdc.mjhcsp.xyz/api/activate`

要求：PHP ≥ 7.4，开启 `pdo_sqlite` 扩展。确保 `data/` 目录可写
（首次访问会自动建库 `data/cdc.db`）。`.htaccess`（Apache）/ `web.config`（IIS）
已内置，用于把 `/api/activate` 路由到 `api/activate/index.php`，并禁止直接访问 `.db`/`.log`/`.exe`/`.dll`。

> 若你的虚拟主机不支持“无后缀”的 `/api/activate`，把 `cdc_srv.dat` 的
> `server` 改成 `https://你的域名/api/activate.php` 并在 `api/` 下放一个
> `activate.php`（内容同 `api/activate/index.php`）即可。

---

## 二、管理后台功能

| 功能 | 说明 |
|------|------|
| 用户数量 | 统计 `users` 表总人数、已封禁人数 |
| 私信内容 | 列出 `messages` 全部记录，支持按用户名（发/收件人）过滤 |
| API 绑定设备数 | 统计 `devices` 表绑定数量，并展示明细；客户端调用 `/api/activate` 的 `bind_device` 会自动入库 |
| 批量生成 AI 对话激活码 | 一次生成 N 个（`CDC-AI-XXXXXXXX` 格式），可选批次名与有效期，支持一键复制未使用码 |
| 封禁/解封用户 | 置 `users.banned=1`；被封禁用户登录时接口返回 `err:banned`，无法进入程序 |
| 发布软件更新 | 后台填「版本名字 + 下载网址 + 更新说明」并发布；客户端「检查更新」走 `/api/activate` 的 `version`，自动拿到最新一条 |

---

## 三、客户端接口契约（`/api/activate`，POST，form 表单）

| op | 参数 | 返回 |
|----|------|------|
| register | user, pass | `{"ok":1,"admin":0}` 或 `{"ok":0,"err":"exists"}` |
| login | user, pass | `{"ok":1,"admin":0,"uid":N}` 或 `{"ok":0}` / `{"ok":0,"err":"banned"}` |
| reset | user, pass | `{"ok":1}` |
| msg_users | — | `{"ok":1,"users":["a","b",...]}` |
| msg_send | from, to, content | `{"ok":1}` |
| msg_unread | user | `{"ok":1,"count":N}` |
| msg_read | from, to | `{"ok":1}` |
| msg_conv | user, peer | `{"ok":1,"msgs":[{"from","to","content","isread","time"}]}` |
| bind_device | user, pass, device_id, api_key? | `{"ok":1}` |
| version | — | `{"ok":1,"version":"...","url":"...","notice":"..."}` |

数据库表：`users / messages / notifications`（沿用源码）+ `activation_codes / devices / updates`（新增）。

### 客户端鉴权（X-CDC-Token）

官方客户端调用接口时都要带请求头 `X-CDC-Token: <令牌>`（也支持 POST 字段 `tk=`）。
令牌只写在 `config.php` 的 `CDC_API_TOKEN` 一处，客户端把它混淆后编译进 exe（不在明文里出现）。

`config.php` 的 `CDC_TOKEN_MODE` 决定「没带令牌」时怎么处置：

| 模式 | 行为 |
|------|------|
| `grace`（默认） | **高敏操作**一律要令牌（改密码 `reset`、私信 `msg_*`、枚举用户 `msg_users`、绑设备 `bind_device`、AI 代理 `chat=1`）；**低敏操作**（兑换 / 登录 / 注册 / 查额度 / 版本）宽限期内放行，且限速 60 次/分钟/IP |
| `strict` | 所有操作都要令牌（最安全） |
| `log` | 全部放行、只记录（排障或应急回退用，安全性最低） |

`CDC_TOKEN_GRACE_UNTIL` 到点后，`grace` 会**自动**升级为 `strict`，不需要再改配置。

没带令牌被拒的调用会记到 `data/api_deny.log`（只记时间 / IP / 原因 / 操作名，**不记令牌与请求正文**，超 512KB 自动滚动一代）。

> 轮换令牌 = 改 `config.php` 的 `CDC_API_TOKEN` + 重编两个客户端（`cdc_backend.exe`、CDC Kit），两边必须一致。

---

## 四、这套源码上传到服务器哪个目录

把 **整个 `cdc_server/` 文件夹** 上传到网站（虚拟主机 / 服务器）的 **网站根目录** 下即可。
不同环境的根目录常见位置：

| 环境 | 应上传到的目录（示例） |
|------|------------------------|
| 虚拟主机（cPanel / 面板） | `public_html/cdc_server/` 或 `www/cdc_server/`（也可直接把内容放 `public_html/` 根） |
| 自己装的 Apache | `/var/www/html/cdc_server/` |
| 自己装的 Nginx + php-fpm | `/usr/share/nginx/html/cdc_server/`（以你 `root` 指令为准） |
| Windows IIS | `C:\inetpub\wwwroot\cdc_server\` |

上传后访问地址就是：

- 管理后台： `https://你的域名/cdc_server/admin.php`
- 客户端接口： `https://你的域名/cdc_server/api/activate`

> 想让接口地址更干净（如 `https://你的域名/api/activate`），就把 `cdc_server/` 里的
> **全部内容**直接上传到网站根目录，而不是套一层 `cdc_server/` 子目录。
> 本仓库默认域名是 `cdc.mjhcsp.xyz`，所以根目录放内容后即为
> `https://cdc.mjhcsp.xyz/api/activate`。

---

## 五、客户端的 API 填在哪里（cdc_srv.dat）

客户端（`WindowsProject1.cpp`）通过 **`cdc_srv.dat`** 里的 `server` 字段知道云端地址。
它访问的接口路径固定为 **`/api/activate`**（POST，表单格式），所以你只需要：

1. 打开打包/部署里的 **`cdc_srv.dat`**（在 `打包/` 目录）。
2. 把 `server` 改成你的后台地址（**只写到域名/根，不要带 `/api/activate`**，程序会自动拼接）：

   ```
   server=https://cdc.mjhcsp.xyz
   ```

   若你把 `cdc_server` 内容放在子目录（如 `cdc_server/`），则应填：

   ```
   server=https://你的域名/cdc_server
   ```

3. 客户端启动后会自动向 `https://<server>/api/activate` 发 `register/login/msg_*/bind_device/version` 等请求。
   管理后台「发布软件更新」后，客户端「检查更新」调用的就是这个 `version` 接口。

小结：**API 地址 = `cdc_srv.dat` 里的 `server=` 那一行；接口路径固定 `/api/activate`，无需手填。**

---

## 六、AI 语言大模型填在哪里

AI 对话（千问/Qwen 风格窗口）的模型有两处可填，**二选一**，优先级：本地密钥 > 服务器代理。

### 方式 A：用户在自己电脑填（本地密钥，优先）
文件：`%APPDATA%\CDCExceed\cdc_ai.cfg`（首次运行会自动生成空模板）。
内容（OpenAI 兼容，字段通用，不只限 deepseek）：

```
api_key=你的模型APIKey
model=qwen-plus
base_url=https://dashscope.aliyuncs.com/compatible-mode/v1
```

- `api_key`：模型提供方的密钥（阿里云百炼 / DeepSeek / OpenAI 等）
- `model`：模型名，如 `qwen-plus`、`deepseek-chat`、`gpt-4o-mini`、`qwen-max`
- `base_url`：OpenAI 兼容的接口地址（末尾不要带 `/chat/completions`，程序会自动补）

> 旧版文件名 `cdc_deepseek.cfg` 同样兼容，优先读 `cdc_ai.cfg`。

### 方式 B：管理员在服务器统一填（服务器代理，推荐集中管理）
后台 `admin.php` → 「**AI 语言大模型（服务器代理设置）**」卡片，
填 API Key / 模型名 / base_url 并保存。效果：

- 客户端本机**没填** `cdc_ai.cfg` 时，会自动把对话转给
  `https://<server>/api/activate?chat=1`，由服务器用 `server_key.cfg`
  里的密钥去调模型，回传结果。即「所有用户共用管理员的模型额度」。
- 密钥存在服务器 `cdc_server/server_key.cfg`，已被 `.htaccess`/`web.config`
  禁止浏览器直接下载，请勿把它放进 `data/` 以外可遍历处。
- 客户端本机填了 `cdc_ai.cfg` 的，仍优先用自己的密钥（互不影响）。

> 若你只想让自己/团队用、不想每人配密钥，就走方式 B，在后台填一次即可。
> 本仓库的 `api/activate` 已新增 `?chat=1` 代理，无需再改客户端。

---

## 七、切换到 MySQL（数据库导入 SQL）

默认用 SQLite（零配置）。若要改用 MySQL：

1. 在 MySQL 建库并导入表结构：
   ```bash
   mysql -u 用户名 -p -e "CREATE DATABASE cdc_server CHARACTER SET utf8mb4;"
   mysql -u 用户名 -p cdc_server < cdc_server/mysql_schema.sql
   ```
   （或在 phpMyAdmin 里「导入」`mysql_schema.sql`。）
2. 编辑 `config.php`：
   ```php
   define('DB_TYPE', 'mysql');          // 默认是 'sqlite'
   define('MYSQL_HOST', 'localhost');
   define('MYSQL_PORT', '3306');
   define('MYSQL_DB',   'cdc_server');  // 第 1 步建的库名
   define('MYSQL_USER', 'cdc_user');
   define('MYSQL_PASS', 'cdc_pass');
   ```
3. 刷新后台即可，原有功能不变。`mysql_schema.sql` 已包含全部表：
   `users / messages / notifications / activation_codes / devices / updates`。

---

## 八、mingw64 “总是显示缺少啥” 修复说明

源码 `WindowsProject1.cpp` 的“生成 .out”按钮会把用户 `.cpp` 用 `g++.exe`
编译成 `temp_ac.exe` 再运行生成 `.out`。最新 mingw64 在两处会报“缺少 xxx”：

1. **缺 `libgcc_s_seh-1.dll` / `libstdc++-6.dll`** —— 用 `-static -static-libgcc
   -static-libstdc++` 静态链接，把运行时打进 exe 内部（已写入 `CompileAndRunBatch`/
   `CompileAndRunSingle`）。
2. **缺 `api-ms-win-crt-*.dll`**（Win7/8 等未装 UCRT 的系统）—— 新增
   `CopyRuntimeDeps()`，把打包目录 `文件依赖扩展/` 里的 UCRT dll 以及 mingw 运行时
   dll 一并复制到 `temp_ac.exe` 所在输出目录，实现“分离/附带”运行时。

另外把编译报错重定向到 `compile.log` 并弹窗展示，不再静默失败（解决“看不出缺什么”）。

### 单独验证（已在本机用打包内 mingw64 跑通）

```
cdc_server/cpp_demo/
  temp_ac.cpp          示例源码
  build_temp_ac.bat     一键：编译(静态) -> 复制运行时 -> 生成 .out
  # 手动：
  "打包/MinGW64/bin/g++.exe" temp_ac.cpp -o temp_ac.exe -static -static-libgcc -static-libstdc++ -O2
  temp_ac.exe < 1.in > 1.out
```

本机实测：`g++ temp_ac.cpp ...` 成功，`temp_ac.exe < 1.in > 1.out` 生成
`1.out = "3 + 4 = 7 / 10 + 20 = 30 / -1 + 5 = 4"`。

---

## 九、目录结构

```
cdc_server/
  config.php            后台密码、库路径、激活码格式
  db.php                PDO 连接 + 建表 + 工具函数
  index.php             根目录跳转 admin.php
  admin.php             管理员面板（密码见 `config.php`）
  api/activate/index.php 客户端云端接口
  .htaccess / web.config 路由 + 保护 data/
  data/                 运行时生成 cdc.db
  mysql_schema.sql      MySQL 建表导入脚本（切换到 MySQL 时用）
  server_key.cfg        服务器 AI 模型密钥（后台「AI 模型设置」写入；已禁止 web 下载）
  cpp_demo/             生成 .out 的示例源码与一键脚本
  README.md
```

## 十、激活码与每码 15 次通话机制

后台「批量生成 AI 对话激活码」默认 **每码 15 次** AI 通话（生成表单的「次/码」可改，范围 1–100000）。
客户端「激活码」按钮（付费解锁对话）完整流程：

1. 客户端 `PayActivateThread` 发 JSON `{"code","device","user"}` 到 `/api/activate`（无 op 字段）。
2. 服务器 `redeem_code()` 校验：
   - 码不存在 → `{ok:0, reason:"invalid"}`（客户端提示「激活码无效或不存在」）
   - 该码已绑满 3 台设备且本机未绑 → `{ok:0, reason:"used_out"}`（提示「已在 3 台电脑上使用」）
   - 本机已绑此码 → `{ok:1, add:0}`（提示「无需重复」）
   - 成功 → 本设备 `device_quota.granted += max_uses`，返回 `{ok:1, add:15, left:剩余}`
3. 之后每次走服务器代理 AI 对话（`/api/activate?chat=1`），模型返回成功后服务器把本设备 `used + 1`；
   剩余次数耗尽时返回 `{ok:false, reason:"quota"}`，客户端提示「AI 通话次数已用完」。
4. **管理员设备免限额**：在 `server_key.cfg` 的 `admin_device=` 填设备 ID（逗号分隔多个），这些设备走服务器代理不限次。
5. **自备密钥不受限**：客户端本地 `%APPDATA%\CDCExceed\cdc_ai.cfg` 填了 api_key，则直连模型，不经过服务器配额。

数据表：`activation_codes`（含 `max_uses`/`used_count`）、`code_bindings`（码-设备绑定）、`device_quota`（设备剩余次数）。
切换 MySQL 时 `mysql_schema.sql` 已含全部新表与字段，重新导入即可生效。

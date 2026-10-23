# CDCCMD — Code date creation 官方命令行工具

给系统加一组新的 CMD 命令，方便克隆仓库、下载 CDC、打开网址、自更新、卸载。

## 一、安装

有两种安装包，**任选其一**：

| 安装包 | 大小 | 说明 |
| --- | --- | --- |
| `cdccmd-setup.exe` | ~9.4 MB | 自包含（内嵌主程序+卸载程序），双击即装 |
| `cdc-setup-iss.exe` | ~3.1 MB | Inno Setup 版，**装完默认自动开一个新 CMD**（省得你以为没装上） |

双击 → 若弹出安全提示点"仍要运行" → 看到"安装成功"后按回车退出。

> **重要**：安装完必须**关闭当前命令行窗口，重新打开一个**，PATH 才会生效。
> 旧窗口的 PATH 是启动时的快照，安装器改的是注册表，不会穿透到已开的窗口。

安装位置：`%LOCALAPPDATA%\Programs\cdccmd`（**不需要管理员权限**）

### 为什么 `jcgx` 也能直接敲？

Windows 的 `CreateProcess` / `PATHEXT` 只认**可执行文件名**，不认 exe 的子命令——
所以光把 `cdccmd.exe` 放进 PATH，`jcgx` 是调不到的（会报"不是内部或外部命令"）。

解决办法：安装时用**硬链接**（失败退回复制）给主程序做 4 个同名副本：

```
cdccmd.exe   cdc.exe   cdcgb.exe   jcgx.exe   uncdccmd.exe
```

`cdccmd.exe` 内部会读 `argv[0]` 判断自己是"谁"，从而分派到对应子命令。
硬链接与源文件共享同一份磁盘数据，**不额外占空间**。

> 极少数文件系统不支持硬链接时，会退化为写 `.cmd` 转发脚本（`jcgx.cmd` 等），
> 内容形如 `@echo off` + `"%~dp0cdccmd.exe" jcgx %*`。

### 为什么"整台电脑到处都能用"？

命令别名会**同时投放两个位置**：

1. 安装目录 `%LOCALAPPDATA%\Programs\cdccmd`
2. `%LOCALAPPDATA%\Microsoft\WindowsApps` —— Windows 10/11 默认就在 PATH 里的目录

第 2 个位置是**不依赖注册表**的兜底：即使 PATH 被改坏，或者你从 Win+R /
其它终端 / 任务计划启动，都能直接敲出这 5 条命令。

## 二、命令一览

| 命令 | 作用 |
| --- | --- |
| `cdcgb <仓库URL> [存放目录]` | **免 git**：下载源码 zip 并解压到 `<存放目录>\<仓库名>` |
| `cdcjs` | **交互式加速下载**：输网址 → 选 Releases / 直接给 URL → 自动挑最快节点 |
| `cdc download` | 下载并打开 Code date creation 最新安装包 |
| `cdc releases [数量]` | 列出所有发布版本（默认 10 个，对标 akams.cn 的 Releases 列表）
| `<有效网址>` | 用默认浏览器打开网址 |
| `jcgx` | 检查 CDCCMD 自身更新，有新版本会问 Y/N（输入 Y 更新） |
| `uncdccmd` | 打开卸载程序 |
| `cdccmd help` | 显示帮助 |

### 示例

```cmd
cdccmd help
cdcgb https://github.com/starryfast/Code-date-creation
cdcgb https://github.com/starryfast/Code-date-creation D:\codes
cdc download
cdccmd https://cdc.mjhcsp.xyz
jcgx
uncdccmd
```

> `cdc` / `cdcgb` / `jcgx` / `uncdccmd` 都可以**直接单独敲**（不必写成 `cdccmd jcgx`）。

## 三、每条命令的行为细节

### 1. `cdcgb url [存放目录]` — 下载并解压仓库源码

**不依赖 git**。Git 在不少机器上压根没装、不在 PATH、或者走不通代理，所以这里改成
「下载仓库源码压缩包 + 本地解压」—— 对网络而言就是一次普通的 HTTP 下载。

不写目录时解压到**当前目录**下的 `<仓库名>`；写了就解压到 `<存放目录>\<仓库名>`。

```cmd
cdcgb https://github.com/starryfast/Code-date-creation
cdcgb https://github.com/starryfast/Code-date-creation D:\codes
::  第二条 -> D:\codes\Code-date-creation
```

流程：

1. 归档地址用 `https://github.com/<用户名>/<仓库名>/archive/HEAD.zip`（默认分支，
   不用查 API，也不产生重定向以外的额外请求）
2. 对全部加速节点**并发测速**，挑最快的下载；某节点失败自动换下一个
3. **校验下载结果是不是真 ZIP**（读文件头 `PK`）—— 代理有时回一张 HTML 错误页，
   Content-Length 还自洽，只看长度会被骗
4. 解压：优先用系统自带的 `System32\tar.exe`（Win10 1803+ 的 bsdtar），
   失败回退 PowerShell 的 `Expand-Archive`
5. 把解压出来的顶层目录搬到 `<存放目录>\<仓库名>`

实测输出：

```
[信息] 目标仓库: octocat/Hello-World
[信息] 方式: 源码压缩包下载 (不依赖 git)
[信息] 归档: https://github.com/octocat/Hello-World/archive/HEAD.zip
[信息] 解压到: D:\codes\Hello-World

[信息] 正在为源码包并发测速 10 个加速节点 ...
  [测速] gh.zwy.one               首字节   828 ms   吞吐        1 KB/s
  [测速] ghproxy.net 不可用
[信息] 首选节点: gh.zwy.one (1 KB/s)
[下载] gh.zwy.one (1/10)
[信息] 正在解压 ...
---------------------------------------------------------
[成功] 已下载并解压到: D:\codes\Hello-World
       注: 这是源码快照, 不含 .git 历史。需要版本控制请自行 git init。
```

> 目标目录已存在时**直接报错退出**，不会动你原来的文件。

### 2. `cdcjs` — 交互式加速下载

不带参数，全程问答。适合"我知道要下什么，但不想手拼 URL"的场景。

```
C:\> cdcjs
=========================================================
  CDCCMD 加速下载  (cdcjs)
  不依赖 git, 通过加速节点下载 GitHub 上的内容
=========================================================

请输入 GitHub 网址 (仓库页 / release 页 / 文件直链都可以):
> https://github.com/starryfast/CDCCMD
[检查] 网址合法   仓库: starryfast/CDCCMD

是否获取该仓库的 Releases 列表? (Y/N) > Y
获取多少项? (1-30, 直接回车 = 5) > 5

  ------------------ Releases 列表 ------------------
  [1] v1.2.0   正式版   2026-10-03
        (1) cdccmd-setup.exe                            9.40 MB
        (2) cdc-setup-iss.exe                           3.10 MB
  [2] v1.1.1   正式版   2026-10-03
        (1) cdccmd-setup.exe                            9.39 MB
  --------------------------------------------------

请输入要下载的编号, 或直接粘贴 URL > 1
  该版本有多个文件:
    [1] cdccmd-setup.exe                            9.40 MB
    [2] cdc-setup-iss.exe                           3.10 MB
  选哪个? > 2

使用加速节点下载? (回车/Y = 自动查找最快节点, N = 直连 GitHub) >

[信息] 正在为文件并发测速 10 个加速节点 ...
[信息] 首选节点: gh.zwy.one (512 KB/s)
[下载] gh.zwy.one (1/10)
[gh.zwy.one            ][########################] 100%  3.1/3.1 MB
[完成] 已保存: D:\Users\ASUS\Downloads\cdc-setup-iss.exe
```

**两个分支**：

| 分支 | 行为 |
|---|---|
| **Y**（获取列表） | 问数量 → 列出该仓库的 Releases（标签 / 正式版或预发布 / 日期 / 每个资产的名字与大小）→ 你输编号选，或直接粘 URL |
| **N**（不获取） | 跳过列表，直接问要下载的 URL；**直接回车** = 该仓库的源码 zip |

最后一步「使用加速节点下载」：**回车或 Y = 自动查找最快节点**，`N` = 直连 GitHub。
文件保存到系统的「下载」文件夹，下完自动在资源管理器中选中它。

> 网址不合法（不含 github.com）会当场提示并退出；仓库名解析不出来也会给出示例。

### 3. `cdc download` — 下载 CDC 安装包

从 `https://github.com/starryfast/Code-date-creation/releases` 取**最新版**，
挑出 `.exe` 安装包（优先 setup/install 命名），下载到 `下载` 文件夹并**显示进度条**，
完成后**自动打开**安装包。

```
[信息] 正在查询 Code date creation 最新发布 ...
[信息] 最新版本: 7.1.0
[信息] 安装包名: CodeDateCreation7.1.0_Setup.exe
[信息] 文件大小: 122.99 MB
[下载] CodeDateCreation7.1.0_Setup.exe -> 下载文件夹
[##########################] 100%  123.0/123.0 MB
[完成] 下载完成, 正在打开安装包 ...
```

下载后有完整性校验（对比 Content-Length），不完整会提示。

**自动加速**：下载前会并发给全部 10 个加速节点测速（取样 96KB，扣除首字节等待，
按真实吞吐排序），从最快的开始下；某节点中途失败会自动换下一个并**从断点续传**。

```
[信息] 正在为安装包并发测速 9 个加速节点 ...
  [测速] gh.dpik.top              首字节  1047 ms   吞吐      256 KB/s
  [测速] ghfile.geekertao.top     首字节  1844 ms   吞吐      246 KB/s
  [测速] GitHub 官方              首字节  2000 ms   吞吐       70 KB/s
  [测速] ghproxy.net              首字节  1141 ms   吞吐       32 KB/s   (响应不完整)
[信息] 首选节点: gh.dpik.top (256 KB/s)
[下载] gh.dpik.top (1/9)
```

实测本机直连 GitHub 只有 **14 KB/s**，akams 节点可达 **660 KB/s**（约 47 倍）。

加速节点包括 akams.cn 的 `gh.dpik.top` / `ghfile.geekertao.top` / `github.tbap.top`，
以及 `gh-proxy.com` / `ghproxy.net` / `kkgithub.com` / `ghfast.top` / `gh.llkk.cc` / `hub.gitmirror.com` / `mirror.ghproxy.com` / `moeyy.cn/gh-proxy` /
`ghproxy.cc` / `gitclone.com`。每个节点按**实测能力**标注能否代理 API / 下载 / clone，
不支持的会被自动跳过（例如 `github.tbap.top` 只能下载、不能 clone）。
设 `CDC_NO_SPEED=1` 可跳过测速。

### 4. `<有效网址>` — 打开网址

`http://` / `https://` / `ftp://` / 裸域名（自动补 `https://`）都能识别。

```cmd
cdccmd https://cdc.mjhcsp.xyz
cdccmd www.baidu.com
cdccmd example.com/path
```

### 5. `jcgx` — 自更新

从 `https://github.com/starryfast/CDCCMD/releases` 取最新版，与本程序**内置版本号**比较：

- 有新版 → `发现新版本 x.y.z, 是否更新? (Y/N)` → 输入 **Y** 下载并覆盖安装
- 已最新 → 提示"已是最新版本, 无需更新。"
- 还没建发布页 → 提示"尚未创建 CDCCMD 发布页, 无需更新。"（不报错）

更新采用"另起隐藏 cmd 延迟替换"，不会因为文件被占用而失败。

### 6. `uncdccmd` — 卸载

打开卸载程序，它会：从用户 PATH 移除安装目录 → 删注册表卸载项 → 删除整个安装目录。

也可以从 **设置 → 应用 → 已安装的应用** 里找到「CDCCMD」直接卸载。

## 四、可选：GitHub API 限流

GitHub 匿名 API 每小时 60 次，超了会返回 403。想提高额度可设环境变量：

```cmd
set CDC_GH_TOKEN=你的GitHubToken
```

程序会用这个 token 请求（也认 `GITHUB_TOKEN` / `GH_TOKEN`）。

## 五、Win7 / Win8 兼容

程序本体和安装包都已按 Win7 可用来处理：

| 问题 | 处理方式 |
| --- | --- |
| **TLS 1.2 默认没开**（Win7 的老毛病） | 每次发起 HTTPS 请求前显式启用 TLS 1.0/1.1/1.2，否则连 GitHub 一律握手失败 |
| **系统没有 UCRT** | Win7/8 不自带 `api-ms-win-crt-*.dll`，缺了程序直接起不来。安装时检测系统有没有 `ucrtbase.dll`：**没有就把 46 个官方可再分发 DLL（2.37 MB）释放到安装目录**，exe 同目录优先加载，不污染 System32、不需要管理员 |
| **安装包不让装** | 最低系统要求已从 Windows 10 降到 **Windows 7 SP1** |

| 系统 | 行为 |
| --- | --- |
| Win10 / Win11 | 系统自带 UCRT → **自动跳过**，不占额外空间 |
| Win7 / Win8 / 8.1 | 自动释放 UCRT DLL，**不用装 KB2999226 补丁** |

> ⚠️ Win7 上 TLS 1.2 还依赖系统更新 **KB3140245**（Win7 SP1 的补丁）。
> 没打这个补丁的系统本身就不支持 TLS 1.2，任何现代 HTTPS 服务都连不上。


## 六、自己编译

```cmd
python build.py          # 产物在 build\
python build.py clean    # 清理
```

依赖：mingw64 工具链（默认复用 `cdc_app\mingw64`）。

产物：
- `build\cdccmd.exe` — 主程序（内部按 `argv[0]` 分派别名）
- `build\uninstall.exe` — 卸载程序
- `build\cdccmd-setup.exe` — **自包含安装包**（内嵌上面两个，双击即装）

### 打 Inno Setup 包

```cmd
cd iss
python make_icon.py              # 可选: 重新生成多尺寸图标
"%LOCALAPPDATA%\Programs\Inno Setup 7\ISCC.exe" cdc_setup.iss
```

产出 `build\cdc-setup-iss.exe`。ISS 里已包含：
- `PrivilegesRequired=lowest` + 装到 `{localappdata}` → 免管理员
- `[Registry]` 写用户 PATH，`Check: NeedsAddPath('{app}')` 防重复
- `[Code] MakeAliases()` 用 `CreateHardLinkW` 建 4 个别名 exe
- `[Run]` + `Tasks: opennewcmd` 装完自动开一个已生效的新 CMD
- 卸载时 `RemoveFromUserPath()` 精确摘掉 PATH 项

> **红线**：`CreateHardLinkW` 在 Inno Pascal Script 里**必须先 `external` 声明**，
> 直接调用会报 `Unknown identifier 'CreateHardLink'`。

### 验收测试

```cmd
python e2e_test.py       # 27 项：自包含包 安装/命令/卸载 全链路
python iss_e2e.py        # 21 项：ISS 包 安装/PATH/别名/卸载 全链路
python iss\real_check.py # 真机 PATH 环境敲命令验证
```

## 七、发布新版给用户用 jcgx 更新

1. 改 `src\version.h` 里的 `CDCCMD_VERSION`（如 `1.1.0`）
2. 同时改 `src\payload.rc` 里的 `FILEVERSION` / `PRODUCTVERSION` / `FileVersion` / `ProductVersion`
3. `python build.py`
4. 在 `https://github.com/starryfast/CDCCMD/releases` 建新 release，
   tag 写 `1.1.0`，**把 `build\cdccmd-setup.exe` 作为附件上传**
5. 老用户跑 `jcgx` 即可检测到并一键更新

> tag 必须是 `x.y.z` 格式（可带 `v` 前缀），程序按这个比大小。

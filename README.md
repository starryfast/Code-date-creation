# Code date creation（CDC）测试数据生成器 —— 客户端源码 / 开源版

CDC 是一个面向算法竞赛（洛谷 / acgo 等平台）的 **C++ 图形化测试数据生成器**：
用可视化界面描述数据形态，一键批量生成 `.in` 测试点；也可以直接写生成器代码调用
内置的 CDCSCQ 函数库。本仓库是它的**客户端源码开源版**。

- 上游项目主页：<http://cdc.mjhcsp.xyz>
- 许可：GNU GPL v3（见 `LICENSE`）

> **本开源版不含任何官方服务器信息。** 官方云端（账号 / 激活 / AI 代理 / 更新分发）
> 的地址与接口令牌均未包含在源码中，需由使用者自行部署服务端后接入，见下文「自建云端」。

---

## 一、目录结构

| 路径 | 说明 |
| --- | --- |
| `cdc_backend.cpp` | 本地后端主程序。单文件 C++23：内嵌 HTTP 服务（127.0.0.1:18889）、生成器编译与运行、文件写入、AI 代理、自动更新检查 |
| `cdc.rc` | Windows 资源脚本：应用图标 + 把 `index.html` / `cpp_bridge.js` 内嵌进 exe |
| `icon.ico` / `icon.rc` | 应用图标 |
| `index.html` | 内嵌前端页面（由 `build_front.py` 生成，改动前端后必须重新生成并重编资源） |
| `cpp_bridge.js` | 前端 ←→ 本地后端桥接层：探测 `127.0.0.1:18889`，在线则把本地操作改走 C++ API |
| `build.bat` | Windows 一键编译（windres + g++） |
| `build_front.py` | 前端构建脚本：`ui/` 三个源文件 → 单文件 `index.html` |
| `cdc_lib/cdcscq.hpp` | **CDCSCQ v2.0**：内置数据生成函数库，纯 C++23 单头、零外部依赖、约 120 个接口 |
| `cdc_lib/examples/demo_all.cpp` | CDCSCQ 全功能示例生成器 |
| `md/` | AI 回答的 Markdown + LaTeX 渲染栈（胶水层、样式、构建与测试脚本） |
| `md/vendor/` | 第三方前端库：markdown-it / KaTeX / markdown-it-texmath（均 MIT，见 `NOTICE`） |
| `ui/` | 前端**上游**源码：`index.html` / `app.js` / `style.css` |
| `License.txt` | 许可摘要（中文） |
| `LICENSE` | GNU GPL v3 全文 |
| `NOTICE` | 第三方组件署名与版本 |
| `cdc_srv.dat.example` | 自建云端接入配置模板 |

---

## 二、编译

### 环境要求

- Windows 10/11
- 支持 **C++23** 的编译器，二选一：
  - MinGW-w64 的 `g++`（需附带 `windres`，推荐 <https://www.mingw-w64.org/> 或 MSYS2）
  - Visual Studio 2019+（用「x64 Native Tools 命令提示符」）
- Python 3.8+（仅在需要重新构建前端时用到）

### 一键编译（MinGW）

在本目录打开 cmd，直接运行：

```bat
build.bat
```

脚本内部依次执行两步（等价于手工命令）：

```bat
windres cdc.rc -O coff -o cdc_res.o
g++ -O2 -std=c++23 -static -static-libgcc -static-libstdc++ -mwindows ^
  cdc_backend.cpp cdc_res.o -o cdc_backend.exe ^
  -lws2_32 -lwinhttp -lcomdlg32 -lole32 -lshell32 -ladvapi32 -luser32
```

要点：

- `-static -static-libgcc -static-libstdc++`：把 `libwinpthread` / `libgcc` / `libstdc++`
  全部静态编入，产物单文件、换机器不弹「缺少 DLL」。
- `-mwindows`：编成 GUI 子系统，双击不弹控制台黑框。需要看日志时设环境变量
  `CDC_CONSOLE=1` 再启动。
- **改过 `index.html` 或 `cpp_bridge.js` 后必须先重跑 `windres`**，否则 exe 里还是旧前端
  （这是最常见的「改了没生效」原因）。

### 用 Visual Studio 编译

新建「空项目」，把 `cdc_backend.cpp` 与 `cdc_res.o` 加入，链接库填
`ws2_32.lib;winhttp.lib;comdlg32.lib;ole32.lib;shell32.lib;advapi32.lib;user32.lib`，
字符集选 **Unicode**，语言标准选 **/std:c++latest** 或 `/std:c++23`。
资源文件用 `rc` 单独编译成 `.res` 再一起链接（`windres` 产出的 `.o` 只适用于 MinGW）。

### 重新构建前端（可选）

只有改了 `ui/` 下的 `index.html` / `app.js` / `style.css` 时才需要：

```bat
python build_front.py
```

脚本会把 CSS / JS 内联进单文件，产出仓库根目录的 `index.html`，之后**重新编译资源与 exe**。

---

## 三、运行

双击 `cdc_backend.exe`：后端在本机 `127.0.0.1:18889` 起服务，并自动用默认浏览器打开页面。
前端只与本机后端通信，后端再按需转发到云端。

### 生成器（写代码生成数据）模式

该模式需要本机有 `g++`：

- 首次使用时在界面「设置」里指定 `g++` 路径，程序会记到 exe 同目录的 `cdc_gpp.cfg`；
- 也可以直接手写 `cdc_gpp.cfg`（一行纯路径，例如 `C:\mingw64\bin\g++.exe`）；
- 什么都不配时程序会尝试在常见位置自动检测。

---

## 四、自建云端（可选）

开源版**移除了官方服务器地址与接口令牌**，云端相关接口（账号 / 激活 / AI 代理 /
更新分发 / 私信）在未配置时统一返回 `cloud_not_configured`，本地功能不受影响。

接入自建服务端，任选一种方式：

**方式 ① 配置文件（推荐）**

把 `cdc_srv.dat.example` 复制为 `cdc_srv.dat`，放在 **exe 同目录**，填写一行：

```ini
server=https://你的域名/你的接口基础路径
```

也支持十六进制 XOR 混淆写法（键 `cdc_7_1_xor` 循环异或），避免地址明文落盘：

```ini
x:0A1B2C3D...
```

**方式 ② 编译进 exe**

把地址按 `kCloudXorKey`（`"cdc_7_1_xor"`）循环异或成字节数组，填进 `cdc_backend.cpp`
里的 `kCloudEnc[]`，并把 `kCloudEncLen` 改成地址字节数。这样无需附带配置文件。

若服务端启用了接口令牌校验，同理填 `kTokenEnc` / `kTokenEncLen`（键 `"cdc_7_1_tok"`），
令牌会以 `X-CDC-Token` 请求头发出。**两端令牌必须一致**，改一端漏一端会导致全量 401。

### 服务端需要实现的接口

后端按下列路径转发（`server=` 配置的基础路径会拼在最前）：

| 路径 | 用途 |
| --- | --- |
| `/api/activate` | 激活码兑换 / 校验（同时接受 JSON body 与 form 提交） |
| `/api/version` | 版本检查（读库，发布即生效） |
| `/api/chat` | AI 对话代理（服务端配额模式） |
| 其余 `api/*` | 账号、私信、公告、AI 次数等 |

响应统一为 JSON，失败时建议带 `ok:false` 与 `reason` 字段，前端会按 `reason` 显示具体文案。

---

## 五、开源版与官方发布版的差异

| 项目 | 官方发布版 | 本开源版 |
| --- | --- | --- |
| 云端服务器地址 | 内置（XOR 混淆） | **已移除**，需自行配置 |
| 接口令牌 | 内置（XOR 混淆） | **已移除**，服务端可不校验 |
| 官方服务端（PHP 后台）源码 | 另行提供 | **不包含** |
| 本机路径等开发环境残留 | — | **已清理**（构建脚本改为相对路径） |
| 预编译二进制 / 安装包 | 提供 | **不包含**，请自行编译 |
| 许可证 | 专有免费授权 | **GNU GPL v3** |
| 功能代码 | 与开源版同源 | 与发布版同源，仅上述差异 |

---

## 六、许可证与第三方组件

- 本项目以 **GNU GPL v3**（或更新版本）发布，全文见 `LICENSE`，中文摘要见 `License.txt`。
- 随附第三方前端库均为 **MIT** 许可，其版本、作者与许可原文见 `NOTICE`。
- `cdc_lib/cdcscq.hpp` 为项目自研库，同样遵循 GPL v3。

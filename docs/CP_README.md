# CDCCP — Code date creation Compiler platform

**CDC 编译器平台**。基于 [RedPanda-CPP](https://github.com/royqh1979/RedPanda-CPP)（小熊猫 C++）
v3.5.0-beta.2 改造，**完整保留小熊猫全部功能**，并新增 **「打开 CDC」** 一键启动 Code date creation。

- 主程序：`CDCCP.exe`
- 作者：starryfast（Code date creation）
- 项目主页：<http://cdc.mjhcsp.xyz>
- 上游版权：RedPanda-CPP © Roy Qu（GPL-3.0），本改造同样以 GPL-3.0 发布

---

## 一、这是什么

| 组成 | 说明 |
|---|---|
| **CDCCP.exe** | 完整的小熊猫 C++（Qt6 + Scintilla 编辑器），已换 CDCCP 图标与名称 |
| **mingw64/** | 自带 GCC 编译器（给用户写 C++ 用，与 CDC 同一套） |
| **cdc_backend.exe** | Code date creation 主程序，供 CDCCP 一键启动 |
| **templates/** | 小熊猫的全部项目模板 |

「打开 CDC」入口：**工具菜单 → Open CDC Data Generator**，快捷键 **Ctrl+M**。

查找 `cdc_backend.exe` 的顺序：
1. CDCCP.exe 同目录（安装后默认就在这里）
2. 小熊猫的 appDir 配置目录
3. 环境变量 `CDCCP_CDC_DIR`
4. 你上次手动定位的目录（存在 `cdccp.ini` 的 `CDC/Path`）

找不到就弹文件框让你手动选一次，选完会记住位置。

---

## 二、编译（开发者）

### 依赖

| 工具 | 版本 | 说明 |
|---|---|---|
| Qt | 6.8.3 mingw_64 | **必须**，用 aqtinstall 下载 |
| MinGW | **13.1.0 posix 线程模型** | **必须**，见下方红线 |
| CMake | ≥ 3.24 | aqt `tools_cmake` |
| Ninja | ≥ 1.11 | aqt `tools_ninja` |

### 一键构建

```bash
cd CDCCP
bash build.sh          # 配置 + 编译
bash build.sh clean    # 清理构建目录
```

### ⚠️ 关键红线：MinGW 线程模型

**Qt 6.8.3 的 mingw_64 预编译包只能用 GCC 13.1 + posix 线程模型编译。**

用户自带的 `CDCCP/mingw64` 是 GCC 16.1 但 `--enable-threads=win32`，用它编 Qt 会报：

```
error: '__gthread_cond_t' does not name a type
error: '__gthread_cond_timedwait' was not declared in this scope
```

原因：win32 线程模型下 `condition_variable` / `mutex` 的 `__gthread_*` 家族全部缺失。

所以：**编译 CDCCP 用 `QtTools/mingw1310_64`；`CDCCP/mingw64` 只作为运行时随包分发的编译器。**

下载命令（会自动走镜像）：

```bash
python -m aqt install-qt windows desktop 6.8.3 win64_mingw -O C:/Qt
python -m aqt install-tool windows desktop tools_mingw1310 --outputdir ./QtTools
python -m aqt install-tool windows desktop tools_cmake   --outputdir C:/Qt/Tools
python -m aqt install-tool windows desktop tools_ninja    --outputdir C:/Qt/Tools
```

> `aqt` 会把工具套两层目录，实际路径是 `C:/Qt/Tools/Tools/CMake_64/bin/cmake.exe`。

### 其他红线

- 沙箱/CI 里 `%TEMP%` 不可写时 gcc 会失败 → `build.sh` 已把 `TMPDIR` 指向 `build/tmp`
- 批量删构建目录会被安全策略拦截 → 用 `python _tools/clean_build.py` 逐文件删
- Qt 预编译包静态链了 `libstdc++` / `libgcc`，不要手工往 exe 旁边塞不匹配的版本

---

## 三、打包

```bash
# 1. 先编译并收集产物到 dist/
bash build.sh

# 2. 编译安装包（ISCC.exe cdccp.iss）
```

安装包做的事：
- 装到 `{autopf}\CDCCP`（`UsePreviousAppDir=no`，不会被注册表残留带去别的盘）
- 建开始菜单（CDCCP + 打开 CDC 数据生成器）与可选桌面快捷方式
- 关联 `.cpp` / `.h`，图标用 exe 的第 1、2 号图标（`CDCCP.exe,1` / `,2`）
- 卸载时清理 `log/` 与 `res/`

`.iss` 必须是 **UTF-8 BOM + CRLF**，否则中文乱码。

---

## 四、目录结构

```
CDCCP/
├── build.sh              构建脚本（cmake + ninja）
├── cdc_backend.exe       Code date creation 主程序
├── mingw64/              随包分发的 GCC（win32 线程模型，仅用于编译用户代码）
├── QtTools/              Qt 配套 MinGW 13.1（只用于构建 CDCCP 本身）
├── res/                  CDCCP 图标（.ico + 各尺寸 .png）
├── src_base/             RedPanda-CPP 源码（已改造）
│   └── RedPandaIDE/src/
│       ├── mainwindow.ui        + actionOpen_CDC（Ctrl+M）
│       ├── mainwindow.cpp        + on_actionOpen_CDC_triggered()
│       ├── mainwindow.h          + 槽声明
│       └── systemconsts.h        配置名改 cdccp.ini
├── packaging/cdccp.iss   Inno Setup 安装包脚本
└── _tools/
    ├── make_icons.py     从白底 logo 生成多尺寸 .ico（彩色反解）
    ├── apply_icons.py    把图标装进源码树
    └── clean_build.py    分批清理构建目录
```

---

## 五、图标说明

`_tools/make_icons.py` 从一张**白底彩色** logo 生成 `.ico`。

**不能**用"亮度当 alpha"反解——彩色图形在白底上做亮度反解无法重建原色（实测颜色发灰）。
正确做法是**按通道求覆盖率**：

```
a = max_c (255 - P_c) / (255 - C0_c)      # C0 = 最暗 1% 像素的中位色
C = (P - 255*(1-a)) / a
```

图形与下方字标之间用**行 alpha 覆盖率**自动找分割线（阈值取峰值 18%，避免 logo 光晕干扰），
不写死百分比，排版变了也不会切坏。

产出：

| 文件 | 用途 | exe 内图标序号 |
|---|---|---|
| `cdccp.ico` | 程序图标（`images/devcpp.ico`） | 0 |
| `cdccp_cpp.ico` | 源文件关联（`images/associations/cpp.ico` 等） | 1 |
| `cdccp_h.ico` | 头文件关联（`images/associations/h.ico`） | 2 |
| `cdccp_logo.ico` | 带字标版（关于框/卸载器） | 3 |

**图标组顺序即 ABI**，Inno 里用 `CDCCP.exe,1` / `,2` 引用，改顺序会静默引用错图标。

---

## 六、许可与致谢

- 上游：[RedPanda-CPP](https://github.com/royqh1979/RedPanda-CPP)，作者 Roy Qu，GPL-3.0
- 本改造由 starryfast 完成，仅更换图标、名称、配置隔离，并新增「打开 CDC」
- 上游功能全部保留，本项目未做任何功能删减
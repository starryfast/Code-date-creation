@echo off
chcp 65001 >nul
REM ============================================================
REM  CDC DataGen v8.0.0「幻想」— 全量构建脚本
REM  1) cdc_backend.exe  主程序（含图标 + 内嵌前端，单文件可分发）
REM  2) cdcfantasy.dll    幻想增强动态库（可选外挂，缺失不影响主程序）
REM  3) duel.exe          增强库对拍程序（20 项能力 x N 组随机用例交叉验证）
REM  需要：支持 c++17 的 g++（含 windres），且在本目录运行
REM ============================================================
cd /d %~dp0

REM 优先用项目自带的 mingw64（g++ 16.1.0），避免系统PATH 里的旧版
set GPP=g++
if exist "%~dp0mingw64\bin\g++.exe" (
  set "PATH=%~dp0mingw64\bin;%PATH%"
  set GPP=%~dp0mingw64\bin\g++.exe
  set WINRES=%~dp0mingw64\bin\windres.exe
) else (
  set WINRES=windres
)
where %GPP% >nul 2>nul
if errorlevel 1 (
  echo [错误] 未找到 g++，请把支持 c++17 的 MinGW/bin 加入 PATH，或用 VS 开发者命令行。
  pause
  exit /b 1
)

echo ============================================================
echo  CDC DataGen v8.0.0「幻想」构建
echo ============================================================

echo.
echo [1/4] 编译资源 cdc.rc -> cdc_res.o（图标 + 内嵌前端）
"%WINRES%" cdc.rc -O coff -o cdc_res.o
if errorlevel 1 (
  echo [错误] windres 失败，请确认 cdc.rc / icon.ico / index.html / cpp_bridge.js 存在。
  pause
  exit /b 1
)

echo [2/4] 编译 cdc_backend.cpp + cdc_res.o -> cdc_backend.exe
echo       注意：-static 把 libwinpthread/libgcc/libstdc++ 全部静态编入，避免缺 DLL 弹窗
echo       -mwindows 编成 GUI 子系统：双击不再弹控制台黑框（要看日志时 set CDC_CONSOLE=1）
%GPP% -O2 -std=c++23 -static -static-libgcc -static-libstdc++ -mwindows ^
  cdc_backend.cpp cdc_res.o -o cdc_backend.exe ^
  -lws2_32 -lwinhttp -lcomdlg32 -lole32 -lshell32 -ladvapi32 -luser32 -luuid
if errorlevel 1 (
  echo [错误] 编译失败，请检查工具链（需支持 c++23 的 g++ 或 VS 开发者命令行）。
  pause
  exit /b 1
)

echo.
echo [3/4] 编译 cdcfantasy.dll（幻想增强库，可选外挂）
echo       同样 -static，DLL 不依赖外部运行时；导出 CDC_FX_LIST / CDC_FX_CALL / CDC_FX_ABI
%GPP% -O2 -std=c++17 -shared -static -static-libgcc -static-libstdc++ ^
  -o cdcfantasy.dll cdcfantasy.cpp -Wl,--out-implib,cdcfantasy.lib
if errorlevel 1 (
  echo [警告] 增强库编译失败。**不影响主程序**，只是「幻想增强」页面会显示未启用。
  goto duel_skip
)
echo       [OK] 已生成 cdcfantasy.dll

:duel_skip
echo.
echo [4/4] 编译对拍程序 duel.exe
%GPP% -O2 -std=c++17 -static -static-libgcc -static-libstdc++ ^
  duel.cpp -o duel.exe -L. -lcdcfantasy
if errorlevel 1 (
  echo [警告] 对拍程序编译失败，可跳过此步。
) else (
  echo       运行对拍（20 项能力交叉验证）...
  echo.
  .\duel.exe
  echo.
  echo       对拍返回码：%errorlevel%  （0 = 全部一致）
)

echo.
echo ============================================================
echo  [4/4] 安装包（Inno Setup · 主方案）
echo ============================================================
REM 版本号单一来源（与 cdc_setup.iss 的 MyAppVersion 保持一致）
set VER=8.0.0
REM 说明：
REM   安装包不用命令行出——Inno Setup 只保证 GUI 构建，直接打开 cdc_setup.iss
REM   按 Ctrl+F9 即可，产物在 Output\CodeDateCreation%VER%_Setup.exe。
REM   若本机没装 Inno Setup 6，装完再回来看这一步。
where ISCC >nul2>nul
if errorlevel 1 (
  if exist "%ProgramFiles(x86)%\Inno Setup 6\ISCC.exe" (
    echo       检测到 Inno Setup 6，跳过命令行编译（GUI 构建即可）
  ) else (
    echo [跳过] 未检测到 Inno Setup 6。
    echo        请安装后打开 cdc_setup.iss，按 Ctrl+F9 生成安装包。
  )
) else (
  echo       使用 ISCC 命令行编译 cdc_setup.iss ...
  ISCC cdc_setup.iss
  if errorlevel 1 (
    echo [警告] ISCC 编译失败，请改为 GUI 打开 cdc_setup.iss 后 Ctrl+F9。
  ) else (
    echo       安装包已生成：Output\CodeDateCreation%VER%_Setup.exe
  )
)

echo.
echo ============================================================
echo  [完成]
echo    cdc_backend.exe  主程序（单文件，内嵌前端，双击即用）
echo    cdcfantasy.dll    幻想增强库（可选；缺失时主程序照常运行）
echo    duel.exe          对拍程序（验证增强库 20 项能力输出正确性）
echo    Output\CodeDateCreation%VER%_Setup.exe  安装包（Inno Setup，Ctrl+F9 生成）
echo  ============================================================
pause
@echo off
chcp 65001 >nul
REM ============================================================
REM  CDC DataGen v7.1.0 — cdc_backend.exe 构建脚本
REM  把「图标 + 前端(index.html / cpp_bridge.js)」一起编进 exe
REM  产物：单文件 cdc_backend.exe，可单独分发给别人，无需附带前端文件
REM  需要：支持 c++17 的 g++（含 windres），且在本目录运行
REM ============================================================
cd /d %~dp0

set GPP=g++
where %GPP% >nul 2>nul
if errorlevel 1 (
  echo [错误] 未找到 g++，请把支持 c++17 的 MinGW/bin 加入 PATH，或用 VS 开发者命令行。
  pause
  exit /b 1
)

echo [1/2] 编译资源 cdc.rc -> cdc_res.o（图标 + 内嵌前端）
windres cdc.rc -O coff -o cdc_res.o
if errorlevel 1 (
  echo [错误] windres 失败，请确认 cdc.rc / icon.ico / index.html / cpp_bridge.js 存在。
  pause
  exit /b 1
)

echo [2/2] 编译 cdc_backend.cpp + cdc_res.o -> cdc_backend.exe
echo       注意：-static 把 libwinpthread/libgcc/libstdc++ 全部静态编入，避免缺 DLL 弹窗
echo       -mwindows 编成 GUI 子系统：双击不再弹控制台黑框（要看日志时 set CDC_CONSOLE=1）
%GPP% -O2 -std=c++23 -static -static-libgcc -static-libstdc++ -mwindows ^
  cdc_backend.cpp cdc_res.o -o cdc_backend.exe ^
  -lws2_32 -lwinhttp -lcomdlg32 -lole32 -lshell32 -ladvapi32 -luser32

if errorlevel 1 (
  echo [错误] 编译失败，请检查工具链（需支持 c++23 的 g++ 或 VS 开发者命令行）。
  pause
  exit /b 1
)

echo.
echo [完成] 已生成 cdc_backend.exe（含图标 + 内嵌前端，单文件可分发）
echo   双击 cdc_backend.exe 即可启动本地后端，前端已在 exe 内部，无需 index.html。
pause

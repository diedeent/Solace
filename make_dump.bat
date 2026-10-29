@echo off
setlocal enabledelayedexpansion

set "OUTPUT=project_dump.txt"
set "ROOT=%~dp0"

echo ===================================================
echo   Solace Project Dump
echo   Root: %ROOT%
echo   Output: %OUTPUT%
echo ===================================================
echo.

:: Очищаем выходной файл
> "%OUTPUT%" echo.

:: =========================================================
:: 1. Корневые файлы
:: =========================================================
echo [1/5] Root CMake files...

call :append "%ROOT%CMakeLists.txt"
call :append "%ROOT%CMakePresets.json"

:: =========================================================
:: 2. engine/ — исходники и заголовки
:: =========================================================
echo [2/5] Engine sources...

for /r "%ROOT%engine" %%f in (*.cpp *.hpp *.h *.cc) do (
    call :append "%%f"
)

:: =========================================================
:: 3. game/ — исходники и заголовки
:: =========================================================
echo [3/5] Game sources...

for /r "%ROOT%game" %%f in (*.cpp *.hpp *.h *.cc) do (
    call :append "%%f"
)

:: =========================================================
:: 4. assets/shaders — только GLSL
:: =========================================================
echo [4/5] Shaders...

if exist "%ROOT%assets\shaders" (
    for /r "%ROOT%assets\shaders" %%f in (*.glsl *.vert *.frag *.vs *.fs) do (
        call :append "%%f"
    )
)

:: =========================================================
:: 5. assets/configs — только JSON
:: =========================================================
echo [5/5] Configs...

if exist "%ROOT%assets\configs" (
    for /r "%ROOT%assets\configs" %%f in (*.json) do (
        call :append "%%f"
    )
)

:: =========================================================
:: Статистика
:: =========================================================
echo.
echo ===================================================
for %%A in ("%OUTPUT%") do (
    set /a "SIZE_KB=%%~zA / 1024"
    echo   Done! Output size: !SIZE_KB! KB
)
echo   File: %ROOT%%OUTPUT%
echo ===================================================
echo.
echo Copy this file to the chat for analysis.
echo.
pause
exit /b

:: =========================================================
:: Подпрограмма: добавить файл в дамп
:: =========================================================
:append
if not exist "%~1" goto :eof
if "%~z1"=="" goto :eof
if %~z1 EQU 0 goto :eof

(
    echo.
    echo ============================================================
    echo === FILE: %~f1
    echo ============================================================
    type "%~1"
    echo.
) >> "%OUTPUT%"

goto :eof
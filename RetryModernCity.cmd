@echo off
setlocal
chcp 65001 >nul
set "PROJECT_DIR=%~dp0"
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo Сначала закройте Unreal Engine.
    pause
    exit /b 1
)
set "UE_ROOT="
for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v InstalledDirectory 2^>nul') do set "UE_ROOT=%%B"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo Unreal Engine 5.8 не найден.
    pause
    exit /b 1
)
set "CITYLIVE_CITY_PREPARED=%PROJECT_DIR%Saved\CityImport\%RANDOM%_%RANDOM%_%RANDOM%"
mkdir "%CITYLIVE_CITY_PREPARED%"
echo 1/3. Проверка ранее подготовленных файлов. Blender повторно не запускается.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%Scripts\ReusePreparedCity.ps1" -ProjectDir "%PROJECT_DIR%." -OutputDir "%CITYLIVE_CITY_PREPARED%"
if errorlevel 1 goto failure
echo 2/3. Сборка текущего кода.
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" CityLiveRPEditor Win64 Development "-Project=%PROJECT_DIR%CityLiveRP.uproject" -WaitMutex -NoHotReloadFromIDE > "%CITYLIVE_CITY_PREPARED%\Build.log" 2>&1
if errorlevel 1 goto failure
echo 3/3. Импорт города и создание отдельной карты. Дождитесь окончания.
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%CityLiveRP.uproject" "-ExecutePythonScript=%PROJECT_DIR%Scripts\ImportModernCity.py" "-abslog=%CITYLIVE_CITY_PREPARED%\Import.log" -unattended -nosplash
if errorlevel 1 goto failure
if not exist "%CITYLIVE_CITY_PREPARED%\import.ok" goto failure
echo Готово. Откройте BuildAndOpen.cmd: запустится карта импортированного города.
echo Это карта для проверки в редакторе. Android APK пока не собирайте.
pause
exit /b 0
:failure
echo Импорт остановлен. Открывается папка журналов.
echo Пришлите Prepare.log, Build.log или Import.log с ошибкой.
set "CITY_ERROR_LOG="
for %%L in (Prepare Build Import) do if exist "%CITYLIVE_CITY_PREPARED%\%%L.log" set "CITY_ERROR_LOG=%CITYLIVE_CITY_PREPARED%\%%L.log"
if defined CITY_ERROR_LOG powershell.exe -NoProfile -Command "Get-Content -LiteralPath $env:CITY_ERROR_LOG -Encoding UTF8 -Tail 35"
start "" "%CITYLIVE_CITY_PREPARED%"
pause
exit /b 1

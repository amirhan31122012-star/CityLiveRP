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
if not exist "%PROJECT_DIR%Assets\Maps\PoliceDistrictGame\manifest.json" (
    echo Пакет новой карты не найден. Обновите проект в GitHub Desktop.
    pause
    exit /b 1
)
set "UE_ROOT=%~1"
if not defined UE_ROOT (
    for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v InstalledDirectory 2^>nul') do set "UE_ROOT=%%B"
)
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo Unreal Engine 5.8 не найден.
    pause
    exit /b 1
)
set "CITYLIVE_DISTRICT_TOKEN=%RANDOM%_%RANDOM%_%RANDOM%"
set "DISTRICT_LOG=%PROJECT_DIR%Saved\PoliceDistrictImport\%CITYLIVE_DISTRICT_TOKEN%"
mkdir "%DISTRICT_LOG%"
echo 1/2. Проверка сборки проекта.
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" CityLiveRPEditor Win64 Development "-Project=%PROJECT_DIR%CityLiveRP.uproject" -WaitMutex -NoHotReloadFromIDE > "%DISTRICT_LOG%\Build.log" 2>&1
if errorlevel 1 goto failure
echo 2/2. Импорт новой основной карты: полиция, дома, дороги и дворы.
echo Материалы уже подготовлены. Blender запускать не требуется.
echo Журнал: %DISTRICT_LOG%\Import.log
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%CityLiveRP.uproject" /Engine/Maps/Entry "-ExecutePythonScript=%PROJECT_DIR%Scripts\ImportPoliceDistrict.py" "-abslog=%DISTRICT_LOG%\Import.log" -unattended -nosplash
if errorlevel 1 goto failure
if not exist "%DISTRICT_LOG%\complete.ok" goto failure
echo Готово. Новая карта стала основной. Старый город в новую сборку не включается.
echo Для проверки запустите BuildAndOpen.cmd. Для APK закройте редактор и запустите BuildAndroidV1Shipping.cmd.
pause
exit /b 0
:failure
echo Импорт не завершён. Пришлите Build.log или Import.log из открывшейся папки.
start "" "%DISTRICT_LOG%"
pause
exit /b 1

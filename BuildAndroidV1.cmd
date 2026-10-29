@echo off
setlocal
chcp 65001 >nul
set "PROJECT_DIR=%~dp0"
set "UE_ROOT=%~1"
if not defined UE_ROOT (
    for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v InstalledDirectory 2^>nul') do set "UE_ROOT=%%B"
)
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" (
    echo Unreal Engine не найден. Укажите папку движка первым аргументом.
    pause
    exit /b 1
)
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo Закройте Unreal Engine и запустите сборку снова.
    pause
    exit /b 1
)
if not exist "%PROJECT_DIR%Content\CityLiveMaps\CityLive_V1.umap" goto missing_content
if not exist "%PROJECT_DIR%Content\CityLiveCharacters\Beige\SK_CityBeige_v3.uasset" goto missing_content
if not exist "%PROJECT_DIR%Saved" mkdir "%PROJECT_DIR%Saved"
set "OUTPUT_DIR=%PROJECT_DIR%Releases\AndroidV1"
echo Проверяется Android SDK. При ошибке откройте инструкцию Docs\AndroidV1.md.
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" Turnkey -command=VerifySdk -platform=Android "-project=%PROJECT_DIR%CityLiveRP.uproject" -utf8output
if errorlevel 1 (
    echo Проверка SDK не пройдена. См. Docs\AndroidV1.md.
    pause
    exit /b 1
)
echo Собирается тестовый APK ARM64 ASTC. Первый запуск может занять длительное время.
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=%PROJECT_DIR%CityLiveRP.uproject" -noP4 -platform=Android -cookflavor=ASTC -clientconfig=Development -build -cook -map=/Game/CityLiveMaps/CityLive_V1 -stage -pak -package -archive "-archivedirectory=%OUTPUT_DIR%" -utf8output > "%PROJECT_DIR%Saved\BuildAndroidV1.log" 2>&1
if errorlevel 1 (
    echo Сборка не удалась. Пришлите Saved\BuildAndroidV1.log.
    pause
    exit /b 1
)
set "FOUND_APK="
for /r "%OUTPUT_DIR%" %%F in (*.apk) do set "FOUND_APK=1"
if not defined FOUND_APK (
    echo APK не найден. Пришлите Saved\BuildAndroidV1.log.
    pause
    exit /b 1
)
echo Готово. Папка с APK открывается.
start "" "%OUTPUT_DIR%"
pause
exit /b 0

:missing_content
echo Сначала запустите BuildAndOpen.cmd и проверьте карту и персонажа.
echo Затем закройте редактор и снова запустите BuildAndroidV1.cmd.
pause
exit /b 1

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
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%PROJECT_DIR%Scripts\PrepareAndroidCity.ps1" -ProjectDir "%PROJECT_DIR%."
if errorlevel 1 goto missing_content
set "CITY_MAP="
set /p CITY_MAP=<"%PROJECT_DIR%Saved\AndroidCityMap.txt"
if not defined CITY_MAP goto missing_content
set "OUTPUT_DIR=%PROJECT_DIR%Releases\AndroidV1\%RANDOM%_%RANDOM%_%RANDOM%"
if exist "%OUTPUT_DIR%" (
    echo Папка сборки уже существует. Запустите файл снова.
    pause
    exit /b 1
)
echo Проверяется Android SDK. При ошибке откройте инструкцию Docs\AndroidV1.md.
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" Turnkey -command=VerifySdk -platform=Android "-project=%PROJECT_DIR%CityLiveRP.uproject" -utf8output > "%PROJECT_DIR%Saved\VerifyAndroidSDK.log" 2>&1
if errorlevel 1 (
    type "%PROJECT_DIR%Saved\VerifyAndroidSDK.log"
    echo Проверка SDK не пройдена. Пришлите Saved\VerifyAndroidSDK.log.
    pause
    exit /b 1
)
set "COOK_MODE=-iterativecooking"
if "%CITYLIVE_ANDROID_FULL_COOK%"=="1" set "COOK_MODE="
if defined COOK_MODE (
    echo Ускоренная подготовка: используются сохранённые обработанные ресурсы.
) else (
    echo Полная подготовка ресурсов без iterative cooking.
)
echo Результат в обоих режимах - полный APK для установки поверх игры.
echo Карта: %CITY_MAP%
echo Журнал сборки: Saved\BuildAndroidV1.log
echo Собирается тестовый APK ARM64 ASTC. Первый запуск может занять длительное время.
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=%PROJECT_DIR%CityLiveRP.uproject" -noP4 -platform=Android -cookflavor=ASTC -clientconfig=Development -build -cook %COOK_MODE% "-map=%CITY_MAP%" -stage -pak -package -archive "-archivedirectory=%OUTPUT_DIR%" -utf8output > "%PROJECT_DIR%Saved\BuildAndroidV1.log" 2>&1
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
echo Проверьте сообщение выше. Нужны импортированный город и установленный персонаж.
echo Затем закройте редактор и снова запустите BuildAndroidV1.cmd.
pause
exit /b 1

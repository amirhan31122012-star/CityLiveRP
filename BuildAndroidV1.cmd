@echo off
setlocal
chcp 65001 >nul
set "PROJECT_DIR=%~dp0"
set "BUILD_CONFIG=Development"
if /I "%~2"=="Shipping" set "BUILD_CONFIG=Shipping"
if not "%~2"=="" if /I not "%~2"=="Development" if /I not "%~2"=="Shipping" (
    echo Допустимые конфигурации: Development или Shipping.
    pause
    exit /b 1
)
set "BUILD_LOG=%PROJECT_DIR%Saved\BuildAndroidV1.log"
if "%BUILD_CONFIG%"=="Shipping" set "BUILD_LOG=%PROJECT_DIR%Saved\BuildAndroidV1Shipping.log"
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
if "%BUILD_CONFIG%"=="Shipping" set "OUTPUT_DIR=%PROJECT_DIR%Releases\AndroidV1\Shipping\%RANDOM%_%RANDOM%_%RANDOM%"
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
echo Карта: %CITY_MAP%
echo Конфигурация: %BUILD_CONFIG%. Графика и содержимое карты сохраняются.
echo Журнал сборки: %BUILD_LOG%
echo Собирается тестовый APK ARM64 ASTC. Первая сборка этой конфигурации может занять длительное время.
call "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=%PROJECT_DIR%CityLiveRP.uproject" -noP4 -platform=Android -cookflavor=ASTC "-clientconfig=%BUILD_CONFIG%" -build -cook "-map=%CITY_MAP%" -stage -pak -package -archive "-archivedirectory=%OUTPUT_DIR%" -utf8output > "%BUILD_LOG%" 2>&1
if errorlevel 1 (
    echo Сборка не удалась. Пришлите журнал: %BUILD_LOG%
    pause
    exit /b 1
)
set "FOUND_APK="
for /r "%OUTPUT_DIR%" %%F in (*.apk) do set "FOUND_APK=1"
if not defined FOUND_APK (
    echo APK не найден. Пришлите журнал: %BUILD_LOG%
    pause
    exit /b 1
)
> "%OUTPUT_DIR%\BuildInfo.txt" echo Configuration=%BUILD_CONFIG%
>> "%OUTPUT_DIR%\BuildInfo.txt" echo Map=%CITY_MAP%
>> "%OUTPUT_DIR%\BuildInfo.txt" echo CookFlavor=ASTC
>> "%OUTPUT_DIR%\BuildInfo.txt" echo Package=com.cityliverp.game
>> "%OUTPUT_DIR%\BuildInfo.txt" echo BuildLog=%BUILD_LOG%
echo Готово. Папка с APK открывается.
start "" "%OUTPUT_DIR%"
pause
exit /b 0

:missing_content
echo Проверьте сообщение выше. Нужны импортированный город и установленный персонаж.
echo Затем закройте редактор и снова запустите BuildAndroidV1.cmd.
pause
exit /b 1

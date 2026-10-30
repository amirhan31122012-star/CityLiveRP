@echo off
setlocal
chcp 65001 >nul
set "PROJECT_DIR=%~dp0"
set "UE_ROOT=%~1"
if not exist "%PROJECT_DIR%CityLiveRP.uproject" (
    echo Файл должен находиться рядом с CityLiveRP.uproject.
    pause
    exit /b 1
)
tasklist /FI "IMAGENAME eq UnrealEditor.exe" /NH 2>nul | find /I "UnrealEditor.exe" >nul
if not errorlevel 1 (
    echo Сначала закройте уже открытый Unreal Engine.
    pause
    exit /b 1
)
if not defined UE_ROOT (
    for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v InstalledDirectory 2^>nul') do set "UE_ROOT=%%B"
)
if not exist "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" (
    if "%~1"=="" set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
)
if not exist "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" (
    echo Unreal Engine 5.8 не найден. Укажите папку движка первым аргументом.
    pause
    exit /b 1
)
echo Открывается проект с пустым уровнем, без загрузки города.
echo Для SDK: Правка - Настройки проекта - Платформы - Android SDK.
echo Игровую карту и стартовую карту проекта этот файл не меняет.
start "" "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT_DIR%CityLiveRP.uproject" /Engine/Maps/Entry
exit /b 0

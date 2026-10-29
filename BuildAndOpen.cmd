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
    echo Закройте Unreal Engine перед сборкой и снова запустите этот файл.
    pause
    exit /b 1
)
if not defined UE_ROOT (
    for /f "tokens=2,*" %%A in ('reg query "HKLM\SOFTWARE\EpicGames\Unreal Engine\5.8" /v InstalledDirectory 2^>nul') do set "UE_ROOT=%%B"
)
rem Epic Launcher may not register the engine. Also check its default install path.
if "%~1"=="" (
    if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
        if exist "%ProgramFiles%\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" set "UE_ROOT=%ProgramFiles%\Epic Games\UE_5.8"
    )
)
if not exist "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" (
    echo Unreal Engine 5.8 не найден в реестре.
    echo Запустите этот файл из командной строки, указав папку движка:
    echo BuildAndOpen.cmd "D:\Epic Games\UE_5.8"
    pause
    exit /b 1
)
if not exist "%PROJECT_DIR%Saved" mkdir "%PROJECT_DIR%Saved"
echo Собирается проект из папки: %PROJECT_DIR%
echo Дождитесь окончания. При ошибке редактор не будет запущен.
call "%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" CityLiveRPEditor Win64 Development "-Project=%PROJECT_DIR%CityLiveRP.uproject" -WaitMutex -NoHotReloadFromIDE "-Log=%PROJECT_DIR%Saved\BuildCityLiveRP.log"
if errorlevel 1 (
    echo Сборка не удалась. Пришлите файл Saved\BuildCityLiveRP.log.
    pause
    exit /b 1
)
echo Сборка завершена. Открывается обновлённый проект.
if exist "%PROJECT_DIR%Assets\Characters\Beige\SK_CityBeige.fbx" (
    call :ensure_character_skin
    if errorlevel 1 (
        echo Установка персонажа не завершена. Пришлите Saved\Logs\CityLiveRP.log.
        pause
        exit /b 1
    )
)
start "" "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT_DIR%CityLiveRP.uproject"
exit /b 0

:ensure_character_skin
if exist "%PROJECT_DIR%Saved\CharacterImport\Installed_v2.ok" if exist "%PROJECT_DIR%Content\CityLiveCharacters\Beige\SK_CityBeige_v2.uasset" exit /b 0
set "CITYLIVE_SKIN_TOKEN=%RANDOM%_%RANDOM%_%RANDOM%"
echo Первый запуск: устанавливается новый персонаж. Дождитесь окончания.
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%PROJECT_DIR%CityLiveRP.uproject" "-ExecutePythonScript=%PROJECT_DIR%Scripts\ImportCharacterSkin.py" -unattended -nosplash
if errorlevel 1 exit /b 1
if not exist "%PROJECT_DIR%Saved\CharacterImport\Import_%CITYLIVE_SKIN_TOKEN%.ok" exit /b 1
exit /b 0

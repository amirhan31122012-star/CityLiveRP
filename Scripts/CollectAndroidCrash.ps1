param([Parameter(Mandatory = $true)][string]$ProjectDir)
$ErrorActionPreference = 'Stop'
$utf8 = New-Object System.Text.UTF8Encoding($false)

function Read-Adb([string[]]$AdbArguments) {
    foreach ($argument in $AdbArguments) {
        if ($argument.Contains('"')) { throw 'Invalid diagnostic argument' }
    }
    $info = New-Object System.Diagnostics.ProcessStartInfo
    $info.FileName = $script:adb
    $info.Arguments = ($AdbArguments | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.StandardOutputEncoding = $utf8
    $info.StandardErrorEncoding = $utf8
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $info
    try {
        [void]$process.Start()
        $out = $process.StandardOutput.ReadToEndAsync()
        $err = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(60000)) {
            $process.Kill()
            throw 'Телефон не ответил за минуту. Проверьте кабель и разрешение USB-отладки.'
        }
        return [pscustomobject]@{ Code = $process.ExitCode; Text = $out.Result + $err.Result }
    } finally { $process.Dispose() }
}

try {
    $roots = @($env:ANDROID_HOME, $env:ANDROID_SDK_ROOT, $env:LOCALAPPDATA,
               (Join-Path $env:LOCALAPPDATA 'Android\Sdk'))
    $script:adb = $null
    foreach ($root in $roots) {
        if ($root) {
            $candidate = Join-Path $root 'platform-tools\adb.exe'
            if (Test-Path -LiteralPath $candidate -PathType Leaf) { $script:adb = $candidate; break }
        }
    }
    if (-not $script:adb) {
        $command = Get-Command adb.exe -ErrorAction SilentlyContinue
        if ($command) { $script:adb = $command.Source }
    }
    if (-not $script:adb) {
        throw 'ADB не найден. Android Studio → SDK Manager → SDK Tools → Android SDK Platform-Tools → Apply.'
    }
    $list = Read-Adb @('devices')
    if ($list.Code -ne 0) { throw $list.Text }
    $devices = @([regex]::Matches($list.Text, '(?m)^(\S+)\s+(device|unauthorized|offline)\b') |
        Where-Object { $_.Groups[1].Value -notmatch '^emulator-|:' })
    if ($devices.Count -eq 0) {
        throw 'Телефон по USB не найден. Подключите кабель передачи данных, разблокируйте телефон и включите USB-отладку.'
    }
    if ($devices.Count -ne 1) { throw 'Подключите только один телефон для снятия журнала.' }
    if ($devices[0].Groups[2].Value -eq 'unauthorized') {
        throw 'На телефоне подтвердите «Разрешить отладку по USB», затем запустите файл снова.'
    }
    if ($devices[0].Groups[2].Value -ne 'device') { throw 'Телефон offline. Переподключите USB-кабель.' }
    $serial = $devices[0].Groups[1].Value
    if ($serial -notmatch '^[A-Za-z0-9._-]+$') { throw 'Необычный идентификатор USB-устройства.' }
    $commands = @(
        @{ Name='MODEL'; Values=@('shell','getprop','ro.product.model') },
        @{ Name='MANUFACTURER'; Values=@('shell','getprop','ro.product.manufacturer') },
        @{ Name='ANDROID'; Values=@('shell','getprop','ro.build.version.release') },
        @{ Name='CPU_ABI'; Values=@('shell','getprop','ro.product.cpu.abi') },
        @{ Name='RAM'; Values=@('shell','cat','/proc/meminfo') },
        @{ Name='CITYLIVE_PACKAGE'; Values=@('shell','dumpsys','package','com.cityliverp.game') },
        @{ Name='CITYLIVE_EXIT_REASONS'; Values=@('shell','dumpsys','activity','exit-info','com.cityliverp.game') },
        @{ Name='CRASH_BUFFER'; Values=@('logcat','-b','crash','-d','-v','threadtime','-t','1200') },
        @{ Name='ENGINE_LOG'; Values=@('logcat','-b','main','-b','system','-d','-v','threadtime','-t','6000',
            'UE:V','UE4:V','Unreal:V','AndroidRuntime:E','libc:F','DEBUG:V','ActivityManager:I','lmkd:I','*:S') }
    )
    $folder = Join-Path $ProjectDir ('Saved\AndroidCrash\' + (Get-Date -Format 'yyyyMMdd_HHmmss'))
    New-Item -ItemType Directory -Path $folder -Force | Out-Null
    $file = Join-Path $folder 'AndroidCrash.txt'
    [IO.File]::WriteAllText($file, ('CityLive RP diagnostic, ' + (Get-Date).ToString('s') + "`r`n"), $utf8)
    foreach ($command in $commands) {
        Write-Host ('Чтение: ' + $command.Name)
        $result = Read-Adb (@('-s', $serial) + $command.Values)
        [IO.File]::AppendAllText($file, ("`r`n===== " + $command.Name + " =====`r`n" + $result.Text + "`r`n"), $utf8)
    }
    Write-Host 'Готово. Прикрепите AndroidCrash.txt из открывшейся папки.'
    Start-Process explorer.exe -ArgumentList ('"' + $folder + '"')
} catch {
    Write-Host $_.Exception.Message
    exit 1
}

param([Parameter(Mandatory = $true)][string]$ProjectDir)
$ErrorActionPreference = 'Stop'
try {
    $marker = Join-Path $ProjectDir 'Saved\ModernCityMap.txt'
    if (-not (Test-Path -LiteralPath $marker -PathType Leaf)) {
        throw 'Сначала успешно завершите ImportPoliceDistrict.cmd: он установит новую основную карту.'
    }
    $map = [System.IO.File]::ReadAllText($marker).Trim()
    if ($map -notmatch '^/Game/CityLiveMaps/ModernCity/Run_[0-9_]+/CityLive_ModernCity$') {
        throw 'Неверный путь карты в Saved\ModernCityMap.txt. Запустите ImportPoliceDistrict.cmd.'
    }
    $relativeMap = $map.Substring('/Game/'.Length).Replace('/', '\') + '.umap'
    $mapFile = Join-Path (Join-Path $ProjectDir 'Content') $relativeMap
    if (-not (Test-Path -LiteralPath $mapFile -PathType Leaf)) {
        throw "Карта не найдена: $mapFile"
    }
    $skin = Join-Path $ProjectDir 'Content\CityLiveCharacters\Beige\SK_CityBeige_v3.uasset'
    if (-not (Test-Path -LiteralPath $skin -PathType Leaf)) {
        throw 'Сначала установите персонажа через BuildAndOpen.cmd.'
    }
    foreach ($relativeIcon in @('Build\Android\res\drawable\icon.png',
                                'Build\Android\res\drawable\splashscreen_landscape.png',
                                'Build\Android\res\drawable\splashscreen_portrait.png',
                                'Build\Android\res\drawable-anydpi-v26\icon.xml',
                                'Build\Android\res\drawable\citylive_tiger_foreground.png',
                                'Build\Android\res\values\citylive_icon_colors.xml')) {
        if (-not (Test-Path -LiteralPath (Join-Path $ProjectDir $relativeIcon) -PathType Leaf)) {
            throw "Файл значка не найден: $relativeIcon. Обновите проект в GitHub Desktop."
        }
    }
    $mobileMarker = Join-Path $ProjectDir 'Saved\CityV11.json'
    if (-not (Test-Path -LiteralPath $mobileMarker -PathType Leaf)) {
        throw 'Сначала выполните ImportPoliceDistrict.cmd: новая основная карта игры.'
    }
    $mobile = Get-Content -LiteralPath $mobileMarker -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($mobile.map -ne $map -or $mobile.version -ne '1.1.2' -or
        $mobile.graphics_baseline -ne 'v1' -or -not $mobile.mobile_sky -or
        $mobile.source -ne 'PoliceDistrict') {
        throw 'Активна прежняя карта. Сначала запустите ImportPoliceDistrict.cmd.'
    }
    $packageManifest = Join-Path $ProjectDir 'Assets\Maps\PoliceDistrictGame\manifest.json'
    if ($mobile.lighting_revision -ne 1) {
        throw 'Сначала выполните UpdateDistrictPolish.cmd: исправление освещения установленной карты.'
    }
    if (-not (Test-Path -LiteralPath $packageManifest -PathType Leaf)) {
        throw 'Пакет новой карты отсутствует. Обновите проект в GitHub Desktop.'
    }
    $packageHash = (Get-FileHash -LiteralPath $packageManifest -Algorithm SHA256).Hash
    if ($mobile.package_hash -ne $packageHash) {
        throw 'Файлы новой карты обновились. Повторите ImportPoliceDistrict.cmd перед сборкой APK.'
    }
    # Pulling the repository may restore its default map. Use the map confirmed
    # by the successful local import for both cooking and the packaged startup.
    $ini = Join-Path $ProjectDir 'Config\DefaultEngine.ini'
    $text = [System.IO.File]::ReadAllText($ini)
    foreach ($key in @('GameDefaultMap', 'EditorStartupMap')) {
        $pattern = '(?m)^' + $key + '=[^\r\n]*'
        if (-not [regex]::IsMatch($text, $pattern)) {
            throw "В DefaultEngine.ini отсутствует $key. Пришлите этот файл."
        }
        $text = [regex]::Replace($text, $pattern, $key + '=' + $map + '.CityLive_ModernCity')
    }
    $utf8 = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($ini, $text, $utf8)
    [System.IO.File]::WriteAllText((Join-Path $ProjectDir 'Saved\AndroidCityMap.txt'), $map, $utf8)
    Write-Host "Карта для APK: $map"
} catch {
    Write-Host $_.Exception.Message
    exit 1
}

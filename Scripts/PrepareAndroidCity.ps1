param([Parameter(Mandatory = $true)][string]$ProjectDir)
$ErrorActionPreference = 'Stop'
try {
    $marker = Join-Path $ProjectDir 'Saved\ModernCityMap.txt'
    if (-not (Test-Path -LiteralPath $marker -PathType Leaf)) {
        throw 'Сначала успешно завершите ImportModernCity.cmd.'
    }
    $map = [System.IO.File]::ReadAllText($marker).Trim()
    if ($map -notmatch '^/Game/CityLiveMaps/ModernCity/Run_[0-9_]+/CityLive_ModernCity$') {
        throw 'Неверный путь карты в Saved\ModernCityMap.txt. Повторите импорт города.'
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

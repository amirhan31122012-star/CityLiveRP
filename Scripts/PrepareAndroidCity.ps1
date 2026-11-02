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
    foreach ($clip in @('Enter', 'Exit', 'EnterRight', 'ExitRight', 'DriverIdle', 'ScooterMount', 'ScooterExit')) {
        $clipPath = Join-Path $ProjectDir ("Content\CityLiveCharacters\Vehicle\AS_City" + $clip + '.uasset')
        if (-not (Test-Path -LiteralPath $clipPath -PathType Leaf)) {
            throw 'Сначала выполните InstallVehicleEntryAnimations.cmd: анимации посадки без уменьшения персонажа.'
        }
    }
    if (-not (Test-Path -LiteralPath (Join-Path $ProjectDir 'Content\CityLiveVehicles\Courier\SM_CityScooter_v2.uasset'))) {
        throw 'Сначала запустите InstallDistrictContent.cmd для установки скутера.'
    }
    if (-not (Test-Path -LiteralPath (Join-Path $ProjectDir 'Content\CityLiveUI\M_DistrictSurface.uasset'))) {
        throw 'Выполните InstallDistrictContent.cmd: цветные материалы интерьеров.'
    }
    foreach ($role in @('Civilian', 'Police', 'Medic', 'Courier', 'CourierPack')) {
        if (-not (Test-Path -LiteralPath (Join-Path $ProjectDir ("Content\CityLiveCharacters\Roles\SK_City" + $role + '.uasset')))) {
            throw 'Выполните InstallDistrictContent.cmd: установка новых персонажей.'
        }
    }
    $rolesMarker = Join-Path $ProjectDir 'Saved\RoleCharacters\Installed.json'
    $interfaceMarker = Join-Path $ProjectDir 'Saved\CityInterface.json'
    if (-not (Test-Path -LiteralPath $interfaceMarker) -or
        -not (Test-Path -LiteralPath (Join-Path $ProjectDir 'Content\CityLiveUI\T_LoginCity.uasset'))) {
        throw 'Выполните InstallDistrictContent.cmd: установка нового интерфейса.'
    }
    $interface = Get-Content -LiteralPath $interfaceMarker -Raw -Encoding UTF8 | ConvertFrom-Json
    if ($interface.revision -ne 2) {
        throw 'Интерфейс обновился. Повторите InstallDistrictContent.cmd.'
    }
    $uiAssets = @{
        'T_LoginCity' = 'Assets/Branding/CityLiveLoginCity.png'
        'T_AccountCity' = 'Assets/Branding/CityLiveAccountCity.png'
        'T_SpawnOrganization' = 'Assets/UI/Spawn/Organization.png'
        'T_SpawnStart' = 'Assets/UI/Spawn/Start.png'
        'T_SpawnLast' = 'Assets/UI/Spawn/Last.png'
        'T_SpawnParking' = 'Assets/UI/Spawn/Parking.png'
        'T_SpawnHome' = 'Assets/UI/Spawn/Home.png'
    }
    foreach ($entry in $uiAssets.GetEnumerator()) {
        $artHash = (Get-FileHash -LiteralPath (Join-Path $ProjectDir $entry.Value) -Algorithm SHA256).Hash
        $savedHash = $interface.assets.PSObject.Properties[$entry.Value].Value
        if ($savedHash -ne $artHash -or
            -not (Test-Path -LiteralPath (Join-Path $ProjectDir ('Content/CityLiveUI/' + $entry.Key + '.uasset')))) {
            throw 'Фоны и карточки обновились. Повторите InstallDistrictContent.cmd.'
        }
    }
    $scooterMarker = Join-Path $ProjectDir 'Saved/CourierScooter.json'
    if (-not (Test-Path -LiteralPath $scooterMarker)) {
        throw 'Выполните InstallDistrictContent.cmd: установка цветного скутера.'
    }
    $scooter = Get-Content -LiteralPath $scooterMarker -Raw -Encoding UTF8 | ConvertFrom-Json
    $scooterHash = (Get-FileHash -LiteralPath (Join-Path $ProjectDir 'Assets/Vehicles/Courier/SM_CityScooter.fbx') -Algorithm SHA256).Hash
    if ($scooter.revision -ne 2 -or $scooter.import_revision -ne 3 -or
        $scooter.asset_path -ne '/Game/CityLiveVehicles/Courier/SM_CityScooter_v2' -or
        $scooter.mesh_sha256 -ne $scooterHash) {
        throw 'Скутер обновился. Повторите InstallDistrictContent.cmd.'
    }
    if (-not (Test-Path -LiteralPath $rolesMarker -PathType Leaf)) {
        throw 'Выполните InstallDistrictContent.cmd: установка подробных лиц персонажей.'
    }
    $roles = Get-Content -LiteralPath $rolesMarker -Raw -Encoding UTF8 | ConvertFrom-Json
    $rolesHash = (Get-FileHash -LiteralPath (Join-Path $ProjectDir 'Assets\Characters\Roles\manifest.json') -Algorithm SHA256).Hash
    if ($roles.face_revision -ne 2 -or $roles.wardrobe_revision -ne 3 -or $roles.package_hash -ne $rolesHash) {
        throw 'Персонажи обновились. Повторите InstallDistrictContent.cmd перед сборкой APK.'
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
    foreach ($relative in @('Content\CityLiveMaps\CityLive_Boot.umap', 'Content\CityLiveUI\T_LoadingCity.uasset')) {
        if (-not (Test-Path -LiteralPath (Join-Path $ProjectDir $relative) -PathType Leaf)) {
            throw 'Сначала выполните UpdateDistrictPolish.cmd: подготовка экрана загрузки с процентами.'
        }
    }
    $text = [System.IO.File]::ReadAllText($ini)
    foreach ($key in @('GameDefaultMap', 'EditorStartupMap')) {
        $pattern = '(?m)^' + $key + '=[^\r\n]*'
        if (-not [regex]::IsMatch($text, $pattern)) {
            throw "В DefaultEngine.ini отсутствует $key. Пришлите этот файл."
        }
        $target = $map + '.CityLive_ModernCity'
        if ($key -eq 'GameDefaultMap') { $target = '/Game/CityLiveMaps/CityLive_Boot.CityLive_Boot' }
        $text = [regex]::Replace($text, $pattern, $key + '=' + $target)
    }
    $utf8 = New-Object System.Text.UTF8Encoding($false)
    [System.IO.File]::WriteAllText($ini, $text, $utf8)
    $gameIni = Join-Path $ProjectDir 'Config\DefaultGame.ini'
    $gameText = [System.IO.File]::ReadAllText($gameIni)
    if (-not [regex]::IsMatch($gameText, '(?m)^CityMapPath=[^\r\n]*')) {
        throw 'Отсутствует настройка CityMapPath. Обновите проект в GitHub Desktop.'
    }
    $gameText = [regex]::Replace($gameText, '(?m)^CityMapPath=[^\r\n]*', 'CityMapPath=' + $map)
    [System.IO.File]::WriteAllText($gameIni, $gameText, $utf8)
    [System.IO.File]::WriteAllText((Join-Path $ProjectDir 'Saved\AndroidCityMap.txt'), $map, $utf8)
    Write-Host "Карта для APK: $map"
} catch {
    Write-Host $_.Exception.Message
    exit 1
}

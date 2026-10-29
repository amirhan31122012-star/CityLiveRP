param(
    [Parameter(Mandatory = $true)][string]$ProjectDir,
    [Parameter(Mandatory = $true)][string]$OutputDir
)
$ErrorActionPreference = 'Stop'
try {
    $cache = Join-Path $ProjectDir 'Saved\CityImport'
    $candidates = Get-ChildItem -LiteralPath $cache -Directory | Sort-Object LastWriteTime -Descending
    foreach ($folder in $candidates) {
        $manifest = Join-Path $folder.FullName 'manifest.json'
        if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) { continue }
        $report = Get-Content -LiteralPath $manifest -Raw -Encoding UTF8 | ConvertFrom-Json
        if ($report.version -lt 2 -or -not $report.parts) { continue }
        $valid = $true
        foreach ($part in $report.parts) {
            if (-not (Test-Path -LiteralPath $part.fbx -PathType Leaf) -or -not $part.materials) {
                $valid = $false
                break
            }
            foreach ($material in $part.materials) {
                foreach ($role in @('base', 'normal', 'packed', 'emissive')) {
                    $info = $material.$role
                    if ($info -and -not (Test-Path -LiteralPath $info.file -PathType Leaf)) {
                        $valid = $false
                        break
                    }
                }
            }
        }
        if (-not $valid) { continue }
        Copy-Item -LiteralPath $manifest -Destination (Join-Path $OutputDir 'manifest.json')
        Write-Host "Используются готовые файлы: $($folder.FullName)"
        exit 0
    }
    throw 'Готовые файлы не найдены. Запустите ImportModernCity.cmd.'
} catch {
    Write-Host $_.Exception.Message
    exit 1
}

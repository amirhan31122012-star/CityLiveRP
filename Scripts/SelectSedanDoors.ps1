param([Parameter(Mandatory = $true)][string]$OutputPath)

Add-Type -AssemblyName System.Windows.Forms
$dialog = New-Object System.Windows.Forms.OpenFileDialog
try {
    $dialog.Title = 'Выберите скачанный SK_CitySedan_4Doors_v2.fbx'
    $dialog.Filter = 'Модель FBX (*.fbx)|*.fbx'
    $dialog.CheckFileExists = $true
    $dialog.Multiselect = $false
    if ($dialog.ShowDialog() -ne [System.Windows.Forms.DialogResult]::OK) {
        exit 1
    }
    [System.IO.File]::WriteAllText($OutputPath, $dialog.FileName,
        (New-Object System.Text.UTF8Encoding($false)))
} finally {
    $dialog.Dispose()
}

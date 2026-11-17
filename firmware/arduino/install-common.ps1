# Funciones compartidas por los dos instaladores; no se ejecutan durante el build.
Set-StrictMode -Version 2.0
$ErrorActionPreference = 'Stop'

function Get-SentinelDependencies {
    Get-Content -LiteralPath (Join-Path $PSScriptRoot 'dependencies.json') -Raw | ConvertFrom-Json
}

function Get-ArduinoDirectory([string] $ExplicitPath, [string] $Kind) {
    if ($ExplicitPath) { return [IO.Path]::GetFullPath($ExplicitPath) }
    $configFile = Join-Path $env:USERPROFILE '.arduinoIDE\arduino-cli.yaml'
    $command = Get-Command arduino-cli.exe -ErrorAction SilentlyContinue
    $cli = if ($command) { $command.Source } else {
        Join-Path $env:LOCALAPPDATA 'Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe'
    }
    if (Test-Path -LiteralPath $cli -PathType Leaf) {
        $arguments = @('config', 'dump', '--json')
        if (Test-Path -LiteralPath $configFile) { $arguments += @('--config-file', $configFile) }
        $dump = (& $cli @arguments) -join "`n"
        if ($LASTEXITCODE -ne 0) { throw 'No se pudo consultar Arduino CLI; indica la ruta explicitamente.' }
        $settings = ($dump | ConvertFrom-Json).config
        if ($settings.PSObject.Properties['directories']) {
            $directory = $settings.directories.PSObject.Properties[$Kind]
            if ($directory -and $directory.Value) { return [IO.Path]::GetFullPath($directory.Value) }
        }
    } elseif (Test-Path -LiteralPath $configFile) {
        throw 'Hay configuracion personalizada de Arduino pero no se encontro su CLI; indica la ruta explicitamente.'
    }
    if ($Kind -eq 'user') { return Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'Arduino' }
    return Join-Path $env:LOCALAPPDATA 'Arduino15'
}

function Assert-ExternalDestination([string] $Path) {
    $repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..')).TrimEnd('\')
    $destination = [IO.Path]::GetFullPath($Path).TrimEnd('\')
    if ($destination.Equals($repository, [StringComparison]::OrdinalIgnoreCase) -or
        $destination.StartsWith($repository + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Las dependencias y respaldos deben instalarse fuera del repositorio.'
    }
    if ((Test-Path -LiteralPath $Path) -and
        ((Get-Item -LiteralPath $Path).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
        throw "El destino es un enlace o junction; revisalo antes de instalar: $Path"
    }
}

function Test-FileDigest([string] $Path, [string] $Expected) {
    return (Test-Path -LiteralPath $Path -PathType Leaf) -and
        ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -eq $Expected)
}

function Test-LibraryFiles([string] $Directory, [hashtable] $Expected) {
    if (-not (Test-Path -LiteralPath $Directory -PathType Container)) { return $false }
    $files = @(Get-ChildItem -LiteralPath $Directory -File -Recurse -Force)
    if ($files.Count -ne $Expected.Count) { return $false }
    foreach ($relative in $Expected.Keys) {
        if (-not (Test-FileDigest (Join-Path $Directory $relative) $Expected[$relative])) { return $false }
    }
    return $true
}

function Write-ConfigWithBackup([string] $Path, [string] $Content) {
    $temporary = $Path + '.sentinel-' + [Guid]::NewGuid().ToString('N') + '.tmp'
    try {
        [IO.File]::WriteAllText($temporary, $Content, (New-Object Text.UTF8Encoding($false)))
        if (Test-Path -LiteralPath $Path) {
            $backup = $Path + '.sentinel-' + [Guid]::NewGuid().ToString('N') + '.bak'
            [IO.File]::Replace($temporary, $Path, $backup)
            Write-Host "Respaldo: $backup"
        } else { [IO.File]::Move($temporary, $Path) }
    } finally {
        if (Test-Path -LiteralPath $temporary) { Remove-Item -LiteralPath $temporary -Force }
    }
}

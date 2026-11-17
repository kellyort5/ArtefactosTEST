# Preparacion inicial de wolfMQTT. Usar desde PowerShell, sin administrador.
[CmdletBinding(SupportsShouldProcess = $true)]
param(
    [string] $SketchbookPath,
    [string] $ArchivePath
)
. (Join-Path $PSScriptRoot 'install-common.ps1')
$manifest = Get-SentinelDependencies
$dependency = @($manifest.libraries | Where-Object { $_.name -eq 'wolfMQTT' })[0]
$distribution = $dependency.arduino_zip
$sketchbook = Get-ArduinoDirectory $SketchbookPath 'user'
$libraries = Join-Path $sketchbook 'libraries'
$destination = Join-Path $libraries 'wolfMQTT'
Assert-ExternalDestination $destination

# Inventario exacto: fuentes originales verificadas y los tres archivos Sentinel.
$expected = @{}
foreach ($entry in $distribution.files.PSObject.Properties) { $expected[$entry.Name] = $entry.Value }
$customFiles = @('library.properties', 'wolfMQTT.h', 'wolfmqtt/options.h')
foreach ($relative in $customFiles) {
    $expected[$relative] = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot "wolfmqtt/$relative") -Algorithm SHA256).Hash
}
if (Test-LibraryFiles $destination $expected) {
    Write-Host "wolfMQTT $($dependency.version) ya esta correctamente instalado: $destination"
    return
}
if (-not $PSCmdlet.ShouldProcess($destination, 'Instalar wolfMQTT verificado y respaldar la version anterior')) { return }

$work = Join-Path ([IO.Path]::GetTempPath()) ('sentinel-wolfmqtt-' + [Guid]::NewGuid().ToString('N'))
$stage = Join-Path $libraries ('.sentinel-wolfmqtt-' + [Guid]::NewGuid().ToString('N'))
$backup = $null
$activated = $false
New-Item -ItemType Directory -Path $work -Force | Out-Null
try {
    $archive = Join-Path $work 'wolfMQTT.zip'
    if ($ArchivePath) { Copy-Item -LiteralPath $ArchivePath -Destination $archive } else {
        $protocol = [Net.ServicePointManager]::SecurityProtocol
        try {
            [Net.ServicePointManager]::SecurityProtocol = $protocol -bor [Net.SecurityProtocolType]::Tls12
            Invoke-WebRequest -Uri $distribution.source -OutFile $archive -UseBasicParsing -TimeoutSec 120
        } finally { [Net.ServicePointManager]::SecurityProtocol = $protocol }
    }
    if (-not (Test-FileDigest $archive $distribution.sha256)) { throw 'SHA256 incorrecto: la biblioteca existente no se ha modificado.' }
    Expand-Archive -LiteralPath $archive -DestinationPath (Join-Path $work 'source')
    $source = Join-Path (Join-Path $work 'source') $distribution.root
    New-Item -ItemType Directory -Path $stage -Force | Out-Null
    foreach ($entry in $distribution.files.PSObject.Properties) {
        $relative = $entry.Name
        $original = if ($relative.EndsWith('.c')) { Join-Path $source "src/$relative" } else { Join-Path $source $relative }
        if (-not (Test-FileDigest $original $entry.Value)) { throw "Fuente original inesperada: $relative" }
        $target = Join-Path $stage $relative
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        Copy-Item -LiteralPath $original -Destination $target
    }
    foreach ($relative in $customFiles) {
        Copy-Item -LiteralPath (Join-Path $PSScriptRoot "wolfmqtt/$relative") -Destination (Join-Path $stage $relative) -Force
    }
    if (-not (Test-LibraryFiles $stage $expected)) { throw 'La biblioteca preparada no coincide con el inventario esperado.' }

    # La copia anterior queda fuera de libraries para evitar bibliotecas duplicadas.
    if (Test-Path -LiteralPath $destination) {
        $backupRoot = Join-Path $sketchbook '.sentinel-backups'
        New-Item -ItemType Directory -Path $backupRoot -Force | Out-Null
        $backup = Join-Path $backupRoot ('wolfMQTT-' + [Guid]::NewGuid().ToString('N'))
        [IO.Directory]::Move($destination, $backup)
    }
    try {
        [IO.Directory]::Move($stage, $destination)
        $activated = $true
        if (-not (Test-LibraryFiles $destination $expected)) { throw 'Fallo al verificar la biblioteca instalada.' }
    } catch {
        if ($activated) { [IO.Directory]::Move($destination, $stage); $activated = $false }
        if ($backup) { [IO.Directory]::Move($backup, $destination); $backup = $null }
        throw
    }
    Write-Host "wolfMQTT $($dependency.version) instalado: $destination"
    if ($backup) { Write-Host "Respaldo: $backup" }
    Write-Host 'Reinicia Arduino IDE para reconocer la biblioteca.'
} finally {
    if (Test-Path -LiteralPath $stage) { Remove-Item -LiteralPath $stage -Recurse -Force }
    Remove-Item -LiteralPath $work -Recurse -Force
}

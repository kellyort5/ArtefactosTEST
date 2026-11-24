# Instala solo la opcion Sentinel en el core oficial ya instalado.
[CmdletBinding(SupportsShouldProcess = $true)]
param([string] $ArduinoDataPath)
. (Join-Path $PSScriptRoot 'install-common.ps1')
$manifest = Get-SentinelDependencies
$data = Get-ArduinoDirectory $ArduinoDataPath 'data'
$core = Join-Path $data "packages/esp32/hardware/esp32/$($manifest.core)"
Assert-ExternalDestination $core
if (-not (Test-Path -LiteralPath (Join-Path $core 'boards.txt') -PathType Leaf)) {
    throw "Instala esp32 de Espressif Systems $($manifest.core) desde el gestor de placas. No se modifico el core."
}
$desired = [Collections.Generic.Dictionary[string,string]]::new([StringComparer]::Ordinal)
$sourceLines = @(Get-Content -LiteralPath (Join-Path $PSScriptRoot 'boards.local.txt'))
foreach ($line in $sourceLines) {
    if ($line -match '^\s*([^#=\s][^=]*?)\s*=(.*)$') { $desired[$matches[1].Trim()] = $matches[2] }
}
if ($desired.Count -ne 4) { throw 'La configuracion aprobada debe contener exactamente cuatro claves.' }
# Retirar exclusivamente las cinco recetas Sentinel de la distribucion anterior.
# Comparacion ordinal: una opcion ajena parecida no es nuestra propiedad.
$obsolete = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
foreach ($suffix in @(
    'tools.esptool_py.upload.pattern_args',
    'recipe.hooks.objcopy.postobjcopy.3.pattern.windows',
    'recipe.hooks.objcopy.postobjcopy.4.pattern.windows',
    'recipe.hooks.objcopy.postobjcopy.5.pattern.windows',
    'recipe.hooks.savehex.postsavehex.4.pattern.windows'
)) { [void] $obsolete.Add('esp32s3.menu.PartitionScheme.sentinel.' + $suffix) }
$path = Join-Path $core 'boards.local.txt'
Assert-ExternalDestination $path
$original = if (Test-Path -LiteralPath $path) { [IO.File]::ReadAllText($path) } else { '' }
$lines = if ($original) { [regex]::Split($original, '\r\n|\n|\r') } else { @() }
$counts = @{}
$correct = $true
foreach ($line in $lines) {
    if ($line -match '^\s*([^#=\s][^=]*?)\s*=(.*)$') {
        $key = $matches[1].Trim()
        if ($obsolete.Contains($key)) { $correct = $false }
        if ($desired.ContainsKey($key)) {
            if (-not $counts.ContainsKey($key)) { $counts[$key] = 0 }
            $counts[$key]++
            if ($matches[2] -cne $desired[$key]) { $correct = $false }
        }
    }
}
foreach ($key in $desired.Keys) { if (-not $counts.ContainsKey($key) -or $counts[$key] -ne 1) { $correct = $false } }
if ($correct) { Write-Host "Configuracion Sentinel ya correcta: $path"; return }
if (-not $PSCmdlet.ShouldProcess($path, 'Combinar cuatro claves Sentinel y retirar cinco recetas obsoletas conservando las demas opciones')) { return }

# Se conservan las lineas y comentarios ajenos, incluso bajo el prefijo Sentinel.
$kept = New-Object 'Collections.Generic.List[string]'
foreach ($line in $lines) {
    if ($line -match '^\s*([^#=\s][^=]*?)\s*=(.*)$') {
        $key = $matches[1].Trim()
        if ($desired.ContainsKey($key) -or $obsolete.Contains($key)) { continue }
    }
    $kept.Add($line)
}
$newline = if ($original.Contains("`r`n")) { "`r`n" } else { "`n" }
$content = ($kept -join $newline).TrimEnd("`r", "`n")
if ($content.Length) { $content += $newline + $newline }
$content += ($sourceLines -join $newline) + $newline
Write-ConfigWithBackup $path $content
Write-Host "Configuracion Sentinel instalada: $path"
Write-Host 'Reinicia Arduino IDE y selecciona Sentinel standard USB (4MiB factory, no OTA).'

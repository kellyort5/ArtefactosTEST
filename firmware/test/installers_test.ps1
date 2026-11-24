# Pruebas focalizadas en destinos temporales; no usa ni altera el Sketchbook real.
[CmdletBinding()]
param([Parameter(Mandatory = $true)][string] $ArchivePath, [switch] $TestDownload)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$support = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\arduino'))
$wolf = Join-Path $support 'install-wolfmqtt.ps1'
$board = Join-Path $support 'install-board-config.ps1'
$manifest = Get-Content (Join-Path $support 'dependencies.json') -Raw | ConvertFrom-Json
$root = Join-Path ([IO.Path]::GetTempPath()) ('sentinel-installers-test-' + [Guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $root | Out-Null

function Assert($Condition, [string] $Message) {
    if (-not $Condition) { throw "FAIL: $Message" }
    Write-Host "PASS: $Message"
}
function Snapshot([string] $Directory) {
    $records = @(Get-ChildItem -LiteralPath $Directory -File -Recurse -Force | Sort-Object FullName | ForEach-Object {
        '{0}|{1}|{2}' -f $_.FullName, (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash, $_.LastWriteTimeUtc.Ticks
    })
    return $records -join "`n"
}
function MustFail([scriptblock] $Action, [string] $Message) {
    $failed = $false
    try { & $Action } catch { $failed = $true }
    Assert $failed $Message
}
try {
    $book = Join-Path $root 'Sketchbook con espacios'
    & $wolf -SketchbookPath $book -ArchivePath $ArchivePath
    $library = Join-Path $book 'libraries\wolfMQTT'
    Assert (Test-Path (Join-Path $library 'mqtt_client.c')) 'Instalacion desde ZIP verificado'
    Assert ((Get-Content (Join-Path $library 'wolfmqtt\options.h') -Raw).Contains('#define WOLFMQTT_V5')) 'MQTT5 y configuracion personalizada'
    $before = Snapshot $book
    & $wolf -SketchbookPath $book -ArchivePath (Join-Path $root 'no-existe.zip')
    Assert ((Snapshot $book) -ceq $before) 'No-op sin red, archivo ZIP ni cambios de fechas'

    [IO.File]::WriteAllText((Join-Path $library 'resto-antiguo.c'), 'old')
    [IO.File]::WriteAllText((Join-Path $library 'mqtt_client.c'), 'altered')
    & $wolf -SketchbookPath $book -ArchivePath $ArchivePath
    Assert (-not (Test-Path (Join-Path $library 'resto-antiguo.c'))) 'Reparacion sin archivos mezclados'
    $backups = @(Get-ChildItem (Join-Path $book '.sentinel-backups') -Directory)
    Assert ($backups.Count -eq 1 -and (Test-Path (Join-Path $backups[0].FullName 'resto-antiguo.c'))) 'Respaldo completo fuera de libraries'
    $expected = $manifest.libraries[0].arduino_zip.files.PSObject.Properties['mqtt_client.c'].Value
    Assert ((Get-FileHash (Join-Path $library 'mqtt_client.c') -Algorithm SHA256).Hash -eq $expected) 'Fuentes originales restauradas'

    [IO.File]::WriteAllText((Join-Path $library 'resto-antiguo.c'), 'preserve')
    $invalid = Join-Path $root 'incorrecto.zip'
    [IO.File]::WriteAllText($invalid, 'incorrect archive')
    $before = Snapshot $book
    MustFail { & $wolf -SketchbookPath $book -ArchivePath $invalid } 'Hash incorrecto rechazado'
    Assert ((Snapshot $book) -ceq $before) 'Descarga incorrecta conserva instalacion y respaldos'

    # Un archivo de una biblioteca en uso no debe provocar una instalacion parcial.
    $libraryLock = [IO.File]::Open((Join-Path $library 'mqtt_client.c'), [IO.FileMode]::Open, [IO.FileAccess]::Read, [IO.FileShare]::Read)
    try { MustFail { & $wolf -SketchbookPath $book -ArchivePath $ArchivePath } 'Biblioteca en uso falla sin sustituirla' } finally { $libraryLock.Dispose() }
    Assert ((Snapshot $book) -ceq $before) 'Biblioteca bloqueada y respaldos conservados'

    $preview = Join-Path $root 'solo-preview'
    & $wolf -SketchbookPath $preview -WhatIf
    Assert (-not (Test-Path $preview)) 'WhatIf no crea destino ni descarga'

    $data = Join-Path $root 'ArduinoData con espacios'
    $core = Join-Path $data "packages\esp32\hardware\esp32\$($manifest.core)"
    New-Item -ItemType Directory -Path $core -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $core 'boards.txt'), 'existing core')
    $local = Join-Path $core 'boards.local.txt'
    $prefix = 'esp32s3.menu.PartitionScheme.sentinel'
    $desired = @(Get-Content (Join-Path $support 'boards.local.txt') | Where-Object { $_ -match '^[^#=]+=' })
    $desiredKeys = @($desired | ForEach-Object { ($_ -split '=', 2)[0] })
    $obsolete = @(
        "$prefix.tools.esptool_py.upload.pattern_args",
        "$prefix.recipe.hooks.objcopy.postobjcopy.3.pattern.windows",
        "$prefix.recipe.hooks.objcopy.postobjcopy.4.pattern.windows",
        "$prefix.recipe.hooks.objcopy.postobjcopy.5.pattern.windows",
        "$prefix.recipe.hooks.savehex.postsavehex.4.pattern.windows"
    )
    $foreign = @(
        '# comentario ajeno', 'other.option=keep=value',
        "$prefix.custom.option=keep-sentinel-extension",
        'esp32s3.menu.PartitionScheme.other.tools.esptool_py.upload.pattern_args=keep-upload',
        'recipe.hooks.objcopy.postobjcopy.3.pattern.windows=keep-global',
        "$prefix.recipe.hooks.savehex.postsavehex.3.pattern.windows=keep-neighbor",
        '# esp32s3.menu.PartitionScheme.sentinel.tools.esptool_py.upload.pattern_args=comment-kept'
    )
    $oldKeys = @($desiredKeys | ForEach-Object { $_ + '=old=value' }) + @($obsolete | ForEach-Object { "  $_ =stale=recipe" })
    $original = ($foreign + $oldKeys + "$prefix=duplicate" + "$($obsolete[0])=duplicate" -join "`r`n") + "`r`n"
    [IO.File]::WriteAllText($local, $original)
    $before = Snapshot $data
    & $board -ArduinoDataPath $data -WhatIf
    Assert ((Snapshot $data) -ceq $before) 'WhatIf de migracion 9->4 conserva archivo, fechas y respaldos'
    & $board -ArduinoDataPath $data
    $content = [IO.File]::ReadAllText($local)
    foreach ($line in $foreign) { Assert ($content.Contains($line + "`r`n")) "Linea ajena conservada: $line" }
    foreach ($line in $desired) {
        Assert ([regex]::Matches($content, '(?m)^' + [regex]::Escape($line) + '\r?$').Count -eq 1) "Clave aprobada unica: $line"
    }
    foreach ($key in $obsolete) {
        Assert (-not [regex]::IsMatch($content, '(?m)^\s*' + [regex]::Escape($key) + '\s*=')) "Receta obsoleta retirada: $key"
    }
    $managed = @($content -split '\r?\n' | Where-Object { $_ -match '^[^#=]+=' -and $desiredKeys -ccontains ($_ -split '=', 2)[0] })
    Assert ($managed.Count -eq 4) 'Migracion nueve a cuatro claves propias sin duplicados'
    $saved = @(Get-ChildItem -LiteralPath $core -Filter '*.bak')
    Assert ($saved.Count -eq 1 -and [IO.File]::ReadAllText($saved[0].FullName) -ceq $original) 'Respaldo exacto de boards.local'
    $before = Snapshot $data
    & $board -ArduinoDataPath $data
    Assert ((Snapshot $data) -ceq $before) 'Configuracion repetida sin escrituras ni respaldo adicional'

    # Cuatro claves correctas no deben ocultar una receta antigua agregada despues.
    [IO.File]::AppendAllText($local, "$($obsolete[4])=stale-again`r`n")
    $beforeCleanup = [IO.File]::ReadAllText($local)
    & $board -ArduinoDataPath $data
    Assert (-not [IO.File]::ReadAllText($local).Contains("$($obsolete[4])=stale-again")) 'Retira receta obsoleta aunque las cuatro claves ya sean correctas'
    $saved = @(Get-ChildItem -LiteralPath $core -Filter '*.bak')
    Assert ($saved.Count -eq 2 -and @($saved | Where-Object { [IO.File]::ReadAllText($_.FullName) -ceq $beforeCleanup }).Count -eq 1) 'Limpieza adicional guarda respaldo exacto'
    $before = Snapshot $data
    & $board -ArduinoDataPath $data
    Assert ((Snapshot $data) -ceq $before) 'Limpieza repetida vuelve a ser no-op'

    $emptyData = Join-Path $root 'sin-core'
    MustFail { & $board -ArduinoDataPath $emptyData } 'Core ausente rechazado'
    Assert (-not (Test-Path $emptyData)) 'No crea una instalacion parcial de Arduino'

    $freshData = Join-Path $root 'core-sin-opciones'
    $freshCore = Join-Path $freshData "packages\esp32\hardware\esp32\$($manifest.core)"
    New-Item -ItemType Directory -Path $freshCore -Force | Out-Null
    [IO.File]::WriteAllText((Join-Path $freshCore 'boards.txt'), 'existing core')
    & $board -ArduinoDataPath $freshData -WhatIf
    Assert (-not (Test-Path (Join-Path $freshCore 'boards.local.txt'))) 'WhatIf de configuracion no escribe'
    & $board -ArduinoDataPath $freshData
    Assert (Test-Path (Join-Path $freshCore 'boards.local.txt')) 'Configuracion creada cuando no existia'

    $casePath = Join-Path $freshCore 'boards.local.txt'
    $wrongCase = foreach ($line in (Get-Content $casePath)) {
        if ($line -match '^([^#=]+)=(.*)$') { $matches[1].ToUpperInvariant() + '=' + $matches[2] } else { $line }
    }
    [IO.File]::WriteAllText($casePath, ($wrongCase -join "`n"))
    & $board -ArduinoDataPath $freshData
    $caseContent = [IO.File]::ReadAllText($casePath)
    Assert ([regex]::Matches($caseContent, '(?m)^esp32s3\.menu\.PartitionScheme\.sentinel[^=]*=').Count -eq 4) 'Claves con mayusculas incorrectas no se consideran validas'
    foreach ($line in $wrongCase) { Assert ($caseContent.Contains($line)) 'Mayusculas ajenas conservadas sin sobrescritura' }

    # Un bloqueo real de Windows debe dejar el archivo previo intacto.
    [IO.File]::WriteAllText($local, 'other.option=locked')
    $lock = [IO.File]::Open($local, [IO.FileMode]::Open, [IO.FileAccess]::ReadWrite, [IO.FileShare]::Read)
    try { MustFail { & $board -ArduinoDataPath $data } 'Archivo bloqueado falla sin sobrescribir' } finally { $lock.Dispose() }
    Assert ([IO.File]::ReadAllText($local) -ceq 'other.option=locked') 'Configuracion bloqueada conservada'

    if ($TestDownload) {
        & $wolf -SketchbookPath (Join-Path $root 'descarga-real')
        Assert (Test-Path (Join-Path $root 'descarga-real\libraries\wolfMQTT\mqtt_client.c')) 'Descarga HTTPS real instalada y verificada'
    }
    Write-Host 'Instaladores PowerShell: PASS (solo destinos temporales; sin placa ni cambios al core real).'
} finally { Remove-Item -LiteralPath $root -Recurse -Force }

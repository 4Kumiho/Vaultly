# Scarica le dipendenze che non stanno nel repository (cartella third_party/, ignorata da git).
# Da lanciare una volta dopo il clone:  powershell -File scripts\fetch-deps.ps1

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$thirdParty = Join-Path $root 'third_party'

# WinSparkle: aggiornamenti automatici. Versione e hash fissati: se cambiano, si aggiornano qui.
$winSparkleVersion = '0.9.4'
$winSparkleSha256 = '6037DF37FC263BD1650A1C4949681A9D40FFE991D01F35892A406CB5D103C976'
$winSparkleDir = Join-Path $thirdParty "WinSparkle-$winSparkleVersion"

if (Test-Path $winSparkleDir) {
    Write-Host "WinSparkle $winSparkleVersion presente, niente da scaricare."
    return
}

New-Item -ItemType Directory -Force $thirdParty | Out-Null
$zip = Join-Path $thirdParty "WinSparkle-$winSparkleVersion.zip"
$url = "https://github.com/vslavik/winsparkle/releases/download/v$winSparkleVersion/WinSparkle-$winSparkleVersion.zip"
Write-Host "Scarico $url"
Invoke-WebRequest $url -OutFile $zip

$hash = (Get-FileHash $zip -Algorithm SHA256).Hash
if ($hash -ne $winSparkleSha256) {
    Remove-Item $zip
    throw "Hash di WinSparkle non valido ($hash): download corrotto o manomesso."
}
Expand-Archive $zip -DestinationPath $thirdParty -Force
Remove-Item $zip
Write-Host "WinSparkle $winSparkleVersion pronto in $winSparkleDir"

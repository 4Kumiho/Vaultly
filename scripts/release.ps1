# Prepara una release di Vaultly:
#   1. compila in Release con l'URL degli aggiornamenti ed esegue i test
#   2. raccoglie exe + DLL Qt + WinSparkle in dist/
#   3. crea l'installer con Inno Setup
#   4. firma l'installer con la chiave privata EdDSA e verifica la firma
#   5. scrive appcast.xml (il file che le app installate leggono per sapere se c'e' una nuova versione)
#   6. con -Publish, crea la release su GitHub con installer e appcast
#
# Uso:
#   1. alza la versione in CMakeLists.txt:  project(Vaultly VERSION 1.1.0 ...)
#   2. powershell -File scripts\release.ps1 -Notes "Novita' 1`nNovita' 2" [-Publish]
#
# Nota: il file e' volutamente solo ASCII (PowerShell 5.1 legge gli script senza BOM come ANSI).

param(
    [string]$Notes = '',
    [switch]$Publish,
    [string]$Repo = ''   # sostituisce githubRepo di release.json (utile per prove)
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
Set-Location $root

# --- Configurazione ---
$config = Get-Content (Join-Path $PSScriptRoot 'release.json') -Raw | ConvertFrom-Json
$repo = if ($Repo) { $Repo } else { $config.githubRepo }
if (-not $repo -or $repo -like '*CAMBIAMI*') { throw "Imposta githubRepo in scripts\release.json (es. 'mionome/Vaultly')." }

$signingKey = Join-Path $env:USERPROFILE '.vaultly\update-signing.key'
$publicKey = 'YgjC4rRMQjdPz1CfZzWaX7JYjNnO7yXSm7nfsL0MNZ8='
$qtDir = 'C:\Qt\6.10.3\mingw_64'
$winSparkleDir = Join-Path $root 'third_party\WinSparkle-0.9.4'
$winSparkleTool = Join-Path $winSparkleDir 'bin\winsparkle-tool.exe'
$iscc = Join-Path $env:LOCALAPPDATA 'Programs\Inno Setup 6\ISCC.exe'

foreach ($required in @($signingKey, $winSparkleTool, $iscc)) {
    if (-not (Test-Path $required)) { throw "Manca $required (vedi CLAUDE.md, sezione Release)." }
}

$env:Path = [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';' + [Environment]::GetEnvironmentVariable('Path', 'User')
$env:CMAKE_PREFIX_PATH = $qtDir

function Invoke-Checked([string]$what, [scriptblock]$command) {
    & $command
    if ($LASTEXITCODE -ne 0) { throw "$what fallito (exit $LASTEXITCODE)." }
}

# --- Versione ---
$cmake = Get-Content (Join-Path $root 'CMakeLists.txt') -Raw
if ($cmake -notmatch 'project\(Vaultly VERSION (\d+\.\d+\.\d+)') { throw 'Versione non trovata in CMakeLists.txt.' }
$version = $Matches[1]
$tag = "v$version"
Write-Host "== Vaultly $version ==" -ForegroundColor Cyan

$status = git status --porcelain
if ($status) { Write-Warning 'Ci sono modifiche non committate: la release non corrispondera'' a un commit preciso.' }

# --- 1. Build e test ---
$buildDir = Join-Path $root 'build-release'
Invoke-Checked 'Configurazione CMake' { cmake -S $root -B $buildDir -G Ninja -DCMAKE_BUILD_TYPE=Release "-DVAULTLY_GITHUB_REPO=$repo" }
Invoke-Checked 'Build' { cmake --build $buildDir }
Invoke-Checked 'Test' { ctest --test-dir $buildDir --output-on-failure }

# --- 2. Cartella da installare ---
$dist = Join-Path $root 'dist'
if (Test-Path $dist) { Remove-Item $dist -Recurse -Force }
New-Item -ItemType Directory $dist | Out-Null
Copy-Item (Join-Path $buildDir 'Vaultly.exe') $dist
Copy-Item (Join-Path $winSparkleDir 'x64\Release\WinSparkle.dll') $dist
Invoke-Checked 'windeployqt' { windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw (Join-Path $dist 'Vaultly.exe') | Out-Null }

# --- 3. Installer ---
$out = Join-Path $root "release\$version"
if (Test-Path $out) { Remove-Item $out -Recurse -Force }
New-Item -ItemType Directory $out | Out-Null
Invoke-Checked 'Inno Setup' { & $iscc /Q "/DAppVersion=$version" "/DSourceDir=$dist" "/DOutputDir=$out" (Join-Path $root 'installer\Vaultly.iss') }
$setupName = "Vaultly-Setup-$version.exe"
$setup = Join-Path $out $setupName

# --- 4. Firma ---
$signLine = & $winSparkleTool sign --verbose --private-key-file $signingKey $setup
if ($LASTEXITCODE -ne 0 -or "$signLine" -notmatch 'sparkle:edSignature="([^"]+)" length="(\d+)"') { throw "Firma fallita: $signLine" }
$signature = $Matches[1]
$length = $Matches[2]
Invoke-Checked 'Verifica firma' { & $winSparkleTool verify --public-key $publicKey --signature $signature $setup | Out-Null }

# --- 5. Appcast ---
$downloadUrl = "https://github.com/$repo/releases/download/$tag/$setupName"
$items = ($Notes -split "`n" | Where-Object { $_.Trim() } | ForEach-Object { '<li>' + [Security.SecurityElement]::Escape($_.Trim()) + '</li>' }) -join ''
$description = if ($items) { "<ul>$items</ul>" } else { "<p>Versione $version</p>" }
$pubDate = (Get-Date).ToUniversalTime().ToString('r')
$appcast = @"
<?xml version="1.0" encoding="utf-8"?>
<rss version="2.0" xmlns:sparkle="http://www.andymatuschak.org/xml-namespaces/sparkle">
  <channel>
    <title>Vaultly</title>
    <item>
      <title>Vaultly $version</title>
      <pubDate>$pubDate</pubDate>
      <description><![CDATA[$description]]></description>
      <enclosure url="$downloadUrl"
                 sparkle:version="$version"
                 sparkle:os="windows-x64"
                 sparkle:installerArguments="/SILENT /SP- /NOCANCEL"
                 sparkle:edSignature="$signature"
                 length="$length"
                 type="application/octet-stream" />
    </item>
  </channel>
</rss>
"@
$appcastPath = Join-Path $out 'appcast.xml'
[IO.File]::WriteAllText($appcastPath, $appcast, (New-Object Text.UTF8Encoding $false))

Write-Host ''
Write-Host "Pronti in $out :" -ForegroundColor Green
Write-Host "  $setupName  ($length byte, firmato)"
Write-Host '  appcast.xml'

# --- 6. Pubblicazione ---
if (-not $Publish) {
    Write-Host ''
    Write-Host "Per pubblicare: rilancia con -Publish, oppure crea a mano la release '$tag' su"
    Write-Host "https://github.com/$repo/releases/new e allega ENTRAMBI i file."
    return
}
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) { throw 'Serve GitHub CLI (winget install GitHub.cli, poi: gh auth login).' }
$notesText = if ($Notes) { $Notes } else { "Versione $version" }
Invoke-Checked 'Tag git' { git tag -a $tag -m "Vaultly $version" }
Invoke-Checked 'Push del tag' { git push origin $tag }
Invoke-Checked 'Release GitHub' { gh release create $tag $setup $appcastPath --repo $repo --title "Vaultly $version" --notes $notesText }
Write-Host "Pubblicata: https://github.com/$repo/releases/tag/$tag" -ForegroundColor Green
Write-Host 'Le app installate la troveranno al prossimo controllo (all''avvio o entro 24 ore).'

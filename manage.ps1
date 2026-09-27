param(
    [Parameter(Mandatory=$true)][ValidateSet('install','uninstall')][string]$Action,
    [Parameter(Mandatory=$true)][string]$GameDir
)
$ErrorActionPreference = 'Stop'
$expectedExe = '2210CA88E410746AC3A898EDD38A4520F6CDA315F61380C0077EDCAE2F47DDAF'
$expectedOriginal = 'A23D944BEA101C574875C13883088798CFDA712DE969DD14F529E870A0DE87DA'
$expectedMod = '09A589524C34D06FBAFE5C7A7AD4F1249C91C2789B09E447AAD7A6C9F827FF50'
function Assert-Hash([string]$Path,[string]$Expected) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Missing: $Path" }
    $stream = [IO.File]::OpenRead($Path)
    $sha = [Security.Cryptography.SHA256]::Create()
    try { $actual = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '') }
    finally { $stream.Dispose(); $sha.Dispose() }
    if ($actual -ne $Expected) { throw "Unexpected file: $Path (SHA-256 $actual)" }
}
try {
    $dir = (Resolve-Path -LiteralPath $GameDir).Path
    $exe = Join-Path $dir 'unmetal.exe'
    $active = Join-Path $dir 'SDL2.dll'
    $backup = Join-Path $dir 'SDL2_orig.dll'
    $payload = Join-Path $dir 'UnMetalMod.dll'
    Assert-Hash $exe $expectedExe
    if (Get-Process -Name unmetal -ErrorAction SilentlyContinue) { throw 'Close UnMetal before changing DLLs.' }
    if ($Action -eq 'install') {
        Assert-Hash $payload $expectedMod
        # Migrate this Mod's earlier Steam proxy, only when both files match.
        $steam = Join-Path $dir 'steam_api.dll'
        $steamBackup = Join-Path $dir 'steam_api_orig.dll'
        if (Test-Path -LiteralPath $steamBackup) {
            Assert-Hash $steamBackup '4C83D75FCAA6166BE46F0BF3C0ACDF6B5FFEADFF673AB64A7CDEAC8FF3F21AC1'
            Assert-Hash $steam '8EED0FAEBE869D0987823D2733A64F4064C271032FDD482AFF758DF75DFAD363'
            Copy-Item -LiteralPath $steamBackup -Destination $steam -Force
            Assert-Hash $steam '4C83D75FCAA6166BE46F0BF3C0ACDF6B5FFEADFF673AB64A7CDEAC8FF3F21AC1'
            Remove-Item -LiteralPath $steamBackup
            Write-Output 'Previous Steam proxy removed; original Steam DLL restored.'
        }

        if (Test-Path -LiteralPath $backup) {
            Assert-Hash $backup $expectedOriginal
            Assert-Hash $active $expectedMod
            Write-Output 'Mod already installed.'
        } else {
            Assert-Hash $active $expectedOriginal
            Move-Item -LiteralPath $active -Destination $backup
            try {
                Copy-Item -LiteralPath $payload -Destination $active
                Assert-Hash $active $expectedMod
            } catch {
                if (Test-Path -LiteralPath $active) { Remove-Item -LiteralPath $active }
                Move-Item -LiteralPath $backup -Destination $active
                throw
            }
            Write-Output 'Mod installed. Original SDL2 DLL kept as SDL2_orig.dll.'
        }
    } else {
        Assert-Hash $backup $expectedOriginal
        Assert-Hash $active $expectedMod
        Remove-Item -LiteralPath $active
        Copy-Item -LiteralPath $backup -Destination $active
        Assert-Hash $active $expectedOriginal
        Remove-Item -LiteralPath $backup
        Write-Output 'Original SDL2 DLL restored.'
    }
} catch {
    Write-Error $_
    exit 1
}

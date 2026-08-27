param(
    [string] $GccPath = ""
)

$ErrorActionPreference = "Stop"

if ($GccPath -eq "") {
    if ($env:CC) {
        $GccPath = $env:CC
    } else {
        $command = Get-Command gcc -ErrorAction SilentlyContinue
        if ($null -ne $command) {
            $GccPath = $command.Source
        }
    }
}
if ($GccPath -eq "" -or !(Test-Path -LiteralPath $GccPath)) {
    throw "GCC not found. Pass -GccPath or set CC."
}

$toolchainBin = Split-Path -Parent (Resolve-Path -LiteralPath $GccPath)
$env:PATH = "$toolchainBin;$env:PATH"

$outDir = Join-Path $PSScriptRoot "out"
$testExe = Join-Path $outDir "async_input_core_tests.exe"
New-Item -ItemType Directory -Force -Path $outDir | Out-Null

& $GccPath `
  -std=c11 `
  -O2 `
  -Wall `
  -Wextra `
  -Werror `
  -o $testExe `
  (Join-Path $PSScriptRoot "async_input_core_tests.c")
if ($LASTEXITCODE -ne 0) {
    throw "async input core tests failed to compile"
}

& $testExe
if ($LASTEXITCODE -ne 0) {
    throw "async input core tests failed"
}

param(
    [string]$QtRoot = "",
    [string]$MinGwBin = "",
    [string]$CMakeExe = "",
    [string]$BuildType = "Release",
    [string]$InstallPrefix = "dist",
    [string]$PythonExe = "",
    [switch]$Clean
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Resolve-CMakeExe {
    param([string]$Hint)
    if ($Hint -and (Test-Path $Hint)) { return $Hint }
    $candidates = @(
        "C:\Qt\Tools\CMake_64\bin\cmake.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2019\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe",
        "cmake"
    )
    foreach ($c in $candidates) {
        if ($c -eq "cmake") {
            try { $null = & $c --version 2>$null; return $c } catch { }
        } else {
            if (Test-Path $c) {
                try { $null = & $c --version 2>$null; return $c } catch { }
            }
        }
    }
    throw "CMake not found. Specify path via -CMakeExe"
}

function Resolve-PythonExe {
    param([string]$Hint)
    if ($Hint -and (Test-Path $Hint)) { return (Resolve-Path $Hint).Path }
    $repoRoot = Split-Path -Parent $PSScriptRoot
    # Сначала проверяем Python из ide/python
    $idePy = Join-Path $repoRoot "python\python.exe"
    if (Test-Path $idePy) { return (Resolve-Path $idePy).Path }
    # Затем проверяем thonny-master\build\python.exe
    $thonnyPy = Join-Path $repoRoot "thonny-master\build\python.exe"
    if (Test-Path $thonnyPy) { return (Resolve-Path $thonnyPy).Path }
    # В последнюю очередь системный Python
    try {
        $sysPy = (Get-Command python.exe -ErrorAction Stop).Source
        return $sysPy
    } catch {
        throw "Python not found. Specify path via -PythonExe"
    }
}

function Ensure-Dir {
    param([string]$Path)
    if (-not (Test-Path $Path)) { New-Item -ItemType Directory -Path $Path | Out-Null }
}

function Resolve-QtRoot {
    param([string]$Hint)
    if ($Hint -and (Test-Path $Hint)) { return (Resolve-Path $Hint).Path }
    if ($env:QTDIR -and (Test-Path $env:QTDIR)) { return (Resolve-Path $env:QTDIR).Path }
    $hits = Get-ChildItem -Path 'C:\Qt' -Recurse -ErrorAction SilentlyContinue -Filter windeployqt.exe |
        Where-Object { $_.FullName -match 'mingw.*_64\\bin' } |
        Select-Object -ExpandProperty FullName
    foreach ($h in $hits) {
        $bin = Split-Path -Parent $h
        $root = Split-Path -Parent $bin
        return $root
    }
    throw "QtRoot (64-bit) not found. Specify path via -QtRoot"
}

function Resolve-MinGwBin {
    param([string]$Hint)
    if ($Hint -and (Test-Path $Hint)) { return (Resolve-Path $Hint).Path }
    if ($env:MINGW_BIN -and (Test-Path $env:MINGW_BIN)) { return (Resolve-Path $env:MINGW_BIN).Path }
    $hits = Get-ChildItem -Path 'C:\Qt' -Recurse -ErrorAction SilentlyContinue -Filter g++.exe |
        Where-Object { $_.FullName -match 'mingw.*_64\\bin' } |
        Select-Object -ExpandProperty DirectoryName
    foreach ($d in $hits) { if (Test-Path $d) { return (Resolve-Path $d).Path } }
    throw "MinGwBin (64-bit) not found. Specify path via -MinGwBin"
}

Push-Location $PSScriptRoot
try {
    if ($Clean) {
        if (Test-Path build-mingw) { Remove-Item -Recurse -Force build-mingw }
        if (Test-Path $InstallPrefix) { Remove-Item -Recurse -Force $InstallPrefix }
    }

    $cmake = Resolve-CMakeExe -Hint $CMakeExe
    $qtResolved = Resolve-QtRoot -Hint $QtRoot
    $mingwResolved = Resolve-MinGwBin -Hint $MinGwBin
    $windeploy = Join-Path $qtResolved "bin\windeployqt.exe"
    $python = Resolve-PythonExe -Hint $PythonExe
    Write-Host "Using Python: $python" -ForegroundColor Cyan

    # PATH: put 64-bit MinGW/Qt at the beginning, remove other MinGW/Qt from PATH
    $filteredPath = ($env:PATH -split ';' | Where-Object { $_ -and ($_ -notmatch 'mingw.*\\bin') -and ($_ -notmatch '\\Qt\\.*\\bin') }) -join ';'
    $env:PATH = "$mingwResolved;$qtResolved\bin;$filteredPath"

    # Convert paths to forward slashes for CMake (needed for correct CMake handling of paths with backslashes)
    $qtPathForCmake = $qtResolved -replace '\\', '/'
    $mingwBinForCmake = $mingwResolved -replace '\\', '/'
    $gccForCmake = "$mingwBinForCmake/gcc.exe"
    $gppForCmake = "$mingwBinForCmake/g++.exe"
    $makeForCmake = "$mingwBinForCmake/mingw32-make.exe"
    $pythonForCmake = $python -replace '\\', '/'

    # Configure (temporarily change ErrorActionPreference to handle CMake warnings)
    $oldErrorAction = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $cmakeOutput = & $cmake -S . -B build-mingw -G "MinGW Makefiles" `
            -DCMAKE_PREFIX_PATH="$qtPathForCmake" `
            -DCMAKE_BUILD_TYPE=$BuildType `
            -DPython3_EXECUTABLE="$pythonForCmake" `
            -DCMAKE_C_COMPILER="$gccForCmake" `
            -DCMAKE_CXX_COMPILER="$gppForCmake" `
            -DCMAKE_MAKE_PROGRAM="$makeForCmake" 2>&1
        
        # Check configuration result
        if ($LASTEXITCODE -ne 0) {
            $cmakeOutput | Write-Host
            throw "CMake configure failed with error (code: $LASTEXITCODE)"
        }
        # Show only important messages
        $cmakeOutput | Where-Object { $_ -and ($_ -notmatch "^--") } | Write-Host
    } finally {
        $ErrorActionPreference = $oldErrorAction
    }

    # Build
    & $cmake --build build-mingw --parallel

    # Install
    Ensure-Dir $InstallPrefix
    $dist = Join-Path $PSScriptRoot $InstallPrefix
    $exe = Join-Path $dist 'Vuzhyk.exe'
    $p = Get-Process -Name Vuzhyk -ErrorAction SilentlyContinue
    if ($p) { $p | Stop-Process -Force -ErrorAction SilentlyContinue }
    if (Test-Path $exe) { Remove-Item -Force $exe -ErrorAction SilentlyContinue }
    & $cmake --install build-mingw --prefix $InstallPrefix

    # Deploy Qt runtime (64-bit)
    if (Test-Path $windeploy) {
        & $windeploy --release --compiler-runtime --dir $dist $exe
    } else {
        Write-Warning "windeployqt not found at path: $windeploy"
    }

    Write-Host "`nBuild (x64) completed. Run: $exe" -ForegroundColor Green
}
finally {
    Pop-Location
}



$ErrorActionPreference = "Stop"

# Install Intel MPI on a Windows GitHub Actions runner

# Go to https://www.intel.com/content/www/us/en/developer/tools/oneapi/mpi-library-download.html to get the latest version of Intel MPI
# Update the $downloadUrl variable with the new version if needed
$downloadUrl = "https://registrationcenter-download.intel.com/akdlm/IRC_NAS/c4bfa0bf-a209-4049-921c-8c6aa89ec6be/intel-mpi-2021.18.1.800_offline.exe"
$downloadUri = [Uri]$downloadUrl
$packageName = [System.IO.Path]::GetFileName($downloadUri.AbsolutePath)
$packageMatch = [regex]::Match(
    $packageName,
    "^intel-mpi-(?<version>\d+\.\d+\.\d+)\.(?<build>\d+)_offline\.exe$"
)
if (-not $packageMatch.Success) {
    throw "Intel MPI download URL has an unexpected package name: $packageName."
}
$version = $packageMatch.Groups["version"].Value
$build = $packageMatch.Groups["build"].Value
$installerPath = Join-Path $env:RUNNER_TEMP $packageName

Write-Host "::group::Download Intel MPI $version"
Invoke-WebRequest -Uri $downloadUrl -OutFile $installerPath -UseBasicParsing
Write-Host "::endgroup::"

Write-Host "::group::Install Intel MPI $version"
$installer = Start-Process -FilePath $installerPath -ArgumentList @(
    "-s",
    "-a",
    "--silent",
    "--eula",
    "accept"
) -Wait -PassThru
if ($installer.ExitCode -ne 0) {
    throw "Intel MPI installer failed with exit code $($installer.ExitCode)."
}
Write-Host "::endgroup::"

$oneApiRoot = Join-Path ${env:ProgramFiles(x86)} "Intel\oneAPI"
$mpiRoot = Join-Path $oneApiRoot "mpi\latest"
$mpiBin = Join-Path $mpiRoot "bin"
$ofiBin = Join-Path $mpiRoot "opt\mpi\libfabric\bin"

if (-not (Test-Path -LiteralPath $mpiBin)) {
    throw "Intel MPI was not installed at $mpiRoot."
}

$env:ONEAPI_ROOT = $oneApiRoot
$env:I_MPI_ROOT = $mpiRoot
$env:I_MPI_OFI_LIBRARY_INTERNAL = "1"
$env:Path = "$mpiBin;$ofiBin;$env:Path"

@(
    "ONEAPI_ROOT=$oneApiRoot"
    "I_MPI_ROOT=$mpiRoot"
    "I_MPI_OFI_LIBRARY_INTERNAL=1"
) | Out-File -FilePath $env:GITHUB_ENV -Encoding utf8 -Append

@(
    $mpiBin
    $ofiBin
) | Out-File -FilePath $env:GITHUB_PATH -Encoding utf8 -Append

Write-Host "::group::Configure Intel MPI service"
& (Join-Path $mpiBin "hydra_service.exe") -install
if ($LASTEXITCODE -ne 0) {
    throw "Intel MPI hydra service installation failed with exit code $LASTEXITCODE."
}
Write-Host "::endgroup::"

Write-Host "::group::Verify Intel MPI"
& (Join-Path $mpiBin "impi_info.exe")
if ($LASTEXITCODE -ne 0) {
    throw "Intel MPI verification failed with exit code $LASTEXITCODE."
}
Write-Host "Intel MPI verification passed."
Write-Host "::endgroup::"

Remove-Item -LiteralPath $installerPath -Force
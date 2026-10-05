param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Release',
    [Parameter(Mandatory)][string] $OutputDirectory,
    [ValidateRange(30, 600)][int] $TimeoutSeconds = 180
)
. "$PSScriptRoot/Development.ps1"
$captureRoot = [System.IO.Path]::GetFullPath((Join-Path $script:RepoRoot $OutputDirectory))
[System.IO.Directory]::CreateDirectory($captureRoot) | Out-Null
$process = Start-Process -FilePath (Get-WodExecutable Client $Configuration) -ArgumentList @('--capture-monsters', ('"' + $captureRoot + '"')) -WorkingDirectory (Join-Path $script:RepoRoot 'Client/WarOfDimension') -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (-not $process.WaitForExit(1000)) {
        if ([DateTime]::UtcNow -gt $deadline) { throw '몬스터 촬영 시간 초과' }
    }
    if ($process.ExitCode -ne 0) {
        if (Test-Path "$captureRoot/error.txt") { Get-Content "$captureRoot/error.txt" -Encoding utf8 | Write-Host }
        throw "몬스터 촬영 실패: $captureRoot"
    }
    $capture = Get-Content "$captureRoot/capture.json" -Raw | ConvertFrom-Json
    $memory = Get-Content "$captureRoot/memory.json" -Raw | ConvertFrom-Json
    $lifetime = Get-Content "$captureRoot/lifetime.json" -Raw | ConvertFrom-Json
    if ($capture.views.Count -ne 18 -or -not $memory.ok -or $capture.npcUploadsAfterRelease -ne 0 -or $lifetime.objectConstantPagesAfterOnDestroy -ne 0) { throw '몬스터 캡처/자원 수명 검사 미완료' }
    foreach ($view in $capture.views) {
        if ((Get-Item (Join-Path $captureRoot $view.file)).Length -le 10000) { throw "빈 모델 화면: $($view.file)" }
    }
    if ($Configuration -eq 'Debug') {
        foreach ($phase in @('capture-end', 'ui-release')) {
            $gpu = Get-Content "$captureRoot/gpu-$phase.json" -Raw | ConvertFrom-Json
            if ($gpu.errors -ne 0 -or $gpu.removedReason -ne 0) { throw 'GPU 렌더링/정상 종료 오류' }
        }
    }
    [ordered]@{ executableSha256=(Get-FileHash (Get-WodExecutable Client $Configuration) -Algorithm SHA256).Hash.ToLowerInvariant(); configuration=$Configuration; capturedAt=[DateTime]::UtcNow.ToString('o') } | ConvertTo-Json | Set-Content "$captureRoot/run.json" -Encoding utf8
    Write-Host "몬스터 9종 앞/뒤 촬영 PASS ($Configuration): DDS $($capture.npcTextureResources)개, arena 종료 잔류 0"
} finally {
    if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    $process.Dispose()
}

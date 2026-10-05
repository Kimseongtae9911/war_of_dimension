param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Release',
    [ValidateSet('player', 'boss')][string] $Role = 'player',
    [Parameter(Mandatory)][string] $OutputDirectory,
    [ValidateRange(30, 600)][int] $TimeoutSeconds = 240
)
. "$PSScriptRoot/Development.ps1"
$captureRoot = [System.IO.Path]::GetFullPath((Join-Path $script:RepoRoot $OutputDirectory))
[System.IO.Directory]::CreateDirectory($captureRoot) | Out-Null
$executable = Get-WodExecutable Client $Configuration
$process = Start-Process -FilePath $executable -ArgumentList @('--capture-ui', ('"' + $captureRoot + '"'), $Role) -WorkingDirectory (Join-Path $script:RepoRoot 'Client/WarOfDimension') -WindowStyle Hidden -PassThru
try {
    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while (-not $process.WaitForExit(1000)) {
        if ([DateTime]::UtcNow -gt $deadline) { throw 'UI 촬영 시간 초과' }
    }
    if ($process.ExitCode -ne 0) {
        if (Test-Path "$captureRoot/error.txt") { Get-Content "$captureRoot/error.txt" -Encoding utf8 | Write-Host }
        throw "UI 촬영 실패: $captureRoot"
    }
    $capture = Get-Content "$captureRoot/capture.json" -Raw | ConvertFrom-Json
    $memory = Get-Content "$captureRoot/memory.json" -Raw | ConvertFrom-Json
    $allocations = Get-Content "$captureRoot/allocations.json" -Raw | ConvertFrom-Json
    $lifetime = Get-Content "$captureRoot/ui-lifetime.json" -Raw | ConvertFrom-Json
    if ($capture.views.Count -ne 16 -or -not $memory.ok -or $allocations.uniqueTextures -ne 20 -or
        $allocations.layoutFailureCases -ne 9 -or $lifetime.checkedResources -ne 40 -or
        $lifetime.resourcesRetainedAfterOnDestroy -ne 0) { throw 'UI 생성/표시/할당량/수명 검사 미완료' }
    foreach ($view in $capture.views) {
        if ((Get-Item (Join-Path $captureRoot $view.file)).Length -le 10000) { throw "빈 UI 화면: $($view.file)" }
    }
    if ($Configuration -eq 'Debug') {
        foreach ($phase in @('initialization', 'ui-capture-end', 'ui-release')) {
            $gpu = Get-Content "$captureRoot/gpu-$phase.json" -Raw | ConvertFrom-Json
            if ($gpu.errors -ne 0 -or $gpu.removedReason -ne 0) { throw "GPU UI 오류: $phase" }
        }
    }
    [ordered]@{ executableSha256=(Get-FileHash $executable -Algorithm SHA256).Hash.ToLowerInvariant();
        role=$Role; configuration=$Configuration; capturedAtUtc=[DateTime]::UtcNow.ToString('o') } |
        ConvertTo-Json | Set-Content "$captureRoot/run.json" -Encoding utf8
    Write-Host "UI $Role 16화면/20 texture PASS ($Configuration), 종료 DEFAULT/UPLOAD 잔류 0"
} finally {
    if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
    $process.Dispose()
}

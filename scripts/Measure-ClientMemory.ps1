param(
    [ValidateSet('Debug', 'Release')][string] $Configuration = 'Release',
    [ValidateRange(1, 10)][int] $Runs = 3,
    [ValidateRange(30, 600)][int] $TimeoutSeconds = 180,
    [ValidateSet('legacy', 'shared', 'full', 'selected')][string[]] $Modes = @('legacy', 'shared'),
    [ValidateSet('Geometry', 'HeroParts')][string] $Scenario = 'Geometry',
    [string] $OutputDirectory = 'artifacts/logs/client-memory'
)
. "$PSScriptRoot/Development.ps1"
if ($Scenario -eq 'HeroParts' -and -not $PSBoundParameters.ContainsKey('Modes')) { $Modes = @('full', 'selected') }
$allowedModes = if ($Scenario -eq 'HeroParts') { @('full', 'selected') } else { @('legacy', 'shared') }
if (@($Modes | Where-Object { $_ -notin $allowedModes }).Count) { throw 'Scenario와 Modes가 일치하지 않습니다.' }
$entryPoint = if ($Scenario -eq 'HeroParts') { '--profile-hero-memory' } else { '--profile-client-memory' }
$executable = Get-WodExecutable Client $Configuration
if (-not (Test-Path -LiteralPath $executable)) { throw '먼저 클라이언트를 빌드하세요.' }
$logRoot = Join-Path $script:RepoRoot $OutputDirectory
[System.IO.Directory]::CreateDirectory($logRoot) | Out-Null
$results = @()
for ($run = 1; $run -le $Runs; ++$run) {
    # 순서에 따른 driver cache 영향을 줄이기 위해 시작 모드를 교대한다.
    $runModes = @($Modes | Select-Object -Unique)
    if ($run % 2 -eq 0) { [array]::Reverse($runModes) }
    foreach ($mode in $runModes) {
        $report = Join-Path $logRoot "$mode-$Configuration-$run.json"
        $process = Start-Process -FilePath $executable -ArgumentList @($entryPoint, ('"' + $report + '"'), $mode) -WorkingDirectory (Join-Path $script:RepoRoot 'Client/WarOfDimension') -WindowStyle Hidden -PassThru
        $started = [DateTime]::UtcNow
        [long] $peakPrivate = 0
        [long] $peakWorkingSet = 0
        try {
            while (-not $process.HasExited) {
                $process.Refresh()
                if ($process.HasExited) { break }
                $peakPrivate = [Math]::Max($peakPrivate, $process.PrivateMemorySize64)
                $peakWorkingSet = [Math]::Max($peakWorkingSet, $process.WorkingSet64)
                if (([DateTime]::UtcNow - $started).TotalSeconds -gt $TimeoutSeconds) { throw "측정 시간 초과: $mode" }
                Start-Sleep -Milliseconds 50
            }
            $process.WaitForExit()
            if ($process.ExitCode -ne 0) { throw "측정 프로세스 실패: $mode (exit $($process.ExitCode)). 마지막 checkpoint: $report" }
            $data = Get-Content -LiteralPath $report -Raw | ConvertFrom-Json
            if (-not $data.ok -or $data.snapshots[-1].phase -ne 'ingame_ready') { throw "인게임 진입 측정 미완료: $report" }
            $results += [ordered]@{ run=$run; mode=$mode; elapsedSeconds=([DateTime]::UtcNow - $started).TotalSeconds; sampledPeakPrivateBytes=$peakPrivate; sampledPeakWorkingSetBytes=$peakWorkingSet; report=(Split-Path $report -Leaf); data=$data }
            Write-Host "$mode $Configuration run $run : PASS"
        } finally {
            if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
            $process.Dispose()
        }
    }
}
[ordered]@{
    schemaVersion=1; configuration=$Configuration; runsPerMode=$Runs; scenarioKind=$Scenario;
    measuredAtUtc=[DateTime]::UtcNow.ToString('o'); sourceCommit=(git -C $script:RepoRoot rev-parse HEAD);
    executableSha256=(Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash.ToLowerInvariant();
    measurementSourcesSha256=$(
        $hashes = [ordered]@{}
        foreach ($path in @('Client/WarOfDimension/ClientMemoryProfile.cpp', 'Client/WarOfDimension/ClientMemoryProfile.h',
            'Client/WarOfDimension/Scene.cpp', 'Client/WarOfDimension/Mesh.cpp', 'Client/WarOfDimension/GameFramework.cpp',
            'Client/WarOfDimension/Object.cpp', 'Client/WarOfDimension/NetworkManager.cpp',
            'Client/WarOfDimension/WarOfDimension.cpp', 'scripts/Measure-ClientMemory.ps1',
            'Client/WarOfDimension/ModelPartSelection.cpp', 'Client/WarOfDimension/ModelPartSelection.h',
            'Client/WarOfDimension/NetworkManager.h', 'Client/WarOfDimension/Player.cpp',
            'Client/WarOfDimension/GameFramework.h', 'Client/WarOfDimension/HeroSelectionTests.cpp',
            'Server/Lobby_Server/Job.h', 'Server/Lobby_Server/JobQueue.h', 'Server/Lobby_Server/Zone.h')) {
            $hashes[$path] = (Get-FileHash -LiteralPath (Join-Path $script:RepoRoot $path) -Algorithm SHA256).Hash.ToLowerInvariant()
        }
        $hashes
    );
    samplingIntervalMs=50; results=$results
} | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $logRoot "summary-$Configuration.json") -Encoding utf8
Write-Host "메모리 비교 완료: $logRoot/summary-$Configuration.json"

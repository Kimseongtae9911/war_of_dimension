param([ValidateSet('Debug','Release')][string] $Configuration = 'Release')
. "$PSScriptRoot/Development.ps1"
$logRoot = Join-Path $script:RepoRoot 'artifacts/logs/database'
[System.IO.Directory]::CreateDirectory($logRoot) | Out-Null
$saved = @{}
foreach ($name in @('WOD_DB_MODE','WOD_DB_CONNECTION','WOD_DB_CONFIG')) { $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
$results = [Collections.Generic.List[object]]::new()
try {
    foreach ($case in @('invalid-mode','missing-connection','missing-driver')) {
        $env:WOD_DB_MODE = if ($case -eq 'invalid-mode') { 'invalid' } else { 'odbc' }
        $env:WOD_DB_CONFIG = $null
        $env:WOD_DB_CONNECTION = if ($case -eq 'missing-driver') { 'Driver={NewWodMissingTestDriver};Server=127.0.0.1;' } else { $null }
        $stderr = Join-Path $logRoot "startup-$Configuration-$case.err.log"
        $process = Start-Process -FilePath (Get-WodExecutable LobbyServer $Configuration) -WorkingDirectory (Join-Path $script:RepoRoot 'Server/Lobby_Server') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $logRoot "startup-$Configuration-$case.out.log") -RedirectStandardError $stderr
        $null = $process.Handle
        if (-not $process.WaitForExit(10000)) { Stop-Process -Id $process.Id; throw "DB startup failure timeout: $case" }
        $process.Refresh()
        if ($process.ExitCode -ne 1) { throw "DB startup must fail closed: $case" }
        if ((Get-Content -LiteralPath $stderr -Raw) -notmatch 'ServerCore stop pending=0 sockets=0 leased=0') { throw "DB startup leaked resources: $case" }
        $results.Add(@{case=$case;passed=$true;exit_code=1;pending=0;sockets=0;leased=0})
    }
} finally {
    foreach ($entry in $saved.GetEnumerator()) { [Environment]::SetEnvironmentVariable($entry.Key,$entry.Value,'Process') }
}
@{ok=$true;cases=$results.ToArray()} | ConvertTo-Json -Depth 5 | Set-Content -Encoding utf8 (Join-Path $logRoot "startup-$Configuration.json")
Write-Host "DB 초기화 실패 3경로·자원 회수: PASS ($Configuration)"

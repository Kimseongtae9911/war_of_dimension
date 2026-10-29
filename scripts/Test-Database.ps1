param([ValidateSet('Debug','Release')][string] $Configuration = 'Debug')
. "$PSScriptRoot/Development.ps1"
$python = Get-WodPython
& $python -c 'import sys; sys.path.insert(0,".runtime/db-test-deps"); import pyodbc'
if ($LASTEXITCODE -ne 0) { throw '검사 전용 pyodbc가 필요합니다: python -m pip install --target .runtime/db-test-deps pyodbc' }
$config = Join-Path $script:RepoRoot '.runtime/db-test.connection.txt'
[System.IO.Directory]::CreateDirectory((Join-Path $script:RepoRoot 'artifacts/logs/database')) | Out-Null
& "$PSScriptRoot/Initialize-LocalDatabase.ps1" -Name NewWodTest -ConfigPath $config
$oldConfig = $env:WOD_DB_CONFIG
$oldConnection = $env:WOD_DB_CONNECTION
try {
    $env:WOD_DB_CONNECTION = $null
    $env:WOD_DB_CONFIG = $config
    & "$PSScriptRoot/Start-Local.ps1" -Configuration $Configuration -ServersOnly -Database
    try {
        & $python (Join-Path $script:RepoRoot 'tools/server_lab/tests/database.py') --config $config --output (Join-Path $script:RepoRoot "artifacts/logs/database/integration-$Configuration.json")
        if ($LASTEXITCODE -ne 0) { throw 'DB 통합 검사 실패' }
    } finally {
        & "$PSScriptRoot/Stop-Local.ps1"
        foreach ($module in @('LobbyServer','GameServer')) {
            $log = Join-Path $script:RepoRoot "artifacts/logs/run-$module.err.log"
            Copy-Item -LiteralPath $log -Destination (Join-Path $script:RepoRoot "artifacts/logs/database/$Configuration-$module.err.log")
            if ((Get-Content -LiteralPath $log -Raw) -notmatch 'ServerCore stop pending=0 sockets=0 leased=0') { throw "$module 종료 자원 검증 실패" }
        }
    }
} finally {
    $env:WOD_DB_CONFIG = $oldConfig
    $env:WOD_DB_CONNECTION = $oldConnection
}

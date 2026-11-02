param([Parameter(Mandatory)][string] $BeforeExecutable)
. "$PSScriptRoot/Development.ps1"
$python = Get-WodPython
$instance = 'NewWodTestProbe' + [guid]::NewGuid().ToString('N').Substring(0,8)
$config = Join-Path $script:RepoRoot ".runtime/$instance.connection.txt"
& SqlLocalDB create $instance | Out-Null
if ($LASTEXITCODE -ne 0) { throw '비교용 독립 LocalDB 생성 실패' }
try {
    & "$PSScriptRoot/Initialize-LocalDatabase.ps1" -Name $instance -Instance $instance -ConfigPath $config
    & $python -X utf8 (Join-Path $script:RepoRoot 'tools/server_lab/tests/database_request_cost.py') --config $config --instance $instance --before $BeforeExecutable --after (Join-Path $script:RepoRoot 'artifacts/bin/Release/LobbyServer/Lobby_Server.exe')
    if ($LASTEXITCODE -ne 0) { throw '정상 DB 요청 비용 비교 실패' }
} finally {
    & SqlLocalDB stop $instance -k | Out-Null
}

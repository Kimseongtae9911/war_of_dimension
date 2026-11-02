param([ValidateSet('Debug','Release')][string] $Configuration = 'Debug')
. "$PSScriptRoot/Development.ps1"
$python = Get-WodPython
$instance = 'NewWodTestReconnect' + [guid]::NewGuid().ToString('N').Substring(0,8)
$config = Join-Path $script:RepoRoot ".runtime/$instance.connection.txt"
& SqlLocalDB create $instance | Out-Null
if ($LASTEXITCODE -ne 0) { throw '검사 전용 LocalDB 인스턴스 생성 실패' }
try {
    & "$PSScriptRoot/Initialize-LocalDatabase.ps1" -Name $instance -Instance $instance -ConfigPath $config
    & $python -X utf8 (Join-Path $script:RepoRoot 'tools/server_lab/tests/database_reconnect.py') --configuration $Configuration --config $config --instance $instance
    if ($LASTEXITCODE -ne 0) { throw 'DB 연결 장애/재연결 검사 실패' }
} finally {
    # 자신이 만든 독립 인스턴스만 중지한다. DB 파일과 설정은 실패 조사/복구를 위해 보존한다.
    & SqlLocalDB stop $instance -k | Out-Null
}

param([ValidateSet('Debug','Release')][string] $Configuration = 'Debug')
. "$PSScriptRoot/Development.ps1"
$python = Get-WodPython
$config = Join-Path $script:RepoRoot '.runtime/db-recovery-test.connection.txt'
& "$PSScriptRoot/Initialize-LocalDatabase.ps1" -Name NewWodTestRecovery -ConfigPath $config
& $python (Join-Path $script:RepoRoot 'tools/server_lab/tests/database_consistency.py') --configuration $Configuration --config $config
if ($LASTEXITCODE -ne 0) { throw 'DB 정합성/재시작 복구 검사 실패' }

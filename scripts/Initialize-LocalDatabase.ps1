param(
    [switch] $CheckOnly,
    [ValidatePattern('^NewWod(Local|Test[a-zA-Z0-9]*)$')][string] $Name = 'NewWodLocal',
    [string] $ConfigPath = ''
)
. "$PSScriptRoot/Development.ps1"
if (-not [Environment]::Is64BitProcess) { throw '64-bit PowerShell이 필요합니다.' }
$driver = @('ODBC Driver 18 for SQL Server', 'ODBC Driver 17 for SQL Server') | Where-Object {
    Get-OdbcDriver -Name $_ -Platform '64-bit' -ErrorAction SilentlyContinue
} | Select-Object -First 1
if (-not $driver) { throw 'Microsoft ODBC Driver 17 또는 18 for SQL Server x64를 설치하세요.' }
$localdb = Get-Command SqlLocalDB -ErrorAction SilentlyContinue
if (-not $localdb) { throw 'SQL Server Express LocalDB를 설치하세요.' }
& $localdb.Source start MSSQLLocalDB | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'LocalDB 시작 실패' }
$base = "Driver={$driver};Server=(localdb)\MSSQLLocalDB;Trusted_Connection=Yes;Encrypt=No;"
$connection = [System.Data.Odbc.OdbcConnection]::new("${base}Database=master;")
try {
    $connection.Open()
    $command = $connection.CreateCommand()
    $command.CommandTimeout = 10
    if (-not $CheckOnly) {
        $command.CommandText = "IF DB_ID('$Name') IS NULL CREATE DATABASE [$Name];"
        [void]$command.ExecuteNonQuery()
    }
    $connection.ChangeDatabase($Name)
    if (-not $CheckOnly) {
        $schema = Get-Content -LiteralPath (Join-Path $script:RepoRoot 'database/local-schema.sql') -Raw -Encoding utf8
        foreach ($batch in [regex]::Split($schema, '(?im)^GO\s*$')) {
            if ([string]::IsNullOrWhiteSpace($batch)) { continue }
            $command.CommandText = $batch
            [void]$command.ExecuteNonQuery()
        }
    }
    $command.CommandText = 'SELECT COUNT(*) FROM sys.procedures WHERE schema_id=SCHEMA_ID(''dbo'');'
    $count = [int]$command.ExecuteScalar()
    if ($count -lt 16) { throw '필수 저장 프로시저가 부족합니다.' }
    $config = if ($ConfigPath) { [System.IO.Path]::GetFullPath($ConfigPath) } else { Join-Path $script:RepoRoot '.runtime/database.connection.txt' }
    if (-not $CheckOnly) {
        [System.IO.Directory]::CreateDirectory((Split-Path $config)) | Out-Null
        [System.IO.File]::WriteAllText($config, "${base}Database=$Name;")
    }
    Write-Host "로컬 DB 준비 확인: $Name, 저장 프로시저 $count 개, DSN 등록 불필요"
    Write-Host './scripts/Start-Local.ps1 -ServersOnly -Database'
} finally {
    $connection.Dispose()
}

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
$migration = $null
$invalidCredential = "DATALENGTH(Player_Password)<>118 OR LEFT(Player_Password,21) COLLATE Latin1_General_100_BIN2 <> 'pbkdf2-sha256`$600000`$' OR SUBSTRING(Player_Password,54,1)<>'`$' OR SUBSTRING(Player_Password,22,32) COLLATE Latin1_General_100_BIN2 LIKE '%[^0-9a-f]%' OR SUBSTRING(Player_Password,55,64) COLLATE Latin1_General_100_BIN2 LIKE '%[^0-9a-f]%'"
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
        $migration = $connection.BeginTransaction()
        $command.Transaction = $migration
        $schema = Get-Content -LiteralPath (Join-Path $script:RepoRoot 'database/local-schema.sql') -Raw -Encoding utf8
        foreach ($batch in [regex]::Split($schema, '(?im)^GO\s*$')) {
            if ([string]::IsNullOrWhiteSpace($batch)) { continue }
            $command.CommandText = $batch
            [void]$command.ExecuteNonQuery()
        }
        $command.CommandText = "SELECT Player_ID,Player_Password FROM dbo.user_data WHERE Player_Password NOT LIKE 'pbkdf2-sha256`$%';"
        $reader = $command.ExecuteReader()
        $credentials = [Collections.Generic.List[object]]::new()
        try {
            while ($reader.Read()) {
                $password = $reader.GetString(1)
                if ($password -match '[^\x00-\x7F]') { throw '기존 비ASCII 비밀번호는 인코딩을 확인한 별도 이전이 필요합니다. 변경을 취소합니다.' }
                $credentials.Add(@{Account=$reader.GetString(0);Password=$password})
            }
        } finally { $reader.Close() }
        foreach ($credential in $credentials) {
            $salt = [Security.Cryptography.RandomNumberGenerator]::GetBytes(16)
            $derive = [Security.Cryptography.Rfc2898DeriveBytes]::new($credential.Password, $salt, 600000, [Security.Cryptography.HashAlgorithmName]::SHA256)
            try { $hash = $derive.GetBytes(32) } finally { $derive.Dispose() }
            $encoded = 'pbkdf2-sha256$600000$' + [Convert]::ToHexString($salt).ToLowerInvariant() + '$' + [Convert]::ToHexString($hash).ToLowerInvariant()
            $command.Parameters.Clear()
            $command.CommandText = 'UPDATE dbo.user_data SET Player_Password=? WHERE Player_ID=?;'
            [void]$command.Parameters.Add('hash',[Data.Odbc.OdbcType]::VarChar,256)
            $command.Parameters[0].Value = $encoded
            [void]$command.Parameters.Add('account',[Data.Odbc.OdbcType]::VarChar,10)
            $command.Parameters[1].Value = $credential.Account
            [void]$command.ExecuteNonQuery()
        }
        $credentials.Clear()
        $password = $null
        $command.Parameters.Clear()
        $command.CommandText = "SELECT COUNT(*) FROM dbo.user_data WHERE $invalidCredential;"
        if ([int]$command.ExecuteScalar() -ne 0) { throw '잘못된 자격 증명 형식이 있어 스키마 이전을 취소합니다.' }
        $command.CommandText = 'UPDATE dbo.WodLocalSchema SET Version=3 WHERE Version IN (1,2);'
        [void]$command.ExecuteNonQuery()
        $migration.Commit()
        $migration.Dispose()
        $migration = $null
        $command.Transaction = $null
    }
    $command.CommandText = 'SELECT COUNT(*) FROM sys.procedures WHERE schema_id=SCHEMA_ID(''dbo'');'
    $count = [int]$command.ExecuteScalar()
    if ($count -lt 18) { throw '필수 저장 프로시저가 부족합니다.' }
    $command.CommandText = 'SELECT Version FROM dbo.WodLocalSchema;'
    if ([int]$command.ExecuteScalar() -ne 3) { throw '로컬 DB 스키마 버전이 일치하지 않습니다.' }
    $command.CommandText = "SELECT COUNT(*) FROM dbo.user_data WHERE $invalidCredential;"
    if ([int]$command.ExecuteScalar() -ne 0) { throw '이전되지 않은 자격 증명이 있습니다.' }
    $config = if ($ConfigPath) { [System.IO.Path]::GetFullPath($ConfigPath) } else { Join-Path $script:RepoRoot '.runtime/database.connection.txt' }
    if (-not $CheckOnly) {
        [System.IO.Directory]::CreateDirectory((Split-Path $config)) | Out-Null
        [System.IO.File]::WriteAllText($config, "${base}Database=$Name;")
    }
    Write-Host "로컬 DB 준비 확인: $Name, 저장 프로시저 $count 개, DSN 등록 불필요"
    Write-Host './scripts/Start-Local.ps1 -ServersOnly -Database'
} finally {
    if ($migration) { try { $migration.Rollback() } finally { $migration.Dispose() } }
    $connection.Dispose()
}

param([ValidateSet('Debug', 'Release')][string] $Configuration = 'Debug')
. "$PSScriptRoot/Development.ps1"
$installation = Split-Path (Split-Path (Split-Path (Split-Path (Get-WodMSBuild) -Parent) -Parent) -Parent) -Parent
$compilerVersion = (Get-Content (Join-Path $installation 'VC/Auxiliary/Build/Microsoft.VCToolsVersion.default.txt') -Raw).Trim()
$compilerRoot = Join-Path $installation "VC/Tools/MSVC/$compilerVersion"
$compiler = Join-Path $compilerRoot 'bin/Hostx64/x64/cl.exe'
$sdkRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10'
$sdkVersion = Get-ChildItem (Join-Path $sdkRoot 'Include') -Directory | Sort-Object { [version] $_.Name } -Descending | Select-Object -First 1 -ExpandProperty Name
$includePaths = @((Join-Path $compilerRoot 'include')) + @('ucrt', 'shared', 'um', 'winrt', 'cppwinrt' | ForEach-Object { Join-Path $sdkRoot "Include/$sdkVersion/$_" })
$entries = [System.Collections.Generic.List[object]]::new()
$codeProjects = @(Get-WodModules) + @(
    @{ Name='ServerCore'; Project='ServerCore'; Directory='Server/ServerCore' },
    @{ Name='ServerCore.Tests'; Project='ServerCore.Tests'; Directory='Server/ServerCore/tests' }
)
foreach ($module in $codeProjects) {
    $directory = Join-Path $script:RepoRoot $module.Directory
    [xml] $project = Get-Content (Join-Path $directory "$($module.Project).vcxproj") -Raw
    $files = $project.SelectNodes("//*[local-name()='ClCompile' and @Include]")
    $subsystemDefine = if ($module.Name -eq 'Client') { '/D_WINDOWS' } else { '/D_CONSOLE' }
    $runtime = if ($Configuration -eq 'Debug') { '/MDd' } else { '/MD' }
    $defines = @('/DUNICODE', '/D_UNICODE', $subsystemDefine) + $(if ($Configuration -eq 'Debug') { '/D_DEBUG' } else { '/DNDEBUG' })
    $arguments = @($compiler, '/nologo', '/c', '/std:c++20', '/EHsc', $runtime, '/utf-8') + $defines
    $arguments += @($includePaths + $directory + (Join-Path $script:RepoRoot 'Server/ServerCore/include') + (Join-Path $script:RepoRoot 'Shared') + $(if ($module.Name -notlike 'ServerCore*') { Join-Path $script:RepoRoot 'Server/Game_Server/Include' }) | ForEach-Object { '/I' + $_ })
    foreach ($file in $files) {
        $source = [IO.Path]::GetFullPath((Join-Path $directory $file.Include))
        $entries.Add(@{ directory = $directory; file = $source; arguments = @($arguments + $source) })
    }
}
$entries.ToArray() | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $script:RepoRoot 'compile_commands.json') -Encoding utf8
Write-Host "Compilation database 생성: $($entries.Count)개 소스 ($Configuration x64)"

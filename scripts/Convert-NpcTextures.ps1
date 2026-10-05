param(
    [string] $Texconv = 'artifacts/tools/directxtex-may2026/texconv.exe',
    [string] $SourceDirectory = '.',
    [string] $OutputDirectory = 'artifacts/logs/npc-bc7-converted',
    [switch] $Apply
)
. "$PSScriptRoot/Development.ps1"
$converter = [System.IO.Path]::GetFullPath((Join-Path $script:RepoRoot $Texconv))
$expectedToolHash = 'dcfdec10244e02cf5037fba089c55fb7e1326b1c8181742d77d15fa5cb5eef06'
if ((Get-FileHash $converter -Algorithm SHA256).Hash.ToLowerInvariant() -ne $expectedToolHash) { throw 'Microsoft DirectXTex may2026 texconv 해시 불일치' }
$sourceRoot = [System.IO.Path]::GetFullPath((Join-Path $script:RepoRoot $SourceDirectory))
$outputRoot = [System.IO.Path]::GetFullPath((Join-Path $script:RepoRoot $OutputDirectory))
[System.IO.Directory]::CreateDirectory($outputRoot) | Out-Null
$review = Get-Content (Join-Path $script:RepoRoot 'docs/portfolio/evidence/npc-sharing-review-20261005.json') -Raw -Encoding utf8 | ConvertFrom-Json
$results = @()
foreach ($property in $review.textures.PSObject.Properties) {
    $texture = $property.Value
    $source = Join-Path $sourceRoot $texture.path
    if ((Get-FileHash $source -Algorithm SHA256).Hash.ToLowerInvariant() -ne $texture.assetSha256) { throw "원본 DDS 해시 불일치: $($texture.path)" }
    # 이미 8bpp인 R8 Metallic은 BC7(8bpp)으로 바꿔도 절감되지 않는다.
    if ($texture.bitsPerPixel -le 8) {
        $preserved = Join-Path $outputRoot (Split-Path $texture.path -Leaf)
        Copy-Item -LiteralPath $source -Destination $preserved
        $results += [ordered]@{ path=$texture.path; action='preserved'; reason='원본이 이미 8bpp이므로 BC7 압축 이득 없음';
            beforeSha256=$texture.assetSha256; afterSha256=$texture.assetSha256; width=$texture.width; height=$texture.height;
            mipLevels=$texture.effectiveMipLevels; format='R8_UNORM'; beforePayloadBytes=$texture.payloadBytes; afterPayloadBytes=$texture.payloadBytes;
            beforeFileBytes=$texture.fileBytes; afterFileBytes=$texture.fileBytes }
        Write-Host "원본 유지 PASS: $($property.Name)"
        continue
    }
    # 색 공간·채널 swizzle·해상도를 변경하지 않고 기존 단일 mip를 보존한다.
    $log = & $converter -nologo -y -m 1 -f BC7_UNORM -bc x -gpu 0 -o $outputRoot $source 2>&1
    $log | Out-File (Join-Path $outputRoot ($property.Name + '.log')) -Encoding utf8
    if ($LASTEXITCODE -ne 0) { throw "BC7 변환 실패: $($texture.path)" }
    $converted = Join-Path $outputRoot (Split-Path $texture.path -Leaf)
    $bytes = [System.IO.File]::ReadAllBytes($converted)
    $payloadBytes = [long]([math]::Ceiling($texture.width / 4) * [math]::Ceiling($texture.height / 4) * 16)
    if ($bytes.Length -ne $payloadBytes + 148 -or [System.Text.Encoding]::ASCII.GetString($bytes, 0, 4) -ne 'DDS ' -or
        [System.Text.Encoding]::ASCII.GetString($bytes, 84, 4) -ne 'DX10' -or [BitConverter]::ToUInt32($bytes, 128) -ne 98 -or
        [BitConverter]::ToUInt32($bytes, 12) -ne $texture.height -or [BitConverter]::ToUInt32($bytes, 16) -ne $texture.width -or
        [math]::Max(1, [BitConverter]::ToUInt32($bytes, 28)) -ne 1) { throw "BC7 DDS metadata/payload 검증 실패: $($texture.path)" }
    $results += [ordered]@{ path=$texture.path; action='compressed'; beforeSha256=$texture.assetSha256; afterSha256=(Get-FileHash $converted -Algorithm SHA256).Hash.ToLowerInvariant();
        width=$texture.width; height=$texture.height; mipLevels=1; format='BC7_UNORM'; beforePayloadBytes=$texture.payloadBytes; afterPayloadBytes=$payloadBytes; beforeFileBytes=$texture.fileBytes; afterFileBytes=$bytes.Length }
    Write-Host "BC7 검증 PASS: $($property.Name)"
}
if ($Apply) {
    foreach ($result in $results) { Copy-Item -LiteralPath (Join-Path $outputRoot (Split-Path $result.path -Leaf)) -Destination (Join-Path $script:RepoRoot $result.path) }
    git -C $script:RepoRoot lfs track @($results | ForEach-Object { $_.path })
    if ($LASTEXITCODE -ne 0) { throw '변환한 DDS Git LFS 등록 실패' }
}
[ordered]@{ schemaVersion=1; applied=[bool]$Apply; toolRelease='Microsoft DirectXTex may2026'; toolSha256=$expectedToolHash;
    officialSource='https://github.com/microsoft/DirectXTex/releases/tag/may2026'; arguments=@('-m','1','-f','BC7_UNORM','-bc','x','-gpu','0'); textures=$results } |
    ConvertTo-Json -Depth 6 | Set-Content (Join-Path $outputRoot 'conversion.json') -Encoding utf8
Write-Host "NPC DDS 압축 $(@($results | Where-Object action -eq compressed).Count)개, 원본 유지 $(@($results | Where-Object action -eq preserved).Count)개: applied=$([bool]$Apply)"

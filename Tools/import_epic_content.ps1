param(
    [Parameter(Mandatory=$true)][string]$LyraPath,
    [string]$Editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
)
$ErrorActionPreference = 'Stop'
$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Source = (Resolve-Path (Join-Path $LyraPath 'Content')).Path
$ContentRoot = Join-Path $Root 'Content'
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { throw '에디터를 종료한 뒤 가져오기를 실행하세요.' }
if (-not (Test-Path -LiteralPath $Editor)) { throw "에디터를 찾을 수 없습니다: $Editor" }
$Manifest = Join-Path $PSScriptRoot 'epic_content_manifest.txt'
if (-not (Test-Path -LiteralPath $Manifest)) { throw '고정된 Epic 콘텐츠 목록이 없습니다.' }
# 모든 경로를 검증한 뒤 복사한다. 원본과 정리된 복사본의 해시로 재실행을 판별한다.
$Files = @(Get-Content -LiteralPath $Manifest -Encoding UTF8 | Where-Object { $_ -and -not $_.StartsWith('#') } | ForEach-Object { [string]$_ })
foreach ($Relative in $Files) {
    $Target = [IO.Path]::GetFullPath((Join-Path $ContentRoot $Relative))
    $Original = [IO.Path]::GetFullPath((Join-Path $Source $Relative))
    if (-not $Target.StartsWith($ContentRoot + '\', [StringComparison]::OrdinalIgnoreCase) -or
        -not $Original.StartsWith($Source + '\', [StringComparison]::OrdinalIgnoreCase) -or
        [IO.Path]::GetExtension($Relative) -ne '.uasset') { throw "잘못된 목록 경로: $Relative" }
    if (-not (Test-Path -LiteralPath $Original -PathType Leaf)) { throw "Lyra 에셋 없음: $Original" }
}
$StatePath = Join-Path $Root 'Saved/import_epic_content-state.json'
$Previous = @{}
$CleanupHash = (Get-FileHash -LiteralPath "$PSScriptRoot\prepare_epic_content.py").Hash + (Get-FileHash -LiteralPath "$Root\Binaries\Win64\UnrealEditor-DrSample.dll").Hash
if (Test-Path -LiteralPath $StatePath) {
    $State = Get-Content -LiteralPath $StatePath -Raw | ConvertFrom-Json
    if ($State.Cleanup -eq $CleanupHash) {
        foreach ($Entry in $State.Files) { $Previous[$Entry.Path] = $Entry }
    }
}
$Copied = 0
$SourceHashes = @{}
foreach ($Relative in $Files) {
    $Target = Join-Path $ContentRoot $Relative
    $SourceHashes[$Relative] = (Get-FileHash -LiteralPath (Join-Path $Source $Relative)).Hash
    $Old = $Previous[$Relative]
    if ($Old -and $Old.Source -eq $SourceHashes[$Relative] -and (Test-Path -LiteralPath $Target) -and
        $Old.Output -eq (Get-FileHash -LiteralPath $Target).Hash) { continue }
    New-Item -ItemType Directory -Force (Split-Path $Target -Parent) | Out-Null
    Copy-Item -LiteralPath (Join-Path $Source $Relative) -Destination $Target -Force
    ++$Copied
}
if ($Copied -eq 0) {
    Write-Output "Epic 콘텐츠 $($Files.Count)개가 이미 정리되어 있습니다(해시 일치)."
    return
}
New-Item -ItemType Directory -Force (Join-Path $Root 'Saved') | Out-Null
$PreviousDiscovery = $env:DRSAMPLE_DISCOVER
try {
    $env:DRSAMPLE_DISCOVER = '0'
    & $Editor (Join-Path $Root 'DrSample.uproject') '-run=pythonscript' "-script=$PSScriptRoot\prepare_epic_content.py" '-EnablePlugins=PythonScriptPlugin' '-nullrhi' '-unattended' '-nosplash' "-abslog=$Root\Saved\import_epic_content.log"
    if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath "$Root\Saved\import_epic_content.log" -Pattern 'DRSAMPLE_EPIC_READY' -Quiet)) { throw 'Epic 콘텐츠 정리에 실패했습니다. Saved/import_epic_content.log를 확인하세요.' }
} finally { $env:DRSAMPLE_DISCOVER = $PreviousDiscovery }
$NewState = @($Files | ForEach-Object {
    [PSCustomObject]@{Path=[string]$_; Source=$SourceHashes[$_]; Output=(Get-FileHash -LiteralPath (Join-Path $ContentRoot $_)).Hash}
})
[PSCustomObject]@{Cleanup=$CleanupHash; Files=$NewState} | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath $StatePath -Encoding UTF8
Write-Output "Epic 콘텐츠 $($Files.Count)개 준비 완료(새 복사 $Copied개)"

param(
    [ValidateSet('audit','draft','upload','publish','verify','replace')]
    [string]$Mode='audit',
    [ValidatePattern('^\d+\.\d+\.\d+(\.\d+)?$')]
    [string]$Version = '1.3.2.1',
    [string]$Commit
)
$ErrorActionPreference = 'Stop'
$repo = Split-Path $PSScriptRoot -Parent
$slug = 'differentlightproductions-cyber/snap-fe'
$api = "https://api.github.com/repos/$slug"
$tag = "V$Version"
$zipName = "SnapFE-Alpha-$Version.zip"
$sumName = "SHA256SUMS-$Version.txt"
# latest.json is what Snap FE's Check for Updates reads; releases from 1.3.2 carry it.
$assets = @($zipName, $sumName)
if (Test-Path -LiteralPath (Join-Path $repo 'dist/latest.json') -PathType Leaf) { $assets += 'latest.json' }
$notesName = "RELEASE-NOTES-$Version.txt"
if (-not (Test-Path -LiteralPath (Join-Path $repo $notesName) -PathType Leaf)) { $notesName = "RELEASE-NOTES-$Version.md" }
$env:GCM_INTERACTIVE = 'never'
$start = New-Object System.Diagnostics.ProcessStartInfo
$gitRoot=Split-Path (Split-Path (Get-Command git).Source -Parent) -Parent
$start.FileName=Join-Path $gitRoot 'mingw64/bin/git-credential-manager.exe'; $start.Arguments='get'
$start.UseShellExecute=$false; $start.CreateNoWindow=$true
$start.EnvironmentVariables['PATH']=(Join-Path $gitRoot 'cmd')+';'+(Join-Path $gitRoot 'mingw64/bin')+';'+(Join-Path $env:SystemRoot 'System32')
$start.RedirectStandardInput=$true; $start.RedirectStandardOutput=$true; $start.RedirectStandardError=$true
$process=New-Object System.Diagnostics.Process; $process.StartInfo=$start
[void]$process.Start()
$process.StandardInput.WriteLine('protocol=https')
$process.StandardInput.WriteLine('host=github.com')
$process.StandardInput.WriteLine('username=differentlightproductions-cyber')
$process.StandardInput.WriteLine(''); $process.StandardInput.Close()
$credentialLines=$process.StandardOutput.ReadToEnd() -split "`r?`n"
$credentialError=$process.StandardError.ReadToEnd(); $process.WaitForExit()
if ($process.ExitCode -ne 0) { throw ('Existing GitHub credential unavailable: ' + $credentialError.Trim()) }
$credential = @{}
foreach ($line in $credentialLines) { $parts = $line -split '=',2; if($parts.Count -eq 2){$credential[$parts[0]]=$parts[1]} }
if (-not $credential.password) { throw 'GitHub credential has no token' }
$headers = @{ Authorization="Bearer $($credential.password)"; Accept='application/vnd.github+json'; 'X-GitHub-Api-Version'='2022-11-28' }
function Request($Method, $Url, $Body) {
    $args = @{Method=$Method; Uri=$Url; Headers=$headers}
    if ($null -ne $Body) { $args.Body=[Text.Encoding]::UTF8.GetBytes(($Body | ConvertTo-Json -Depth 12)); $args.ContentType='application/json' }
    Invoke-RestMethod @args
}
$releases = Request GET "$api/releases?per_page=30" $null
$release = @($releases | Where-Object tag_name -eq $tag)
if($release.Count -gt 1){throw "Duplicate $Version releases"}
if($Mode -eq 'audit') {
    $branch=Request GET "$api/branches/main" $null
    [ordered]@{main=$branch.commit.sha; releases=@($releases | Select-Object -First 5 id,tag_name,name,draft,prerelease,html_url,@{n='assets';e={@($_.assets | Select-Object id,name,size,digest)}})} | ConvertTo-Json -Depth 10
    exit
}
if($Mode -eq 'draft') {
    if($Commit -notmatch '^[0-9a-f]{40}$'){throw 'Exact source commit required'}
    $body=@{tag_name=$tag;target_commitish=$Commit;name="Snap FE Alpha $Version";body=[IO.File]::ReadAllText((Join-Path $repo $notesName));draft=$true;prerelease=$false}
    if($release.Count) { if(-not $release[0].draft){throw 'Existing release is public; inspect before updating'}; $release=Request PATCH "$api/releases/$($release[0].id)" $body }
    else{$release=Request POST "$api/releases" $body}
} else {
    if(-not $release.Count){throw 'Release is missing'}
    $release=$release[0]
}
if($Mode -eq 'upload') {
    if(-not $release.draft){throw 'Upload requires the prepared draft'}
    foreach($name in $assets){
        $path=Join-Path $repo "dist/$name"
        if(@($release.assets | Where-Object name -eq $name).Count){throw "Asset already exists: $name; verify before retrying"}
        $upload=$release.upload_url -replace '\{.*$',''
        $uploaded=Invoke-RestMethod -Method POST -Uri "$($upload)?name=$([uri]::EscapeDataString($name))" -Headers $headers -InFile $path -ContentType 'application/octet-stream' -TimeoutSec 300
        if($uploaded.size -ne (Get-Item -LiteralPath $path).Length){throw 'Uploaded asset size mismatch'}
        if($uploaded.digest -and $uploaded.digest -ne ('sha256:'+(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower())){throw 'Uploaded asset hash mismatch'}
    }
    $release=Request GET "$api/releases/$($release.id)" $null
}
if($Mode -eq 'replace') {
    # Quietly swap the two public assets of a published release for rebuilt ones.
    if($release.draft){throw 'Replace is for a published release; use upload for a draft'}
    foreach($name in $assets){if(-not (Test-Path -LiteralPath (Join-Path $repo "dist/$name"))){throw "Missing dist/$name"}}
    foreach($name in $assets){
        foreach($old in @($release.assets | Where-Object name -eq $name)){ Request DELETE "$api/releases/assets/$($old.id)" $null | Out-Null }
        $path=Join-Path $repo "dist/$name"
        $upload=$release.upload_url -replace '\{.*$',''
        $uploaded=Invoke-RestMethod -Method POST -Uri "$($upload)?name=$([uri]::EscapeDataString($name))" -Headers $headers -InFile $path -ContentType 'application/octet-stream' -TimeoutSec 300
        if($uploaded.size -ne (Get-Item -LiteralPath $path).Length){throw 'Uploaded asset size mismatch'}
        if($uploaded.digest -and $uploaded.digest -ne ('sha256:'+(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLower())){throw 'Uploaded asset hash mismatch'}
    }
    $release=Request GET "$api/releases/$($release.id)" $null
}
if($Mode -eq 'publish') {
    $expected=$assets
    if($release.assets.Count -ne $expected.Count){throw "Expected exactly $($expected.Count) public assets"}
    foreach($name in $expected){$asset=@($release.assets | Where-Object name -eq $name);if($asset.Count -ne 1){throw "Missing asset: $name"};$path=Join-Path $repo "dist/$name";if($asset[0].size -ne (Get-Item $path).Length){throw 'Asset size mismatch'};if($asset[0].digest -ne ('sha256:'+(Get-FileHash $path -Algorithm SHA256).Hash.ToLower())){throw 'Asset digest mismatch'}}
    if($release.body -ne [IO.File]::ReadAllText((Join-Path $repo $notesName))){throw 'Release notes differ'}
    $release=Request PATCH "$api/releases/$($release.id)" @{draft=$false;prerelease=$false;make_latest='true'}
}
if($Mode -eq 'verify'){
    $latest=Request GET "$api/releases/latest" $null
    if($latest.id -ne $release.id -or $release.draft){throw "$Version is not the public latest release"}
}
$release | Select-Object id,tag_name,name,draft,prerelease,html_url,published_at,@{n='assets';e={@($_.assets | Select-Object name,size,digest,browser_download_url)}} | ConvertTo-Json -Depth 8

param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^\d+\.\d+\.\d+$')]
    [string]$Version,

    [switch]$OpenFolder
)

$ErrorActionPreference = 'Stop'

$repoRoot = $PSScriptRoot
$zipName = "SnapFE-Alpha-$Version.zip"
$sumName = "SHA256SUMS-$Version.txt"
$notesName = "RELEASE-NOTES-$Version.md"
$zipPath = Join-Path $repoRoot "dist\$zipName"
$sumPath = Join-Path $repoRoot "dist\$sumName"
$notesPath = Join-Path $repoRoot $notesName

foreach ($required in @($zipPath, $sumPath, $notesPath)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing release file: $required"
    }
}

$downloads = Join-Path ([Environment]::GetFolderPath('UserProfile')) 'Downloads'
$target = Join-Path $downloads "SNAP-FE-$Version-READY-TO-PUBLISH"
[System.IO.Directory]::CreateDirectory($target) | Out-Null

Copy-Item -LiteralPath $zipPath -Destination (Join-Path $target $zipName) -Force
Copy-Item -LiteralPath $sumPath -Destination (Join-Path $target $sumName) -Force
Copy-Item -LiteralPath $notesPath -Destination (Join-Path $target $notesName) -Force

$publishInstructions = @"
SNAP FE $Version - READY TO PUBLISH
==================================

Upload ONLY these two files to the GitHub release:

  1. $zipName
  2. $sumName

GitHub release fields:

  Tag:    V$Version
  Target: main
  Title:  Snap FE Alpha $Version
  Label:  Latest

Open $notesName, copy all of its text, and paste it into Release description.

If an older draft already contains files with these names, remove those two
attachments before adding the copies from this folder. Do not upload this
instruction file or the Claude handoff file.

Final check before Publish release:

  - ZIP and checksum both appear in the attachment list.
  - The title and tag show $Version.
  - The description includes the Updating section.
  - Latest is selected.
"@

$claudeHandoff = @"
# SNAP FE handoff for Claude - local/private, do not upload

## Current release

- Version: $Version
- Git tag intended: V$Version
- Release title: Snap FE Alpha $Version
- Git branch: main
- GitHub repository: https://github.com/differentlightproductions-cyber/snap-fe
- Windows source checkout: $repoRoot
- WSL working mirror used by Nick: /home/nick/snapos-ui
- Final public attachments are `$zipName` and `$sumName` in this folder.
- Paste `$notesName` into GitHub's Release description.

## Handheld development target

- SSH: root@192.168.4.29
- SSH password is intentionally not stored in this handoff. Ask Nick or use
  interactive SSH authentication when access is needed.
- Device family: Allwinner H700 / Knulli / Anbernic RG SP family

## Verified build and deployment commands (run from WSL)

```bash
cd "/mnt/c/Users/NickO/Desktop/Downloads/SNAP OS Backup/snapos-backup/bugfix-work/repo-publish"
bash ./build-knulli.sh --sysroot /home/nick/knulli-sysroot
bash ./knulli/deploy.sh root@192.168.4.29
bash ./knulli/package.sh $Version
```

The package command now creates the ZIP, checksum, validates the archive, and
automatically exports a clean Windows Downloads folder like this one.

## Important current implementation details

- App Focused has two icon packs: Simple and Pixel Art (AI Art).
- Simple is the default and contains 13 real SVG files plus matching PNG
  fallbacks in `assets/icons/home/simple`.
- SVG rendering uses Knulli SDL_image 2.8's `IMG_LoadSizedSVG_RW`; no extra
  runtime renderer library is bundled.
- The illustrated console views are labeled Carousel (AI Art) and Bookshelf
  (AI Art).
- Pixel Art remains preserved separately under `assets/icons/home/pixel-art`.
- The packager removes Windows Zone.Identifier sidecars and refuses to include
  ROMs, saves, API keys, user settings, account files, or scraped personal art.
- Preserve all brightness and in-game hotkey behavior when making later edits.
- Do not stage or commit the local-only untracked PC helpers
  `launch_pc_preview.sh` or `snapos_ui.desktop` unless Nick explicitly asks.

## GitHub state at handoff

A V$Version release draft was open. Its two older attachments were marked for
deletion in the GitHub editor, but the corrected attachments had not yet been
uploaded and the release had not been published when this handoff was made.
Use `PUBLISH-INSTRUCTIONS.txt` beside this file to finish manually.
"@

$readFirst = @"
SNAP FE $Version

START HERE:

1. Read PUBLISH-INSTRUCTIONS.txt to publish this release yourself.
2. Upload only the ZIP and SHA256SUMS files.
3. RELEASE-NOTES-$Version.md is text to paste into GitHub, not an attachment.
4. CLAUDE-HANDOFF-DO-NOT-UPLOAD.md is private context for your next coding
   assistant and must not be attached to a public release.
"@

$utf8 = [System.Text.UTF8Encoding]::new($false)
[System.IO.File]::WriteAllText((Join-Path $target 'README-FIRST.txt'), $readFirst, $utf8)
[System.IO.File]::WriteAllText((Join-Path $target 'PUBLISH-INSTRUCTIONS.txt'), $publishInstructions, $utf8)
[System.IO.File]::WriteAllText((Join-Path $target 'CLAUDE-HANDOFF-DO-NOT-UPLOAD.md'), $claudeHandoff, $utf8)

Write-Host "Ready-to-publish folder: $target"
Write-Host "Upload: $zipName and $sumName"

if ($OpenFolder) {
    Start-Process explorer.exe -ArgumentList ('"' + $target + '"')
}

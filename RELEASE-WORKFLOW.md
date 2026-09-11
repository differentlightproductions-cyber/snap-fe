# Release workflow

Every Windows/WSL release should be exported locally before opening GitHub.

Write `RELEASE-NOTES-<version>.txt` first: a short New / Improved / Fixed /
Install list. It ships in the ZIP as `WHATS-NEW.txt` and is pasted into GitHub.
Wildkins is packaged from its sibling checkout (`../../wildkins`, or set
`WILDKINS_DIR`), so build its release binary first.

From WSL, build and package the requested semantic version:

```bash
cd "/mnt/c/Users/NickO/Desktop/Downloads/SNAP OS Backup/snapos-backup/wildkins"
bash tools/build-h700.sh
cd "/mnt/c/Users/NickO/Desktop/Downloads/SNAP OS Backup/snapos-backup/bugfix-work/repo-publish"
bash ./build-knulli.sh --sysroot /home/nick/knulli-sysroot --cc /usr/bin/aarch64-linux-gnu-gcc
bash ./knulli/package.sh 1.3.2
```

`package.sh` validates the archive, writes its checksum and `dist/latest.json`,
and—when running in WSL—calls `prepare-release-windows.ps1`.

`latest.json` is what Snap FE's **Check for Updates** reads (1.3.2 and later):
version, ZIP address, SHA-256, size and the release notes. Upload it with the
ZIP and checksum; GitHub then serves the newest release's copy at
`https://github.com/differentlightproductions-cyber/snap-fe/releases/latest/download/latest.json`,
so publishing a release is all it takes to offer it in-app. `tools/github-release.ps1`
uploads and verifies all three files.

```json
{
  "format": 1,
  "version": "1.3.2",
  "name": "Snap FE Alpha 1.3.2",
  "tag": "V1.3.2",
  "zip": "SnapFE-Alpha-1.3.2.zip",
  "url": "https://github.com/differentlightproductions-cyber/snap-fe/releases/download/V1.3.2/SnapFE-Alpha-1.3.2.zip",
  "sha256": "<64 hex characters>",
  "size": 218067488,
  "published": "2026-09-12",
  "notes": "Snap FE Alpha 1.3.2\n..."
}
```

The Windows exporter creates:

```text
%USERPROFILE%\Downloads\SNAP-FE-<version>-READY-TO-PUBLISH\
```

That folder contains the two public attachments, release notes, manual GitHub
steps, and a private Claude handoff. Only the ZIP and checksum are uploaded as
release assets. The Markdown release notes are pasted into GitHub's description.

To recreate or reopen a folder without repackaging, run in PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\prepare-release-windows.ps1 -Version 1.3.2 -OpenFolder
```

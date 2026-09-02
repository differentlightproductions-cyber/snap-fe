# Release workflow

Every Windows/WSL release should be exported locally before opening GitHub.

From WSL, build and package the requested semantic version:

```bash
cd "/mnt/c/Users/NickO/Desktop/Downloads/SNAP OS Backup/snapos-backup/bugfix-work/repo-publish"
bash ./build-knulli.sh --sysroot /home/nick/knulli-sysroot
bash ./knulli/package.sh 1.2.4
```

`package.sh` validates the archive, writes its checksum, and—when running in
WSL—calls `prepare-release-windows.ps1`. The Windows exporter creates:

```text
%USERPROFILE%\Downloads\SNAP-FE-<version>-READY-TO-PUBLISH\
```

That folder contains the two public attachments, release notes, manual GitHub
steps, and a private Claude handoff. Only the ZIP and checksum are uploaded as
release assets. The Markdown release notes are pasted into GitHub's description.

To recreate or reopen a folder without repackaging, run in PowerShell:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\prepare-release-windows.ps1 -Version 1.2.4 -OpenFolder
```

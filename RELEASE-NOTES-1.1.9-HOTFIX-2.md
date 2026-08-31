# Snap FE Alpha 1.1.9 Hotfix 2

This urgent stability hotfix repairs the RetroArch/controller and in-game
brightness regressions introduced during the 1.1.9 settings bridge work.

- Restores Knulli's native `udev` controller setup for both games and the Home
  RetroArch app, removing the false “RG34XX-SP Controller not configured”
  notification.
- Restores the default handheld hotkeys: Menu + Select opens RetroArch, Menu +
  Start exits to Snap FE, and Menu + R2 fast-forwards.
- Moves Function + Volume brightness into Knulli's system key service. It works
  in RetroArch games independently of RetroArch hotkeys, uses clean 5% steps,
  reaches a true 1% night floor, and cannot wrap back to maximum.
- Shares only portable RetroArch preferences such as menu and notification
  options between the Home app and games. Controller, driver, path, and
  protected brightness settings are no longer allowed through that bridge.
- Keeps the normal Knulli restore path intact when Snap FE is disabled.

Install by extracting the ZIP to the root of Knulli's SHARE partition, then
open **Ports → Snap FE (Set As Default)** and reboot.

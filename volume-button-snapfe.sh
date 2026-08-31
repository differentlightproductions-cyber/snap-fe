#!/bin/sh
# Snap FE owns the handheld's dedicated level buttons while it is the active
# frontend. This file is bind-mounted over Knulli's duplicate callback; the
# original /usr/bin/volume-button is never modified and returns after unmount.
exit 0

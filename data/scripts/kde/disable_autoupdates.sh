#!/bin/bash
echo Disabling KDE updates at startup
echo Safe to ignore errors: may occur if it was already disabled
sudo mkdir /etc/xdg/autostart.disabled
sudo mv /etc/xdg/autostart/org.kde.discover.notifier.desktop /etc/xdg/autostart.disabled/org.kde.discover.notifier.desktop
echo Done
read -t 3 -p "Press any key (or wait 3 seconds) ..."

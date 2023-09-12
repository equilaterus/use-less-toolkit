#!/bin/bash
echo Optimizing Fedora startup
sudo systemctl mask systemd-udev-settle
sudo systemctl disable NetworkManager-wait-online.service
echo Done
read -t 3 -p "Press any key (or wait 3 seconds) ..."

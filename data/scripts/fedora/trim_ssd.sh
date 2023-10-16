#!/bin/bash
echo Trim ssd
sudo fstrim -av
echo Done
read -t 3 -p "Press any key (or wait 3 seconds) ..."

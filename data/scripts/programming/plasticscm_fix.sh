#!/bin/bash
echo Applying plastic fix
sudo systemctl stop plasticscm-server
sudo systemctl start plasticscm-server
echo Done
read -t 3 -p "Press any key (or wait 3 seconds) ..."

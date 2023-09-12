#!/bin/bash
echo Installing additional codecs
sudo dnf groupupdate multimedia --setop="install_weak_deps=False" --exclude=PackageKit-gstreamer-plugin
sudo dnf groupupdate sound-and-video
echo Done
read -t 3 -p "Press any key (or wait 3 seconds) ..."

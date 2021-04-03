#! /bin/bash
#step 1 to scan a batch of duplex pages into one pdf -> scan all odd pages.

#cleanup before we start
rm /tmp/scan*.pnm 2> /dev/null

#scan odd pages into files
sleep 1 #for some reasonse scanadf fails if it is started immediatly after a key press - so just sleep one second...
scanadf --device-name="brother4:net1;dev0" --mode="True Gray" --resolution=200 -x 210 -y 297 --output-file="/tmp/scan135-%02d.pnm"

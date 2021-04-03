#! /bin/bash
#"repeate step" to scan multiple pages into one pdf -> scan page(s)

#create a filename based on the current date/time
TIMESTAMP="$(/bin/date +%s)"

#scan odd pages into files
sleep 1 #for some reasonse scanadf fails if it is started immediatly after a key press - so just sleep one second...
scanadf --device-name="brother4:net1;dev0" --mode="True Gray" --resolution=200 -x 210 -y 297 --output-file="/tmp/scan-${TIMESTAMP}-%02d.pnm"

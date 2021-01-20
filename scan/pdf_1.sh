#! /bin/bash
#create a filename based on the current date/time
DATE="$(/bin/date +%Y%m%d-%H%M%S)"
OUTPUT="/tmp/${DATE}.pdf"

#cleanup before we start
rm /tmp/scan*.pnm 2> /dev/null

#scan into files
sleep 1 #for some reasonse scanadf fails if it is started immediatly after a key press - so just sleep one second...
scanadf --device-name="brother4:net1;dev0" --mode="True Gray" --resolution=200 -x 210 -y 297 --output-file="/tmp/scan-%02d.pnm"

#convert the files into the one pdf
# this may require you to set " <policy domain="coder" rights="read | write" pattern="PDF" />" rights in /etc/ImageMagick-6/policy.xml
convert /tmp/scan*.pnm -page A4 -trim -compress jpeg -quality 75 ${OUTPUT}

#cleanup
rm /tmp/scan*.pnm

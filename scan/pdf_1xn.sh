#! /bin/bash
#scan multiple pages at once but store each page as seperte pdf

#create a filename based on the current date/time
DATE="$(/bin/date +%Y%m%d-%H%M%S)"
DESTINATION="/home/manuel/Programme/paperless-ng/consume/"

#cleanup before we start
rm /tmp/scan*.pnm 2> /dev/null

#scan into files
sleep 1 #for some reasonse scanadf fails if it is started immediatly after a key press - so just sleep one second...
scanadf --device-name="brother4:net1;dev0" --mode="True Gray" --resolution=200 -x 210 -y 297 --output-file="/tmp/scan-%02d.pnm"

#convert each file into a pdf
CNT=0
for f in /tmp/scan*.pnm
do
  CNT=$((CNT + 1))
  OUTPUT="/tmp/${DATE}-${CNT}.pdf"
  # this may require you to set " <policy domain="coder" rights="read | write" pattern="PDF" />" rights in /etc/ImageMagick-6/policy.xml
  convert $f -page A4 -trim -compress jpeg -quality 75 ${OUTPUT}
  mv ${OUTPUT} ${DESTINATION}
done


#cleanup
rm /tmp/scan*.pnm

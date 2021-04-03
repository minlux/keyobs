#! /bin/bash
#last step to scan multiple pages into one pdf -> combine all scanned pages into one pdf.

#create a filename based on the current date/time
DATE="$(/bin/date +%Y%m%d-%H%M%S)"
OUTPUT="/tmp/${DATE}.pdf"
DESTINATION="/home/manuel/Programme/paperless-ng/consume/"

#convert the files into the one pdf
# this may require you to set " <policy domain="coder" rights="read | write" pattern="PDF" />" rights in /etc/ImageMagick-6/policy.xml
convert /tmp/scan*.pnm -page A4 -trim -compress jpeg -quality 75 ${OUTPUT}
mv ${OUTPUT} ${DESTINATION}

#cleanup
rm /tmp/scan*.pnm

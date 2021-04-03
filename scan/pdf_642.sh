#! /bin/bash
#step 2 to scan a batch of duplex pages into one pdf -> scan all even pages in reverse order and create pdf

#create a filename based on the current date/time
DATE="$(/bin/date +%Y%m%d-%H%M%S)"
OUTPUT="/tmp/${DATE}.pdf"
DESTINATION="/home/manuel/Programme/paperless-ng/consume/"

#scan even pages (in reverse order) into files
sleep 1 #for some reasonse scanadf fails if it is started immediatly after a key press - so just sleep one second...
scanadf --device-name="brother4:net1;dev0" --mode="True Gray" --resolution=200 -x 210 -y 297 --output-file="/tmp/scan642-%02d.pnm"

##get list of "odd" pages into array
ODD=(/tmp/scan135-*.pnm)
ODD_CNT=${#ODD[*]}
#get list of "even" pages into array
EVEN=(/tmp/scan642-*.pnm)
EVEN_CNT=${#EVEN[*]}

#shuffle "odd" and "even" pages into desired order
SHUFFLE=()
for ((i=0; i<${ODD_CNT}; i++))
do
  SHUFFLE+=(${ODD[$i]})
  SHUFFLE+=(${EVEN[$EVEN_CNT - $i - 1]})
done

#output
convert "${SHUFFLE[@]}" -page A4 -trim -compress jpeg -quality 75 ${OUTPUT}
mv ${OUTPUT} ${DESTINATION}

#cleanup
rm /tmp/scan*.pnm

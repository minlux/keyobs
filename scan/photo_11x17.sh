#! /bin/bash
#cleanup before we start
rm /tmp/photo*.pnm 2> /dev/null

#scan into files
sleep 1 #for some reasonse scanadf fails if it is started immediatly after a key press - so just sleep one second...
scanadf --device-name="brother4:net1;dev0" --mode="24bit Color" --resolution=300 -x 168 -y 110 --output-file="/tmp/photo-%02d.pnm"

#trim image(s) and convert them files into jpg
j=1000
for i in /tmp/photo*.pnm
do
  # convert $i -fuzz 30% -trim -compress jpeg -quality 95 "/tmp/IMG${j}.jpg"
  convert $i -compress jpeg -quality 95 "/tmp/IMG${j}.jpg"
  j=$((j + 1))
done


#cleanup
rm /tmp/photo*.pnm

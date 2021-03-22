#!/bin/bash

set -e

if true; then
    echo "Creating distday csv…"
    jq -r '[.Frequency,(.Time|strptime("%Y-%m-%dT%H:%M:%S%z")|mktime),.Message] | @csv' < wxlog.json \
	| sed 's/"//g;s/,/ /g' \
	| awk '$3 == "CQ" && $4 != "DX" && length($5) == 4 { print $1 " " ($2%86400) " " $5 }' \
	| uniq -c \
	| sort -n -k3 \
	       > locationdata.txt
fi

echo "Creating distday data…"
./todistday.py 7074000 < locationdata.txt > distday40.data &
./todistday.py 14074000 < locationdata.txt > distday20.data
wait
echo "Plotting distday…"
exec ./distday.plot

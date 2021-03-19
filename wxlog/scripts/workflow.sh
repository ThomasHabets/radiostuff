#!/bin/bash
set -e

REMOTE_FILE="$1"

if [[ ! "$REMOTE_FILE" = "" ]]; then
    echo "Transferring…"
    rsync -zvP "$REMOTE_FILE" wxlog.json
fi

echo "Creating CSV…"
jq -r \
   '[.Frequency,(.Time|strptime("%Y-%m-%dT%H:%M:%S+01:00")|mktime)] | @csv' \
   wxlog.json \
    | awk -F, '{print $1 " " $2%86400}' \
    | uniq -c \
    | sort -n -k3 \
	   > daydata.txt

echo "Creating day data…"
./toavg.py 7074000 < daydata.txt > day40.data &
./toavg.py 14074000 < daydata.txt > day20.data
wait
echo "Plotting…"
exec ./day.plot

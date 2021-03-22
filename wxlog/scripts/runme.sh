#!/bin/bash
set -e

REMOTE_FILE="$1"

if [[ ! "$REMOTE_FILE" = "" ]]; then
    echo "Transferring…"
    rsync -zvP "$REMOTE_FILE" wxlog.json
fi
./mkday.sh
exec ./mkdistday.sh

#!/bin/bash
set -e

INPUT=indata/test-20210506_185818.csv.gz

#
SORTED_INPUT=${INPUT}.sorted

#
#
#
echo "Step 1: Sort the input…"
# Takes about 16min for 1.3GB gzipped input.
if [[ ! -f "${SORTED_INPUT}" ]]; then
    echo "Sort the input…"
    zcat "${INPUT}" \
	| awk -F, '{print (15*int(($1%86400)/15)) "," $0}' \
	| sort -t, -k 1,1 -n -S 500M \
	| uniq \
	| gzip -9 > "${SORTED_INPUT}"
fi

#
#
#
echo "Step 2: Generate data…"
pv "${SORTED_INPUT}" | zcat | ./todistday

#
#
#
# TODO: parallelize
echo "Step 3: Generate distance graphs…"
LOCS=$(python3 -c 'print(" ".join(["%c%c%d%d" % (ord("A")+x, ord("A")+y, a+1,b+1) for x in range(18) for y in range(18) for a in range(9) for b in range(9)]))')
for LOC in ${LOCS}; do
    FN20="out.distday/${LOC}.20"
    FN40="out.distday/${LOC}.40"
    PLOT=""
    if [[ -s ${FN20} ]]; then
	PLOT="'${FN20}' using (\$1/3600):(\$2/1000) w l title '20m'"
    fi
    if [[ -s ${FN40} ]]; then
	if [[ ! $PLOT = "" ]]; then
	    PLOT="$PLOT,"
	fi
	PLOT="$PLOT '${FN40}' using (\$1/3600):(\$2/1000) w l title '40m'"
    fi
    if [[ $PLOT = "" ]]; then
	continue
    fi
    TMPF=t.plot
    cat > "${TMPF}" <<EOF
#!/usr/bin/gnuplot
set terminal pngcairo size 1280,480
set output "out.distday/${LOC}-distance.png"
set title "Avg FT8 decode distance per time slot for maidenhead ${LOC}"
set ylabel "km"
set xlabel "Hour (UTC)"

plot [0:24] [0:] $PLOT
EOF
    gnuplot "${TMPF}"
done

#
#
#
# TODO: parallelize
echo "Step 4: Generate count graphs…"
for LOC in ${LOCS}; do
    FN20="out.distday/${LOC}.20"
    FN40="out.distday/${LOC}.40"
    PLOT=""
    if [[ -s ${FN20} ]]; then
	PLOT="'${FN20}' using (\$1/3600):3 w l title '20m'"
    fi
    if [[ -s ${FN40} ]]; then
	if [[ ! $PLOT = "" ]]; then
	    PLOT="$PLOT,"
	fi
	PLOT="$PLOT '${FN40}' using (\$1/3600):3 w l title '40m'"
    fi
    if [[ $PLOT = "" ]]; then
	continue
    fi
    TMPF=t.plot
    cat > "${TMPF}" <<EOF
#!/usr/bin/gnuplot
set terminal pngcairo size 1280,480
set output "out.distday/${LOC}-count.png"
set title "Avg FT8 decodes per time slot for maidenhead ${LOC}"
set ylabel "Decodes"
set xlabel "Hour (UTC)"

plot [0:24] [0:] $PLOT
EOF
    gnuplot "${TMPF}"
done

# If generating jpegs, run jpegoptim -m80 on the jpeg.
echo "Step 5: Converting to webm…"
(
    set -e
    cd out.distday
    for i in *.png; do
	cwebp "$i" -quiet -o "$(basename "$i" .png).webp"
    done
)

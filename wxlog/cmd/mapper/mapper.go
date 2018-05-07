package main

import (
	"bytes"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"io/ioutil"
	"os"
	"regexp"
	"time"

	"github.com/golang/geo/s2"
	log "github.com/sirupsen/logrus"

	"github.com/ThomasHabets/radiostuff/wxlog"
)

var (
	in      = flag.String("in", "", "Input file.")
	out     = flag.String("out", "", "Output HTML")
	apiKey  = flag.String("api_key", "", "Google Maps API key")
	minSeen = flag.Int("min_seen", 2, "Minimum times stations must have been seen to count")

	res = []*regexp.Regexp{
		regexp.MustCompile(`^CQ ([A-Z0-9]+) ([A-Z]{2}\d{2})$`),
		regexp.MustCompile(`^[A-Z0-9]+ ([A-Z0-9]+) ([A-Z]{2}\d{2})$`),
	}

	blacklist = map[string]bool{
		"MU6PQE": true,
	}
)

func main() {
	flag.Parse()
	if flag.NArg() > 0 {
		log.Fatalf("Trailing args on cmdline: %q", flag.Args())
	}

	f, err := os.Open(*in)
	if err != nil {
		log.Fatalf("Failed to open %q: %v", *in, err)
	}
	defer f.Close()
	dec := json.NewDecoder(f)

	type datum struct {
		Lat, Long float64
		Weight    float64
		Callsign  string
		Seen      []time.Time
	}
	var data []datum
	seenData := make(map[string]int)
	for {
		var r wxlog.JSONLog
		if err := dec.Decode(&r); err == io.EOF {
			break
		} else if err != nil {
			log.Fatal(err)
		}
		for _, reg := range res {
			m := reg.FindStringSubmatch(r.Message)
			if len(m) == 0 {
				log.Debugf("Not matching: %q", r.Message)
				continue
			}
			callsign := m[1]
			if blacklist[callsign] {
				continue
			}
			latlong := m[2]
			if latlong == "RR73" {
				// While a valid locator, it's unlikely.
				continue
			}
			lat, long, err := wxlog.Locator2LatLong(latlong)
			if err != nil {
				log.Errorf("%q is not a locator: %v", m[2], err)
				continue
			}

			key := latlong + callsign
			n, found := seenData[key]
			if !found {
				seenData[key] = len(data)
				data = append(data, datum{
					Lat:      lat,
					Long:     long,
					Weight:   1,
					Callsign: callsign,
				})
			}
			data[n].Seen = append(data[n].Seen, r.Time)
			if len(data[n].Seen) > 10 {
				data[n].Seen = data[n].Seen[len(data[n].Seen)-10:]
			}
			if true {
				sc := s2.CellIDFromLatLng(s2.LatLngFromDegrees(lat, long))
				fmt.Printf("%v (%f,%f, aka %v): %v %v\n", m[2], lat, long, sc.Parent(9).ToToken(), r.SNR, m[1])
			}
		}
	}

	// Filter out stations only seen once.
	{
		var odata []datum
		for _, d := range data {
			if len(d.Seen) < *minSeen {
				continue
			}
			odata = append(odata, d)
		}
		data = odata
	}

	var buf bytes.Buffer
	if err := tmplMap.Execute(&buf, &struct {
		APIKey string
		Data   []datum
	}{
		APIKey: *apiKey,
		Data:   data,
	}); err != nil {
		log.Fatal(err)
	}
	if err := ioutil.WriteFile(*out, buf.Bytes(), 0644); err != nil {
		log.Fatal(err)
	}
}

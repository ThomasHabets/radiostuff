package main

import (
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"os"
	"regexp"
	"time"

	log "github.com/sirupsen/logrus"

	"github.com/ThomasHabets/radiostuff/wxlog"
)

var (
	in     = flag.String("in", "", "Input file.")
	out    = flag.String("out", "", "Output KML")
	apiKey = flag.String("api_key", "", "Google Maps API key")

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
	}
	type station struct {
		Name      string
		Lat, Long float64
		Seen      []time.Time
	}
	var data []datum
	var stations []station
	seenStations := make(map[string]int)
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
			data = append(data, datum{
				Lat:  lat,
				Long: long,
				//Weight: math.Log2(float64(r.SNR + 23)),
				Weight: 1,
			})
			n, found := seenStations[callsign]
			if !found {
				n = len(stations)
				seenStations[callsign] = n
				stations = append(stations, station{
					Name: callsign,
					Lat:  lat,
					Long: long,
				})
			}
			stations[n].Seen = append(stations[n].Seen, r.Time)
			if len(stations[n].Seen) > 10 {
				stations[n].Seen = stations[n].Seen[len(stations[n].Seen)-10:]
			}
			if false {
				fmt.Printf("%v (%f,%f): %v %v\n", m[2], lat, long, r.SNR, m[1])
			}
		}
	}
	if err := tmplMap.Execute(os.Stdout, &struct {
		APIKey   string
		Data     []datum
		Stations []station
	}{
		APIKey:   *apiKey,
		Data:     data,
		Stations: stations,
	}); err != nil {
		log.Fatal(err)
	}
}

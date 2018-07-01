// wait until a given offset after a given interval and then exit.
//
// Useful to wait until 1 second past the minute to start transferring JT65.
package main

import (
	"flag"
	"log"
	"time"
)

var (
	offset   = flag.Duration("offset", time.Second, "Offset from interval multiple.")
	interval = flag.Duration("duration", 60*time.Second, "Interval size.")
)

func main() {
	flag.Parse()
	now := int64(time.Now().Unix())
	down := time.Unix(now-now%int64(interval.Seconds()), 0)
	period := time.Until(down.Add(*interval).Add(*offset))
	log.Printf("Sleeping %v", period)
	time.Sleep(period)
}

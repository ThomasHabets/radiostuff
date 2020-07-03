package main

import (
	"context"
	"flag"
	"fmt"
	"net"
	"os/exec"
	"time"

	log "github.com/sirupsen/logrus"
)

var (
	alsadev    = flag.String("alsa_dev", "1,0", "ALSA device")
	rig        = flag.String("rig", "127.0.0.1:4532", "Address of rigctld")
	startDelay = flag.Float64("start_delay", 0.2, "Time between PTT and talk")
)

func main() {
	flag.Parse()
	ctx := context.Background()

	conn, err := net.Dial("tcp", *rig)
	if err != nil {
		log.Fatal(err)
	}

	fmt.Fprintf(conn, "\\set_ptt 1\n")
	time.Sleep(time.Duration(*startDelay * float64(time.Second)))
	for _, fn := range flag.Args() {
		cmd := exec.CommandContext(ctx, "mplayer", "-ao", "alsa:device=hw="+*alsadev, fn)
		if err := cmd.Run(); err != nil {
			log.Fatal(err)
		}
	}
	fmt.Fprintf(conn, "\\set_ptt 0\n")
}

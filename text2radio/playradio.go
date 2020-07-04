package main

import (
	"bufio"
	"context"
	"flag"
	"fmt"
	"net"
	"os"
	"os/exec"
	"os/signal"
	"strings"
	"time"

	"github.com/pkg/errors"
	log "github.com/sirupsen/logrus"
)

var (
	alsadev    = flag.String("alsa_dev", "1,0", "ALSA device")
	rig        = flag.String("rig", "127.0.0.1:4532", "Address of rigctld")
	startDelay = flag.Float64("start_delay", 0.4, "Time between PTT and talk")
	power      = flag.Float64("power", -1, "Set power between 0.0 and 1.0. -1 means don't touch power.")
	mode       = flag.String("mode", "", "Set mode before TX. Blank means don't change. Allowed modes: FM,PKTFM,USB,PKTUSB,LSB,PKTLSB,D-Star")

	modes = map[string]string{
		"D-STAR": "D-STAR 1",
		"FM":     "FM 15000",
		"PKTFM":  "PKTFM 15000",
		"USB":    "USB 3000",
		"PKTUSB": "PKTUSB 3000",
		"LSB":    "LSB 3000",
		"PKTLSB": "PKTLSB 3000",
	}
)

func play(ctx context.Context, fs []string) error {
	for _, fn := range fs {
		log.Infof("Playing %s", fn)
		cmd := exec.CommandContext(ctx, "mplayer", "-ao", "alsa:device=hw="+*alsadev, fn)
		if err := cmd.Run(); err != nil {
			return errors.Wrapf(err, "playing %q", fn)
		}
	}
	return nil
}

func command(ctx context.Context, conn net.Conn, rd *bufio.Reader, cmd string) (string, error) {
	fmt.Fprintf(conn, cmd+"\n")
	l, err := rd.ReadString('\n')
	if err != nil {
		return "", err
	}
	if want := "RPRT 0\n"; l != want {
		return "", fmt.Errorf("command %q: got %q, want %q", cmd, l, want)
	}
	return l, err
}

func tx(ctx context.Context, conn net.Conn, rd *bufio.Reader, fs []string) error {
	if _, err := command(ctx, conn, rd, "\\set_ptt 1"); err != nil {
		return err
	}
	defer func() {
		if _, err := command(ctx, conn, rd, fmt.Sprintf("\\set_ptt 0")); err != nil {
			log.Errorf("Failed to turn off PTT: %v", err)
		}
	}()
	time.Sleep(time.Duration(*startDelay * float64(time.Second)))
	return play(ctx, fs)
}

func main() {
	flag.Parse()
	ctx, cancel := context.WithCancel(context.Background())

	sig := make(chan os.Signal)

	conn, err := net.Dial("tcp", *rig)
	if err != nil {
		log.Fatal("Dialing into %q: %v", *rig, err)
	}
	rd := bufio.NewReader(conn)

	if *power >= 0 {
		log.Infof("Setting power to %.2f…", *power)
		if _, err := command(ctx, conn, rd, fmt.Sprintf("L RFPOWER %.2f", *power)); err != nil {
			log.Fatal("Setting power %f: %v", *power, err)
		}
	}

	if *mode != "" {
		ms, found := modes[strings.ToUpper(*mode)]
		if !found {
			log.Fatalf("Invalid mode %q", *mode)
		}
		log.Infof("Setting mode to %q…", ms)
		if _, err := command(ctx, conn, rd, fmt.Sprintf("M %s", ms)); err != nil {
			log.Fatal("Setting mode %q: %v", *mode, err)
		}
	}

	signal.Notify(sig, os.Interrupt)
	go func() {
		<-sig
		cancel()
	}()
	if err := tx(ctx, conn, rd, flag.Args()); err != nil {
		log.Fatalf("Transmitting: %v", err)
	}
}

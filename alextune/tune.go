package main

import (
	"context"
	"os/signal"
	"syscall"
	"time"
	"flag"
	"fmt"
	"io"
	"os"
	"os/exec"
	"strings"

	log "github.com/sirupsen/logrus"
)

var (
	speed = flag.Int("speed", 38400, "")
	dev   = flag.String("dev", "/dev/ttyUSB0", "")

	displayMap = map[byte]byte{
		'>': '-',
		'<': 'l',
		'@': ' ',
		'K': 'H',
		'M': 'N',
		'Q': 'O',
		'V': 'U',
		'W': 'I',
		'X': '?', // c-bar
		'Z': 'c',
		'[': '?', // r-bar
		'\\': '?', // lambda
		']': '?', // RX/TX eq level 4
		'^': '?', // RX/TX eq level 4
	}
)

func command(f io.ReadWriter, cmd string, reply bool) (string, error) {
	_, err := f.Write([]byte(cmd))
	if err != nil {
		return "", err
	}
	if !reply {
		return "", nil
	}
	var ss []string
	for {
		b := make([]byte, 1, 1)
		n, err := f.Read(b)
		if err != nil {
			return "", err
		}
		s := string(b[:n])
		ss = append(ss, s)
		if s[len(s)-1] == ';' {
			return strings.Join(ss, ""), nil
		}
	}
}

func main() {
	flag.Parse()
	ctx := context.Background()
	if err := exec.CommandContext(ctx, "stty", "-F", *dev, "raw", fmt.Sprint(*speed)).Run(); err != nil {
		log.Fatalf("Setting terminal speed: %v", err)
	}
	f, err := os.OpenFile(*dev, os.O_RDWR|os.O_SYNC, 0)
	if err != nil {
		log.Fatalf("Opening serial port: %v", err)
	}
	defer f.Close()

	// Confirm we're using the right protocol.
	if id, err := command(f, "ID;", true);err != nil {
		log.Fatalf("Command failed: %v", err)
	} else if id != "ID017;" {
		log.Fatalf("Not protocol 17: %q", id)
	}

	// Confirm we're using a KX2.
	if id, err := command(f, "OM;", true);err != nil {
		log.Fatalf("Command failed: %v", err)
	} else if !strings.HasSuffix(id, "01;") {
		log.Fatalf("Not a KX2. OM: %q", id)
	}

	// Get old K2/K3 mode.
	oldK2, err := command(f, "K2;", true)
	if err != nil {
		log.Fatalf("Failed to get K2 mode: %v", err)
	}
	log.Printf("Old K2 mode: %q", oldK2)
	oldK3, err := command(f, "K3;", true)
	if err != nil {
		log.Fatalf("Failed to get K3 mode: %v", err)
	}
	log.Printf("Old K3 mode: %q", oldK3)

	signalCh := make(chan os.Signal)
	signal.Notify(signalCh, os.Interrupt, syscall.SIGTERM)

	// Set K2 extended mode.
	if r, err := command(f, "K22;K2;", true);err != nil {
		log.Fatalf("Command failed: %v", err)
	} else if false &&  r != "K20;" {
		log.Fatalf("Bad K20 reply: %q", r)
	}
	defer func() {
		if _, err := command(f, oldK2, false); err != nil {
			log.Fatalf("Failed to set old K2 mode %q: %v", oldK2, err)
		}
	}()

	// Set normal K3 mode.
	if r, err := command(f, "K31;K3;", true);err != nil {
		log.Fatalf("Command failed: %v", err)
	} else if false &&  r != "K30;" {
		log.Fatalf("Bad K30 reply: %q", r)
	}
	defer func() {
		if _, err := command(f, oldK3, false); err != nil {
			log.Fatalf("Failed to set old K2 mode %q: %v", oldK2, err)
		}
	}()

	// Get old power.
	oldPC, err := command(f, "PC;", true)
	if err != nil {
		log.Fatalf("Command failed: %v", err)
	}
	log.Printf("Old power: %q", oldPC)

	defer func() {
		// Set back old power.
		if _, err := command(f, oldPC, false); err != nil {
			log.Fatalf("Command failed: %v", err)
		}

		// Confirm power.
		if pc, err := command(f, "PC;", true); err != nil {
			log.Fatalf("Command failed: %v", err)
		} else if pc != oldPC {
			log.Fatalf("Power not actually set back to %q: %q", oldPC, pc)
		}
	}()

	// Get ATU status, and TX power.
	var oldATU bool
	if r, err := command(f, "DS;", true); err != nil {
		log.Fatalf("Command failed: %v", err)
	} else {
		if g,w:=len(r),13; g != w {
			log.Fatalf("DS reply wrong length. Got %d (%q), want %d.", g,r,w)
		}
		oldATU = (r[11] & 16) != 0
		log.Printf("ATU status: %v", oldATU)
	}

	// Turn off ATU and set power to 1W.
	if r, err := command(f, "MN023;MP001;MN255;PC0100;PC;", true); err != nil {
		log.Fatalf("Command failed: %v", err)
	} else {
		log.Printf("ATU off&power command return: %q", r)
	}

	// Confirm power.
	if pc, err := command(f, "PC;", true); err != nil {
		log.Fatalf("Command failed: %v", err)
	} else if pc != "PC0100;" {
		log.Fatalf("Power not actually set to 1W: %q", pc)
	}

	// Press TUNE.
	if _, err := command(f, "SWH16;", false); err != nil {
		log.Fatalf("Command failed: %v", err)
	}

	// Report SWR.
	log.Printf("Tune down SWR and press enter...\n")
	die := make(chan struct{})
	printLoopDone := make(chan struct{})
	go func() {
		defer close(printLoopDone)
		t := time.NewTicker(100*time.Millisecond)
		defer t.Stop()
		for {
			select {
			case <-die:
				fmt.Printf("\n")
				return
			case <-signalCh:
				fmt.Printf("\n")
				log.Infof("Got signal")
				return
			case <-t.C:
				o, err := command(f, "DS;", true)
				if err != nil {
					log.Fatalf("DS; failed: %v", err)
				}
				// Example: DS@@@@1³>I
				v := strings.TrimLeft(strings.TrimPrefix(o, "DS"), "@")
				out := ""
				bs := []byte(v)
				if len(bs) <3 {
					log.Errorf("Invalid DS reply: %q", o) 
				}
				for _, b := range bs[:len(bs)-3] {
					if b & 128  > 0{
						out += "."
						b &= 127
					}
					if b2, found := displayMap[b]; found {
						b = b2
					}
					out += fmt.Sprintf("%c", b)
				}
				fmt.Printf("\r                                              \rDS: %q (at %s)", out, time.Now().Format("2006-01-02 15:04:05"))
			}
		}
	}()

	// Wait for enter.
	go func() {
		defer close(die)
		b := make([]byte, 10, 10)
		if _, err := os.Stdin.Read(b); err != nil {
			log.Fatalf("Did you press ^D or something? %v", err)
		}
	}()
	<-printLoopDone
	
	// Turn off TUNE.
	if _, err := command(f, "RX;", false); err != nil {
		log.Fatalf("Command failed: %v", err)
	}

	if oldATU {
		log.Printf("Re-enabling ATU")
		time.Sleep(time.Second)
		// Turn on ATU.
		if _, err := command(f, "MN023;MP002;SWT09;", false); err != nil {
			log.Fatalf("Command failed: %v", err)
		}
	}

	if oldATU {
		// Tune ATU.
		if _, err := command(f, "SWT20;", false); err != nil {
			log.Fatalf("Command failed: %v", err)
		}
	}
	log.Infof("Done")
}

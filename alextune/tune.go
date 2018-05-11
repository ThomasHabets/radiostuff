package main

import (
	"context"
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
		b := make([]byte, 64, 64)
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
	if err := exec.CommandContext(ctx, "stty", "-F", *dev, fmt.Sprint(*speed)).Run(); err != nil {
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

	// Get old power.
	oldPC, err := command(f, "PC;", true)
	if err != nil {
		log.Fatalf("Command failed: %v", err)
	}

	// Turn off ATU and set power to 1W.
	if _, err := command(f, "MN023;MP001;SWT09;PC001;", false); err != nil {
		log.Fatalf("Command failed: %v", err)
	}

	// Confirm power.
	if pc, err := command(f, "PC;", true); err != nil {
		log.Fatalf("Command failed: %v", err)
	} else if pc != "PC001;" {
		log.Fatalf("Power not actually set to 1W: %q", pc)
	}

	// Press TUNE.
	if _, err := command(f, "SWH16;", false); err != nil {
		log.Fatalf("Command failed: %v", err)
	}

	// Report SWR.
	log.Printf("Tune down SWR and press enter...\n")
	done := make(chan struct{})
	donedone := make(chan struct{})
	go func() {
		defer close(donedone)
		t := time.NewTicker(100*time.Millisecond)
		defer t.Stop()
		for {
			select {
			case <-done:
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
				fmt.Printf("\r                                              \rDS: %q", out)
			}
		}
	}()
	
	// Wait for enter.
	{
		b := make([]byte, 10, 10)
		if _, err := os.Stdin.Read(b); err != nil {
			log.Fatalf("Did you press ^D or something? %v", err)
		}
		close(done)
	}
	<-donedone
	
	// Turn off TUNE.
	if _, err := command(f, "RX;", false); err != nil {
		log.Fatalf("Command failed: %v", err)
	}
	time.Sleep(time.Second)
	// Turn on ATU.
	if _, err := command(f, "MN023;MP002;SWT09;", false); err != nil {
		log.Fatalf("Command failed: %v", err)
	}

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

	time.Sleep(time.Second)

	// Tune ATU.
	if _, err := command(f, "SWT20;", false); err != nil {
		log.Fatalf("Command failed: %v", err)
	}
}

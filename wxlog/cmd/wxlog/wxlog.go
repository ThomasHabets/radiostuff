package main

import (
	"bytes"
	"encoding/binary"
	"encoding/json"
	"flag"
	"fmt"
	"io"
	"net"
	"os"
	"time"

	log "github.com/sirupsen/logrus"

	"github.com/ThomasHabets/radiostuff/wxlog"
)

const (
	magic  uint32 = 0xadbccbda
	schema uint32 = 2
)

var (
	addr    = flag.String("addr", ":2237", "Port to listen to")
	out     = flag.String("out", "", "JSON Output")
	comment = flag.String("comment", "", "Extra info about antenna setup etc to put in log")
)

type Packet struct {
	Magic  uint32
	Schema uint32
	Type   wxlog.PacketType

	// Decode
	ID             string
	New            bool
	Time           time.Time
	SNR            int32
	Delta          float64
	DeltaFrequency uint32
	DecodeMode     string
	Message        string
	LowConfidence  bool
	OffAir         bool

	// Status
	// ID string
	Dial         uint64
	Mode         string
	DXCall       string
	Report       string
	TXMode       string
	TXEnabled    bool
	Transmitting bool
	Decoding     bool
	RXDF         int32
	TXDF         int32
	DECall       string
	DEGrid       string
	DXGrid       string
	TXWatchdog   bool
	SubMode      string
	FastMode     bool
}

func readString(r io.Reader) (string, error) {
	var l uint32
	if err := binary.Read(r, binary.BigEndian, &l); err != nil {
		return "", err
	}
	if l == 4294967295 {
		return "", nil
	}
	b := make([]byte, l, l)
	n, err := r.Read(b)
	if n != int(l) {
		return "", fmt.Errorf("short read reading string, wanted %d, got %d", l, n)
	}
	return string(b), err
}

func readBool(r io.Reader) (bool, error) {
	var t uint8
	err := binary.Read(r, binary.BigEndian, &t)
	return t != 0, err
}

func readTime(r io.Reader) (time.Time, error) {
	var t uint32
	err := binary.Read(r, binary.BigEndian, &t)
	n := int64(time.Now().Unix())
	return time.Unix(n-(n%86400)+int64(t)/1000, int64(t)%1000), err
}

func readStatusPacket(buf *bytes.Buffer, pp *Packet) error {
	var err error

	pp.ID, err = readString(buf)
	if err != nil {
		return fmt.Errorf("reading ID: %v", err)
	}

	if err := binary.Read(buf, binary.BigEndian, &pp.Dial); err != nil {
		return err
	}

	pp.Mode, err = readString(buf)
	if err != nil {
		return fmt.Errorf("reading Mode: %v", err)
	}
	pp.DXCall, err = readString(buf)
	if err != nil {
		return fmt.Errorf("reading DXCall: %v", err)
	}
	pp.Report, err = readString(buf)
	if err != nil {
		return fmt.Errorf("reading Report: %v", err)
	}
	pp.TXMode, err = readString(buf)
	if err != nil {
		return fmt.Errorf("reading TXMode: %v", err)
	}

	pp.TXEnabled, err = readBool(buf)
	if err != nil {
		return err
	}
	pp.Transmitting, err = readBool(buf)
	if err != nil {
		return err
	}
	pp.Decoding, err = readBool(buf)
	if err != nil {
		return err
	}
	if err := binary.Read(buf, binary.BigEndian, &pp.RXDF); err != nil {
		return err
	}
	if err := binary.Read(buf, binary.BigEndian, &pp.TXDF); err != nil {
		return err
	}
	pp.DECall, err = readString(buf)
	if err != nil {
		return fmt.Errorf("reading DECall: %v", err)
	}
	pp.DEGrid, err = readString(buf)
	if err != nil {
		return fmt.Errorf("reading DEGrid: %v", err)
		return err
	}
	pp.DXGrid, err = readString(buf)
	if err != nil {
		return fmt.Errorf("reading DXGrid: %v", err)
		return err
	}
	pp.TXWatchdog, err = readBool(buf)
	if err != nil {
		return err
	}
	pp.SubMode, err = readString(buf)
	if err != nil {
		return fmt.Errorf("reading SubMode: %v", err)
		return err
	}
	pp.FastMode, err = readBool(buf)
	if err != nil {
		return err
	}
	return nil
}

func readDecodePacket(buf io.Reader, pp *Packet) error {
	var err error

	pp.ID, err = readString(buf)
	if err != nil {
		return err
	}
	pp.New, err = readBool(buf)
	if err != nil {
		return err
	}
	pp.Time, err = readTime(buf)
	if err != nil {
		return err
	}

	if err := binary.Read(buf, binary.BigEndian, &pp.SNR); err != nil {
		return err
	}

	if err := binary.Read(buf, binary.BigEndian, &pp.Delta); err != nil {
		return err
	}

	if err := binary.Read(buf, binary.BigEndian, &pp.DeltaFrequency); err != nil {
		return err
	}

	pp.DecodeMode, err = readString(buf)
	if err != nil {
		return err
	}
	pp.Message, err = readString(buf)
	if err != nil {
		return err
	}

	pp.LowConfidence, err = readBool(buf)
	if err != nil {
		return err
	}
	pp.OffAir, err = readBool(buf)
	if err != nil {
		return err
	}
	return nil
}

func decodePacket(p []byte) (*Packet, error) {
	var pp Packet
	buf := bytes.NewBuffer(p)

	if err := binary.Read(buf, binary.BigEndian, &pp.Magic); err != nil {
		return nil, err
	}
	if pp.Magic != magic {
		return nil, fmt.Errorf("incorrect magic %v", pp.Magic)
	}

	if err := binary.Read(buf, binary.BigEndian, &pp.Schema); err != nil {
		return nil, err
	}
	if pp.Schema != schema {
		return nil, fmt.Errorf("incorrect schema %v", pp.Schema)
	}

	if err := binary.Read(buf, binary.BigEndian, &pp.Type); err != nil {
		return nil, err
	}
	switch pp.Type {
	case wxlog.PacketTypeDecode:
		if err := readDecodePacket(buf, &pp); err != nil {
			return nil, err
		}
		if buf.Len() > 0 {
			log.Errorf("Bytes left in Decode packet: %v", buf.Bytes())
		}
	case wxlog.PacketTypeStatus:
		if err := readStatusPacket(buf, &pp); err != nil {
			return nil, err
		}
		if buf.Len() > 0 {
			log.Errorf("Bytes left in Status packet: %v", buf.Bytes())
		}
	}

	return &pp, nil
}

func makeJSON(remote *net.UDPAddr, deGrid string, freq uint64, mode string, pp *Packet) *wxlog.JSONLog {
	return &wxlog.JSONLog{
		Addr:           *remote,
		DEGrid:         deGrid,
		Frequency:      freq,
		Mode:           mode,
		ID:             pp.ID,
		Time:           pp.Time,
		SNR:            pp.SNR,
		Delta:          pp.Delta,
		DeltaFrequency: pp.DeltaFrequency,
		Message:        pp.Message,
		LowConfidence:  pp.LowConfidence,
		Comment:        *comment,
	}
}

func main() {
	flag.Parse()
	if flag.NArg() > 0 {
		log.Fatalf("Trailing args on cmdline: %q", flag.Args())
	}

	fo, err := os.OpenFile(*out, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0644)
	if err != nil {
		log.Fatalf("Failed to open %q: %v", *out, err)
	}
	defer fo.Close()
	fos := json.NewEncoder(fo)

	a, err := net.ResolveUDPAddr("udp", *addr)
	if err != nil {
		log.Fatalf("Failed to resolve %q: %v", *addr, err)
	}

	sock, err := net.ListenUDP("udp", a)
	if err != nil {
		log.Fatalf("Failed to listen to %#v: %v", a)
	}
	fmt.Printf("%20s %10s %6s %4s %4s %4s %s\n", "Time", "Dial", "Mode", "SNR", "DT", "Freq", "Message")
	mode := "???"
	deGrid := "???"
	var freq uint64
	for {
		b := make([]byte, 1500, 1500)
		oob := make([]byte, 1500, 1500)
		n, oobn, fl, remote, err := sock.ReadMsgUDP(b, oob)
		if err != nil {
			log.Errorf("Reading UDP packet: %v", err)
			continue
		}
		if oobn != 0 {
			log.Warningf("OOB data received: %d", oobn)
		}
		fl = fl
		p := b[:n]
		pp, err := decodePacket(p)
		if err != nil {
			log.Error(err)
		}
		switch pp.Type {
		case wxlog.PacketTypeStatus:
			mode = pp.Mode
			freq = pp.Dial
			deGrid = pp.DEGrid
		case wxlog.PacketTypeDecode:
			fmt.Printf("%v %10d %6s %4d %4.1f %4d %s\n", pp.Time.UTC().Format("2006-01-02 15:04:05Z"), freq, mode, pp.SNR, pp.Delta, pp.DeltaFrequency, pp.Message)
			if err := fos.Encode(makeJSON(remote, deGrid, freq, mode, pp)); err != nil {
				log.Fatalf("Failed to log to JSON: %v", err)
			}
		}
	}
	if err := fo.Close(); err != nil {
		log.Fatalf("Failed to close JSON output: %v", err)
	}
}

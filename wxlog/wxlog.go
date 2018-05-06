package main

import (
	"bytes"
	"encoding/binary"
	"flag"
	"fmt"
	"io"
	"net"
	"time"

	log "github.com/sirupsen/logrus"
)

type PacketType uint32

const (
	magic  uint32 = 0xadbccbda
	schema uint32 = 2

	PacketTypeHeartbeat       PacketType = 0
	PacketTypeStatus          PacketType = 1
	PacketTypeDecode          PacketType = 2
	PacketTypeClear           PacketType = 3
	PacketTypeReply           PacketType = 4
	PacketTypeQSOLogged       PacketType = 5
	PacketTypeClose           PacketType = 6
	PacketTypeReplay          PacketType = 7
	PacketTypeHaltTx          PacketType = 8
	PacketTypeFreeText        PacketType = 9
	PacketTypeWSPRDecode      PacketType = 10
	PacketTypeLocation        PacketType = 11
	PacketTypeLoggedADIF      PacketType = 12
	PacketTypeHilightCallsign PacketType = 13
)

var (
	addr = flag.String("addr", ":2237", "Port to listen to")
)

type Packet struct {
	Magic  uint32
	Schema uint32
	Type   PacketType

	// Decode
	ID             string
	New            bool
	Time           time.Time
	SNR            int32
	Delta          float64
	DeltaFrequency uint32
	Mode           string
	Message        string
	LowConfidence  bool
	OffAir         bool
}

func readString(r io.Reader) (string, error) {
	var l uint32
	if err := binary.Read(r, binary.BigEndian, &l); err != nil {
		return "", err
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

func printPacket(p []byte) error {
	var pp Packet
	buf := bytes.NewBuffer(p)

	if err := binary.Read(buf, binary.BigEndian, &pp.Magic); err != nil {
		return err
	}
	if pp.Magic != magic {
		return fmt.Errorf("incorrect magic %v", pp.Magic)
	}

	if err := binary.Read(buf, binary.BigEndian, &pp.Schema); err != nil {
		return err
	}
	if pp.Schema != schema {
		return fmt.Errorf("incorrect schema %v", pp.Schema)
	}

	if err := binary.Read(buf, binary.BigEndian, &pp.Type); err != nil {
		return err
	}
	switch pp.Type {
	case PacketTypeDecode:
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

		pp.Mode, err = readString(buf)
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
		if buf.Len() > 0 {
			log.Errorf("Bytes left in Decode packet: %v", buf.Bytes())
		}
		fmt.Printf("%v %4d %4.1f %4d %s\n", pp.Time.UTC().Format("2006-01-02 15:04:05Z"), pp.SNR, pp.Delta, pp.DeltaFrequency, pp.Message)
	}

	return nil
}

func main() {
	flag.Parse()

	a, err := net.ResolveUDPAddr("udp", *addr)
	if err != nil {
		log.Fatalf("Failed to resolve %q: %v", *addr, err)
	}

	sock, err := net.ListenUDP("udp", a)
	if err != nil {
		log.Fatalf("Failed to listen to %#v: %v", a)
	}
	fmt.Printf("%20s %4s %4s %4s %s\n", "Time", "SNR", "DT", "Freq", "Message")
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
		remote = remote
		fl = fl
		p := b[:n]
		if err := printPacket(p); err != nil {
			log.Error(err)
		}
	}
}

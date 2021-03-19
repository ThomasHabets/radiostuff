package wxlog

import (
	"fmt"
	"net"
	"regexp"
	"time"
)

type PacketType uint32

const (
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

type JSONLog struct {
	Addr           net.UDPAddr
	DEGrid         string
	Frequency      uint64
	Mode           string
	ID             string
	Time           time.Time
	SNR            int32
	Delta          float64
	DeltaFrequency uint32
	Message        string
	LowConfidence  bool
	ConfigName     string
	TxMessage      string
	Comment        string

	Callsign string
	Power    int32
	Grid     string
}

var locatorRE = regexp.MustCompile(`^[A-Z]{2}[0-9]{2}$`)

func Locator2LatLong(s string) (float64, float64, error) {
	if !locatorRE.MatchString(s) {
		return 0, 0, fmt.Errorf("code only supports maidenhead of length 4")
	}
	lng := 2.0*float64((s[0]-'A')*10+(s[2]-'0')) - 180
	lat := 1.0*float64((s[1]-'A')*10+(s[3]-'0')) - 90
	return float64(lat), float64(lng), nil
}

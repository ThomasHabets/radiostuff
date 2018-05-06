package wxlog

import (
	"net"
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
	Comment        string
}


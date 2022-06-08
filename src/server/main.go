package main

import (
	"strconv"
	"strings"

	"e.coding.net/xverse-git/xmedia/xmit-analysis/xmit"
)

var (
	address = "175.178.27.150"
	demo    xmit.DemoServer
)

func IPV4ToUint(ip string) uint32 {
	ip = strings.TrimSpace(ip)
	bits := strings.Split(ip, ".")
	if len(bits) != 4 {
		return 0
	}

	b0, _ := strconv.Atoi(bits[0])
	b1, _ := strconv.Atoi(bits[1])
	b2, _ := strconv.Atoi(bits[2])
	b3, _ := strconv.Atoi(bits[3])
	if b0 > 255 || b1 > 255 || b2 > 255 || b3 > 255 {
		return 0
	}

	var sum uint32

	sum += uint32(b0) << 24
	sum += uint32(b1) << 16
	sum += uint32(b2) << 8
	sum += uint32(b3)

	return sum
}

func main() {
	demo.Start(IPV4ToUint(address))
}

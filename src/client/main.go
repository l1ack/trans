package main

import (
	"strconv"
	"strings"

	zmq "github.com/pebbe/zmq4"

	"fmt"
	"os"
)

var (
	address = "127.0.0.1"
	demo    DemoClient
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
	if len(os.Args) < 2 {
		fmt.Printf("I: syntax: %s <endpoint>\n", os.Args[0])
		return
	}
	server, _ := zmq.NewSocket(zmq.REP)
	server.Bind(os.Args[1])
        msg, err := server.RecvMessage(0)
	if err != nil {
	}
	fmt.Println("%s", msg)
        
	fmt.Println("I: echo service is ready at", os.Args[1])
//	for {
//		msg, err := server.RecvMessage(0)
//		if err != nil {
//			break //  Interrupted
//		}
//		server.SendMessage(msg)
//	}
	// fmt.Println("W: interrupted")
	demo.Start(IPV4ToUint(msg[0]))
}

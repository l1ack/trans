package main

import (
	"fmt"
	"math/rand"
	"time"
	"unsafe"

	"github.com/pion/randutil"

	xmit "e.coding.net/xverse-git/xmedia/xmit-lib/go"
)

type (
	DemoClient struct {
		connId        []byte
		client        *xmit.Client
		callbacks     *xmit.ClientCallbacks
		serverAddress uint32
		dataStream    *xmit.ClientStream
		ctrlStream    *xmit.ClientStream
	}
)

func (demo *DemoClient) recvHandler(msgType uint16, data []byte) {
	switch msgType {
	case 1:
		demo.ctrlStream.Send(data)
		xmit.Stop()
		fmt.Println("Close Connection")
	case 2:
		seq := *(*uint64)(unsafe.Pointer(&data[0]))
		fmt.Println("Msg: (", seq, ", TYPE: ", msgType, ", SIZE: ", len(data), ") Echoed")
	}
}

func (demo *DemoClient) connectedHandler() {
	fmt.Println("Connected!")
}

func (demo *DemoClient) Start(serverAddress uint32) {
	rand.Seed(time.Now().UTC().UnixNano())
	demo.serverAddress = serverAddress
	demo.client, _ = xmit.GetClient(xmit.Transport_kUdp)
	demo.callbacks = &xmit.ClientCallbacks{
		OnMessage:   demo.recvHandler,
		OnConnected: demo.connectedHandler,
	}

	//, interval int32, seq_size int

	xmit.Start()

	xmit.SignalServerAddress = serverAddress
	offer, err := demo.client.Offer(demo.callbacks)
	if err == nil {
		fmt.Println("OFFER:", string(offer))
	} else {
		fmt.Println("OFFER ERR:", err)
		return
	}

	xmit.GetSignal().SendOffer(offer)
	answer := xmit.GetSignal().RecvAnswer()
	err = demo.client.Answer(answer)
	if err != nil {
		fmt.Println("ANSWER ERR:", err)
		return
	}

	demo.ctrlStream, _ = demo.client.NewStream(1, 3, 1, 0)
	demo.dataStream, _ = demo.client.NewStream(2, 3, 1, 0)

	seq := uint64(0)
	for range time.NewTicker(3 * time.Second).C {
		message := RandSeq(rand.Intn(11111))

		// Send the message as text
		data := []byte(message)
		*(*uint64)(unsafe.Pointer(&data[0])) = seq
		seq++
		if err = demo.dataStream.Send(data); err != nil {
			fmt.Println("SEND ERR:", err)
		}
	}

	xmit.Join()
}

func RandSeq(n int) string {
	val, err := randutil.GenerateCryptoRandomString(n, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
	if err != nil {
		fmt.Println("RAND ERR:", err)
	}

	return val
}

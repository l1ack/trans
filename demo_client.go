package xmit

import (
	"fmt"
	"math/rand"
	"time"
	"unsafe"

	"github.com/pion/randutil"
)

type (
	DemoClient struct {
		connId        []byte
		client        *Client
		callbacks     *ClientCallbacks
		serverAddress uint32
		dataStream    *ClientStream
		ctrlStream    *ClientStream
	}
)

func (demo *DemoClient) recvHandler(msgType uint16, data []byte) {
	switch msgType {
	case 1:
		demo.ctrlStream.Send(data)
		Stop()
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
	demo.client, _ = GetClient(Transport_kWdc)
	demo.callbacks = &ClientCallbacks{
		OnMessage:   demo.recvHandler,
		OnConnected: demo.connectedHandler,
	}

	Start()

	signalServerAddress = serverAddress
	offer, err := demo.client.Offer(demo.callbacks)
	if err == nil {
		fmt.Println("OFFER:", string(offer))
	} else {
		fmt.Println("OFFER ERR:", err)
		return
	}

	getSignal().sendOffer(offer)
	answer := getSignal().recvAnswer()
	err = demo.client.Answer(answer)
	if err != nil {
		fmt.Println("ANSWER ERR:", err)
		return
	}

	demo.ctrlStream, _ = demo.client.NewStream(1, 3, 1, 0)
	demo.dataStream, _ = demo.client.NewStream(2, 3, 1, 0)

	seq := uint64(0)
	for range time.NewTicker(2 * time.Second).C {
		message := RandSeq(rand.Intn(11111))

		// Send the message as text
		data := []byte(message)
		*(*uint64)(unsafe.Pointer(&data[0])) = seq
		seq++
		if err = demo.dataStream.Send(data); err != nil {
			fmt.Println("SEND ERR:", err)
		}
	}

	Join()
}

func RandSeq(n int) string {
	val, err := randutil.GenerateCryptoRandomString(n, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ")
	if err != nil {
		fmt.Println("RAND ERR:", err)
	}

	return val
}

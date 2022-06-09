package main

import (
	"fmt"
	"io"
	"net/http"
	"unsafe"

	"github.com/google/uuid"

	xmit "e.coding.net/xverse-git/xmedia/xmit-lib/go"
)

type (
	DemoServer struct {
		server        *xmit.Server
		callbacks     *xmit.ServerCallbacks
		serverAddress uint32
		dataStream    *xmit.ServerStream
		ctrlStream    *xmit.ServerStream
	}
)

func (demo *DemoServer) signalHttp(rsp http.ResponseWriter, req *http.Request) {
	req.Close = true
	offer, _ := io.ReadAll(req.Body)

	rsp.Header().Set("Content-Type", "application/octet-stream")
	rsp.Header().Set("Access-Control-Allow-Origin", "*")
	rsp.Header().Set("Access-Control-Allow-Credentials", "true")
	rsp.Header().Set("Access-Control-Allow-Methods", "GET,HEAD,OPTIONS,POST,PUT")
	rsp.Header().Set("Access-Control-Allow-Headers", "Access-Control-Allow-Headers, Origin,Accept, X-Requested-With, Content-Type, Access-Control-Request-Method, Access-Control-Request-Headers")

	if _, err := rsp.Write(demo.Accept(offer)); err != nil {
		fmt.Println("HTTP ERR:", err)
	}
}

func (demo *DemoServer) echoHandler(connId []byte, msgType uint16, data []byte) {
	switch msgType {
	case 1:
		demo.ctrlStream.Send(data)
		demo.server.Close(connId)
		fmt.Println("Close Connection:", string(connId))
	case 2:
		demo.dataStream.Send(data)
		seq := *(*uint64)(unsafe.Pointer(&data[0]))
		fmt.Println("Msg: (", seq, ", TYPE: ", msgType, ", SIZE: ", len(data), ") Echoed")
	}
}

func (demo *DemoServer) connectedHandler(connId []byte) {
	fmt.Println("Connected!")
}

func (demo *DemoServer) Start(serverAddress uint32) {
	http.HandleFunc("/signal", demo.signalHttp)
	go http.ListenAndServe(":51913", nil)

	demo.serverAddress = serverAddress
	demo.server, _ = xmit.GetServer(serverAddress, 0, 0)
	demo.callbacks = &xmit.ServerCallbacks{
		OnMessage:   demo.echoHandler,
		OnConnected: demo.connectedHandler,
	}

	xmit.Start()

	xmit.SignalServerAddress = serverAddress
	go func(demo *DemoServer) {
		for {
			xmit.GetSignal().SendAnswer(demo.Accept(xmit.GetSignal().RecvOffer()))
		}
	}(demo)

	xmit.Join()
}

func (demo *DemoServer) Accept(offer []byte) (answer []byte) {
	uuid := uuid.New()
	connIdArray := [len(uuid)]byte(uuid)
	connId := connIdArray[:]
	answer, err := demo.server.Accept(connId, offer, demo.callbacks)
	if err != nil {
		fmt.Println("ERROR: ", err)
	}
	demo.ctrlStream, _ = demo.server.NewStream(connId, 1, xmit.Mode_kSemiReliableOrdered, xmit.Priority_kNormal, 0)
	demo.dataStream, _ = demo.server.NewStream(connId, 2, xmit.Mode_kSemiReliableOrdered, xmit.Priority_kNormal, 0)
	return answer
}

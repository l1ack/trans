package xmit

import (
	"fmt"
	"io"
	"net/http"
	"unsafe"

	"github.com/google/uuid"
)

type (
	DemoServer struct {
		server        *Server
		callbacks     *ServerCallbacks
		serverAddress uint32
		dataStream    *ServerStream
		ctrlStream    *ServerStream
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
	demo.server, _ = GetServer(serverAddress, 0, 0)
	if demo.server.rawServer == nil {
		fmt.Println("fail to Init Xmit Server")
	}
	demo.callbacks = &ServerCallbacks{
		OnMessage:   demo.echoHandler,
		OnConnected: demo.connectedHandler,
	}

	Start()

	signalServerAddress = serverAddress
	go func(demo *DemoServer) {
		for {
			getSignal().sendAnswer(demo.Accept(getSignal().recvOffer()))
		}
	}(demo)

	Join()
}

func (demo *DemoServer) Accept(offer []byte) (answer []byte) {
	uuid := uuid.New()
	connIdArray := [len(uuid)]byte(uuid)
	connId := connIdArray[:]
	answer, err := demo.server.Accept(connId, offer, demo.callbacks)
	if err != nil {
		fmt.Println("ERROR: ", err)
	}
	demo.ctrlStream, _ = demo.server.NewStream(connId, 1, Mode_kSemiReliableOrdered, Priority_kNormal, 0)
	demo.dataStream, _ = demo.server.NewStream(connId, 2, Mode_kSemiReliableOrdered, Priority_kNormal, 0)
	return answer
}
package xmit

/*
#cgo CFLAGS: -I${SRCDIR}
#cgo LDFLAGS: -L${SRCDIR} -lxmit -lglog -lprotobuf -lzmq -lstdc++
#include <stdlib.h>
#include "bridge.h"
*/
import "C"
import (
	"encoding/binary"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net"
	"sync"
	"unsafe"

	"github.com/pion/webrtc/v3"
)

type (
	ServerOnMessage       func(connId []byte, msgType uint16, data []byte)
	ServerOnConnected     func(connId []byte)
	ServerOnInterrupted   func(connId []byte, intervalMs int)
	ServerOnFailtoconnect func(connId []byte)

	Server struct {
		PublicAddress uint32
		LocalPort     uint16
		conns         map[uint16]*ServerConn
		udpListener   *net.UDPConn
		webrtc        ServerWebrtc
		lock          sync.Mutex
		recvMsg       ServerMsg
		rawServer     unsafe.Pointer
	}

	ServerCallbacks struct {
		OnMessage       ServerOnMessage
		OnConnected     ServerOnConnected
		OnInterrupted   ServerOnInterrupted
		OnFailtoconnect ServerOnFailtoconnect
	}

	ServerConn struct {
		connId         []byte
		callbacks      *ServerCallbacks
		peerConnection *webrtc.PeerConnection
	}

	ServerStream struct {
		Type       uint16
		Mode       Mode
		Priority   Priority
		LifetimeMs int
		server     *Server
		rawStream  unsafe.Pointer
	}

	ServerMsg struct {
		Type   uint16
		Size   C.int
		Data   *C.char
		cnid   uint16
		rawMsg unsafe.Pointer
	}

	ServerWebrtc struct {
		api           *webrtc.API
		config        webrtc.Configuration
		settingEngine webrtc.SettingEngine
		sender        map[uint16]io.ReadWriteCloser
		recver        unsafe.Pointer
		buffers       [300000][1500]byte // TODO ReF
		buffer_index  int
	}
)

var (
	serverMap         map[uint32]*Server
	serverLock        sync.Mutex
	kDefaultPortStart uint16 = 10913
	kDefaultPortEnd   uint16 = 60000
	kMaxAnswerSize           = 8192
)

func GetServer(publicAddress uint32, portStart uint16, portEnd uint16) (server *Server, err error) {
	if publicAddress == 0 {
		err = errors.New("invalid public address")
		return
	}
	if portStart == 0 {
		portStart = kDefaultPortStart
	}
	if portEnd == 0 {
		portEnd = kDefaultPortEnd
	}

	serverLock.Lock()
	defer serverLock.Unlock()

	if serverMap == nil {
		serverMap = make(map[uint32]*Server)
	} else {
		server = serverMap[publicAddress]
		if server != nil {
			return
		}
	}

	recvMsg := ServerMsg{
		rawMsg: C.XmitInitRecvmsg(getCore(), unsafe.Pointer(nil)),
	}
	server = &Server{
		PublicAddress: publicAddress,
		conns:         make(map[uint16]*ServerConn),
		recvMsg:       recvMsg,
	}

	server.udpListener, err = net.ListenUDP("udp4", &net.UDPAddr{
		IP:   net.IP{0, 0, 0, 0},
		Port: 20913,
	})
	if err != nil {
		fmt.Println("UDP LISTEN ERR:", err)
		return
	}

	server.webrtc.settingEngine.SetICEUDPMux(webrtc.NewICEUDPMux(nil, server.udpListener))
	server.webrtc.settingEngine.SetLite(true)
	server.webrtc.settingEngine.DetachDataChannels()
	ip := make(net.IP, 4)
	binary.BigEndian.PutUint32(ip, publicAddress)
	ips := []string{ip.String()}
	server.webrtc.settingEngine.SetNAT1To1IPs(ips, webrtc.ICECandidateTypeHost)
	server.webrtc.api = webrtc.NewAPI(webrtc.WithSettingEngine(server.webrtc.settingEngine))
	server.rawServer = C.XmitServer(getCore(), C.uint(publicAddress), C.ushort(portStart), C.ushort(portEnd), (*C.char)(unsafe.Pointer(server))) // TODO(Qi) not safe
	if server.rawServer == nil {
		fmt.Println("fail to Init Server")
	}
	server.webrtc.sender = make(map[uint16]io.ReadWriteCloser)
	server.webrtc.recver = C.XmitWdcServer(server.rawServer, (*C.char)(unsafe.Pointer(&server.webrtc)))

	serverMap[publicAddress] = server

	return
}

func (server *Server) Accept(connId []byte, offer []byte, callbacks *ServerCallbacks) (answer []byte, err error) {
	if server.rawServer == nil {
		err = errors.New("server uninitialized")
		return
	}
	if connId == nil || len(connId) == 0 {
		err = errors.New("connId is empty")
		return
	}
	if callbacks == nil {
		err = errors.New("callbacks is nil")
		return
	}
	if callbacks.OnMessage == nil {
		err = errors.New("OnMessage is nil")
		return
	}

	conn := &ServerConn{
		connId:    connId,
		callbacks: callbacks,
	}
	var (
		cnid      uint16
		carry     []byte
		carryData *C.char
		carrySize C.int
	)
	var transport Transport // TODO use pb value
	if len(offer) > 1 {
		fmt.Println("Xmit WebRTC DataChannel")
		transport = Transport_kWdc
		var (
			offerSdp  webrtc.SessionDescription
			answerSdp webrtc.SessionDescription
		)
		conn.peerConnection, err = server.webrtc.api.NewPeerConnection(server.webrtc.config)
		conn.peerConnection.OnDataChannel(func(dc *webrtc.DataChannel) {
			dcid := *dc.ID()
			dc.OnOpen(func() {
				wdc, err := dc.Detach()
				if err != nil {
					fmt.Println("DC DETACH ERROR:", err)
				}
				fmt.Println("DC OPEN", *dc.ID())
				server.webrtc.sender[dcid] = wdc

				go server.recv(wdc, dcid)
			})
		})

		// conn.peerConnection.OnICECandidate(func(c *webrtc.ICECandidate) {
		//	fmt.Println("OnICECandidate: ", c)
		// })
		conn.peerConnection.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
			fmt.Printf("Peer Connection State has changed: %s\n", state.String())
		})

		if err = json.Unmarshal(offer, &offerSdp); err != nil {
			return
		}
		if err = conn.peerConnection.SetRemoteDescription(offerSdp); err != nil {
			return
		}
		answerSdp, err = conn.peerConnection.CreateAnswer(nil)
		if err != nil {
			return
		}
		if err = conn.peerConnection.SetLocalDescription(answerSdp); err != nil {
			return
		}
		carry, err = json.Marshal(answerSdp)
		if err != nil {
			return
		}
		carryData = (*C.char)(unsafe.Pointer(&carry[0]))
		carrySize = C.int(len(carry))
	} else {
		transport = Transport_kUdp
		fmt.Println("Accept Xmit UDP")
	}

	answerData := make([]byte, kMaxAnswerSize)
	answerSize := C.XmitServerAccept(server.rawServer, C.int(transport),
		(*C.char)(unsafe.Pointer(&connId[0])), C.int(len(connId)), carryData, carrySize,
		(*C.char)(unsafe.Pointer(&answerData[0])), C.int(len(answerData)), (*C.ushort)(unsafe.Pointer(&cnid)))
	answer = answerData[:answerSize]

	server.lock.Lock()
	server.conns[cnid] = conn
	server.lock.Unlock()
	return
}

func (server *Server) recv(reader io.Reader, dcid uint16) {
	for {
		buffer := server.webrtc.buffers[server.webrtc.buffer_index]
		server.webrtc.buffer_index++ // TODO ReF
		if server.webrtc.buffer_index == len(server.webrtc.buffers) {
			server.webrtc.buffer_index = 0
		}
		n, err := reader.Read(buffer[300:])
		if err != nil {
			fmt.Println("DC READ ERR:", err)
			return
		}
		C.XmitWdcOnRecv(server.webrtc.recver, (*C.char)(unsafe.Pointer(&buffer[300])), C.int(n), C.ushort(dcid))
	}
}

func (server *Server) Close(connId []byte) (err error) {
	if server.rawServer == nil {
		err = errors.New("server uninitialized")
		return
	}
	if connId == nil || len(connId) == 0 {
		err = errors.New("connId is empty")
		return
	}

	C.XmitServerClose(server.rawServer, (*C.char)(unsafe.Pointer(&connId[0])), C.int(len(connId)))
	return
}

// MUST do Accept first
func (server *Server) NewStream(connId []byte, msgType uint16, mode Mode, priority Priority, lifetimeMs int) (stream *ServerStream, err error) {
	if connId == nil || len(connId) == 0 {
		err = errors.New("connId is empty")
		return
	}

	stream = &ServerStream{
		Type:       msgType,
		Mode:       mode,
		Priority:   priority,
		LifetimeMs: lifetimeMs,
		server:     server,
		rawStream:  C.XmitStream(getCore(), C.ushort(msgType), C.int(mode), C.int(priority), C.int(lifetimeMs)),
	}
	C.XmitServerBind(server.rawServer, stream.rawStream, (*C.char)(unsafe.Pointer(&connId[0])), C.int(len(connId)))

	return
}

func (stream *ServerStream) AllocateMsgToSend(size int) (sendMsg *ServerMsg) {
	sendMsg = &ServerMsg{
		Size: C.int(size),
	}
	sendMsg.rawMsg = C.XmitInitSendmsgAllocBuffer(stream.rawStream, &sendMsg.Data, C.int(sendMsg.Size))

	return
}

func (stream *ServerStream) Send(data []byte) (err error) {
	if len(data) == 0 {
		err = errors.New("data is empty")
	}

	C.XmitServerSend(stream.server.rawServer, C.XmitInitSendmsg(stream.rawStream, (*C.char)(unsafe.Pointer(&data[0])), C.int(len(data))))

	return
}

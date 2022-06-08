package xmit

/*
#cgo CFLAGS: -I${SRCDIR}
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
	"unsafe"

	"github.com/pion/webrtc/v3"
)

type (
	ClientOnMessage       func(msgType uint16, data []byte)
	ClientOnConnected     func()
	ClientOnInterrupted   func(intervalMs int)
	ClientOnFailtoconnect func()

	Client struct {
		TimeoutMs int
		transport Transport
		connId    []byte
		webrtc    ClientWebrtc
		recvMsg   ClientMsg
		callbacks *ClientCallbacks
		rawClient unsafe.Pointer
	}

	ClientCallbacks struct {
		OnMessage       ClientOnMessage
		OnConnected     ClientOnConnected
		OnInterrupted   ClientOnInterrupted
		OnFailtoconnect ClientOnFailtoconnect
	}

	ClientStream struct {
		Type       uint16
		Mode       int
		Priority   int
		LifetimeMs int
		client     *Client
		rawStream  unsafe.Pointer
	}

	ClientMsg struct {
		Type   uint16
		Size   C.int
		Data   *C.char
		cnid   uint16
		rawMsg unsafe.Pointer
	}

	ClientWebrtc struct {
		api            *webrtc.API
		config         webrtc.Configuration
		settingEngine  webrtc.SettingEngine
		candidate      webrtc.ICECandidate
		peerConnection *webrtc.PeerConnection
		dataChannel    *webrtc.DataChannel
		sender         io.ReadWriteCloser
		recver         unsafe.Pointer
		cache          [][]byte
		buffers        [30000][1500]byte // TODO ReF
		buffer_index   int
	}
)

var (
	kDefaultTimeoutMs = 60000
	kMaxOfferSize     = 8192
)

func GetClient(transport Transport) (client *Client, err error) {
	recvMsg := ClientMsg{
		rawMsg: C.XmitInitRecvmsg(getCore(), unsafe.Pointer(nil)),
	}

	client = &Client{
		TimeoutMs: kDefaultTimeoutMs,
		transport: transport,
		recvMsg:   recvMsg,
	}
	client.rawClient = C.XmitClient(getCore(), C.int(client.transport), (*C.char)(unsafe.Pointer(client))) // TODO(Qi) not safe

	client.webrtc.settingEngine.SetLite(true)
	client.webrtc.settingEngine.DetachDataChannels()
	client.webrtc.api = webrtc.NewAPI(webrtc.WithSettingEngine(client.webrtc.settingEngine))
	client.webrtc.candidate = webrtc.ICECandidate{
		Foundation: "XmitPionWebRTCClient",
		Priority:   ^uint32(0),
		Protocol:   webrtc.ICEProtocolUDP,
		Port:       20913,
		Typ:        webrtc.ICECandidateTypeHost,
		Component:  1,
	}
	client.webrtc.peerConnection, err = client.webrtc.api.NewPeerConnection(client.webrtc.config)
	if err != nil {
		fmt.Println("PC ERR:", err)
	}
	client.webrtc.dataChannel, err = client.webrtc.peerConnection.CreateDataChannel("", nil)
	if err != nil {
		fmt.Println("DC ERR:", err)
	}
	client.webrtc.dataChannel.OnOpen(func() {
		wdc, err := client.webrtc.dataChannel.Detach()
		if err != nil {
			fmt.Println("DC DETACH ERROR:", err)
		}
		fmt.Println("DC OPEN")
		client.webrtc.sender = wdc
		for _, data := range client.webrtc.cache {
			wdc.Write(data)
		}
		client.webrtc.cache = nil

		go client.recv(wdc, 0)
	})

	client.webrtc.peerConnection.OnConnectionStateChange(func(state webrtc.PeerConnectionState) {
		fmt.Printf("Peer Connection State has changed: %s\n", state.String())
	})
	client.webrtc.recver = C.XmitWdcClient(client.rawClient, (*C.char)(unsafe.Pointer(&client.webrtc)))

	return
}

func (client *Client) recv(reader io.Reader, dcid uint16) {
	for {
		buffer := client.webrtc.buffers[client.webrtc.buffer_index]
		client.webrtc.buffer_index++ // TODO ReF
		if client.webrtc.buffer_index == len(client.webrtc.buffers) {
			client.webrtc.buffer_index = 0
		}
		n, err := reader.Read(buffer[300:])
		if err != nil {
			fmt.Println("DC READ ERR:", err)
			return
		}
		C.XmitWdcOnRecv(client.webrtc.recver, (*C.char)(unsafe.Pointer(&buffer[300])), C.int(n), C.ushort(dcid))
	}
}

func (client *Client) SetTimeoutMs(timeoutMs int) {
	client.TimeoutMs = timeoutMs
}

func (client *Client) Offer(callbacks *ClientCallbacks) (offer []byte, err error) {
	if client.rawClient == nil {
		err = errors.New("client uninitialized")
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
	client.callbacks = callbacks

	switch client.transport {
	case Transport_kUdp:
		offer = []byte{0}
		return
	case Transport_kWdc:
		var offerSdp webrtc.SessionDescription
		offerSdp, err = client.webrtc.peerConnection.CreateOffer(nil)
		if err != nil {
			return
		}
		if err = client.webrtc.peerConnection.SetLocalDescription(offerSdp); err != nil {
			return
		}
		offer, err = json.Marshal(offerSdp)
		if err != nil {
			return
		}
		return
	}
	return
}

func (client *Client) Answer(answer []byte) (err error) {
	if client.rawClient == nil {
		err = errors.New("client uninitialized")
		return
	}

	var (
		address   uint32
		transport Transport
	)
	answer = answer[:C.XmitClientAccept(client.rawClient, (*C.int)(&transport), (*C.char)(unsafe.Pointer(&answer[0])), C.int(len(answer)), (*C.uint)(&address), nil)]
	// answer = answer[:C.XmitClientAccept(client.rawClient, (*C.int)(&transport), (*C.char)(unsafe.Pointer(&answer[0])), C.int(len(answer)), (*C.uint)(&address), (*C.ushort)(&client.webrtc.candidate.Port))]
	switch transport {
	case Transport_kUdp:
		return
	case Transport_kWdc:
		fmt.Println("ANSWER:", string(answer))
		ip := make(net.IP, 4)
		binary.BigEndian.PutUint32(ip, address)
		client.webrtc.candidate.Address = ip.String()
		fmt.Println("candidate:", client.webrtc.candidate)
		var answerSdp webrtc.SessionDescription
		if err = json.Unmarshal(answer, &answerSdp); err != nil {
			fmt.Println("SDP ERR:", err)
			return
		}
		if err = client.webrtc.peerConnection.SetRemoteDescription(answerSdp); err != nil {
			fmt.Println("SET SDP ERR:", err)
			return
		}
		fmt.Println("remote SDP: ", client.webrtc.peerConnection.RemoteDescription())
		if err = client.webrtc.peerConnection.AddICECandidate(client.webrtc.candidate.ToJSON()); err != nil {
			fmt.Println("CANDIDATE ERR:", err)
		}
	}
	return
}

// MUST do Answer first
func (client *Client) NewStream(msgType uint16, mode int, priority int, lifetimeMs int) (stream *ClientStream, err error) {
	stream = &ClientStream{
		Type:       msgType,
		Mode:       mode,
		Priority:   priority,
		LifetimeMs: lifetimeMs,
		client:     client,
		rawStream:  C.XmitStream(getCore(), C.ushort(msgType), C.int(mode), C.int(priority), C.int(lifetimeMs)),
	}

	C.XmitClientBind(client.rawClient, stream.rawStream)
	return
}

func (stream *ClientStream) AllocateMsgToSend(size int) (sendMsg *ClientMsg) {
	sendMsg = &ClientMsg{
		Size: C.int(size),
	}
	sendMsg.rawMsg = C.XmitInitSendmsgAllocBuffer(stream.rawStream, &sendMsg.Data, C.int(sendMsg.Size))

	return
}

func (stream *ClientStream) Send(data []byte) (err error) {
	if len(data) == 0 {
		err = errors.New("data is empty")
	}
	if stream.client.rawClient == nil {
		err = errors.New("client uninitialized")
		return
	}
	if stream.rawStream == nil {
		err = errors.New("stream uninitialized")
		return
	}

	C.XmitClientSend(stream.client.rawClient, C.XmitInitSendmsg(stream.rawStream, (*C.char)(unsafe.Pointer(&data[0])), C.int(len(data))))

	return
}

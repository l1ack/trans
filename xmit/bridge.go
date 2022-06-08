package xmit

import "C"
import (
	"fmt"
	"unsafe"
)

//export goOnLog
func goOnLog(log *C.char) {
	fmt.Printf("Xmit LOG: %s", log)
}

//export goServerOnMessage
func goServerOnMessage(visitor unsafe.Pointer, data unsafe.Pointer, size C.int, msgType C.ushort, cnid C.ushort) {
	conn := getServerConn(visitor, cnid)
	if conn.callbacks.OnMessage == nil {
		return
	}
	conn.callbacks.OnMessage(conn.connId, uint16(msgType), C.GoBytes(data, size))
}

//export goServerOnConnected
func goServerOnConnected(visitor unsafe.Pointer, cnid C.ushort) {
	conn := getServerConn(visitor, cnid)
	if conn.callbacks.OnConnected == nil {
		return
	}
	conn.callbacks.OnConnected(conn.connId)
}

//export goServerOnInterrupted
func goServerOnInterrupted(visitor unsafe.Pointer, cnid C.ushort, intervalMs C.int) {
	conn := getServerConn(visitor, cnid)
	if conn.callbacks.OnInterrupted == nil {
		return
	}
	conn.callbacks.OnInterrupted(conn.connId, int(intervalMs))
}

//export goServerOnFailtoconnect
func goServerOnFailtoconnect(visitor unsafe.Pointer, cnid C.ushort) {
	conn := getServerConn(visitor, cnid)
	if conn.callbacks.OnFailtoconnect == nil {
		return
	}
	conn.callbacks.OnFailtoconnect(conn.connId)
}

func getServerConn(visitor unsafe.Pointer, cnid C.ushort) *ServerConn {
	server := (*Server)(visitor)
	server.lock.Lock()
	defer server.lock.Unlock()
	return server.conns[uint16(cnid)]
}

//export goClientOnMessage
func goClientOnMessage(visitor unsafe.Pointer, data unsafe.Pointer, size C.int, msgType C.ushort, cnid C.ushort) {
	(*Client)(visitor).callbacks.OnMessage(uint16(msgType), C.GoBytes(data, size))
}

//export goClientOnConnected
func goClientOnConnected(visitor unsafe.Pointer) {
}

//export goClientOnInterrupted
func goClientOnInterrupted(visitor unsafe.Pointer, intervalMs C.int) {
}

//export goClientOnFailtoconnect
func goClientOnFailtoconnect(visitor unsafe.Pointer) {
}

//export goWdcOnSend
func goWdcOnSend(sender unsafe.Pointer, buff *C.char, size C.int, dcid C.ushort) {
	data := C.GoBytes(unsafe.Pointer(buff), size)
	if dcid == 0 {
		webrtc := (*ClientWebrtc)(sender)
		if webrtc.sender == nil {
			webrtc.cache = append(webrtc.cache, data)
			return
		}
		webrtc.sender.Write(data)
	} else {
		webrtc := (*ServerWebrtc)(sender)
		webrtc.sender[uint16(dcid)].Write(data)
	}
}

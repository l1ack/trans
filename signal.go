package xmit

/*
#cgo CFLAGS: -I${SRCDIR}
#cgo LDFLAGS: -L${SRCDIR} -lxmit -lglog -lprotobuf -lzmq -lstdc++
#include <stdlib.h>
#include "bridge.h"
*/
import "C"
import (
	"sync"
	"unsafe"
)

type (
	Signal struct {
		client unsafe.Pointer
		server unsafe.Pointer
		signal unsafe.Pointer
	}
)

var (
	signal              Signal
	signalServerAddress uint32
	signalSingletonLock sync.Mutex
	kOfferLimit         = 8192
	kAnswerLimit        = 8192
	offerBuff           = make([]byte, kOfferLimit)
	answerBuff          = make([]byte, kAnswerLimit)
)

func getSignal() *Signal {
	if signal.signal != nil {
		return &signal
	}

	signalSingletonLock.Lock()
	defer signalSingletonLock.Unlock()

	if signal.signal == nil {
		signal.signal = C.XmitSignalNew(getCore())
	}

	return &signal
}

func (signal *Signal) getServer() unsafe.Pointer {
	if signal.client != nil {
		return nil
	}
	if signal.server != nil {
		return signal.server
	}

	sig := getSignal()

	signalSingletonLock.Lock()
	defer signalSingletonLock.Unlock()

	if sig.server == nil {
		sig.server = C.XmitSignalServer(sig.signal, 0)
	}

	return sig.server
}

func (signal *Signal) getClient() unsafe.Pointer {
	if signal.server != nil {
		return nil
	}
	if signal.client != nil {
		return signal.client
	}

	sig := getSignal()

	signalSingletonLock.Lock()
	defer signalSingletonLock.Unlock()

	if sig.client == nil {
		sig.client = C.XmitSignalClient(sig.signal, C.uint(signalServerAddress), 0)
	}

	return sig.client
}

func (signal *Signal) sendOffer(offer []byte) {
	cOffer := (*C.char)(nil)
	if offer != nil {
		cOffer = (*C.char)(unsafe.Pointer(&offer[0]))
	}
	C.XmitSignalSendOffer(signal.getClient(), cOffer, C.int(len(offer)))
}

func (signal *Signal) recvOffer() (offer []byte) {
	len := C.XmitSignalRecvOffer(signal.getServer(), (*C.char)(unsafe.Pointer(&offerBuff[0])), C.int(len(offerBuff)))
	return offerBuff[:len]
}

func (signal *Signal) sendAnswer(answer []byte) {
	C.XmitSignalSendAnswer(signal.getServer(), (*C.char)(unsafe.Pointer(&answer[0])), C.int(len(answer)))
}

func (signal *Signal) recvAnswer() (answer []byte) {
	len := C.XmitSignalRecvAnswer(signal.getClient(), (*C.char)(unsafe.Pointer(&answerBuff[0])), C.int(len(answerBuff)))
	return answerBuff[:len]
}

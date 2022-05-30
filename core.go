package xmit

/*
#cgo CFLAGS: -I${SRCDIR}
#cgo LDFLAGS: -L${SRCDIR} -lxmit -lglog -lprotobuf -L/usr/local/lib -lzmq -lstdc++
#include <stdlib.h>
#include "bridge.h"
*/
import "C"
import (
	"fmt"
	"sync"
	"sync/atomic"
	"unsafe"
)

type (
	Core struct {
		Workers int32
		rawCore unsafe.Pointer
	}
)

var (
	kDefaultWorkers   int32 = 8
	core                    = Core{Workers: kDefaultWorkers}
	coreSingletonLock sync.Mutex
)

func SetWorkers(workers int32) {
	if workers > 0 {
		atomic.StoreInt32(&core.Workers, workers)
	}
}

func Start() {
	fmt.Println("XMIT START!")
	C.XmitStart(getCore())
}

func Stop() {
	C.XmitStop(getCore())
}

func Join() {
	C.XmitJoin(getCore())
}

func getCore() unsafe.Pointer {
	if core.rawCore != nil {
		return core.rawCore
	}

	coreSingletonLock.Lock()
	defer coreSingletonLock.Unlock()

	if core.rawCore == nil { // double check
		core.rawCore = C.XmitCore(C.int(core.Workers))
	}

	return core.rawCore
}

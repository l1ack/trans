#!/bin/bash -e

cp ~/xmit/lib/libxmit.a ./
cp ~/xmit/src/protocol/api/interface.h ./
cp ~/xmit/src/protocol/pbgo/* ./
go build

cd demo/server/
rm -f server
go build
cd ../..

cd demo/client/
rm -f client
go build
cd ../..

#!/bin/bash -e

cd src/client
go build
cd ../server
go build
cd ../..

mv src/client/client bin/client
mv src/server/server bin/server

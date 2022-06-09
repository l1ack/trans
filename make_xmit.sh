#!/bin/bash -e

cd src/client
go mod tidy
go build
cd ../server
go mod tidy
go build
cd ../..

mv src/client/client bin/client
mv src/server/server bin/server

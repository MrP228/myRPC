#!/bin/bash

# Упаковка клиента
mkdir -p deb_build/myRPC-client/usr/bin
mkdir -p deb_build/myRPC-client/DEBIAN
cp myRPC-client deb_build/myRPC-client/usr/bin/

cat <<EOF > deb_build/myRPC-client/DEBIAN/control
Package: myrpc-client
Version: 1.0
Architecture: amd64
Maintainer: Student
Description: myRPC Client
EOF

dpkg-deb --build deb_build/myRPC-client

# Упаковка сервера
mkdir -p deb_build/myRPC-server/usr/bin
mkdir -p deb_build/myRPC-server/etc/myRPC
mkdir -p deb_build/myRPC-server/DEBIAN

cp myRPC-server deb_build/myRPC-server/usr/bin/
echo "port = 1234" > deb_build/myRPC-server/etc/myRPC/myRPC.conf
echo "socket_type = stream" >> deb_build/myRPC-server/etc/myRPC/myRPC.conf
echo "root" > deb_build/myRPC-server/etc/myRPC/users.conf
echo "$USER" >> deb_build/myRPC-server/etc/myRPC/users.conf

cat <<EOF > deb_build/myRPC-server/DEBIAN/control
Package: myrpc-server
Version: 1.0
Architecture: amd64
Maintainer: Student
Description: myRPC Server
EOF

dpkg-deb --build deb_build/myRPC-server

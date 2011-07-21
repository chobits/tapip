#!/bin/bash

#create  veth0(tap)
tunctl -b -t veth0

#create bridge
brctl addbr br0

#bridge config
brctl addif br0 eth0
brctl addif br0 veth0

# net interface up
ifconfig eth0 0.0.0.0
ifconfig veth0 0.0.0.0 up
ifconfig br0 10.20.133.21 netmask 255.255.255.0 broadcast 10.20.133.255 up

# route config
#route add -net 10.20.133.0 netmask 255.255.255.0 br0
route add default gw 10.20.133.20

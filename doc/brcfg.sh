#!/bin/bash
# See doc/net_topology #TOP1 for more information
#
# TOPOLOGY:
#
#             br0
#            /   \
#        eth0     tap0
#         /         \
#     network(gw)   /dev/net/tun
#                     \
#                    veth (./net_stack)
#
# NOTE: We view tap0, /dev/net/tun and veth(./net_stack) as one interface,
#       They should have only one mac address(eth0 mac), which will
#       handle arp protocol right.
#       Then eth0, br0, tap0, /dev/net/tun and veth(./net_stack) can be
#       viewed as only one interface, which is similar as the original
#       real eth0 interface, and ./net_stack will become its internal
#       TCP/IP stack!
#
#       veth mac address _MUST NOT_ be eth0 mac address,
#       otherwise br0 dont work, in which case arp packet will not be passed
#       to veth
#
# WARNING: This test will suspend real kernel TCP/IP stack!
#          And we should not need kernel route table and arp cache.

openbr() {
	#create tap0
	tunctl -b -t tap0 > /dev/null

	#create bridge
	brctl addbr br0

	#bridge config
	brctl addif br0 eth0
	brctl addif br0 tap0

	# net interface up (clear ip, ./net_stack will set it)
	ifconfig eth0 0.0.0.0 up
	ifconfig tap0 0.0.0.0 up
	ifconfig br0 0.0.0.0 up
	echo "open ok"
}

closebr() {
        ifconfig tap0 down
	ifconfig br0 down
	brctl delif br0 tap0
	brctl delif br0 eth0
	brctl delbr br0
	tunctl -d tap0 > /dev/null
	echo "close ok"
}

usage() {
	echo "Usage: $0 [OPTION]"
	echo "OPTION:"
	echo "      open    config TOP1"
	echo "      close   unconfig TOP1"
	echo "      help    display help information"
}

if [ $UID != 0 ]
then
	echo "Need root permission"
	exit 0
fi

if [[ $# != 1 ]]
then
	usage
fi

case $1 in
open)
	openbr
	;;
close)
	closebr
	;;
*)
	usage
	;;
esac

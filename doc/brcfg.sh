#!/bin/bash
# See doc/net_topology #TOP1 for more information
#
# TOPOLOGY:
#
#             br0
#            /   \
#        eth0     veth0
#         /         \
#     network(gw)   /dev/net/tun
#                     \
#                   ./net_stack
#
# NOTE: We view veth0, /dev/net/tun and ./net_stack as one interface,
#       They should have only one mac address(eth0 mac), which will
#       handle arp protocol right.
#       Then eth0, br0, veth0, /dev/net/tun and ./net_stack can be
#       viewed as only one interface, which is similar as the original
#       real eth0 interface, and ./net_stack will become its internal
#       TCP/IP stack!
#
# WARNING: This test will suspend real kernel TCP/IP stack!

#create  veth0(tap)
tunctl -b -t veth0

#create bridge
brctl addbr br0

#bridge config
brctl addif br0 eth0
brctl addif br0 veth0

# net interface up (clear ip, ./net_stack will set it)
ifconfig eth0 0.0.0.0 up
ifconfig veth0 0.0.0.0 up
ifconfig br0 0.0.0.0 up

########################################
# NO NEED: ./net_stack can handle it!  #
########################################
# config according to your real network environment
# ifconfig br0 10.20.133.21 netmask 255.255.255.0 broadcast 10.20.133.255 up
#
# route config
#route add -net 10.20.133.0 netmask 255.255.255.0 br0
#route add default gw 10.20.133.20

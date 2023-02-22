tapip
=====
A user-mode TCP/IP stack based on linux tap device.  


* :exclamation: It's not a production-ready networking stack, only my practical project after learning network stack.
* :memo: If you have already learned the TCP/IP stack and want to 
  * build one to replace the Linux network stack to play, try [network toplogy (1)](doc/net_topology#L2).
  * build one to communicate with localhost (the Linux network stack and apps on it), try  [network toplogy (2)](doc/net_topology#L126).

Dependence
==========
* linux tun/tap device (/dev/net/tun exists if supported)
* pthreads

Quick start
===========
```
$ make
# ./tapip                    (run as root)
[net shell]: ping 10.0.0.2   (ping linux kernel TCP/IP stack)
(waiting icmp echo reply...)
[net shell]: help            (see all embedded commands)
...
```

The corresponding network topology is here: [network topology (2)](doc/net_topology#L126).

More information for hacking TCP/UDP/IP/ICMP:  
  See [doc/net_topology](doc/net_topology), and select a script from [doc/test](doc/test) to do.

Socket Api
==========
_socket,_read,_write,_send,_recv,_connect,_bind,_close and _listen are provided now.  
Three socket types(SOCK_STREAM, SOCK_DGRAM, SOCK_RAW) can be used.  
You can use these apis to write applications working on tapip.  
Good examples are app/ping.c and app/snc.c.

How to implement
================
I refer to xinu and Linux 2.6.11 TCP/IP stack and use linux tap device to simulate net device(for l2).  
A small shell is embedded in tapip.  
So this is just user-mode TCP/IP stack :)

Any use
=======  
Tapip makes it easy to hack TCP/IP stack, just compiling it to run.  
It can also do some network test: arp snooping, proxy, and NAT!  
I think the most important is that tapip helps me learn TCP/IP, and understand linux TCP/IP stack.

Other
=====
You can refer to [saminiir/level-ip](https://github.com/saminiir/level-ip) and its blog posts for more details.

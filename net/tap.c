#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <linux/if_tun.h>

#include "netif.h"
#include "ether.h"
#include "lib.h"
#include "ip.h"

#define TUNTAPDEV "/dev/net/tun"

static int skfd, skfd6;

void setnetmask_tap(char *name, unsigned int netmask)
{
	struct ifreq ifr = {};
	struct sockaddr_in *saddr;

	strcpy(ifr.ifr_name, name);
	saddr = (struct sockaddr_in *)&ifr.ifr_netmask;
	saddr->sin_family = AF_INET;
	saddr->sin_addr.s_addr = netmask;
	if (ioctl(skfd, SIOCSIFNETMASK, (void *)&ifr) < 0) {
		close(skfd);
		perrx("socket SIOCSIFNETMASK");
	}
	dbg("set Netmask: "IPFMT, ipfmt(netmask));
}

void setflags_tap(char *name, unsigned short flags, int set)
{
	struct ifreq ifr = {};

	strcpy(ifr.ifr_name, name);
	/* get original flags */
	if (ioctl(skfd, SIOCGIFFLAGS, (void *)&ifr) < 0) {
		close(skfd);
		perrx("socket SIOCGIFFLAGS");
	}
	/* set new flags */
	if (set)
		ifr.ifr_flags |= flags;
	else
		ifr.ifr_flags &= ~flags & 0xffff;
	if (ioctl(skfd, SIOCSIFFLAGS, (void *)&ifr) < 0) {
		close(skfd);
		perrx("socket SIOCGIFFLAGS");
	}
}

void setdown_tap(char *name)
{
	setflags_tap(name, IFF_UP | IFF_RUNNING, 0);
	dbg("ifdown %s", name);
}

void setup_tap(char *name)
{
	setflags_tap(name, IFF_UP | IFF_RUNNING, 1);
	dbg("ifup %s", name);
}

void getmtu_tap(char *name, int *mtu)
{
	struct ifreq ifr = {};

	strcpy(ifr.ifr_name, name);
	/* get net order hardware address */
	if (ioctl(skfd6, SIOCGIFMTU, (void *)&ifr) < 0) {
		close(skfd6);
		perrx("ioctl SIOCGIFHWADDR");
	}
	*mtu = ifr.ifr_mtu;
	dbg("mtu: %d", ifr.ifr_mtu);
}

void setipaddr_tap(char *name, unsigned int ipaddr)
{
	struct ifreq ifr = {};
	struct sockaddr_in *saddr;

	strcpy(ifr.ifr_name, name);
	saddr = (struct sockaddr_in *)&ifr.ifr_addr;
	saddr->sin_family = AF_INET;
	saddr->sin_addr.s_addr = ipaddr;
	if (ioctl(skfd, SIOCSIFADDR, (void *)&ifr) < 0) {
		close(skfd);
		perrx("socket SIOCSIFADDR");
	}
	dbg("set IPaddr: "IPFMT, ipfmt(ipaddr));
}

void getipaddr_tap(char *name, unsigned int *ipaddr)
{
	struct ifreq ifr = {};
	struct sockaddr_in *saddr;

	strcpy(ifr.ifr_name, name);
	if (ioctl(skfd, SIOCGIFADDR, (void *)&ifr) < 0) {
		close(skfd);
		perrx("socket SIOCGIFADDR");
	}
	saddr = (struct sockaddr_in *)&ifr.ifr_addr;
	*ipaddr = saddr->sin_addr.s_addr;
	dbg("get IPaddr: "IPFMT, ipfmt(*ipaddr));
}

void getname_tap(int tapfd, char *name)
{
	struct ifreq ifr = {};
	if (ioctl(tapfd, TUNGETIFF, (void *)&ifr) < 0)
		perrx("ioctl SIOCGIFHWADDR");
	strcpy(name, ifr.ifr_name);
	dbg("net device: %s", name);
}

void gethwaddr_tap(int tapfd, unsigned char *ha)
{
	struct ifreq ifr;
	memset(&ifr, 0x0, sizeof(ifr));
	/* get net order hardware address */
	if (ioctl(tapfd, SIOCGIFHWADDR, (void *)&ifr) < 0)
		perrx("ioctl SIOCGIFHWADDR");
	hwacpy(ha, ifr.ifr_hwaddr.sa_data);
	dbg("mac addr: %02x:%02x:%02x:%02x:%02x:%02x",
			ha[0], ha[1], ha[2], ha[3], ha[4], ha[5]);
}

int alloc_tap(char *dev)
{
	struct ifreq ifr;
	int tapfd;

	tapfd = open(TUNTAPDEV, O_RDWR);
	if (tapfd < 0) {
		perror("open");
		return -1;
	}

	memset(&ifr, 0x0, sizeof(ifr));
	ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
	if (*dev)
		strncpy(ifr.ifr_name, dev, IFNAMSIZ);
	/*
	 * create a new tap device
	 * if created already, just bind tun with file
	 */
	if (ioctl(tapfd, TUNSETIFF, (void *)&ifr) < 0) {
		perror("ioctl TUNSETIFF");
		close(tapfd);
		return -1;
	}

	return tapfd;
}

void delete_tap(int tapfd)
{
	if (ioctl(tapfd, TUNSETPERSIST, 0) < 0)
		return;
	close(tapfd);
}

void set_tap(void)
{
	skfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP);
	if (skfd < 0)
		perrx("socket PF_INET");
	skfd6 = socket(PF_INET6, SOCK_DGRAM, IPPROTO_IP);
	if (skfd6 < 0)
		perrx("socket PF_INET6");
}

void unset_tap(void)
{
	close(skfd6);
	close(skfd);
}

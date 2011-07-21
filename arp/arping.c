#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include  <sys/types.h>
#include <sys/socket.h>
#include <net/ethernet.h>	/* the L2 protocols */
#include <linux/if_arp.h>

#include <sys/param.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <linux/if.h>
#include <linux/if_arp.h>

#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

int sockfd;
int ifindex;
char *target, *device = "eth0";	/* default network interface */
struct sockaddr_ll src, dest;
struct sockaddr_ll *hostaddr = &src, *targetaddr = &dest;
struct in_addr hostip, targetip;

void perrx(char *str)
{
	if (errno)
		perror(str);
	else
		fprintf(stderr, "ERROR: %s\n", str);
	exit(EXIT_FAILURE);
}

void gethostip(char *device, struct in_addr *addr)
{
	struct sockaddr_in saddr;
	int fd, len;
	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd == -1)
		perrx("cannot create socket for get host ip");
	/* bind socket to device, then we can get device ip */
	if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, device, strlen(device) + 1) == -1)
		perrx("setsockopt(SOL_SOCKET SO_BINDTODEVICE)");

	memset(&saddr, 0x0, sizeof(saddr));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(1025);
	if (connect(fd, (struct sockaddr *)&saddr, sizeof(saddr)) == -1)
		perrx("cannot connect");

	len = sizeof(saddr);
	if (getsockname(fd, (struct sockaddr *)&saddr, &len) == -1)
		perrx("getsockname");
	printf("host ip address :%s\n", inet_ntoa(saddr.sin_addr));
	*addr = saddr.sin_addr;
	close(fd);
}

int set_device(void)
{
	struct ifreq ifr;

	memset(&ifr, 0x0, sizeof(ifr));
	strncpy(ifr.ifr_name, device, IFNAMSIZ - 1);

	/* get dev id */
	if (ioctl(sockfd, SIOCGIFINDEX, &ifr) == -1)
		perrx("ioctl SIOCGIFINDEX");
	ifindex = ifr.ifr_ifindex;

	/* get flags */
	if (ioctl(sockfd, SIOCGIFFLAGS, (char *)&ifr) == -1)
		perrx("ioctl SIOCGIFFLAGS");

	/* test up */
	if (!(ifr.ifr_flags & IFF_UP)) {
		fprintf(stderr, "net interface %s is down\n", device);
		exit(EXIT_FAILURE);
	}

	/* test arpable */
	if (ifr.ifr_flags & (IFF_NOARP | IFF_LOOPBACK)) {
		fprintf(stderr, "net interface %s isnot ARPable\n", device);
		exit(EXIT_FAILURE);
	}
}

/*
         unsigned short  ar_hrd;         format of hardware address
         unsigned short  ar_pro;         format of protocol address
         unsigned char   ar_hln;         length of hardware address
         unsigned char   ar_pln;         length of protocol address
         unsigned short  ar_op;          ARP opcode (command)
 #if 0
         Ethernet looks like this : This bit is variable sized however...
         unsigned char           ar_sha[ETH_ALEN];       sender hardware address
         unsigned char           ar_sip[4];              sender IP address
         unsigned char           ar_tha[ETH_ALEN];       target hardware address
         unsigned char           ar_tip[4];              target IP address
 #endif
*/
void send_pack(void)
{
	/** fill arp packet */
	char buf[256];
	struct arphdr *ah = (struct arphdr *)buf;
	char *p = (char *)(ah + 1);
	int len;

	ah->ar_hrd = htons(ARPHRD_ETHER);
	ah->ar_pro = htons(ETH_P_IP);
	ah->ar_hln = hostaddr->sll_halen;
	ah->ar_pln = 4;
	ah->ar_op = htons(ARPOP_REQUEST);
	/* sender hw addr */
	memcpy(p, hostaddr->sll_addr, ah->ar_hln);
	/* sender protocol addr */
	memcpy(p + ah->ar_hln, &hostip, 4);
	/* target hw addr */
	memcpy(p + ah->ar_hln + 4, targetaddr->sll_addr, ah->ar_hln);
	/* taget protocol addr */
	memcpy(p + ah->ar_hln + 4 + targetaddr->sll_halen, &targetip, 4);
	len = sizeof(*ah) + (ah->ar_hln + 4) * 2;

	/* debug */
	printf("ARPING %s ", inet_ntoa(targetip));
	printf("from %s %s\n",  inet_ntoa(hostip), device ? : "");

	/* send arp packet */
	if (len != sendto(sockfd, buf, len, 0,
		(struct sockaddr *)targetaddr, sizeof(*targetaddr)))
		perrx("cannot send REQUEST arp packet");
}

void bind_device(void)
{
	memset(hostaddr, 0x0, sizeof(*hostaddr));

	hostaddr->sll_family = AF_PACKET;
	hostaddr->sll_ifindex = ifindex;
	hostaddr->sll_protocol = htons(ETH_P_ARP);
	/* bind to device */
	if (bind(sockfd, (struct sockaddr *)hostaddr, sizeof(src)) == -1)
		perrx("bind");
}

/*
   struct sockaddr_ll {
       unsigned short sll_family;    Always AF_PACKET
       unsigned short sll_protocol;  Physical layer protocol
       int            sll_ifindex;   Interface number
       unsigned short sll_hatype;    Header type
       unsigned char  sll_pkttype;   Packet type
       unsigned char  sll_halen;     Length of address
       unsigned char  sll_addr[8];   Physical layer address
   }
 */
void set_addr(void)
{
	int len;
	/* sender hardware adress */
	/*
	 * AF_PACKET will return sockaddr_ll structure
	 *  it will return sockaddr_ll::hatype/pkttype/halen/addr
	 *  get host hardware addr
	 */
	memset(hostaddr, 0x0, sizeof(*hostaddr));
	len = sizeof(*hostaddr);
	if (getsockname(sockfd, (struct sockaddr *)hostaddr, &len) == -1 ||
		!hostaddr->sll_halen)
		perrx("getsockname");

	/* sender ip address */
	gethostip(device, &hostip);

	/* target hardware address */
	memcpy(targetaddr, hostaddr, sizeof(*targetaddr));
	/** broadcast */
	/*
	 * target hwaddr if set not broadcast,
	 * the target machine will check whether the packet is for it!
	 * so we should use broadcast hwaddr to let target reply directly
	 */
	/*
	* RFC 826 - send:
	* It does not set ar$tha to anything in particular,
	* because it is this value that it is trying to determine.  It
	* could set ar$tha to the broadcast address for the hardware (all
	* ones in the case of the 10Mbit Ethernet) if that makes it
	* convenient for some aspect of the implementation.  It then causes
	* this packet to be broadcast to all stations on the Ethernet cable
	* originally determined by the routing mechanism.
	*/
	memcpy(targetaddr->sll_addr, "\xff\xff\xff\xff\xff\xff", targetaddr->sll_halen);
	targetaddr->sll_halen = hostaddr->sll_halen;
	/* target ip address */
	if (inet_aton(target, &targetip) == 0)
		perrx("cannot get target ip address");
}

void usage(void)
{
	fprintf(stderr, "USAGE: arping ipaddr\n");
	exit(EXIT_FAILURE);
}

void parse_arg(int argc, char **argv)
{
	if (argc != 2)
		usage();
	target = argv[1];
}

int main(int argc, char **argv)
{

	/* parse arguments */
	parse_arg(argc, argv);

	/* create socket */
	sockfd = socket(PF_PACKET, SOCK_DGRAM, 0);
	if (sockfd == -1)
		perrx("cannot create raw socket");

	/* get device id */
	set_device();

	/* bind localhost */
	bind_device();

	/* set sender and target hw address */
	set_addr();

	/* send request arp */
	send_pack();

	close(sockfd);
	return 0;
}

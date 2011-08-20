#ifndef __TAP_H
#define __TAP_H

#define TUNTAPDEV "/dev/net/tun"

extern void unset_tap(void);
extern void set_tap(void);
extern void delete_tap(int tapfd);
extern int alloc_tap(char *dev);
extern void gethwaddr_tap(int tapfd, unsigned char *ha);
extern void getname_tap(int tapfd, unsigned char *name);
extern void getipaddr_tap(unsigned char *name, unsigned int *ipaddr);
extern void setipaddr_tap(unsigned char *name, unsigned int ipaddr);
extern void getmtu_tap(unsigned char *name, int *mtu);
extern void setup_tap(unsigned char *name);
extern void setdown_tap(unsigned char *name);
extern void setflags_tap(unsigned char *name, unsigned short flags, int set);
extern void setnetmask_tap(unsigned char *name, unsigned int netmask);
extern int setpersist_tap(int fd);

#endif	/* tap.h */

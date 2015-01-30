#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "utils.h"
#include "cac.h"

int getWirelessInfo(int	skfd, char *ifname, struct wireless_info *info);
int getIfaceL2ID(u_int8_t *mac_addr, char *if_name);
int compareMAC(const uint8_t* addr1,const uint8_t* addr2);
unsigned long int timedif(struct timeval tm1, struct timeval tm2);
#endif /*UTILS_H_*/

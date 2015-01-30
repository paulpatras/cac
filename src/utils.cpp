#include "utils.h"
#include <sys/ioctl.h>
//-------------------------------------------------------------------------------------
// Get wireless informations & config from the device driver

int getWirelessInfo(int	skfd, char *ifname, struct wireless_info *info)
{

	struct iwreq		wrq;

	memset((char *) info, 0, sizeof(struct wireless_info));

	// Get basic information
	if(iw_get_basic_config(skfd, ifname, &(info->b)) < 0)
    {
		struct ifreq ifr;

	      strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
	      if(ioctl(skfd, SIOCGIFFLAGS, &ifr) < 0)
	    	  return(-ENODEV);
	      else
	    	  return(-ENOTSUP);
    }

	// Get ranges
	if(iw_get_range_info(skfd, ifname, &(info->range)) >= 0)
		info->has_range = 1;

	// Get stats
	if(iw_get_stats(skfd, ifname, &(info->stats), &info->range, info->has_range) >= 0)
		info->has_stats = 1;

	// Get AP address
	if(iw_get_ext(skfd, ifname, SIOCGIWAP, &wrq) >= 0)
    {
		info->has_ap_addr = 1;
		memcpy(&(info->ap_addr), &(wrq.u.ap_addr), sizeof (sockaddr));
    }

  return(0);
}

//------------------------------------------------------------------------------
//   Obtains the MAC address of given interface

int getIfaceL2ID(u_int8_t *mac_addr, char *ifname)
{
    struct ifreq ifr;
    int fd;

	fd = socket (AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) return -1;

	strcpy(ifr.ifr_name, ifname);
    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0){
    	close (fd);
    	return -1;
    }
    close(fd);

    memset(mac_addr, 0, 8);

    memcpy(mac_addr, ifr.ifr_hwaddr.sa_data, 8);
    return 0;
}

//---------------------------------------------------
int compareMAC(const uint8_t* addr1, const uint8_t* addr2)
{
	for(int i=0; i < 6;i++)
		if(addr1[i]!=addr2[i]) return -1;
	return 1;
}

//----------------------------------------------------------
unsigned long int timedif(struct timeval tm1, struct timeval tm2)
{
	unsigned long int d;
	if(tm1.tv_usec < tm2.tv_usec)
	{
		d=1000000+tm1.tv_usec-tm2.tv_usec;
		d+=((tm1.tv_sec-1-tm2.tv_sec)*1000000);
	}
	else
	{
		d = tm1.tv_usec - tm2.tv_usec;
		d+= ((tm1.tv_sec-tm2.tv_sec)*1000000);
	}
	return d/1000;
	//return d;
}



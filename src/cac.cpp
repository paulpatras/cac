//============================================================================
// Name        : cac.cpp
// Author      : Paul Patras
// Version     :
// Copyright   : 
// Description : Centralized Adaptive Control
//============================================================================

#include "cac.h"

//#define DEBUG

#define CWmin 16
#define CWmax 1024

char *ifname, *activeIf, *monIf;
struct ifreq ifr;
int sock;
struct sockaddr_ll skll;
int MPDU = 4095;
int HEADER = 30;

uint8_t *buffer;
iwprivargs *    priv;
int             number;         // Max of private ioctl

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int iRet, running;

const struct ether_addr * ether_ap;
uint8_t hwaddr[8];
unsigned int r,s;
long unsigned int txframes, txframesold;
long unsigned int retries, retriesold;

double pmeas, pown, avgpo, avgpm, pcnt, avcw, cwcnt;
long unsigned int bcnt, frames;

uint8_t mac1[8] = {0x0a, 0x09, 0x5b, 0xe8, 0x3a, 0x63};
uint8_t mac2[8] = {0x0a, 0x0f, 0xb5, 0xe2, 0x46, 0xb4};

FILE *logfile;

double kp, ki, E, popt;

struct timeval current_tv;

pthread_t hSniffingThread, hUpdater;

int main(int argc, char** argv) {
	
	uid_t uid = getuid();

    if ( uid != 0 ) {
         printf("Root permissions required\n");
         exit(0);
    }

#ifdef DEBUG
	char *filename  = (char *)calloc(256,sizeof(char));
	time_t rawtime;
  	struct tm * timeinfo;
  	time ( &rawtime );
  	timeinfo = localtime ( &rawtime );

  	strftime (filename,80,"log_%Y%m%d_%H%M%S.dat",timeinfo);

	logfile = fopen(filename,"w");

	avgpo=0, avgpm=0, pcnt=0;
	bcnt = 0;

	avcw=0, cwcnt=0;
#endif
	
	frames = 0;

	activeIf = (char*)calloc(IFNAMSIZ, sizeof(char));
	monIf = (char*)calloc(IFNAMSIZ, sizeof(char));

	// Parse command line
	if(argc !=4 )
	{
		printf("Usage: %s %s %s %s\n", argv[0], "<active interface>", "<monitor interface>", "<PHY rate (Mbps)>");
		exit(0);
	}
	strcpy(activeIf,argv[1]);
	strcpy(monIf,argv[2]);
	
	double rate, Te, Tc;
	sscanf(argv[3], "%lf", &rate);
	
	Te = 9;
	Tc = 20+1500*8.0/rate+16+24+34;
	
	popt = 1 - exp(-sqrt(2*Te/Tc));
	
	//get wireless info
	struct wireless_info info;
	getIfaceL2ID(hwaddr,activeIf);

	int skfd;
	skfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (skfd < 0)
	{
		 printf("ERROR creating system socket\n");
		 return -1;
	}
	iRet = getWirelessInfo(skfd, activeIf, &info);
	if(iRet < 0)
	{
		printf("Failed to get wireless info\n");
		return -1;
	}
	close(skfd);

	ether_ap = (const struct ether_addr *) &(info.ap_addr).sa_data;

	r=0;s=0;

	// controller parameters
	double ss = 0;
	for (int i = 0; i< 6; i++) ss+=pow(2*popt,i);
	kp= 0.8/(pow(popt,2)*(1+popt*ss));
	ki= 0.4/(0.85*(pow(popt,2)*(1+popt*ss)));
	E = CWmin/ki;

	running = 1;

	// launch sniffing
	iRet = pthread_create(&hSniffingThread, NULL, SnifferFunction, NULL);

	if(iRet <0 )
	{
		printf("Fail to launch sniffer\n");
		return -1;
	}
	printf("Sniffer launched!\n");


	// launch CW updater
	iRet = pthread_create(&hUpdater, NULL, UpdaterFunction, NULL);
	if(iRet <0 )
	{
			printf("Fail to launch updater\n");
			return -1;
	}
	printf("CW updater launched!\n");

	signal(SIGINT, sigproc);
	printf("Press <CTRL+C> to stop execution.\n");

	pthread_join( hSniffingThread, NULL);
	pthread_join( hUpdater, NULL);
	//wait for the sniffer to close

#ifdef DEBUG
	fprintf(logfile,"%lf\n", avgpm/pcnt);
	fprintf(logfile,"%lf\n", avcw/cwcnt);
	fclose(logfile);
	free(filename);
#endif

	return EXIT_SUCCESS;
}

//--------------------------------------------------
int processPacket()
{
	int bytes;
	int i;
	uint8_t bssid[6];
	uint8_t src[6];

	int bcast;

	uint8_t DSstatus;

	//bytes = recvfrom(sock, buffer, MPDU, 0, NULL, NULL);
	bytes = recvfrom(sock, buffer, HEADER, 0, NULL, NULL);

	if (bytes < 14)
	{
		//Ignore frames smaller than 14 Bytes
		return -1;
	}

	uint8_t frameType = buffer[0];
	frameType &= 0x0C; //get the type out of the frame control field
	frameType = frameType >> 2;

	if (frameType != 2)		//frame is not a data frame
		goto __TIME_CHECK;

	DSstatus = buffer[1] & 0x03;

	bcast=0;

	switch(DSstatus){
	case 0: //IBSS
		goto __TIME_CHECK;
	case 1: //To AP
		//check if destination is broadcast
		for(i=0;i<6;i++)
			if(buffer[i+16] != 0xFF)
				bcast=1;

		//BSSID
		for(i=0;i<6;i++)
			bssid[i]=buffer[i+4];

		for(i=0;i<6;i++)
			src[i]=buffer[i+10];

		//printf("%d\n",bytes);

		break;
	case 2:	//From AP
		//check if destination is broadcast
		for(i=0;i<6;i++)
			if(buffer[i+4] != 0xFF)
				bcast=1;
		for(i=0;i<6;i++)
			src[i]=buffer[i+16];

		//BSSID
		for(i=0;i<6;i++)
			bssid[i]=buffer[i+10];
		break;

	case 3: //WDS
		goto __TIME_CHECK;
	}

	//Check it the frame belongs to the BSS
	if ((compareMAC(bssid,hwaddr) == 1) && (compareMAC(src,hwaddr) != 1) && (bcast))
	{
		uint8_t flags = buffer[1];

		pthread_mutex_lock( &lock );
		//check the retry flag
		if((flags & 0x08) == 0x08) r++;
		else s++;

		pthread_mutex_unlock( &lock );
	}

__TIME_CHECK:

	return bytes;
}

//---------------------------------------------------
void *SnifferFunction (void *ptr){
	int size;

	size = 0;

	iRet = prepareSniffSock();

	if(iRet < 1)
	{
		close(sock);
		return ptr;
	}

	//main loop
	while(running)
	{
		size = processPacket();
	}
	close(sock);

	return ptr;
}

void *UpdaterFunction (void *ptr){
	int r;

	r = getPrivIOCTL();

	while(running)
	{
		// Sleep
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        select(0, NULL, NULL, NULL, &tv);

        if(r+s >= 20) {
        	pthread_mutex_lock( &lock );

        	pmeas = (double)(r)/(r+s);
			r=0; s=0;

			pthread_mutex_unlock( &lock );

#ifdef DEBUG
			avgpm+=pmeas; pcnt++;
#endif

			updateCW();
		}
	}

	return ptr;
}

//-----------------------------------------------------
int prepareSniffSock()
{
		ifname = (char*)calloc(IFNAMSIZ, sizeof(char));
		strcpy(ifname, monIf);
		memset(&ifr, 0, sizeof(ifr));
		strncpy(ifr.ifr_name, ifname, IFNAMSIZ);

		int s;
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s < 0)
		{
			 printf("ERROR creating system socket\n");
			 return -1;
		}
		//get interface index
		if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
			printf("ERROR - getting index for adapter %s\n", ifr.ifr_name);
		}
		close(s);

		//prepare sniffing socket
		sock = socket(PF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
		if (sock < 0)
		{
				printf("ERROR creating socket\n");
				return -1;
		}

		//socket address
		memset(&skll, 0, sizeof(struct sockaddr_ll));
		skll.sll_family = AF_PACKET;
		skll.sll_ifindex = ifr.ifr_ifindex;
		skll.sll_protocol = htons(ETH_P_ALL);

		//bind socket
		if(bind(sock, (struct sockaddr *) &skll, sizeof(struct sockaddr_ll)) < 0)
		{
			printf("ERROR binding socket\n");
			close(sock);
			return -1;
		}

		//receive buffer;
		buffer = (uint8_t*)calloc(MPDU, sizeof(uint8_t));
		return 1;
}

//------------------------------------------------
void updateCW()
{
	int CW;
	double e;
	int v;
	char args[3];


	e=pmeas-popt;

	CW = (int)rint(kp*e + ki*E);
	E += e;

	if(CW < CWmin) CW = CWmin;
	if(CW >= CWmax) CW = CWmax;

	v = (int) rint(log2(CW));

#ifdef DEBUG
	avcw+=v; cwcnt++;
	gettimeofday(&current_tv, 0);
    fprintf(logfile,"%ld.%06ld\t%lf\t%d\n",current_tv.tv_sec,current_tv.tv_usec,pmeas,v);
#endif

	args[0]=0; //BE
	args[2]=v; //new CWmin

	args[1]=1; //STAs
	applyCW(0, args);
	//system("iwpriv ath0 cwmin 0 1 4");

	args[1]=0; //AP
	//applyCW(0, args);

}

//--------------------------------------------------
int getPrivIOCTL()
{
     int skfd;

     // Read the private ioctls
     if((skfd = iw_sockets_open()) < 0) return -1;
     number = iw_get_priv_info(skfd, activeIf, &priv);
     close(skfd);
     return 0;
}

//--------------------------------------------------
int applyCW(int win, char args[])
{
	int skfd;
	int		temp;
	int i = 0;
	struct iwreq	wrq;
	//u_char	buf[4096];
	u_char * buf = (u_char*)calloc(4096,sizeof(u_char));
	char *          cmdname = (char*)calloc(64,sizeof(char));
	

	if (win==0) strcpy(cmdname,"cwmin");
	else strcpy(cmdname,"cwmax");

	// Search the correct ioctl
	int k = -1;
	int		subcmd = 0;	// sub-ioctl index
	while((++k < number) && strcmp(priv[k].name, cmdname));

	int		offset = 0;	// Space for sub-ioctl index

	// Watch out for sub-ioctls !
	if(priv[k].cmd < SIOCDEVPRIVATE)
	{
	     int	j = -1;

	     // Find the matching *real* ioctl
		 while((++j < number) && ((priv[j].name[0] != '\0') || (priv[j].set_args != priv[k].set_args) || (priv[j].get_args != priv[k].get_args)));
		 // Save sub-ioctl number
		 subcmd = priv[k].cmd;
		 // Reserve one int (simplify alignment issues)
		 offset = sizeof(__u32);
		 // Use real ioctl definition from now on
		 k = j;
	}
	int count = 3;

	// Number of args to fetch
	wrq.u.data.length = count;
	if(wrq.u.data.length > (priv[k].set_args & IW_PRIV_SIZE_MASK))
	   wrq.u.data.length = priv[k].set_args & IW_PRIV_SIZE_MASK;

	// Fetch args
	for(; i < wrq.u.data.length; i++)
	{
	    temp = args[i];
	    ((__s32 *) buf)[i] = (__s32) temp;
	}

	strncpy(wrq.ifr_name, activeIf, IFNAMSIZ);

	if(offset)
		wrq.u.mode = subcmd;
	memcpy(wrq.u.name + offset, buf, IFNAMSIZ - offset);

	if((skfd = iw_sockets_open()) < 0) return -1;
	if(ioctl(skfd, priv[k].cmd, &wrq) < 0)
	{
		printf("Cannot set value\n");
		printf("%s (%X): %s\n", cmdname, priv[k].cmd, strerror(errno));
		return -1;
	}

	free(cmdname);
	free(buf);

	close(skfd);
	return 0;
}

//--------------------------------------------------
void sigproc(int i)
{
	printf("\nStopping algorithm...\n");
	running = 0;

	struct timeval tv;
	tv.tv_sec = 2;
	tv.tv_usec = 0;
	select(0, NULL, NULL, NULL, &tv);
	// wait 2 seconds  and send some data to unlock recvfrom() --if needed
	send(sock, "STOP", 4, 0);

#ifdef DEBUG
	fprintf(logfile,"%lf\n", avgpm/pcnt);
	fprintf(logfile,"%lf\n", avcw/cwcnt);
#endif
}

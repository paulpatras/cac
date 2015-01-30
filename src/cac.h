#ifndef CAC_H_
#define CAC_H_

// includes

#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <ctype.h>
#include <getopt.h>
#include <err.h>
#include <signal.h>

#include <net/if.h>
#include <linux/types.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netpacket/packet.h>
#include <net/ethernet.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/file.h>

#include "wtools/iwlib.h"
#include "wtools/wireless.h"
#include "wtools/wireless_copy.h"
#include "wtools/iwlib.h"

#include "utils.h"

#include "cac.h"

// ----------------------------------------------------------------------------------------

#pragma pack(1)

// Error codes defined for setting args
#define IWERR_ARG_NUM		-2
#define IWERR_ARG_TYPE		-3
#define IWERR_ARG_SIZE		-4
#define IWERR_ARG_CONFLICT	-5
#define IWERR_SET_EXT		-6
#define IWERR_GET_EXT		-7

#define	IEEE80211_CHAN_BYTES	32	// howmany(IEEE80211_CHAN_MAX, NBBY)
#define	IEEE80211_IOCTL_SETCHANLIST	(SIOCIWFIRSTPRIV+6)

//IOCTLs
#define SIOCG80211STATS                 (SIOCDEVPRIVATE+2)
#define SIOC80211IFCREATE               (SIOCDEVPRIVATE+7)
#define SIOC80211IFDESTROY              (SIOCDEVPRIVATE+8)
#define	SIOCGATHSTATS		(SIOCDEVPRIVATE+0)

struct ieee80211req_chanlist {
	u_int8_t ic_channels[IEEE80211_CHAN_BYTES];
};

//modes
enum ieee80211_opmode {
        IEEE80211_M_STA         = 1,    /* infrastructure station */
        IEEE80211_M_IBSS        = 0,    /* IBSS (adhoc) station */
        IEEE80211_M_AHDEMO      = 3,    /* Old lucent compatible adhoc demo */
        IEEE80211_M_HOSTAP      = 6,    /* Software Access Point */
        IEEE80211_M_MONITOR     = 8,    /* Monitor mode */
        IEEE80211_M_WDS         = 2     /* WDS link */
};

// ----------------------------------------------------------------------------------------
// type definitions

struct ieee80211_clone_params {
	char icp_name[IFNAMSIZ];				// device name
	u_int16_t icp_opmode;					// operating mode
	u_int16_t icp_flags;					// flags - see below
	#define	IEEE80211_CLONE_BSSID	0x0001	// allocate unique mac/bssid
	#define	IEEE80211_NO_STABEACONS	0x0002	// Do not setup the station beacon timers
} ;

//stats
struct ath_stats {
	u_int32_t ast_watchdog;		/* device reset by watchdog */
	u_int32_t ast_hardware;		/* fatal hardware error interrupts */
	u_int32_t ast_bmiss;		/* beacon miss interrupts */
	u_int32_t ast_rxorn;		/* rx overrun interrupts */
	u_int32_t ast_rxeol;		/* rx eol interrupts */
	u_int32_t ast_txurn;		/* tx underrun interrupts */
	u_int32_t ast_mib;		/* mib interrupts */
	u_int32_t ast_tx_packets;	/* packet sent on the interface */
	u_int32_t ast_tx_mgmt;		/* management frames transmitted */
	u_int32_t ast_tx_discard;	/* frames discarded prior to assoc */
	u_int32_t ast_tx_invalid;	/* frames discarded due to is device gone */
	u_int32_t ast_tx_qstop;		/* tx queue stopped because it's full */
	u_int32_t ast_tx_encap;		/* tx encapsulation failed */
	u_int32_t ast_tx_nonode;		/* tx failed due to of no node */
	u_int32_t ast_tx_nobuf;		/* tx failed due to of no tx buffer (data) */
	u_int32_t ast_tx_nobufmgt;	/* tx failed due to of no tx buffer (mgmt)*/
	u_int32_t ast_tx_xretries;	/* tx failed due to of too many retries */
	u_int32_t ast_tx_fifoerr;	/* tx failed due to of FIFO underrun */
	u_int32_t ast_tx_filtered;	/* tx failed due to xmit filtered */
	u_int32_t ast_tx_shortretry;	/* tx on-chip retries (short) */
	u_int32_t ast_tx_longretry;	/* tx on-chip retries (long) */
	u_int32_t ast_tx_badrate;	/* tx failed due to of bogus xmit rate */
	u_int32_t ast_tx_noack;		/* tx frames with no ack marked */
	u_int32_t ast_tx_rts;		/* tx frames with rts enabled */
	u_int32_t ast_tx_cts;		/* tx frames with cts enabled */
	u_int32_t ast_tx_shortpre;	/* tx frames with short preamble */
	u_int32_t ast_tx_altrate;	/* tx frames with alternate rate */
	u_int32_t ast_tx_protect;	/* tx frames with protection */
	u_int32_t ast_rx_orn;		/* rx failed due to of desc overrun */
	u_int32_t ast_rx_crcerr;		/* rx failed due to of bad CRC */
	u_int32_t ast_rx_fifoerr;	/* rx failed due to of FIFO overrun */
	u_int32_t ast_rx_badcrypt;	/* rx failed due to of decryption */
	u_int32_t ast_rx_badmic;		/* rx failed due to of MIC failure */
	u_int32_t ast_rx_phyerr;		/* rx PHY error summary count */
	u_int32_t ast_rx_phy[32];	/* rx PHY error per-code counts */
	u_int32_t ast_rx_tooshort;	/* rx discarded due to frame too short */
	u_int32_t ast_rx_toobig;		/* rx discarded due to frame too large */
	u_int32_t ast_rx_nobuf;		/* rx setup failed due to of no skbuff */
	u_int32_t ast_rx_packets;	/* packet recv on the interface */
	u_int32_t ast_rx_mgt;		/* management frames received */
	u_int32_t ast_rx_ctl;		/* control frames received */
	int8_t ast_tx_rssi;		/* tx rssi of last ack */
	int8_t ast_rx_rssi;		/* rx rssi from histogram */
	u_int32_t ast_be_xmit;		/* beacons transmitted */
	u_int32_t ast_be_nobuf;		/* no skbuff available for beacon */
	u_int32_t ast_per_cal;		/* periodic calibration calls */
	u_int32_t ast_per_calfail;	/* periodic calibration failed */
	u_int32_t ast_per_rfgain;	/* periodic calibration rfgain reset */
	u_int32_t ast_rate_calls;	/* rate control checks */
	u_int32_t ast_rate_raise;	/* rate control raised xmit rate */
	u_int32_t ast_rate_drop;		/* rate control dropped xmit rate */
	u_int32_t ast_ant_defswitch;	/* rx/default antenna switches */
	u_int32_t ast_ant_txswitch;	/* tx antenna switches */
	u_int32_t ast_ant_rx[8];		/* rx frames with antenna */
	u_int32_t ast_ant_tx[8];		/* tx frames with antenna */
};


// ----------------------------------------------------------------------------------------
// Functions

int processPacket();
int prepareSniffSock();
void *SnifferFunction (void *ptr);
void *UpdaterFunction (void *ptr);

int getStats(const char *ifname, long unsigned int &tx, long unsigned int &failed);
int getPrivIOCTL(); // gets private IOCTL
void updateCW();
int applyCW(int win, char args[]); //0 - CWmin, 1 - CWmax
void sigproc(int i);
// ----------------------------------------------------------------------------------------

#pragma pack()


#endif /* CAC_H_ */

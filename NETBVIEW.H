#include <lan_7_c.h>       /* general typedefs */
#include <netb_1_c.h>      /* NCB defines      */
#include <netb_2_c.h>      /* NCB structures   */
#include <netb_4_c.h>      /* NETBIOS external definition */
#ifdef E32TO16
#define CDECL _Optlink
#else
#define CDECL cdecl
#define _Seg16
#endif
union ncb_types
   {
   struct fncb {struct network_control_block bncb; unsigned long ncb_semiphore;} basic_ncb;
   struct ncb_chain_send send;
   struct ncb_reset reset;
   };

#define DEVLEN 8
#define NETBIOS_NAME_LEN 16

typedef struct netbios_info_1 {
    char           nb1_net_name[NETBIOS_NAME_LEN+1];
    char           nb1_driver_name[DEVLEN+1];/* OS/2 device driver name        */
    unsigned char  nb1_lana_num;             /* LAN adapter number of this net */
    char           nb1_pad_1;
    unsigned short nb1_driver_type;
    unsigned short nb1_net_status;
    unsigned long  nb1_net_bandwidth;        /* Network bandwidth, bits/second */
    unsigned short nb1_max_sess;             /* Max number of sessions         */
    unsigned short nb1_max_ncbs;             /* Max number of outstanding NCBs */
    unsigned short nb1_max_names;            /* Max number of names            */
} NETINFO1, *PNETINFO1;

#define NCBSIZE sizeof(union ncb_types)
typedef union ncb_types NCB;
typedef union ncb_types * PNCB, * _Seg16 PNCB16 ;
typedef struct ncb_status_information NCB_STAT;
typedef struct { USHORT Length; PBYTE Buffer; } *PBuf2;

USHORT CDECL NCBAddGroupName(BOOL Netbeui, PNCB Ncb,USHORT lana,PBYTE name);
USHORT CDECL NCBAddName(BOOL Netbeui, PNCB Ncb,USHORT lana,PBYTE name);
USHORT CDECL NCBCall(BOOL Netbeui, PNCB Ncb,USHORT lana,PBYTE lclname,PBYTE rmtname,USHORT recv_timeout,USHORT send_timeout,BOOL wait);
USHORT CDECL NCBCancel(BOOL Netbeui, PNCB Ncb,USHORT lana,PNCB NcbToCancel);
USHORT CDECL NCBChainSend(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE message,USHORT length,PBYTE Buffer2, USHORT Length2,BOOL wait);
USHORT CDECL NCBChainSendNoAck(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE message,USHORT length,PBYTE Buffer2, USHORT Length2,BOOL wait);
USHORT CDECL NCBClose(BOOL Netbeui, PNCB Ncb,USHORT lana);
USHORT CDECL NCBConfig(BOOL Netbeui, PNCB Ncb,USHORT lana,PUSHORT sessions,PUSHORT commands,PUSHORT names);
USHORT CDECL NCBDeleteName(BOOL Netbeui, PNCB Ncb,USHORT lana,PBYTE lclname);
USHORT CDECL NCBHangup(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn);
USHORT CDECL NCBListen(BOOL Netbeui, PNCB Ncb,USHORT lana,PBYTE lclname,PBYTE rmtname,USHORT recv_timeout,USHORT send_timeout,BOOL wait);
USHORT CDECL NCBReceive(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE buffer,USHORT length,BOOL wait);
USHORT CDECL NCBReceiveAny(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE buffer,USHORT length,BOOL wait);
USHORT CDECL NCBReceiveBroadcast(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE buffer,USHORT length,BOOL wait);
USHORT CDECL NCBReceiveDatagram(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE buffer,USHORT length,BOOL wait);
USHORT CDECL NCBReset(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT sessions,USHORT commands,USHORT names);
USHORT CDECL NCBSend(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE message,USHORT length,BOOL wait);
USHORT CDECL NCBSendBroadcast(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE message,USHORT length,BOOL wait);
USHORT CDECL NCBSendDatagram(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE rmtname,PBYTE message,USHORT length,BOOL wait);
USHORT CDECL NCBSendNoAck(BOOL Netbeui, PNCB Ncb,USHORT lana,USHORT lsn,PBYTE message,USHORT length,BOOL wait);
USHORT CDECL netbios_avail(BOOL Netbeui);
USHORT CDECL loadapi(PSZ module, PSZ proc, PFN FAR *addr);

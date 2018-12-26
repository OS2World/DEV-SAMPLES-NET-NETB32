#define INCL_DOSSEMAPHORES
#define INCL_DOSMODULEMGR
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#ifndef E32TO16
#include <dos.h>
#endif
#include "netbview.h"
#define No 0
#define Yes 1

#ifdef E32TO16

APIRET16 APIENTRY16 Dos16SemSet(PULONG);
APIRET16 APIENTRY16 Dos16SemClear(PULONG);
#define DosSemSet Dos16SemSet
#define DosGetModHandle DosQueryModuleHandle
#define DosGetProcAddr(x,y,z) DosQueryProcAddr(x,0,y,z)
#ifdef _loadds
#undef _loadds
#define _loadds
#endif
static APIRET16 (* APIENTRY16 netbios)(PNCB)=NULL;
static APIRET16 (* APIENTRY16 netbios_Submit)(USHORT, USHORT, PNCB)=NULL;
static APIRET16 (* APIENTRY16 netbios_Close)(USHORT, USHORT)=NULL;
static APIRET16 (* APIENTRY16 netbios_Open)(PSZ, PSZ, USHORT, PUSHORT);
static APIRET16 (* APIENTRY16 netbios_Enum)(PSZ, USHORT, PBYTE,USHORT, PUSHORT,PUSHORT)=NULL;
#else
#ifndef FP_SEG
#define FP_SEG(fp) (*((unsigned  *)&(fp)+1))
#define FP_OFF(fp) (*((unsigned  *)&(fp)))
#endif
#define APIRET16 USHORT
#define APIENTRY16 far pascal
static APIRET16 (APIENTRY16 *netbios)(PNCB)=NULL;
static APIRET16 (APIENTRY16 *netbios_Submit)(USHORT, USHORT, PNCB)=NULL;
static APIRET16 (APIENTRY16 *netbios_Close)(USHORT, USHORT)=NULL;
static APIRET16 (APIENTRY16 *netbios_Open)(PSZ, PSZ, USHORT, PUSHORT);
static APIRET16 (APIENTRY16 *netbios_Enum)(PSZ, USHORT, PBYTE,USHORT, PUSHORT,PUSHORT)=NULL;
#endif


static USHORT Netbeui_Handle[4]={0,0,0,0};
static PNETINFO1 pNetinfo=NULL;
static USHORT Netentries=0;

USHORT CDECL loadapi(PSZ module, PSZ proc, PFN FAR *addr)
{
int rc,rc1;
HMODULE mh;
       rc1=0;                         /* load adapter processor */
       rc=DosGetModHandle(module,&mh);
       if(rc==0)
         {                            /* loaded, check for this process */
         rc1=DosGetProcAddr(mh,proc,addr);
         }
       if(rc || rc1)     /* either not loaded, or not loaded for this process */
         {                            /* so load it */
         rc=DosLoadModule(NULL,0,module,&mh);
         rc1=1;                       /* force getprocaddr */
         }
       if(rc==0)
         {                            /* loaded ok? */
         if(rc1)                      /* no address to call, so get it */
           rc=DosGetProcAddr(mh,proc,addr); /* get entry */
         }
       return rc;
}

USHORT CDECL netbios_avail(BOOL Netbeui)
{
int rc=0;
        if(!Netbeui)
          {
          if(!netbios)
            rc=loadapi("ACSNETB","NETBIOS",(PFN *)&netbios);
          } /* end if */
        else
          {
          if(!netbios_Submit)
            {
            rc|=loadapi("NETAPI","NETBIOSSUBMIT",(PFN *) &netbios_Submit);
            rc|=loadapi("NETAPI","NETBIOSCLOSE", (PFN *) &netbios_Close);
            rc|=loadapi("NETAPI","NETBIOSOPEN",  (PFN *) &netbios_Open);
            rc|=loadapi("NETAPI","NETBIOSENUM",  (PFN *) &netbios_Enum);
            } /* end if */
          } /* end else */
        return rc;
}
GetNCBConfig(BOOL Netbeui,USHORT Adapter,PUSHORT S,PUSHORT C, PUSHORT N)
{
NCB WorkNcb;
      NCBConfig(Netbeui,&WorkNcb,Adapter,S,C,N);
}

#ifndef E32TO16
static void _loadds ncbpost(ncb_seg,a,b,c,ncb_off)
USHORT ncb_seg,a,b,c,ncb_off;
{
PNCB ncbp;
USHORT rc;
        _asm {
        mov ncbp+2,bx
        mov ncbp,es
        }
//        FP_SEG(ncbp)=ncb_seg;
//        FP_OFF(ncbp)=ncb_off;
        rc=DosSemClear((HSEM)&ncbp->basic_ncb.ncb_semiphore);
}
#else
#pragma stack16(256)
VOID CDECL16 ncbpost(USHORT Junk, PNCB16 NcbPointer)
{
        Dos16SemClear(&NcbPointer->basic_ncb.ncb_semiphore);
}
#endif

static USHORT CDECL NetbeuiConfig(USHORT lana, PUSHORT sessions, PUSHORT commands,PUSHORT names)
{
USHORT rc=NB_INADEQUATE_RESOURCES,blen,MaxEntries,i;
PNETINFO1 temp=NULL;

        if(netbios_Enum)
          {
          if(!pNetinfo)
            {
            (*netbios_Enum)(NULL,1,(PBYTE)pNetinfo,0,&Netentries,&MaxEntries);
            if(pNetinfo=(PNETINFO1)malloc(blen=sizeof(NETINFO1)*MaxEntries))
              {
              if(rc=(*netbios_Enum)(NULL,1,(PBYTE)pNetinfo,blen,&Netentries,&MaxEntries))
                {
                free(pNetinfo);
                pNetinfo=NULL;
                }
              } /* end if */
            }
          if(pNetinfo)
            {
            if(lana<=Netentries)
              {
              *sessions=pNetinfo[lana].nb1_max_sess;
              *commands=pNetinfo[lana].nb1_max_ncbs;
              *names=   pNetinfo[lana].nb1_max_names;
              rc=NB_COMMAND_SUCCESSFUL;
              } /* end if */
            else
              {
              rc=NB_INVALID_ADAPTER;
              } /* end else */
            } /* end if */
          } /* end if */
        return rc;
}


USHORT CDECL  NCBConfig(BOOL Netbeui, PNCB Ncb, USHORT lana, PUSHORT sessions, PUSHORT commands, PUSHORT names )
{
SHORT rc;

        if(!Netbeui)
          {
          rc=NCBReset(Netbeui, Ncb, lana, 255, 255,255 );
          *sessions=Ncb->basic_ncb.bncb.ncb_name[0];
          *commands=Ncb->basic_ncb.bncb.ncb_name[1];
          *names=   Ncb->basic_ncb.bncb.ncb_name[2];
          NCBClose(Netbeui,Ncb,lana);
          } /* end if */
        else
          {
          memset( Ncb, 0, NCBSIZE );
          if(!(rc=NetbeuiConfig(lana,sessions,commands,names)))
            {
            Ncb->basic_ncb.bncb.ncb_name[8]=*sessions;
            Ncb->basic_ncb.bncb.ncb_name[9]=*commands;
            Ncb->basic_ncb.bncb.ncb_name[10]=*names;
            }
          } /* end else */

        return rc;
}
USHORT CDECL  NCBClose( BOOL Netbeui, PNCB Ncb, USHORT lana)
{
USHORT rc;
        if(!Netbeui)
          {
          memset( Ncb, 0, NCBSIZE );
          Ncb->reset.ncb_command  = NB_RESET_WAIT;
          Ncb->reset.ncb_lsn=255;
          Ncb->reset.ncb_lana_num = lana;
          rc=(*netbios)(Ncb);
          } /* end if */
        else
          {
          if(Netbeui_Handle[lana])
            {
            rc=(*netbios_Close)(Netbeui_Handle[lana],0);
            } /* end if */
          else
            {
            rc=NB_ENVIRONMENT_NOT_DEFINED;
            } /* end else */
          } /* end else */
        return rc;
}
USHORT CDECL  NCBReset( BOOL Netbeui, PNCB Ncb, USHORT lana,USHORT  sessions,USHORT  commands,USHORT names )
{
int i,rc=NB_INADEQUATE_RESOURCES;
        if(!Netbeui)
          {
          memset( Ncb, 0, NCBSIZE );
          Ncb->reset.ncb_command  = NB_RESET_WAIT;
          Ncb->reset.ncb_lana_num = lana;
          Ncb->reset.req_sessions = sessions;
          Ncb->reset.req_commands = commands;
          Ncb->reset.req_names = names;

          (*netbios)(Ncb );

          rc=Ncb->reset.ncb_retcode;
          } /* end if */
        else
          {
          if(!pNetinfo)
            {
            rc=NetbeuiConfig(lana,&sessions,&commands,&names);
            } /* end if */
          if(pNetinfo)
            {
            if(lana<=Netentries)
              {
              if(pNetinfo[lana].nb1_max_sess>=sessions &&
                 pNetinfo[lana].nb1_max_ncbs>=commands &&
                 pNetinfo[lana].nb1_max_names>=names)
                 rc=(*netbios_Open)(pNetinfo[lana].nb1_net_name,NULL,1,&Netbeui_Handle[lana]);
              } /* end if */
            else
              {
              rc=NB_INVALID_ADAPTER;
              } /* end else */
            } /* end if */
          } /* end else */
        return rc;
}
USHORT CDECL NCBAddName( BOOL Netbeui, PNCB Ncb,USHORT lana, PBYTE name )
{

        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = NB_ADD_NAME_WAIT;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        strncpy( Ncb->basic_ncb.bncb.ncb_name, name, 16 );
        Ncb->basic_ncb.bncb.ncb_name[15]=0xff;
        if(Netbeui)
          {
          (*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          (*netbios)(Ncb );
          } /* end else */

        return (Ncb->basic_ncb.bncb.ncb_retcode);
}
USHORT CDECL NCBDeleteName(BOOL Netbeui, PNCB Ncb,USHORT lana, PBYTE name )
{

        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = NB_DELETE_NAME_WAIT;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        strncpy( Ncb->basic_ncb.bncb.ncb_name, name, 16 );
        Ncb->basic_ncb.bncb.ncb_name[15]=0xff;

        if(Netbeui)
          {
          (*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          (*netbios)(Ncb );
          }

        return (Ncb->basic_ncb.bncb.ncb_retcode);
}
USHORT CDECL NCBAddGroupName(BOOL Netbeui, PNCB Ncb,USHORT lana, PBYTE name )
{

        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = NB_ADD_GROUP_NAME_WAIT;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        strncpy( Ncb->basic_ncb.bncb.ncb_name, name, 16 );
        Ncb->basic_ncb.bncb.ncb_name[15]=0xff;

        if(Netbeui)
          {
          (*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          (*netbios)(Ncb );
          }

        return (Ncb->basic_ncb.bncb.ncb_retcode);
}
USHORT CDECL  NCBCall(BOOL Netbeui, PNCB  Ncb, USHORT lana, PBYTE lclname, PBYTE rmtname,USHORT recv_timeout,USHORT send_timeout,BOOL wait)
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_CALL_WAIT:NB_CALL;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_rto      = recv_timeout<<1;  /* times 2 since in   */
        Ncb->basic_ncb.bncb.ncb_sto      = send_timeout<<1;  /* steps of 500 msecs */
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        strncpy( Ncb->basic_ncb.bncb.ncb_name, lclname, 16 );
        Ncb->basic_ncb.bncb.ncb_name[15]=0xff;
        strncpy( Ncb->basic_ncb.bncb.ncb_callname, rmtname, 16 );
        Ncb->basic_ncb.bncb.ncb_callname[15]=0xff;
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}
USHORT CDECL NCBListen(BOOL Netbeui, PNCB  Ncb, USHORT lana, PBYTE lclname, PBYTE rmtname,USHORT recv_timeout,USHORT send_timeout,BOOL wait)
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_LISTEN_WAIT:NB_LISTEN;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_rto      = recv_timeout<<1;   /* times 2 since in   */
        Ncb->basic_ncb.bncb.ncb_sto      = send_timeout<<1;   /* steps of 500 msecs */
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        strncpy( Ncb->basic_ncb.bncb.ncb_name, lclname, 16 );
        Ncb->basic_ncb.bncb.ncb_name[15]=0xff;
        strncpy( Ncb->basic_ncb.bncb.ncb_callname, rmtname, 16 );
        Ncb->basic_ncb.bncb.ncb_callname[15]=0xff;
        if(!wait)
          {
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);
          }

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}

/**********************************************************************/
/*
** NCBSend      Sends data to the session partner as defined by the
**              session number in the NCB.LSN field.  The data to send
**              is in the buffer pointed to by the NCB.BUFFER field.
**
**              Accepts the adapter number, the session number,
**              the char array holding the message to be sent, and
**              the length of the message in that array.
**
**              Returns the NCB return code.
*/

USHORT CDECL  NCBSend(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn, PBYTE message, word length,BOOL wait)
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_SEND_WAIT:NB_SEND;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_lsn      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = message;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}

USHORT CDECL  NCBSendDatagram(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn,PSZ rmtname, PBYTE message, word length,BOOL wait)
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_SEND_DATAGRAM_WAIT:NB_SEND_DATAGRAM;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_num      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = message;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        strncpy( Ncb->basic_ncb.bncb.ncb_callname, rmtname, 16 );
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}
USHORT CDECL  NCBSendBroadcast(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn, PBYTE message, word length,BOOL wait)
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_SEND_BROADCAST_DATAGRAM_WAIT:NB_SEND_BROADCAST_DATAGRAM;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_num      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = message;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}
USHORT CDECL  NCBSendNoAck(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn, PBYTE message, word length,BOOL wait)
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_SEND_NO_ACK_WAIT:NB_SEND_NO_ACK;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_lsn      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = message;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}
/**********************************************************************/
/*
** NCBChainSend      Sends data to the session partner as defined by the
**              session number in the NCB.LSN field.  The data to send
**              is in the buffer pointed to by the NCB.BUFFER field.
**
**              Accepts the adapter number, the session number,
**              the char array holding the message to be sent, and
**              the length of the message in that array.
**
**              Returns the NCB return code.
*/

USHORT CDECL  NCBChainSend(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn,PBYTE  message, word length, PBYTE Buffer2, word Length2,BOOL wait)
{
int rc;
PBuf2 b2;
        memset( Ncb, 0, NCBSIZE );
        b2=(PBuf2)&Ncb->basic_ncb.bncb.ncb_callname;
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_CHAIN_SEND_WAIT:NB_CHAIN_SEND;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_lsn      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = message;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        b2->Length=Length2;
        b2->Buffer=Buffer2;
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}
USHORT CDECL  NCBChainSendNoAck(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn, PBYTE message, word length, PBYTE Buffer2, word Length2,BOOL wait)
{
int rc;
PBuf2 b2;
        memset( Ncb, 0, NCBSIZE );
        b2=(PBuf2)&Ncb->basic_ncb.bncb.ncb_callname;
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_CHAIN_SEND_NO_ACK_WAIT:NB_CHAIN_SEND_NO_ACK;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_lsn      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = message;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        b2->Length=Length2;
        b2->Buffer=Buffer2;
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}

/**********************************************************************/
/*
** NCBReceive   Receives data from the session partner that sends data
**              to this station.
**
**              Accepts the adapter number, the session number,
**              the char array to hold the message received, and
**              the maximum length the message may occupy in that
**              array.
**
**              Returns the NCB return code and, if successful,
**              the received data in the buffer.
*/

USHORT CDECL NCBReceive(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn,PBYTE buffer, word length, BOOL wait )
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_RECEIVE_WAIT:NB_RECEIVE;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_lsn      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = buffer;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}
/**********************************************************************/
/*
** NCBReceiveAny Receives data from the session partner that sends data
**              to this station.
**
**              Accepts the adapter number, the session number,
**              the char array to hold the message received, and
**              the maximum length the message may occupy in that
**              array.
**
**              Returns the NCB return code and, if successful,
**              the received data in the buffer.
*/

USHORT CDECL NCBReceiveAny(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn, PBYTE buffer, word length,BOOL wait )
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_RECEIVE_ANY_WAIT:NB_RECEIVE_ANY;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_num      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = buffer;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}

USHORT CDECL NCBReceiveDatagram(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn,PBYTE buffer, word length,BOOL wait )
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_RECEIVE_DATAGRAM_WAIT:NB_RECEIVE_DATAGRAM;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_num      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = buffer;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}

USHORT CDECL NCBReceiveBroadcast(BOOL Netbeui, PNCB  Ncb,USHORT  lana, USHORT lsn, PBYTE buffer, word length,BOOL wait )
{
int rc;
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = (wait)?NB_RECEIVE_BROADCAST_DATAGRAM_W:NB_RECEIVE_BROADCAST_DATAGRAM;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_num      = lsn;
        Ncb->basic_ncb.bncb.ncb_buffer_address = buffer;
        Ncb->basic_ncb.bncb.ncb_length   = length;
        Ncb->basic_ncb.bncb.off44.ncb_post_address=(address)((!wait)?ncbpost:NULL);
        if(!wait)
          DosSemSet((HSEM)&Ncb->basic_ncb.ncb_semiphore);

        if(Netbeui)
          {
          rc=(*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          rc=(*netbios)(Ncb );
          }

        return (wait)?Ncb->basic_ncb.bncb.ncb_retcode:rc;
}

/**********************************************************************/
/*
** NCBHangup    Closes the session with another name on the network
**              specified by the session number.
**
**              Accepts the adapter number and session number.
**
**              Returns the NCB return code.
*/

USHORT CDECL  NCBHangup(BOOL Netbeui, PNCB  Ncb, USHORT lana, USHORT lsn )
{
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = NB_HANG_UP_WAIT;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_lsn      = lsn;

        if(Netbeui)
          {
          (*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          (*netbios)(Ncb );
          }

        return (Ncb->basic_ncb.bncb.ncb_retcode);
}

/**********************************************************************/
/*
** NCBCancel    Closes the session with another name on the network
**              specified by the session number.
**
**              Accepts the adapter number and session number.
**
**              Returns the NCB return code.
*/

USHORT CDECL  NCBCancel(BOOL Netbeui, PNCB  Ncb, USHORT lana, PNCB NcbToCancel)
{
        memset( Ncb, 0, NCBSIZE );
        Ncb->basic_ncb.bncb.ncb_command  = NB_CANCEL_WAIT;
        Ncb->basic_ncb.bncb.ncb_lana_num = lana;
        Ncb->basic_ncb.bncb.ncb_buffer_address = (address)NcbToCancel;

        if(Netbeui)
          {
          (*netbios_Submit)(Netbeui_Handle[lana],0,Ncb);
          } /* end if */
        else
          {
          (*netbios)(Ncb );
          }

        return (Ncb->basic_ncb.bncb.ncb_retcode);
}

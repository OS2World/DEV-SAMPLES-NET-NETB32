#define INCL_DOS
#include <os2.h>
#include <netbview.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define Local "TEST1"
#define Remote "TEST2"
#define Anyone "*"
#define SendMsg "Message sent from Client to Server"
#define NETBIOS 0
#define NETBEUI 1
BOOL API = NETBIOS;
#define Adapter 0
APIRET16 APIENTRY16 Dos16SemWait(PULONG,ULONG);

PNCB WorkNcb;
main(int argc, char *argv[], char *envp)
{
  BYTE Buffer[100];
  USHORT rc,lsn;
  PBYTE Name,CallName;

  if(!netbios_avail(API))
    {
    WorkNcb=(PNCB)malloc(sizeof(NCBSIZE));
    if(!(rc=NCBReset(API,WorkNcb,Adapter,2,2,2)))
      {
      if(argc==1)
        {
        Name=Local;
        CallName=Remote;
        } /* end if */
      else
        {
        Name=Remote;
        CallName=Local;
        } /* end else */
      if(!(rc=NCBAddName(API,WorkNcb,Adapter,Name)) || rc==13)
        {
        printf("Name added\n");
        if(argc==1)             // process as client
          {
          if(!(rc=NCBCall(API,WorkNcb,Adapter,Name,CallName,0,0,TRUE)))
            {
            lsn=WorkNcb->basic_ncb.bncb.ncb_lsn;
            if(!(rc=NCBSend(API,WorkNcb,Adapter,lsn,SendMsg,sizeof(SendMsg)-1, TRUE)))
              {
              printf("Send completed\n");
              } /* end if */
            else
              {
              printf("Send failed rc=%d\n",rc);
              } /* end else */
            NCBHangup(API,WorkNcb,Adapter,lsn);
            } /* end if */
          else
            {
            printf("Call failed rc=%d\n",rc);
            } /* end else */
          } /* end if */
        else                    // process as server
          {
          if(!(rc=NCBListen(API,WorkNcb,Adapter,Name,Anyone,0,0,TRUE)))
            {
            lsn=WorkNcb->basic_ncb.bncb.ncb_lsn;
            printf("Listen ready\n");
            if(API==NETBIOS)
              {
              if(!(rc=NCBReceive(API,WorkNcb,Adapter,lsn,Buffer,sizeof(Buffer),FALSE)))
                {
                rc=Dos16SemWait(&WorkNcb->basic_ncb.ncb_semiphore,SEM_INDEFINITE_WAIT);
                printf("Data received='%.*s' rc=%d\n",(int)WorkNcb->basic_ncb.bncb.ncb_length,Buffer,rc);
                } /* end if */
              else
                {
                printf("Receive failed rc=%d\n");
                } /* end else */
              } /* end if */
            else
              {
              if(!(rc=NCBReceive(API,WorkNcb,Adapter,lsn,Buffer,sizeof(Buffer),TRUE)))
                {
                printf("Data received='%.*s' rc=%d\n",(int)WorkNcb->basic_ncb.bncb.ncb_length,Buffer,rc);
                } /* end if */
              else
                {
                printf("Receive failed rc=%d\n");
                } /* end else */
              } /* end else */
            NCBHangup(API,WorkNcb,Adapter,lsn);
            } /* end if */
          else
            {
            printf("Listen failed rc=%d\n");
            } /* end else */
          } /* end else */
        NCBDeleteName(API,WorkNcb,Adapter,Name);
        } /* end if */
      else
        {
        printf("AddName Failed rc=%d\n",rc);
        } /* end else */
      NCBClose(API,WorkNcb,Adapter);
      } /* end if */
    else
      {
      printf("Reset failed rc=%d\n",rc);
      } /* end else */

    } /* end if */
  else
    {
    printf("ACSNETB.DLL could not be found, Netbios not installed\n");
    } /* end else */
  return rc;
}

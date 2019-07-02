/* -------------------------------------------------------------------------- *
 * myip.c                                                                     *
 *                                                                            *
 *  This program is free software; you can redistribute it and/or modify      *
 *  it under the terms of the GNU General Public License as published by      *
 *  the Free Software Foundation; either version 2 of the License, or         *
 *  (at your option) any later version.                                       *
 *                                                                            *
 *  This program is distributed in the hope that it will be useful,           *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *  GNU General Public License for more details.                              *
 *                                                                            *
 *  You should have received a copy of the GNU General Public License         *
 *  along with this program; if not, write to the Free Software               *
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA *
 * -------------------------------------------------------------------------- */

#define __STDC_WANT_LIB_EXT1__ 1

#define INCL_DOS
#define INCL_DOSNLS
#define INCL_DOSERRORS
#define INCL_VIO
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Sockets & networking
#include <types.h>
#include <sys\socket.h>
#include <sys\ioctl.h>
#include <net\route.h>
#include <net\if.h>
#include <net\if_arp.h>
#include <sys\time.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet\in.h>
#include <arpa\inet.h>
#include <conio.h>
#include <time.h>

#include "getopt.h"
#include "bldlevel.h"
#include "trace.h"

#ifdef __WATCOMC__
  #define PEXCEPTIONREGISTRATIONRECORD EXCEPTIONREGISTRATIONRECORD *
#endif // __WATCOMC__

#define INCL_LOADEXCEPTQ            // only once per application
#include "exceptq.h"

#define RETURN(val) {UninstallExceptq(&exRegRec); return val;}

// -----------------------------------------------------------------------------
// Constants

#define MESSAGE_NORMAL              1       // normal output
#define MESSAGE_VERBOSE             2       // verbose output
#define MESSAGE_DIAGNOSTIC          3       // very verbose (diagnostic) output

#define UL_SELECT_TIMEOUT           5000    // timeout for os2_select() in ms
#define US_MAX_READS                2       // max. number of socket read attempts
#define US_MAX_SERVERS              10      // max. number of servers to try

#define NUM_PING_SERVERS            2   // max. number of ping servers

#define UL_DATA_LIMIT               4096    // max. amount of data to read from socket

// #define US_SZMESSAGE_MAX            1024    // various string length limits
// #define US_SZCOMMAND_MAX            256
#define US_SZRESPONSE_MAX           1024
#define US_SZCFG_MAX                1024
#define US_SZHOSTNAME_MAX           256
#define US_SZPATH_MAX               256
#define US_SZAFTER_MAX              512

// Various defaults (most of these aren't really used by the program)
#define US_DEFAULT_ECHO_PORT        80

#define SZ_DEFAULT_ECHO_SERVER      "checkip.dyndns.org"
#define SZ_DEFAULT_ECHO_DIR         "/"
#define SZ_DEFAULT_FIND_AFTER       "Address:"

#define SZ_CONFIG_FILE              "MyIP.cfg"  // name of our configuration file

// return values
#define RET_WRONG_PARAMS            255
#define RET_INVALID_VALUE           254

#define TCPIP_TIMEOUT               77      // timeout of TCP/IP stack (estimated)
// -----------------------------------------------------------------------------
// Typedefs

typedef struct sockaddr_in SOCKADDR_IN, *PSOCKADDR_IN;
typedef struct hostent     HOSTENT,     *PHOSTENT;
typedef struct timeval     TIMEVAL,     *PTIMEVAL;

typedef struct _Echo_Parms
  {
  ULONG   ulSocket;
  USHORT  usPort;
  CHAR    szServer[ US_SZHOSTNAME_MAX + 1 ],
          szDir[ US_SZPATH_MAX + 1 ],
          szFindAfter[ US_SZAFTER_MAX + 1 ];
  } ECHOPARMS, *PECHOPARMS;


// -----------------------------------------------------------------------------
// Function prototypes

SHORT ReadConfig( ECHOPARMS echo_p[], ECHOPARMS ping[], USHORT usVerbosity, char *pszConfigFileName, int *iNumPingConfigured );
int   SocketConnect( char *pszDestIP, unsigned short usDestPort, unsigned short usVerbosity, char *pszEchoReturn, int iLen );
short SocketRead( int ulSocket, char *pszResponse, int iMaxLenResponse, char *pszEchoReturn, int iLen);
short SocketWrite( int ulSocket, char *pszData );
ULONG EchoQuery( ECHOPARMS echo, USHORT usVerbosity, char *pszEchoReturn, int iLen );
CHAR  BoxChar( CHAR chPref, CHAR chPlain );
char *strstrip( char *s );
void  message_out( PSZ pszText, USHORT usLevel, USHORT usVerbosity );
int usage(void);

// -----------------------------------------------------------------------------
// gloabal variables
static char  szAppName[CCHMAXPATH] = "unknown";
static int   iNumPingConfigured = 0;
static char  pszEchoReturn[512] = ".";
static ECHOPARMS echo_p[ US_MAX_SERVERS ];     // array of configured servers
static ECHOPARMS ping[NUM_PING_SERVERS];     // ping server

/* -------------------------------------------------------------------------- *
 * -------------------------------------------------------------------------- */
int main( int argc, char *argv[] )
  {
  EXCEPTIONREGISTRATIONRECORD exRegRec;
  USHORT    usConfigured,                 // number of configured servers
              usTry;                      // the current server being attempted

  USHORT usVerbosity = MESSAGE_NORMAL;    // verbosity level
  ULONG     ulRC;
  int i;
  int iDebugLevel = 0;
  int iTemp;
  int iRepTime = 1;
  int iTimeDisp = 0;
  int iChangeOnly = 0, iNumChecks = 0;
  int iPingRepTime = 1;

  time_t timeActual;
  auto struct tm tmbuf;

  char szTemp[CCHMAXPATH * 2] = "";
  char szFilename[CCHMAXPATH] = "undefined";
  char szLogFile[CCHMAXPATH] = "";
  char szIpString[64] = " ";
  char *pszP1, *pszP2, *pszConfigFileName = 0;
  char szLastOutput[256] = "";
  char szTimeString[64] = "";
  char cBuf[256];

  FILE *pfLog = 0;

  strncpy (szAppName, argv[0], sizeof(szAppName) );
  szAppName[sizeof(szAppName) - 1] = '\0';
  while ( pszP1 = strstr(szAppName, "\\") )
    {
    pszP2 = szAppName;
    pszP1++;
    // start copying 1 char after '\'
    do
      {
      *pszP2 = *pszP1;
      pszP1++;
      pszP2++;
      } while ( *pszP1 != '\0' );
    // copy \0 character
    *pszP2 = *pszP1;
    }

  sprintf(cBuf, "MyIP v%d.%02d.%02d   Build: %s", VER_MAJOR, VER_MINOR, VER_BUILD, pszBLD_DATE_TIME);
  TRACE("\n\n");
  TRACE("--------------------------------------------------------------");
  TRACE(" %s", cBuf );
  TRACE("--------------------------------------------------------------");

  LoadExceptq(&exRegRec, "I", cBuf);

  while ( (i = getopt(argc,argv,"dDvVf:hH?r:t:c:s:l:p:")) != -1 )
    {
    switch ( i )
      {
      case 'd':
      case 'v':
        iDebugLevel = 1;
        usVerbosity = MESSAGE_VERBOSE;
        break;
      case 'D':
      case 'V':
        iDebugLevel = 2;    /* debug option level 2 */
        usVerbosity = MESSAGE_DIAGNOSTIC;
        break;
      case 'r':
        iTemp = atoi(optarg);
        if ( 1 <= iTemp && iTemp <= 3600 )
          {
          iRepTime = iTemp;
          }
        else
          {
          TRACE("ERROR: Repetition time %d not allowed", iTemp);
          usage();
          RETURN (RET_INVALID_VALUE);
          }
        printf ("Type <ESC> key to terminate program\n\n");
        break;
      case 'f':
        strncpy(szFilename, optarg, sizeof(szFilename) );
        szFilename[sizeof(szFilename) - 1] = '\0';
        TRACE("Config file='%s'", szFilename );
        pszConfigFileName = szFilename;
        if ( !strlen(szFilename) )
          {   // no (zero lenght) filename
          TRACE("ERROR: -f requires filename");
          RETURN(RET_INVALID_VALUE);
          }
        break;
      case 'l':
        strncpy(szLogFile, optarg, sizeof(szLogFile) );
        szLogFile[sizeof(szLogFile) - 1] = '\0';
        TRACE("Config file='%s'", szLogFile );
        if ( !strlen(szLogFile) )
          {   // no (zero lenght) filename
          TRACE("ERROR: -l requires filename");
          RETURN(RET_INVALID_VALUE);
          }
        break;
      case 's':
        strncpy(szIpString, optarg, sizeof(szIpString) );
        szIpString[sizeof(szIpString) - 1] = '\0';
        TRACE("String before IP address='%s'", szIpString );
        break;
      case 'c':
        TRACE("optarg=0x%X", optarg);
        iTemp = atoi(optarg);
        if ( 0 <= iTemp && iTemp <= 3600 )
          {
          iNumChecks = iTemp;
          iChangeOnly = iTemp;
          }
        else
          {
          TRACE("value for -c out of range (%d)", iTemp);
          usage();
          RETURN(RET_INVALID_VALUE);
          }
        break;
      case 't':
        iTemp = atoi(optarg);
        if ( 0 <= iTemp && iTemp <= 2 ) iTimeDisp = iTemp;
        break;
      case 'p':
        iTemp = atoi(optarg);
        if ( 1 <= iTemp && iTemp <= 300 ) iPingRepTime = iTemp;
        break;
      case '?':
      case 'h':
      case 'H':
        usage();
        RETURN(0);
        break;
      default:
        //usage();
        break;
      }
    }

  argc -= optind;
  argv += optind;
  if ( argc >= 1 )
    {
    usage();
    RETURN(RET_WRONG_PARAMS);
    }

  // Get the configuration
  usConfigured = ReadConfig( echo_p, ping, usVerbosity, pszConfigFileName , &iNumPingConfigured );
  if ( usConfigured < 1 )
    {
    printf("No valid servers were found.\n");
    RETURN(1);
    }

  // warn user if a server is connected in short intervalls
  if ( iRepTime > 1 && iRepTime * usConfigured < 300 )
    {
    printf("WARNING: some providers do not allow repeated requests within a short period\n");
    printf(" configure different providers or consider ping option (-p)\n\n");
    }

  if ( strlen(szLogFile) )
    {   // write to log file is desired
    if ( ( pfLog = fopen( szLogFile, "a+")) == NULL )
      {
      sprintf( szTemp, "Log file %s could not be opened.\n", szLogFile );
      message_out( szTemp, MESSAGE_NORMAL, usVerbosity );
      TRACE("Log file %s could not be opened", szLogFile );
      RETURN(0);
      }

    }

  i = iRepTime + 1;
  usTry = 0;
  // start endless loop
  do
    {

    // Now try each server until we find one that works
    // for ( usTry = 0; usTry < usConfigured; usTry++ )
    {
    if ( usTry >= usConfigured) usTry = 0;

      if ( i > iRepTime)
        { // ask for our IP address
        ulRC = 1;
        i = 0;
        echo_p[ usTry ].ulSocket = SocketConnect( echo_p[ usTry ].szServer,
                                                  echo_p[ usTry ].usPort,
                                                  usVerbosity, pszEchoReturn, sizeof(pszEchoReturn) );
        if ( echo_p[ usTry ].ulSocket )
          {
          ulRC &= EchoQuery( echo_p[usTry], usVerbosity, pszEchoReturn, sizeof(pszEchoReturn) );
          TRACE("ulRC=%d", ulRC);
          soclose( echo_p[ usTry ].ulSocket );
          if ( ulRC != NO_ERROR )
              {
              //return( ulRC );
              // problem getting IP check again in 60s
              i = iRepTime - 60;
              if (i <= 0) i = 1;

              usTry++;
              TRACE("usTry=%d", usTry);
              }
          else
            { // succesfully decoded server return - we got our IP
              // fake usTry to end endless loop when no repetitions are desired
            if (iRepTime < 2) usTry = usConfigured;
            }

          }
        else
          {
          usTry++;
          TRACE("usTry=%d", usTry);
          }
        }
      else
        { // check connection to ping server
        if ( iNumPingConfigured && (i % iPingRepTime == 0) )
          { // connect to ping server
          //strcpy(pszEchoReturn, "ping ok");
          if (ping[0].ulSocket = SocketConnect( ping[0].szServer, echo_p[ usTry ].usPort, usVerbosity, pszEchoReturn, sizeof(pszEchoReturn) ))
            { // successfully opened a socket to ping server
            soclose( ping[0].ulSocket );
            TRACE_L2("successfully connected to ping server");
            }
          else
            {
            TRACE_L2("no connection to ping server");
            // account for TCPIP stack timeout
            i += TCPIP_TIMEOUT;
            ulRC = 1;
            }
          }
        }

      // compose time
      timeActual = time( NULL );
      tmbuf = *localtime(&timeActual);
      //TRACE("Time3 %s", ctime( &timeActual ) );
      switch ( iTimeDisp )
        {
        case 1:
          sprintf(szTimeString, "%04d-%02d-%02d %02d:%02d:%02d", tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min, tmbuf.tm_sec);
          break;

        case 2:
          sprintf(szTimeString, "%04d%02d%02d %02d:%02d:%02d", tmbuf.tm_year + 1900, tmbuf.tm_mon + 1, tmbuf.tm_mday, tmbuf.tm_hour, tmbuf.tm_min, tmbuf.tm_sec);
          break;

        default:
          sprintf(szTimeString, "");
          break;
        }

      // now output whole line if desired
      if ( iNumChecks ) iNumChecks--;
      // if there was a error in EchoQuery (timeout) it probably took about 77s (TCP/IP), account for this
      if ( ulRC )
        {
        if (iRepTime) iNumChecks -= TCPIP_TIMEOUT / iRepTime;
        else iNumChecks = 0;
        if ( iNumChecks < 0 ) iNumChecks = 0;
        }

      if ( !iChangeOnly || !strstr(szLastOutput, pszEchoReturn) || !iNumChecks )
        {
        printf("%s%s%s\n", szTimeString, ulRC ? " " : szIpString, pszEchoReturn);
        if ( pfLog )
          {
          fprintf(pfLog, "%s%s%s\n", szTimeString, ulRC ? " " : szIpString, pszEchoReturn);
          fflush( pfLog );
          }
        fflush( stdout );
        strcpy (szLastOutput, pszEchoReturn);
        iNumChecks = iChangeOnly;
        }

    }

    // wait iRepTime while checking for ESC key every second
    while ( kbhit() )
      {
      if ( getch () == 27 )
        {   // ESC detected, abort
        RETURN(0);
        }
      }
    if (usTry < usConfigured) sleep(iPingRepTime);  // only sleep when not already done (all servers are tried)
    i += iPingRepTime;
    } while ( iRepTime > 1 || usTry < usConfigured); // FOREVER when repeatition is desired or as long as no valid answere and there are additional server configured

  RETURN(255);
  }



/* -------------------------------------------------------------------------- *
 * ReadConfig                                                                 *
 *                                                                            *
 * Parses the configuration file and generates the array of servers to try.   *
 * -------------------------------------------------------------------------- */
SHORT ReadConfig( ECHOPARMS echo_p[], ECHOPARMS ping[], USHORT usVerbosity, char *pszConfigFileName, int *iNumPingConfigured )
  {
  PSZ    pszEtc,                              // value of %ETC%
              token;                               // current token from strtok
  CHAR   szMessage[ US_SZHOSTNAME_MAX + US_SZPATH_MAX + 1 ],   // output message
         szConfigFile[ CCHMAXPATH + 1 ],      // configuration file name
         szBuf[ US_SZCFG_MAX + 1 ],           // a configuration file entry
         szServer[ US_SZHOSTNAME_MAX + 1 ],   // hostname of server
         szDir[ US_SZPATH_MAX + 1 ],          // path on server
         szFindAfter[ US_SZAFTER_MAX + 1 ];   // "find after" text on server
  USHORT usPort,                              // IP port on server
              usCount;                             // number of servers configured
  FILE   *pfConfig;                           // configuration file handle
  //APIRET rc;

  if ( !pszConfigFileName )   // check if config filename is specified
    {
    // no, look for a configuration file under %ETC%
    if ( ( pszEtc = getenv("ETC")) == NULL )
      {
      sprintf( szMessage, "Unable to resolve ETC environment variable.\n");
      message_out( szMessage, MESSAGE_NORMAL, usVerbosity );
      TRACE_L2("Unable to resolve ETC environment variable", szMessage );
      return( 0 );
      }
    sprintf( szConfigFile, "%s\\%s", pszEtc, SZ_CONFIG_FILE );
    strupr( szConfigFile );
    pszConfigFileName = szConfigFile;
    }

  if ( ( pfConfig = fopen( pszConfigFileName, "r")) == NULL )
    {
    sprintf( szMessage, "Configuration file %s could not be opened.\n", pszConfigFileName );
    message_out( szMessage, MESSAGE_NORMAL, usVerbosity );
    TRACE("Configuration file %s could not be opened", pszConfigFileName );
    return( 0 );
    }

  sprintf( szMessage, "Configuration file: %s\n", pszConfigFileName );
  message_out( szMessage, MESSAGE_DIAGNOSTIC, usVerbosity );
  TRACE_L2("Configuration file: %s", pszConfigFileName );

  // Read configuration entries from the file (one per line)
  usCount = 0;
  while ( !feof(pfConfig) )
    {

    memset( szServer,    0, sizeof(szServer)    );
    memset( szDir,       0, sizeof(szDir)       );
    memset( szFindAfter, 0, sizeof(szFindAfter) );

    if ( fgets( szBuf, US_SZCFG_MAX, pfConfig ) == NULL ) break;
    strstrip( szBuf );

    // skip blank or commented lines
    if ( ( strlen(szBuf) == 0 ) || ( szBuf[0] == '#') || ( szBuf[0] == ';') ) continue;

    // parse entry in the format: <host> <port> <path> <after>

    // hostname
    if ( ( token = strtok( szBuf, " ")) == NULL ) continue;
    strncpy( szServer, token, US_SZHOSTNAME_MAX );
    if ( strstr("ping", szServer) )
      {
      if ( ( token = strtok( NULL, " ")) == NULL ) continue;
      strncpy( szServer, token, US_SZHOSTNAME_MAX );
      TRACE_L2("ping Server[%d]=%s", *iNumPingConfigured, szServer);
      strcpy( ping[*iNumPingConfigured].szServer, szServer);
      // port
      if ( ( token = strtok( NULL, " ")) == NULL ) continue;
      if ( sscanf( token, "%d", &usPort ) == 0 ) continue;
      TRACE_L2("  port=%d", usPort);
      ping[*iNumPingConfigured].usPort = usPort;
      if ( (*iNumPingConfigured) + 1 < NUM_PING_SERVERS ) (*iNumPingConfigured)++;
      continue;
      }
    else TRACE_L2(" szServer=%s", szServer);

    // port
    if ( ( token = strtok( NULL, " ")) == NULL ) continue;
    if ( sscanf( token, "%d", &usPort ) == 0 ) continue;
    TRACE_L2(" port=%d", usPort);

    // path
    if ( ( token = strtok( NULL, " ")) == NULL ) continue;
    strncpy( szDir, token, US_SZPATH_MAX );
    TRACE_L2(" szDir=%s", szDir);

    // after
    if ( ( token = strtok( NULL, "\0")) == NULL ) continue;
    strncpy( szFindAfter, token, US_SZAFTER_MAX );
    TRACE_L2(" szFindAfter=%s", szFindAfter);

    echo_p[usCount].usPort = usPort;
    strcpy( echo_p[usCount].szServer,    szServer    );
    strcpy( echo_p[usCount].szDir,       szDir       );
    strcpy( echo_p[usCount].szFindAfter, szFindAfter );

    sprintf( szMessage, " [%d] Adding server \'%s\' to configuration.\n", usCount, echo_p[usCount].szServer );
    message_out( szMessage, MESSAGE_DIAGNOSTIC, usVerbosity );
    TRACE_L2(" [%d] Adding server \'%s\' to configuration", usCount, echo_p[usCount].szServer );

    if ( ++usCount >= US_MAX_SERVERS ) break;

    }
  message_out("\n", MESSAGE_DIAGNOSTIC, usVerbosity );
  fclose( pfConfig );

  return( usCount );
  }


/* -------------------------------------------------------------------------- *
 * SocketConnect                                                              *
 *                                                                            *
 * Open a TCP/IP socket connection to a remote server.                        *
 *                                                                            *
 * PARAMETERS:                                                                *
 *     PSZ    pszDestIP  : A string containing the IP address or hostname of  *
 *                         the server to connect to.                          *
 *     USHORT usDestPort : The IP port number on the server to connect to,    *
 *                         or 0 to use the default port (80).                 *
 *                                                                            *
 * RETURNS: ULONG                                                             *
 *     One of the following values:                                           *
 *       0        Connection error                                            *
 *       (other)  The connected socket                                        *
 * -------------------------------------------------------------------------- */
int SocketConnect( char *pszDestIP, unsigned short usDestPort, unsigned short usVerbosity, char *pszEchoReturn, int iLen )
  {
  SOCKADDR_IN destination;
  PHOSTENT    phost;
  char        szMessage[ US_SZPATH_MAX + US_SZHOSTNAME_MAX + 1 ];   // output message
  int         ulSocket = 0,
              ulRc;


  // Validate parameters
  if ( pszDestIP  == NULL ) pszDestIP  = SZ_DEFAULT_ECHO_SERVER;
  if ( usDestPort == 0 )    usDestPort = US_DEFAULT_ECHO_PORT;

  sprintf( szMessage, "Attempting connection: %s:%d\n", pszDestIP, usDestPort );
  message_out( szMessage, MESSAGE_VERBOSE, usVerbosity );
  TRACE_L2("Attempting connection: %s:%d", pszDestIP, usDestPort );

  // Create the socket
  ulSocket = socket( PF_INET, SOCK_STREAM, 0 );
  if ( ulSocket < 0 )
    {
    sprintf (pszEchoReturn, "Error creating socket (%d)", sock_errno() );
    TRACE("%s", pszEchoReturn);
    return( 0 );
    }

  // Resolve the hostname
  phost = gethostbyname( pszDestIP );
  if ( phost == NULL )
    {
    sprintf (pszEchoReturn, "Could not resolve host \'%.60s\'", pszDestIP );
    TRACE("%s", pszEchoReturn);
    return( 0 );
    }

  // Initialize the connection parameters
  memset( &destination, 0, sizeof(destination) );
  destination.sin_len    = sizeof( destination );
  destination.sin_family = AF_INET;
  destination.sin_port   = htons( usDestPort );
  memcpy( (char *) &destination.sin_addr, phost->h_addr, phost->h_length );

  // Connect the socket
  ulRc = connect( ulSocket, (struct sockaddr *) &destination, sizeof(destination) );
  if ( ulRc != 0 )
    {
    sprintf (pszEchoReturn, "Error connecting to server %.60s (%d)", pszDestIP, sock_errno() );
    TRACE("%s", pszEchoReturn);
    return( 0 );
    }

  return( ulSocket );
  }


/* -------------------------------------------------------------------------- *
 * SocketRead                                                                 *
 *                                                                            *
 * Attempt to read data from the specified socket, with error checking.       *
 * -------------------------------------------------------------------------- */
short SocketRead( int ulSocket, char *pszResponse, int iMaxLenResponse, char *pszEchoReturn, int iLenEcho )
  {
  int  ulRc,
              ulReady,
              ulBytes;
  char *pszData;
  int  readSocks[ 1 ];

  // Check for incoming data on the socket
  readSocks[ 0 ] = ulSocket;
  ulReady = os2_select( readSocks, 1, 0, 0, UL_SELECT_TIMEOUT );

  switch ( ulReady )
    {
    case -1:    // Socket select returned an error
      psock_errno("Error issuing select()");
      sprintf (pszEchoReturn, "Error issuing select() (%d)", sock_errno() );
      TRACE("%s", pszEchoReturn);
      return( 0 );
      break;

    case 0 :    // Socket select timed out
      sprintf (pszEchoReturn, "Timed out waiting for server to respond (%d)", sock_errno() );
      TRACE("%s", pszEchoReturn);
      return( 0 );
      break;

    default:    // Socket reports data available
      // Get the size of data to allocate our buffer
      ulRc = ioctl( ulSocket, FIONREAD, &ulBytes );
      if ( ulBytes == 0 )
        {
        printf("No data available from server.\n");
        sprintf (pszEchoReturn, "No data available from server (%d)", sock_errno() );
        TRACE("%s", pszEchoReturn);
        return( 0 );
        }
      else if ( ulRc == -1 )
        {
        psock_errno("Cannot get data from server");
        sprintf (pszEchoReturn, "Cannot get data from server (%d)", sock_errno() );
        TRACE("%s", pszEchoReturn);
        return( 0 );
        }
      ulBytes = ( ulBytes > UL_DATA_LIMIT ) ? UL_DATA_LIMIT : ulBytes;
      pszData = (char *) malloc( ulBytes );

      // Now read the data
      ulRc = recv( ulSocket, pszData, ulBytes, 0 );
      if ( ulRc == -1 )
        {
        psock_errno("Error reading socket");
        sprintf (pszEchoReturn, "Error reading socket (%d)", sock_errno() );
        TRACE("%s", pszEchoReturn);
        return( 0 );
        }

      if (ulBytes >= iMaxLenResponse )
        {
        ulBytes = iMaxLenResponse - 2;
        pszResponse[iMaxLenResponse - 1] = '\0';
        }
      strncpy( pszResponse, pszData, ulBytes );
      free( pszData );
      break;
    }

  return( 1 );
  }


/* -------------------------------------------------------------------------- *
 * SocketWrite                                                                *
 *                                                                            *
 * Write some data to the specified socket, with error checking.              *
 * -------------------------------------------------------------------------- */
short SocketWrite( int ulSocket, char *pszData )
  {
  int ulRc;

  if ( pszData == NULL ) return( 0 );
  ulRc = send( ulSocket, pszData, strlen(pszData), 0 );
  if ( ulRc < 0 )
    {
    psock_errno("Error writing to socket");
    return( 0 );
    }

  return( 1 );
  }


/* -------------------------------------------------------------------------- *
 * EchoQuery                                                                  *
 *                                                                            *
 * Query the server for our public IP address, and print the result.          *
 *                                                                            *
 * RETURNS: ULONG                                                             *
 *     1 .......... in case of error                                          *
 *     NO_ERROR ... otherwise                                                 *
 * -------------------------------------------------------------------------- */
ULONG EchoQuery( ECHOPARMS echo_p, USHORT usVerbosity, char *pszEchoReturn, int iLenEcho )
  {
  ULONG  ulRc;
  USHORT usTries = 0;
  CHAR   szCommand[ US_SZHOSTNAME_MAX + US_SZPATH_MAX + 256 ],
         szResponse[ US_SZRESPONSE_MAX + 1 ] = "";
  PSZ    pszOffset;
  int    ip1, ip2, ip3, ip4;

  VIOMODEINFO mode;
  USHORT      c, cols;
  CHAR        chSep;


  sprintf( szCommand, "GET http://%s%s HTTP/1.0\r\n\r\n", echo_p.szServer, echo_p.szDir );
//  sprintf( szCommand, "GET http://%s%s\r\n\r\n", echo_p.szServer, echo_p.szDir );
  ulRc = SocketWrite( echo_p.ulSocket, szCommand );
  // remove \r\n\r\n to beauty TRACE output
  szCommand[strlen(szCommand) - 3] = '\0';
  TRACE_L2("sending %s", szCommand);
  if ( ulRc == 0 ) return( 1 );

  TRACE("echo_p.szServer=%s" , echo_p.szServer);
  while ( ++usTries <= US_MAX_READS )
    {
    ulRc = SocketRead( echo_p.ulSocket, szResponse, sizeof(szResponse), pszEchoReturn, sizeof(pszEchoReturn) );
    TRACE("echo_p.szServer=%s" , echo_p.szServer);
    if ( ulRc ) break;
    }
  if ( usTries > US_MAX_READS )
    {
    sprintf (pszEchoReturn, "Could not get response from server.  Giving up.");
    return 1;
    }

  if ( usVerbosity >= MESSAGE_DIAGNOSTIC )
    {
    mode.cb = sizeof(VIOMODEINFO);
    ulRc = VioGetMode( &mode, 0 );
    cols = ( ulRc == NO_ERROR ) ? mode.col - 1 : 0;
    chSep = BoxChar('Ä', '=');
    for ( c = 0; c < cols; c++ ) printf("%c", chSep );
    printf("\nSERVER RESPONSE\n");
    TRACE_L2("SERVER RESPONSE");
    for ( c = 0; c < cols; c++ ) printf("%c", chSep );
    printf("\n%s\n", szResponse );
    TRACE_L2("%s", szResponse );
    for ( c = 0; c < cols; c++ ) printf("%c", chSep );
    printf("\n");
    }

  // Look for the "find after" string
  pszOffset = strstr( szResponse, echo_p.szFindAfter );
  if ( pszOffset == NULL ) pszOffset = szResponse;
  else pszOffset += strlen( echo_p.szFindAfter );

  // Now try and parse the IP address
  ulRc = sscanf( pszOffset, "%3d.%3d.%3d.%3d", &ip1, &ip2, &ip3, &ip4 );
  if ( ulRc != 4 )
    {
    sprintf (pszEchoReturn, "Could not parse IP address from server '%.60s', return string='%.60s'", echo_p.szServer, szResponse);
    TRACE("%s (%.60s)", pszEchoReturn, pszOffset);
    return( 1 );
    }
  message_out("Returned IP Address: ", MESSAGE_VERBOSE, usVerbosity );
  sprintf (pszEchoReturn, "%d.%d.%d.%d", ip1, ip2, ip3, ip4 );
  TRACE("Returned IP address %s", pszEchoReturn);

  return( NO_ERROR );
  }


/* -------------------------------------------------------------------------- *
 * BoxChar                                                                    *
 *                                                                            *
 * Decide if the current codepage is capable of printing the specified box-   *
 * drawing character (single or double line only).  Return the specified      *
 * fallback character if it can't.                                            *
 *                                                                            *
 * PARAMETERS:                                                                *
 *     CHAR  chPref : The preferred box-drawing character.                    *
 *     CHAR  chPlain: The fallback character to use if the codepage can't     *
 *                    handle chPref correctly.                                *
 *                                                                            *
 * RETURNS: CHAR                                                              *
 *     The character to use.                                                  *
 * -------------------------------------------------------------------------- */
CHAR BoxChar( CHAR chPref, CHAR chPlain )
  {
  ULONG ulCP[ 3 ],
              ulSize;

  if ( DosQueryCp( sizeof(ulCP), ulCP, &ulSize ) != NO_ERROR )
    return( chPlain );

  switch ( ulCP[0] )
    {
    case 437:
    case 850:
    case 852:
    case 855:
    case 857:
    case 859:
    case 860:
    case 861:
    case 862:
    case 863:
    case 865:
    case 866:
    case 869: return( chPref );  break;
    default:  break;
    }
  return( chPlain );
  }


/* ------------------------------------------------------------------------- *
 * strstrip                                                                  *
 *                                                                           *
 * A rather quick-and-dirty function to strip leading and trailing white     *
 * space from a string.                                                      *
 *                                                                           *
 * ARGUMENTS:                                                                *
 *   PSZ s: The string to be stripped.  This parameter will contain the      *
 *          stripped string when the function returns.                       *
 *                                                                           *
 * RETURNS: PSZ                                                              *
 *   A pointer to the string s.                                              *
 * ------------------------------------------------------------------------- */
char *strstrip( char *s )
  {
  int  next,
              last,
              i,
              len,
              newlen;
  char *s2;

  len  = strlen( s );
  next = strspn( s, " \t\n\r");
  for ( i = len - 1; i >= 0; i-- )
    {
    if ( s[i] != ' ' && s[i] != '\t' && s[i] != '\r' && s[i] != '\n' ) break;
    }
  last = i;
  if ( ( next >= len ) || ( next > last ) )
    {
    memset( s, 0, len+1 );
    return s;
    }

  newlen = last - next + 1;
  s2 = (char *) malloc( newlen + 1 );
  i = 0;
  while ( next <= last )
    s2[i++] = s[next++];
  s2[i] = 0;

  memset( s, 0, len+1 );
  strncpy( s, s2, newlen );
  free( s2 );

  return( s );
  }


/* -------------------------------------------------------------------------- *
 * message_out                                                                *
 *                                                                            *
 * Prints the specified message if its designated verbosity level falls under *
 * the current verbosity level.                                               *
 *                                                                            *
 * ARGUMENTS:                                                                 *
 *   PSZ pszText       : The message to be printed                            *
 *   USHORT usLevel    : The required verbosity of the message                *
 *   USHORT usVerbosity: The actual verbosity in effect                       *
 *                                                                            *
 * RETURNS: n/a                                                               *
 * -------------------------------------------------------------------------- */
void message_out( PSZ pszText, USHORT usLevel, USHORT usVerbosity )
  {
  if ( usVerbosity >= usLevel ) printf("%s", pszText );
  }


//
// print help (usage) info
//
int usage(void)
  {

  fprintf(stderr, "\n%s  Ver: %d.%02d.%02d by AB  Build: %s\n"
          " checks periodically for activ internet connection \n"
          " and returns your IP address seen by the outside world\n\n"
          "Usage: %s [options...]\n"
          " valid options:\n"
          "  -d: debug output\n"
          "  -D: even more debug output\n"
          "  -f <filename>: configuration file (default %%ETC%%\\MyIP.cfg)\n"
          "  -l <filename>: write to log file\n"
          "  -r <1-3600>: number of seconds between checking IP address\n"
          "  -c <0-3600>: output only when result differs from last but at least every\n"
          "      multiple of r (with '-r 2 -c 1800' at least once a hour)\n"
          "  -t <0-2>: display time before IP address (default 0)\n"
          "      0...no time info at all\n"
          "      1...YYYY-MM-DD HH:MM:SS in front of IP address\n"
          "      2...YYYYMMDD HH:MM:SS in front of IP address\n"
          "  -s \"text\": display 'text' before IP address (default is one space)\n"
          "  -p <1-300>: number of seconds between check connection to the ping server\n"
          "    (value should be smaller than -r value)\n"
          "  -h: display help (this screen)\n"\
          , szAppName, VER_MAJOR, VER_MINOR, VER_BUILD, pszBLD_DATE_TIME, szAppName);

  fprintf(stderr, "\nFor examples see sample.cmd\n");

  TRACE(" MyIP v%d.%02d.%02d   Build: %s", VER_MAJOR, VER_MINOR, VER_BUILD, pszBLD_DATE_TIME );
  return 0;
  }


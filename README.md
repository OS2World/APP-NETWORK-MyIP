# APP-NETWORK-MyIP
MYIP is a simple TCP/IP utility for querying the current system's IP address
**as seen by the outside world**.  This may or may not be the same as the
locally-configured IP address, depending on how the system is connected to
the Internet.

When run, MYIP contacts a special website which provides an 'IP echo'
service: this service reports the IP address of your computer as the remote
server sees it.  This allows you to determine your public IP address even if
you are connecting through a firewall or router that uses port-level network
address translation (also known as IP masquerading or IP address sharing).

For full description see MyIP.txt. Look at the sample .cmd files.

## LICENSE
* GNU GPL V2

## COMPILE TOOLS
* wcc386
* wlink
 
## AUTHORS
* Alex Taylor
* Andreas Buchinger

## LINKS
* https://bitbucket.org/Andi_B/myip/src/trunk/


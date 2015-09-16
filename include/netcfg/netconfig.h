#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ifaddrs.h>
#include <netinet/in.h> 
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <string.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <net/if.h>
#include <net/if_arp.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <errno.h>
#include <sys/signal.h>
#include <net/route.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <termio.h>
#include <pthread.h>

#define BUFSIZE         8192
#define DEFDATALEN      56
#define MAXIPLEN        60
#define MAXICMPLEN      76

// Struct to store gateway info
struct route_info {
  struct in_addr dstAddr;
  struct in_addr srcAddr;
  struct in_addr gateWay;
  char ifName[IF_NAMESIZE];
};

// Struct to pass arguments to ping
struct arg_struct{
  char host[16];
  char net_i[8];
};

namespace NetConfig
{
  class EthernetConfig
  {
    private:
      pthread_t pingThread;
      char ping_host[16];
      int pingResult;

      char iface[8];
      char ip[16];
      char gateway[16];
      char netmask[16];
      char dns[16];
      int setInterfaceDown();

    public:
      EthernetConfig(char*);
      int setActiveInterface();
      char* getActiveInterface();
      char* getIPAddr();
      char* getGateway();
      char* getMask();
      char* getDNS();
      int setIPAddr(char*);
      int setGateway(char*);
      int setMask(char*);
      int setDNS(char*);

      /*Ping function will not be implemented in initial version
      void* runPing(void*);
      static void* callRunPing(void *arg)
      { 
        return ((EthernetConfig*)arg)->runPing();
      }
      int pingInCkSum(unsigned short *buf, int sz);
      void pingNoAnswer(int ign);
      int ping(const char *host);
      int testConnection();
      */
  };
}

#include <stdio.h>
#include <stdlib.h>
#include "netcfg/netconfig.h"

using namespace NetConfig;

int main(){

  EthernetConfig eth0((char*)"eth0");

  printf("IP: %s\n",eth0.getIPAddr());
  printf("Gateway: %s\n",eth0.getGateway());
  printf("NetMask: %s\n",eth0.getMask());
  printf("DNS: %s\n",eth0.getDNS());
  printf("MAC: %s\n",eth0.getMAC(1));

  return(0);
}

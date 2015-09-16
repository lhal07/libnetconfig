#include "netcfg/netconfig.h"

namespace NetConfig
{

	EthernetConfig::EthernetConfig(char* interface)
	{
		strcpy(this->iface, interface);
	}

	int EthernetConfig::setInterfaceDown()
	{
		char command[64];

		sprintf(command,"/sbin/ifconfig %s 0.0.0.0", this->iface);
		printf("command: %s\n",command);
		system(command);

		return(EXIT_SUCCESS);
	}

	int EthernetConfig::setActiveInterface()
	{
		char command[64];

		sprintf(command,"/sbin/ifconfig %s up", this->iface);
		system(command);

		return(EXIT_SUCCESS);
	}

	char* EthernetConfig::getActiveInterface()
	{
		char          buf[1024];
		struct ifconf ifc;
		struct ifreq *ifr;
		int           sck;
		int           nInterfaces;
		int           i;
		char* ret = (char*) malloc(sizeof(char)*8);
    strcpy(ret,"");

		/* Get a socket handle. */
		sck = socket(AF_INET, SOCK_DGRAM, 0);
		if(sck < 0)
		{
			perror("socket");
			return(ret);
		}

		/* Query available interfaces. */
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = buf;
		if(ioctl(sck, SIOCGIFCONF, &ifc) < 0)
		{
			close(sck);
			perror("ioctl(SIOCGIFCONF)");
			return(ret);
		}

		/* Iterate through the list of interfaces. */
		ifr         = ifc.ifc_req;
		ioctl(sck, SIOCGIFFLAGS, (caddr_t)ifr);
		nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
		struct ifreq *item;
		for(i = 0; i < nInterfaces; i++)
		{
			item = &ifr[i];

			if((item->ifr_flags)&IFF_LOOPBACK) continue;
			if((item->ifr_flags)&IFF_UP) break;
			//printf("%s\n",item->ifr_name);
		}
		strcpy(this->iface,item->ifr_name);
		strcpy(ret,item->ifr_name);

		close(sck);
		return(ret);
	}

	char* EthernetConfig::getIPAddr()
	{
		struct ifaddrs *ifaddr, *ifa;
		int s;
		char* ret_ip = (char*) malloc(sizeof(char)*16);
		sprintf(ret_ip, "0.0.0.0");

		if (getifaddrs(&ifaddr) == -1)
		{
			perror("getifaddrs");
			return(ret_ip);
		}

		/* Walk through linked list, maintaining head pointer so we
			 can free list later */
		for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
		{
			if (ifa->ifa_addr == NULL) continue;
			if (ifa->ifa_addr->sa_family != AF_INET) continue;
			if (strcmp(ifa->ifa_name,this->iface)) continue;

			/* For an AF_INET* interface address, display the address */
			s = getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),
					this->ip, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

			if (s != 0)
			{
				printf("getnameinfo() failed: %s\n", gai_strerror(s));
				return(ret_ip);
			}
		}

		freeifaddrs(ifaddr);
		strcpy(ret_ip,this->ip);
		return(ret_ip);
	}

	char* EthernetConfig::getGateway(){
		struct nlmsghdr *nlMsg;
		struct route_info *rtInfo;
		char msgBuf[BUFSIZE];
		char* ret_gw = (char*) malloc(sizeof(char)*16);
		sprintf(ret_gw, "0.0.0.0");

		struct nlmsghdr *nlHdr2;
		int readLen = 0, msgLen = 0;

		int sock, msgSeq = 0;

		sprintf(this->gateway, "0.0.0.0");

		if ((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
		{
			perror("GetGateway: Socket Creation: ");
			return(ret_gw);
		}

		if(setsockopt(sock, SOL_SOCKET, SO_BINDTODEVICE, this->iface, strlen(this->iface)) < 0)
		{
			close(sock);
			perror("GetGateway: setting socket network interface");
			return(ret_gw);
		}

		memset(msgBuf, 0, BUFSIZE);

		/* point the header and the msg structure pointers into the buffer */
		nlMsg = (struct nlmsghdr *) msgBuf;

		/* Fill in the nlmsg header*/
		nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));  // Length of message.
		nlMsg->nlmsg_type = RTM_GETROUTE;   // Get the routes from kernel routing table .

		nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST;    // The message is a request for dump.
		nlMsg->nlmsg_seq = msgSeq++;    // Sequence of the message packet.
		nlMsg->nlmsg_pid = getpid();    // PID of process sending the request.

		/* Send the request */
		if (send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0) {
			close(sock);
			perror("GetGateway: Write To Socket Failed...");
			return(ret_gw);
		}

		char* bufPtr = msgBuf;

		/* Read the response */
		do {
			/* Recieve response from the kernel */
			if ((readLen = recv(sock, bufPtr, BUFSIZE - msgLen, 0)) < 0) {
				close(sock);
				perror("GetGateway: SOCK READ: ");
				return(ret_gw);
			}

			nlHdr2 = (struct nlmsghdr *) bufPtr;

			/* Check if the header is valid */
			if ((NLMSG_OK(nlHdr2, readLen) == 0)
					|| (nlHdr2->nlmsg_type == NLMSG_ERROR)) {
				close(sock);
				perror("GetGateway: Error in recieved packet");
				return(ret_gw);
			}

			/* Check if the its the last message */
			if (nlHdr2->nlmsg_type == NLMSG_DONE) {
				break;
			} else {
				/* Else move the pointer to buffer appropriately */
				bufPtr += readLen;
				msgLen += readLen;
			}

			/* Check if its a multi part message */
			if ((nlHdr2->nlmsg_flags & NLM_F_MULTI) == 0) {
				/* return if its not */
				break;
			}
		} while ((nlHdr2->nlmsg_seq != (unsigned int)msgSeq) || (nlHdr2->nlmsg_pid != (unsigned int)getpid()));

		if (msgLen  < 0) {
			close(sock);
			perror("GetGateway: Read From Socket Failed...\n");
			return(ret_gw);
		}

		/* Parse and print the response */
		rtInfo = (struct route_info *) malloc(sizeof(struct route_info));
		for (; NLMSG_OK(nlMsg, msgLen); nlMsg = NLMSG_NEXT(nlMsg, msgLen)) {
			memset(rtInfo, 0, sizeof(struct route_info));

			struct rtmsg *rtMsg;
			struct rtattr *rtAttr;
			int rtLen;

			rtMsg = (struct rtmsg *) NLMSG_DATA(nlMsg);

			/* If the route is not for AF_INET or does not belong to main routing table
				 then return. */
			if ((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
				break;

			/* get the rtattr field */
			rtAttr = (struct rtattr *) RTM_RTA(rtMsg);
			rtLen = RTM_PAYLOAD(nlMsg);
			for (; RTA_OK(rtAttr, rtLen); rtAttr = RTA_NEXT(rtAttr, rtLen)) {
				if (rtAttr->rta_type == RTA_OIF)
					if_indextoname(*(int *) RTA_DATA(rtAttr), rtInfo->ifName);
				if (rtAttr->rta_type == RTA_GATEWAY)
					rtInfo->gateWay.s_addr= *(u_int *) RTA_DATA(rtAttr);
				if (rtAttr->rta_type == RTA_PREFSRC)
					rtInfo->srcAddr.s_addr= *(u_int *) RTA_DATA(rtAttr);
				if (rtAttr->rta_type == RTA_DST)
					rtInfo->dstAddr .s_addr= *(u_int *) RTA_DATA(rtAttr);
			}

			if ((rtInfo->dstAddr.s_addr == 0) && (!strcmp(rtInfo->ifName,iface)))
				strcpy(this->gateway, (char *) inet_ntoa(rtInfo->gateWay));

		}
		free(rtInfo);
		close(sock);

		strcpy(ret_gw,this->gateway);
		return(ret_gw);
	}

	char* EthernetConfig::getMask() {
		struct ifconf ifc;
		struct ifreq ifr[10];
		int sd, ifc_num, mask, i;

		/* Create a socket so we can use ioctl on the file
		 * descriptor to retrieve the interface info.
		 */

		char* ret_mask = (char*) malloc(sizeof(char)*16);
		sprintf(ret_mask, "0.0.0.0");

		sd = socket(PF_INET, SOCK_DGRAM, 0);
		if (sd > 0)
		{
			ifc.ifc_len = sizeof(ifr);
			ifc.ifc_ifcu.ifcu_buf = (caddr_t)ifr;

			if (ioctl(sd, SIOCGIFCONF, &ifc) == 0)
			{
				ifc_num = ifc.ifc_len / sizeof(struct ifreq);

				for (i = 0; i < ifc_num; ++i)
				{
					if (ifr[i].ifr_addr.sa_family != AF_INET) continue;
					if (strcmp(ifr[i].ifr_name,this->iface)) continue;

					/* Retrieve the IP address, broadcast address, and subnet mask. */
					if (ioctl(sd, SIOCGIFNETMASK, &ifr[i]) == 0)
					{
						mask = ((struct sockaddr_in *)(&ifr[i].ifr_netmask))->sin_addr.s_addr;
						strcpy(ret_mask,inet_ntoa(*(struct in_addr *)&mask));
					}
				}
			}
			close(sd);
			strcpy(this->netmask,ret_mask);
		}

		return(ret_mask);
	}

	char* EthernetConfig::getDNS(){

		FILE * pFile;
		char* ret_dns = (char*) malloc(sizeof(char)*16);
		sprintf(ret_dns, "0.0.0.0");

		pFile = fopen ("/etc/resolv.conf","r");
		if( pFile != NULL)
		{
			fscanf (pFile, "nameserver %s", ret_dns);
			fclose (pFile);
		}

		strcpy(this->dns,ret_dns);
		/* return number of read characters */
		return(ret_dns);
	}

	int EthernetConfig::setIPAddr(char* ip_addr){
		int sockfd;
		struct ifreq ifr;
		struct sockaddr_in sin;


		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if(sockfd == -1){
			fprintf(stderr, "Could not get socket.\n");
			return(EXIT_FAILURE);
		}

		/* get interface name */
		strncpy(ifr.ifr_name, this->iface, IFNAMSIZ);

		/* Read interface flags */
		if (ioctl(sockfd, SIOCGIFFLAGS, &ifr) < 0) {
			fprintf(stderr, "ifdown: shutdown ");
			perror(ifr.ifr_name);
			close(sockfd);
			return(EXIT_FAILURE);
		}

		/*
		 * Expected in <net/if.h> according to
		 * "UNIX Network Programming".
		 */
#ifdef ifr_flags
# define IRFFLAGS       ifr_flags
#else   /* Present on kFreeBSD */
# define IRFFLAGS       ifr_flagshigh
#endif

		// If interface is down, bring it up
		if (ifr.IRFFLAGS | ~(IFF_UP)) {
			ifr.IRFFLAGS |= IFF_UP;
			if (ioctl(sockfd, SIOCSIFFLAGS, &ifr) < 0) {
				fprintf(stderr, "ifup: failed ");
				perror(ifr.ifr_name);
				close(sockfd);
				return(EXIT_FAILURE);
			}
		}

		sin.sin_family = AF_INET;

		// Convert IP from numbers and dots to binary notation
		inet_aton(ip_addr,(struct in_addr *)&sin.sin_addr.s_addr);
		memcpy(&ifr.ifr_addr, &sin, sizeof(struct sockaddr));

		// Set interface address
		if (ioctl(sockfd, SIOCSIFADDR, &ifr) < 0) {
			fprintf(stderr, "Cannot set IP address. ");
			perror(ifr.ifr_name);
			close(sockfd);
			return(EXIT_FAILURE);
		}
#undef IRFFLAGS


		close(sockfd);
		return(EXIT_SUCCESS);
	}

	int EthernetConfig::setGateway(char * gateway){

		int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

		if (sockfd == -1) return(EXIT_FAILURE);
		struct sockaddr_in *dst, *gw, *mask;
		struct rtentry route;

		memset(&route, 0, sizeof (struct rtentry));

		dst = (struct sockaddr_in *) (&(route.rt_dst));
		gw = (struct sockaddr_in *) (&(route.rt_gateway));
		mask = (struct sockaddr_in *) (&(route.rt_genmask));

		// Make sure we're talking about IP here
		dst->sin_family = AF_INET;
		gw->sin_family = AF_INET;
		mask->sin_family = AF_INET;

		// Set up the data for removing the default route
		dst->sin_addr.s_addr = 0;
		gw->sin_addr.s_addr = 0;
		mask->sin_addr.s_addr = 0;
		route.rt_flags = RTF_UP | RTF_GATEWAY;

		// Remove the default route
		ioctl(sockfd, SIOCDELRT, &route);

		// Set up the data for adding the default route
		dst->sin_addr.s_addr = 0;
		gw->sin_addr.s_addr = inet_addr(gateway);
		mask->sin_addr.s_addr = 0;
		route.rt_metric = 1;
		route.rt_flags = RTF_UP | RTF_GATEWAY;

		// Remove this route if it already exists
		ioctl(sockfd, SIOCDELRT, &route);

		//Add the route
		ioctl(sockfd, SIOCADDRT, &route);

		shutdown(sockfd, SHUT_RDWR);
		close(sockfd);

		return(EXIT_SUCCESS);
	}

	int EthernetConfig::setMask(char *Mask) {
		int sock=0;
		struct sockaddr_in* mask=NULL;
		struct ifreq ifr;

		sock = socket( PF_INET, SOCK_DGRAM, 0 );
		if(!sock)
		{
			fprintf(stderr,"Cannot obtain socket\n");
			return(EXIT_FAILURE);
		}

		memset(&ifr,0,sizeof( struct ifreq ) );
		strncpy(ifr.ifr_name,this->iface,IFNAMSIZ);

		mask = (struct sockaddr_in *)&(ifr.ifr_addr);
		mask->sin_family=AF_INET;
		mask->sin_addr.s_addr=inet_addr(Mask);


		if( ioctl( sock, SIOCSIFNETMASK, &ifr ) != 0 )
		{
			printf("Cannot set subnet mask");
			close(sock);
			return(EXIT_FAILURE);
		}
		close(sock);
		return(EXIT_SUCCESS);
	}

	int EthernetConfig::setDNS(char* dns_address){

		FILE * pFile;

		pFile = fopen ("/etc/resolv.conf","w");
		fprintf (pFile,"nameserver %s\n", dns_address);
		fclose (pFile);

		return(EXIT_SUCCESS);
	}

#if 0 /*Ping function will not be implemented in initial version*/
	int EthernetConfig::pingInCkSum(unsigned short *buf, int sz)
	{
		int nleft = sz;
		int sum = 0;
		unsigned short *w = buf;
		unsigned short ans = 0;

		while (nleft > 1) {
			sum += *w++;
			nleft -= 2;
		}

		if (nleft == 1) {
			*(unsigned char *) (&ans) = *(unsigned char *) w;
			sum += ans;
		}

		sum = (sum >> 16) + (sum & 0xFFFF);
		sum += (sum >> 16);
		ans = ~sum;
		return (ans);
	}

	void EthernetConfig::pingNoAnswer(int ign)
	{
		pthread_cancel(pingThread);
		return;
	}

	void *EthernetConfig::runPing(void*){
		struct hostent *h;
		struct sockaddr_in pingaddr;
		struct icmp *pkt;
		int pingsock, c;
		char packet[DEFDATALEN + MAXIPLEN + MAXICMPLEN];

		if ((pingsock = socket(AF_INET, SOCK_RAW, 1)) < 0) {       /* 1 == ICMP */
			perror("ping: creating a raw socket");
			return 0;
		}

		if(setsockopt(pingsock, SOL_SOCKET, SO_BINDTODEVICE, this->iface, strlen(this->iface)) < 0){
			fprintf(stderr,"ping: setting socket network interface: %s\n",this->iface);
			return 0;
		}

		/* drop root privs if running setuid */
		setuid(getuid());

		memset(&pingaddr, 0, sizeof(struct sockaddr_in));

		pingaddr.sin_family = AF_INET;
		if (!(h = gethostbyname(ping_host))) {
			fprintf(stderr,"ping: unknown host %s\n", ping_host);
			return 0;
		}
		memcpy(&pingaddr.sin_addr, h->h_addr, sizeof(pingaddr.sin_addr));
		pkt = (struct icmp *) packet;
		memset(pkt, 0, sizeof(packet));
		pkt->icmp_type = ICMP_ECHO;
		pkt->icmp_cksum = pingInCkSum((unsigned short *) pkt, sizeof(packet));

		c = sendto(pingsock, packet, sizeof(packet), 0,
				(struct sockaddr *) &pingaddr, sizeof(struct sockaddr_in));

		if (c < 0 || c != sizeof(packet)) {
			if (c < 0)
				perror("ping: sendto");
			perror("ping: write incomplete\n");
			return 0;
		}

		/* listen for replies */
		while (1) {
			struct sockaddr_in from;
			size_t fromlen = sizeof(from);

			if ((c = recvfrom(pingsock, packet, sizeof(packet), 0,
							(struct sockaddr *) &from, &fromlen)) < 0) {
				if (errno == EINTR)
					continue;
				perror("ping: recvfrom");
				continue;
			}
			if (c >= 76) {                   /* ip + icmp */
				struct iphdr *iphdr = (struct iphdr *) packet;

				pkt = (struct icmp *) (packet + (iphdr->ihl << 2));      /* skip ip hdr */
				if (pkt->icmp_type == ICMP_ECHOREPLY){
					pingResult = 1;
					return 0;
				}
			}
		}
	}

	int EthernetConfig::ping(const char *host)
	{
		pingResult = 0;
		strcpy(ping_host,host);

		pthread_create(&pingThread, NULL, EthernetConfig::callRunPing, this);

		signal(SIGALRM, pingNoAnswer);
		alarm(5);    /* give the host 5000ms to respond */
		pthread_join(pingThread, NULL);

		return(pingResult);
	}

	int EthernetConfig::testConnection()
	{
		char dns[16];
		int status;

		strcpy(dns,"8.8.8.8");
		status = this->ping(dns);

		if (!status){
			strcpy(dns,"208.67.222.222");
			status = this->ping(dns);
		}

		return(status);
	}
#endif
}

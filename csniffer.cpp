// =====================================================================================
// 
//       Filename:  csniffer.cpp
//
//    Description:  网络数据嗅探类的类实现文件
//
//        Version:  1.0
//        Created:  2013年01月23日 14时05分49秒
//       Revision:  none
//       Compiler:  g++
//
//         Author:  Hurley (LiuHuan), liuhuan1992@gmail.com
//        Company:  Class 1107 of Computer Science and Technology
// 
// =====================================================================================

#ifndef WIN32
	#include <sys/socket.h>
	#include <netinet/in.h>
#else
	#include <pcap.h>
	#define WPCAP
	#define HAVE_REMOTE 
	#include <remote-ext.h>
	#pragma comment(lib, "wpcap.lib")
	#pragma comment(lib, "Packet.lib")
	#pragma comment(lib, "ws2_32.lib")
	#pragma warning(disable:4996)
	#include <winsock.h>
#endif

#include "CSniffer.h"

CSniffer::CSniffer()
{
	pAllNetDevs = NULL;
	iNetDevsNum = 0;
	adHandle    = NULL;
	pDumpFile	= NULL;
}

CSniffer::~CSniffer()
{
	freeNetDevsMem();
}

bool CSniffer::findAllNetDevs(char *szFlag)
{
	freeNetDevsMem();

	if (pcap_findalldevs_ex(szFlag, NULL, &pAllNetDevs, errbuf) == -1) {
		return false;
	}

	for (pcap_if_t *index = pAllNetDevs; index != NULL; index = index->next) {
		iNetDevsNum++;
	}

	return true;
}

void CSniffer::createDevsStr(char *source, char *szFileName)
{
	pcap_createsrcstr(source,				// 源字符串
						PCAP_SRC_FILE,		// 我们要打开的文件
						NULL,				// 远程主机
						NULL,				// 远程主机端口
						szFileName,			// 我们要打开的文件名
						errbuf);			// 错误缓冲区
}

bool CSniffer::openNetDev(char * szDevName, int flag)
{
	if (adHandle != NULL) {
		closeNetDev();
	}
	
	adHandle = pcap_open_live(szDevName,	// 设备名
				65536,						// 65536保证能捕获到不同数据链路层上的每个数据包的全部内容
				flag,						// 网卡设置打开模式
				1000,						// 读取超时时间
				errbuf);					// 错误缓冲

	if (adHandle == NULL) {
		return false;
	}
	
	return true;
}

bool CSniffer::openNetDev(int iDevNum, int flag)
{
	if (iDevNum < 1 || iDevNum > iNetDevsNum) {
		return false;
	}

	pcap_if_t *index = pAllNetDevs;

	for (int i = 0; i < iNetDevsNum-1; i++) {
		index = index->next;
	}

	if (adHandle != NULL) {
		closeNetDev();
	}

	adHandle = pcap_open_live(index->name,	// 设备名
				65536,						// 65536保证能捕获到不同数据链路层上的每个数据包的全部内容
				flag,						// 网卡设置打开模式
				1000,						// 读取超时时间
				errbuf);					// 错误缓冲

	if (adHandle == NULL) {
		return false;
	}
	
	return true;
}

bool CSniffer::closeNetDev()
{
	if (adHandle != NULL) {
		pcap_close(adHandle);
		adHandle = NULL;
		return true;
	}

	return false;
}

void CSniffer::freeNetDevsMem()
{
	if (pAllNetDevs) {
		pcap_freealldevs(pAllNetDevs);
		pAllNetDevs = NULL;
	}
}

bool CSniffer::setDevsFilter(char *szFilter)
{
	// 检查数据链路层，只考虑以太网
	if ( pcap_datalink( adHandle ) != DLT_EN10MB ) {
		return false;
	}

	u_int	netmask;

	// 获取接口第一个地址的掩码
	if ( pAllNetDevs->addresses != NULL ) {
		netmask = ((struct sockaddr_in *)(pAllNetDevs->addresses->netmask))->sin_addr.S_un.S_addr;
	} else {	
		netmask = 0xFFFFFF;		// 如果这个接口没有地址，那么我们假设这个接口在C类网络中	
	}

	struct bpf_program	fcode;

	// 布尔表达式转换过滤引擎能识别的字节码
	if (pcap_compile(adHandle, &fcode, szFilter, 1, netmask) < 0) {
		return false;
	}
	if (pcap_setfilter(adHandle, &fcode) < 0) {
		fprintf(stderr,"\nError setting the filter.\n");
		return false;
	}

	return true;
}

bool CSniffer::captureByCallBack(pSnifferCB func)
{
	if (adHandle != NULL) {
		pcap_loop(adHandle, 0, func, (unsigned char *)pDumpFile);
		return true;
	}

	return false;
}

int CSniffer::captureOnce()
{
	return pcap_next_ex( adHandle, &header, &pkt_data);
}

bool CSniffer::openDumpFile(char *szFileName)
{
	if ((pDumpFile = pcap_dump_open(adHandle, szFileName)) != NULL) {
		return true;
	}

	return false;
}

void CSniffer::saveCaptureData(u_char *dumpfile, struct pcap_pkthdr *header, u_char *pkt_data)
{
	pcap_dump(dumpfile, header, pkt_data);
}

void CSniffer::closeDumpFile()
{
	if (pDumpFile != NULL) {
		pcap_dump_close(pDumpFile);
		pDumpFile = NULL;
	}
}

void CSniffer::consolePrint()
{
	pcap_if_t	*index;
	pcap_addr_t *pAddr;
	int	i = 0;
	char ip6str[128];

	for (index = pAllNetDevs; index != NULL; index = index->next) {
		printf( "%d. %s\n", ++i, index->name );

		if (index->description) {
			printf(" (%s)\n\n", index->description);
		} else {
			printf(" (No description available)\n\n");
		}

		for (pAddr = index->addresses; pAddr != NULL; pAddr = pAddr->next) {
			printf("Address Family: #%d\n", pAddr->addr->sa_family);
			
			switch(pAddr->addr->sa_family)
			{
			case AF_INET:
				printf("\tAddress Family Name: AF_INET(IPV4)\n");
				if (pAddr->addr) {
					printf("\tAddress: %s\n",
						iptos(((struct sockaddr_in *)pAddr->addr)->sin_addr.s_addr));
				}
				if (pAddr->netmask) {
					printf("\tNetmask: %s\n",
						iptos(((struct sockaddr_in *)pAddr->netmask)->sin_addr.s_addr));
				}
				if (pAddr->broadaddr) {
					printf("\tBroadcast Address: %s\n",
						iptos(((struct sockaddr_in *)pAddr->broadaddr)->sin_addr.s_addr));
				}
				if (pAddr->dstaddr) {
					printf("\tDestination Address: %s\n",
						iptos(((struct sockaddr_in *)pAddr->dstaddr)->sin_addr.s_addr));
				}
				break;
			case AF_INET6:
				printf("\tAddress Family Name: AF_INET6(IPV6)\n");
				if (pAddr->addr) {
					printf("\tAddress: %s\n", ip6tos(pAddr->addr, ip6str, sizeof(ip6str)));
				}
				break;
			default:
				printf("\tAddress Family Name: Unknown\n");
			}
			printf("\n");
		}
	}
}

char *CSniffer::iptos(u_long in)
{
	static char output[IPTOSBUFFERS][3*4+3+1];
	static short which;
	u_char *p;

	p = (u_char *)&in;
	which = (which + 1 == IPTOSBUFFERS ? 0 : which + 1);
	sprintf(output[which], "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);

	return output[which];
}

char *CSniffer::ip6tos(struct sockaddr *sockaddr, char *address, int addrlen)
{
	socklen_t sockaddrlen;

#ifdef WIN32
	sockaddrlen = sizeof(struct sockaddr_in6);
#else
	sockaddrlen = sizeof(struct sockaddr_storage);
#endif

	if (getnameinfo(sockaddr, sockaddrlen, address, addrlen, NULL, 0, NI_NUMERICHOST) != 0) {
		address = NULL;
	}

	return address;
}

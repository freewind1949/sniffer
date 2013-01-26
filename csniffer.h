// =====================================================================================
// 
//       Filename:  csniffer.h
//
//    Description:  网络数据嗅探类的类声明头文件
//
//        Version:  1.0
//        Created:  2013年01月23日 14时03分49秒
//       Revision:  none
//       Compiler:  g++
//
//         Author:  Hurley (LiuHuan), liuhuan1992@gmail.com
//        Company:  Class 1107 of Computer Science and Technology
// 
// =====================================================================================

#ifndef CSNIFFER_H_
#define CSNIFFER_H_

#define IPTOSBUFFERS    12

#ifndef WIN32
	
#else
	#include <pcap.h>
	#define WPCAP
	#define HAVE_REMOTE 
	#include <remote-ext.h>
#endif

class CSniffer
{
public:
	CSniffer();
	~CSniffer();

	typedef void (*pSnifferCB)(u_char *, const struct pcap_pkthdr *, const u_char *);

	bool findAllNetDevs(char *szFlag = PCAP_SRC_IF_STRING);						// 获取全部网络设备列表信息
	void createDevsStr(char *source, char *szFileName);							// WinPcap语法创建一个源字符串
	void freeNetDevsMem();														// 释放网络设备信息占据的堆内存
	bool openNetDev(char *szDevName, int flag = PCAP_OPENFLAG_PROMISCUOUS);		// 根据名称打开网络设备
	bool openNetDev(int iDevNum, int flag = PCAP_OPENFLAG_PROMISCUOUS);			// 根据序号打开网络设备
	bool closeNetDev();															// 关闭当前打开的网络设备
	bool setDevsFilter(char *szFilter);											// 对当前打开设备设置过滤器
	int	 captureOnce();															// 捕获一次网络数据包
	bool captureByCallBack(pSnifferCB func);									// 以回调函数方式捕获数据

	// 只有当接口打开时，调用 openDumpFile() 才是有效的
	bool openDumpFile(char *szFileName);										// 打开堆文件（文件保存数据包）
	void saveCaptureData(u_char *, struct pcap_pkthdr *, u_char *);				// 保存捕获的数据到文件
	void closeDumpFile();														// 关闭堆文件

	void consolePrint();														// 控制台打印网络设备信息
	
public:
	struct	pcap_pkthdr *header;
	const u_char		*pkt_data;

protected:
	char *iptos(u_long in);
	char *ip6tos(struct sockaddr *sockaddr, char *address, int addrlen);

	pcap_if_t		*pAllNetDevs;					// 网络设备列表信息链表指针
	int				iNetDevsNum;					// 网络设备数量
	pcap_t			*adHandle;						// 当前打开的设备句柄（指针）
	pcap_dumper_t	*pDumpFile;						// 要保存到的文件指针

	char errbuf[PCAP_ERRBUF_SIZE];					// 错误信息缓冲

};

#endif	// CSNIFFER_H_
#pragma once

#ifdef WIN32                                                            // windows platform

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#else                                                                   // linux platform

#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/poll.h>
#include <sys/file.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>

#endif

#include "xzero.h"
#include "xbuff.h"

#undef  KG_MAX_IPV4STR_LEN                                              // 192.168.255.255
#define KG_MAX_IPV4STR_LEN 15

#undef  KG_MIN_IPV4STR_LEN                                              // 1.1.1.1
#define KG_MIN_IPV4STR_LEN 7

#undef  KG_INVALID_SOCKET
#ifdef  KG_PLATFORM_WINDOWS                                             // windows platform
#define KG_INVALID_SOCKET INVALID_SOCKET                                // type(SOCKET) = UINT32, KG_INVALID_SOCKET = 0xFFFFFFFF
#else                                                                   // linux   platform
#define KG_INVALID_SOCKET -1                                            // type(SOCKET) = signed int,   KG_INVALID_SOCKET = 0xFFFFFFFF
#endif

#undef  KG_DEFAULT_ACCEPT_BACK_LOG
#define KG_DEFAULT_ACCEPT_BACK_LOG 5

#undef  KG_PROCESS_SOCKET_ERROR_Q
#define KG_PROCESS_SOCKET_ERROR_Q(s) \
    KG_PROCESS_ERROR_Q(KG_INVALID_SOCKET != s && s >= 0)

#undef  KG_MAX_PAK_HEAD_SIZE
#define KG_MAX_PAK_HEAD_SIZE 4

#undef  KG_PROCESS_SOCKET_ERROR
#define KG_PROCESS_SOCKET_ERROR(s) \
    KG_PROCESS_ERROR(KG_INVALID_SOCKET != s && s >= 0)

KG_NAMESPACE_BEGIN(xnet)

class KG_NetService : private xzero::KG_UnCopyable
{
    KG_SINGLETON_DCL(KG_NetService)

public:
    KG_NetService();
    ~KG_NetService();

public:
    bool Open(const WORD wHighVersion = 2, const WORD wLowVersion = 2);
    bool Close();

private:
    bool m_bStarted;
};

typedef KG_NetService *PKG_NetService;

inline KG_NetService::KG_NetService() : m_bStarted(false)
{
    Open();
    KG_ASSERT(m_bStarted  && "[ERROR] It seems net service hasn't been started!");
}

inline KG_NetService::~KG_NetService()
{
    Close();
    KG_ASSERT(!m_bStarted && "[ERROR] It seems net service hasn't been stopped!");
}

inline bool KG_NetService::Open(const WORD wHighVersion, const WORD wLowVersion)
{
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform

    int     nResult  = false;
    int     nRetCode = 0;
    WORD    nVersion = 0;
    WSADATA wsaData;

    KG_PROCESS_SUCCESS(m_bStarted);

    nVersion = MAKEWORD(wHighVersion, wLowVersion);
    nRetCode = ::WSAStartup(nVersion, &wsaData);
    KG_PROCESS_ERROR(0 == nRetCode);

Exit1:
    m_bStarted = true;
    nResult    = true;
Exit0:
    return nResult;

#else                                                                   // linux   platform

    m_bStarted = true;
    return true;

#endif
}

inline bool KG_NetService::Close()
{
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform

    int nResult  = false;
    int nRetCode = 0;

    KG_PROCESS_SUCCESS(!m_bStarted);

    nRetCode = ::WSACleanup();
    KG_PROCESS_ERROR(0 == nRetCode);

Exit1:
    m_bStarted = false;
    nResult    = true;
Exit0:
    return nResult;

#else                                                                   // linux   platform

    m_bStarted = false;
    return true;

#endif
}

bool KG_IsValidIpv4Str(const char * const cszIp);

KG_NAMESPACE_END

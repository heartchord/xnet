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

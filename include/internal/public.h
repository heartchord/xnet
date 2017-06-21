#pragma once

#ifdef WIN32                                                            // windows platform

#include <winsock2.h>
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

#undef KG_INVALID_SOCKET
#ifdef  KG_PLATFORM_WINDOWS                                             // windows platform
#define KG_INVALID_SOCKET INVALID_SOCKET                                // type(SOCKET) = unsigned int, KG_INVALID_SOCKET = 0xFFFFFFFF
#else                                                                   // linux   platform
#define KG_INVALID_SOCKET -1                                            // type(SOCKET) = signed int,   KG_INVALID_SOCKET = 0xFFFFFFFF
#endif

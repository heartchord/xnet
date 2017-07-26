#pragma once

#include "public.h"
#include "stream.h"

#undef KG_SOCKET_EVENT_INIT
#undef KG_SOCKET_EVENT_READ
#undef KG_SOCKET_EVENT_WRITE
#undef KG_SOCKET_EVENT_ACCEPT

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform(iocp)

#define KG_SOCKET_EVENT_INIT    0x00000000                              // the initialized state
#define KG_SOCKET_EVENT_READ    0x00000001                              // the state indicates some data has reached the socket now.
#define KG_SOCKET_EVENT_WRITE   0x00000004                              // the state indicates the socket can be written now.
#define KG_SOCKET_EVENT_ACCEPT  0x00800000                              // the state indicates one remote socket is connecting the listening socket.

#else                                                                   // linux   platform(epoll)

/*----------------------------------------------*/
/* Linux(epoll):                                */
/* #define EPOLLIN          0x00000001          */
/* #define EPOLLPRI         0x00000002          */
/* #define EPOLLOUT         0x00000004          */
/* #define EPOLLRDNORM      0x00000040          */
/* #define EPOLLRDBAND      0x00000080          */
/* #define EPOLLWRNORM      0x00000100          */
/* #define EPOLLWRBAND      0x00000200          */
/* #define EPOLLMSG         0x00000400          */
/* #define EPOLLERR         0x00000008          */
/* #define EPOLLHUP         0x00000010          */
/* #define EPOLLET          0x80000000          */
/* #define EPOLLONESHOT     0x40000000          */
/*-----------------------------------------------*/
#define KG_SOCKET_EVENT_INIT    0x00000000                              // the initialized state
#define KG_SOCKET_EVENT_READ    EPOLLIN                                 // the state indicates some data has reached the socket now.
#define KG_SOCKET_EVENT_WRITE   EPOLLOUT                                // the state indicates the socket can be written now.
#define KG_SOCKET_EVENT_ACCEPT  0x00800000                              // the state indicates one remote socket is connecting the listening socket.

#endif // KG_PLATFORM_WINDOWS

KG_NAMESPACE_BEGIN(xnet)

class KG_SocketEvent : private xzero::KG_UnCopyable
{
public:
    UINT32             m_uType;                                         // socket event type
    SPIKG_SocketStream m_spStream;                                      // the socket stream

public:
    KG_SocketEvent();
    ~KG_SocketEvent();
};

typedef KG_SocketEvent *                PKG_SocketEvent;
typedef std::shared_ptr<KG_SocketEvent> SPKG_SocketEvent;

KG_NAMESPACE_END

#pragma once

#include "xevent.h"

KG_NAMESPACE_BEGIN(xnet)

class KG_SocketAcceptor : private xzero::KG_UnCopyable
{
private:
    SOCKET m_nSocket;                                                 // listen socket

public:
    KG_SocketAcceptor();
    ~KG_SocketAcceptor();

public:
    bool Init(const char *pszIpAddr, USHORT uPort);
    bool Close();
    int  Accept(SPIKG_SocketStream &spSocketStream, const timeval *pcTimeout = NULL);
};

typedef KG_SocketAcceptor                 *PKG_SocketAcceptor;
typedef std::shared_ptr<KG_SocketAcceptor> SPKG_SocketAcceptor;

class KG_SocketEvent;
class KG_AsyncSocketAcceptor : private xzero::KG_UnCopyable
{
private:
    SOCKET                   m_nSocket;                                 // listen socket
    UINT32                   m_uMaxAcceptEachActivate;
    UINT32                   m_uRecvBuffSize;
    UINT32                   m_uSendBuffSize;
    KG_AsyncSocketStreamList m_StreamList;
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
#else                                                                   // linux   platform
    int                      m_nEpollHandle;
#endif // KG_PLATFORM_WINDOWS

public:
    KG_AsyncSocketAcceptor();
    ~KG_AsyncSocketAcceptor();

public:
    bool Init(const char *pszIpAddr, USHORT uPort, UINT32 uMaxAcceptEachActivate, UINT32 uRecvBuffSize, UINT32 uSendBuffSize);
    bool Close();
    bool Activate(UINT32 uMaxCount, UINT32 &uCurCount, KG_SocketEvent *pEventList);

private:
    bool _ProcessAccept(UINT32 uMaxCount, UINT32 &uCurCount, KG_SocketEvent *pEventList);
};

typedef KG_AsyncSocketAcceptor                 *PKG_AsyncSocketAcceptor;
typedef std::shared_ptr<KG_AsyncSocketAcceptor> SPKG_AsyncSocketAcceptor;


KG_NAMESPACE_END

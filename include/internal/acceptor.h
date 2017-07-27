#pragma once

#include "event.h"

#include <list>

KG_NAMESPACE_BEGIN(xnet)

class KG_SocketAcceptor : private xzero::KG_UnCopyable
{
private:
    SOCKET m_nSocket;                                                 // listen socket

public:
    KG_SocketAcceptor();
    ~KG_SocketAcceptor();

public:
    bool Init(const char * const cpcIpAddr, const USHORT uPort);
    bool Close();
    int  Accept(SPIKG_SocketStream &spSocketStream, const timeval *const cpcTimeout = NULL);
};

typedef KG_SocketAcceptor                 *PKG_SocketAcceptor;
typedef std::shared_ptr<KG_SocketAcceptor> SPKG_SocketAcceptor;

class KG_AsyncSocketStreamList : private xzero::KG_UnCopyable
{
private:
    std::list<SPIKG_SocketStream> m_StreamList;

public:
    KG_AsyncSocketStreamList();
    ~KG_AsyncSocketStreamList();

public:
    void Insert(SPIKG_SocketStream spStream);
    void Remove(SPIKG_SocketStream spStream);
    void Destroy();

    bool Activate(UINT32 uMaxCount, UINT32 &uCurCount, PKG_SocketEvent pEventList);

private:
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    void ProcessDestroy();
    int  ProcessRecvOrClose(UINT32 uMaxCount, UINT32 &uCurCount, PKG_SocketEvent pEventList);
#else                                                                   // linux   platform

#endif // KG_PLATFORM_WINDOWS
};

class KG_AsyncSocketAcceptor : private xzero::KG_UnCopyable
{
private:
    SOCKET m_nSocket;                                                 // listen socket
};

KG_NAMESPACE_END

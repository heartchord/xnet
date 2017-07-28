#pragma once

#include "event.h"

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

class KG_AsyncSocketAcceptor : private xzero::KG_UnCopyable
{
private:
    SOCKET m_nSocket;                                                 // listen socket
};

KG_NAMESPACE_END

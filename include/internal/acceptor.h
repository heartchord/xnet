#pragma once

#include "stream.h"

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

KG_NAMESPACE_END

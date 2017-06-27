#pragma once

#include "acceptor.h"

KG_NAMESPACE_BEGIN(xnet)

class IKG_ServerProxy : private xzero::KG_UnCopyable
{
public:
    IKG_ServerProxy() {}
    virtual ~IKG_ServerProxy() {}

public:
    virtual bool Close()    = 0;
    virtual bool Activate() = 0;

private:
    virtual bool _OnClientClosed   (SPIKG_SocketStream &spStream)                              = 0;
    virtual bool _OnClientConnected(SPIKG_SocketStream &spStream)                              = 0;
    virtual bool _OnClientDataRecvd(SPIKG_SocketStream &spStream, xbuff::SPIKG_Buffer &spBuff) = 0;
};

KG_NAMESPACE_END

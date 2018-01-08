#pragma once

#include "xacceptor.h"

#include <list>

KG_NAMESPACE_BEGIN(xnet)

class IKG_ServerProxy : private xzero::KG_UnCopyable
{
public:
    IKG_ServerProxy() {}
    virtual ~IKG_ServerProxy() {}

public:
    virtual bool Close()    = 0;
    virtual bool Activate() = 0;

protected:
    virtual bool _OnClientClosed   (SPIKG_SocketStream &spStream);
    virtual bool _OnClientConnected(SPIKG_SocketStream &spStream);
    virtual bool _OnClientDataRecvd(SPIKG_SocketStream &spStream, xbuff::SPIKG_Buffer &spBuff);
};

typedef IKG_ServerProxy                 *PIKG_ServerProxy;
typedef std::shared_ptr<IKG_ServerProxy> SPIKG_ServerProxy;

class KG_SingleClientServerProxy : public IKG_ServerProxy
{
public:
    KG_SingleClientServerProxy();
    virtual ~KG_SingleClientServerProxy();

public:
    bool Init(const char * const cszIpAddr, const USHORT uPort);
    virtual bool Close();
    virtual bool Activate();

protected:
    SPIKG_SocketStream  m_spSocketStream;
    SPKG_SocketAcceptor m_spSocketAcceptor;

private:
    void ProcessAccept();
    void ProcessPackage();
    void CloseConnection();
};

class KG_MultiClientServerProxy : public IKG_ServerProxy
{
    typedef std::list<SPIKG_SocketStream> KG_MySocketStreamList;
public:
    KG_MultiClientServerProxy();
    virtual ~KG_MultiClientServerProxy();

public:
    bool Init(const char * const cszIpAddr, const USHORT uPort);
    virtual bool Close();
    virtual bool Activate();

protected:
    SPKG_SocketAcceptor   m_spSocketAcceptor;
    KG_MySocketStreamList m_SocketStreamList;

private:
    void ProcessAccept();
    void ProcessPackage();
    void CloseConnection(SPIKG_SocketStream &spStream);
    int  ProcessOnePackage(SPIKG_SocketStream &spStream);
};

class KG_EventModelServerProxy : public IKG_ServerProxy
{
protected:
    SPKG_SocketEvent         m_spEventList;
    SPKG_AsyncSocketAcceptor m_spSocketAcceptor;                   // Socket Server Acceptor

public:
    KG_EventModelServerProxy();
    virtual ~KG_EventModelServerProxy();

public:
    bool Init(const char * const cszIpAddr, const USHORT uPort);
    virtual bool Close();
    virtual bool Activate();

private:
    bool ProcessNetEvent();
    void CloseConnection(SPIKG_SocketStream &spStream);
    bool ProcessOnePackage(SPIKG_SocketStream &spStream);
};

KG_NAMESPACE_END

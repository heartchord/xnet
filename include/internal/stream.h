#pragma once

#include "public.h"

KG_NAMESPACE_BEGIN(xnet)

class IKG_SocketStream : private xzero::KG_UnCopyable
{
public:
    IKG_SocketStream() {}
    virtual ~IKG_SocketStream() {}

public:
    virtual bool Open()  = 0;
    virtual bool Close() = 0;

    virtual int  CheckSend(const timeval * const cpcTimeOut = NULL) = 0;
    virtual int  CheckRecv(const timeval * const cpcTimeout = NULL) = 0;

    virtual int  Send(xbuff::SPIKG_Buffer &spBuffer, const UINT32 uPakHeadSize, const timeval *const cpcTimeOut = NULL) = 0;
    virtual int  Recv(xbuff::SPIKG_Buffer &spBuffer, const UINT32 uPakHeadSize, const timeval *const cpcTimeOut = NULL) = 0;

    virtual void SetConnIndex(const int nConnIndex) = 0;
    virtual int  GetConnIndex() const = 0;

    virtual bool IsAlive() = 0;
    virtual bool IsValid() = 0;

    virtual int  GetErrCode() const = 0;
    virtual void GetAddress(struct in_addr *pIp, USHORT *pnPort) const = 0;
};

typedef IKG_SocketStream *                PIKG_SocketStream;
typedef std::shared_ptr<IKG_SocketStream> SPIKG_SocketStream;

class KG_SocketStream : public IKG_SocketStream
{
public:
    KG_SocketStream();
    virtual ~KG_SocketStream();

public:
    bool Init(const SOCKET nSocket, const sockaddr_in &saAddress,
        const UINT32 uRecvBuffSize = 0, const UINT32 uSendBuffSize = 0);

public:
    virtual bool Open();
    virtual bool Close();

    virtual int  CheckSend(const timeval * const cpcTimeOut = NULL);
    virtual int  CheckRecv(const timeval * const cpcTimeOut = NULL);

    virtual int  Send(xbuff::SPIKG_Buffer &spBuffer, const UINT32 uPakHeadSize, const timeval *const cpcTimeOut = NULL);
    virtual int  Recv(xbuff::SPIKG_Buffer &spBuffer, const UINT32 uPakHeadSize, const timeval *const cpcTimeOut = NULL);

    virtual void SetConnIndex(const int nConnIndex);
    virtual int  GetConnIndex() const;

    virtual bool IsAlive();
    virtual bool IsValid();

    virtual int  GetErrCode() const;
    virtual void GetAddress(struct in_addr *pIp, USHORT *pnPort) const;

protected:
    SOCKET             m_nSocket;                                       // Socket descriptor
    struct sockaddr_in m_saAddress;                                     // Socket address
    int                m_nConnIndex;                                    // connection index
    int                m_nErrCode;                                      // Last error code
};

typedef KG_SocketStream *                PKG_SocketStream;
typedef std::shared_ptr<KG_SocketStream> SPKG_SocketStream;

SPIKG_SocketStream KG_GetSocketStreamFromMemoryPool(const SOCKET nSocket, const sockaddr_in &saAddress,
    const UINT32 uRecvBuffSize = 0, const UINT32 uSendBuffSize = 0);

KG_NAMESPACE_END

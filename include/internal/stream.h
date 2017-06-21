#pragma once

#include "public.h"

KG_NAMESPACE_BEGIN(xnet)

class IKG_SocketStream : private xzero::KG_UnCopyable
{
public:
    IKG_SocketStream() {}
    virtual ~IKG_SocketStream() {}

public:
    virtual int Open()  = 0;
    virtual int Close() = 0;

    virtual int CheckSend(const timeval * const cpcTimeOut = NULL) = 0;
    virtual int CheckRecv(const timeval * const cpcTimeout = NULL) = 0;

    virtual int Send(xbuff::SPIKG_Buffer &spBuffer, const timeval *const pTimeOut = NULL) = 0;
    virtual int Recv(xbuff::SPIKG_Buffer &spBuffer, const timeval *const pTimeOut = NULL) = 0;

    virtual int SetConnIndex(const int nConnIndex) = 0;
    virtual int GetConnIndex() const = 0;

    virtual int IsAlive() = 0;
    virtual int IsValid() = 0;

    virtual int GetErrCode() const = 0;
    virtual int GetAddress(struct in_addr *pIp, USHORT *pnPort) const = 0;
};

typedef IKG_SocketStream *                PIKG_SocketStream;
typedef std::shared_ptr<IKG_SocketStream> SPIKG_SocketStream;

class KG_SocketStream : public IKG_SocketStream
{
public:
    KG_SocketStream();
    virtual ~KG_SocketStream();

public:
    int Init(const SOCKET nSocket, const sockaddr_in &saAddress,
        const unsigned int uRecvBuffSize = 0, const unsigned int uSendBuffSize = 0);

public:
    virtual int Open();
    virtual int Close();

    virtual int CheckSend(const timeval * const cpcTimeOut = NULL);
    virtual int CheckRecv(const timeval * const cpcTimeout = NULL);

    virtual int Send(xbuff::SPIKG_Buffer &spBuffer, const timeval *const pTimeOut = NULL);
    virtual int Recv(xbuff::SPIKG_Buffer &spBuffer, const timeval *const pTimeOut = NULL);

    virtual int SetConnIndex(const int nConnIndex);
    virtual int GetConnIndex() const;

    virtual int IsAlive();
    virtual int IsValid();

    virtual int GetErrCode() const;
    virtual int GetAddress(struct in_addr *pIp, USHORT *pnPort) const;

protected:
    int                 m_nInitFlag;                                    // Init flag
    int                 m_nOpenFlag;                                    // Open flag
    SOCKET              m_nSocket;                                      // Socket descriptor
    struct sockaddr_in  m_saAddress;                                    // Socket address
    int                 m_nErrCode;                                     // Last error code
    unsigned            m_uRecvBuffSize;                                // Recv buffer size
    unsigned            m_uSendBuffSize;                                // Send buffer size
};

typedef KG_SocketStream *                PKG_SocketStream;
typedef std::shared_ptr<KG_SocketStream> SPKG_SocketStream;

KG_NAMESPACE_END

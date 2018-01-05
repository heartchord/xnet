#pragma once

#include "public.h"

#include <list>

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

struct KG_SocketStreamListNode
{
    xzero::KG_ListNode m_ListNode;                                      // member for xzero::KG_List
    SPIKG_SocketStream m_spSocketStream;                                // sp to socket stream
};
typedef KG_SocketStreamListNode *PKG_SocketStreamListNode;

/*--------------------------------------------------------------------------------------------------*/
/* KG_SocketStream : A socket stream without any data cache.                                        */
/* When receiving data(KG_SocketStream::Recv), several situations need to be considered:            */
/*     (1) A whole package(pak head + pak body) reaches, KG_SocketStream::Recv() will read the whole*/
/* package body into a SPIKG_Buffer, and the return code will be 1.                                 */
/*     (2) No data in socket buffer, the SPIKG_Buffer will be null and he return code will be 0.    */
/*     (3) If an incomplete package(incomplete head or imcomplete body) found when KG_SocketStream::*/
/* Recv() is executing, we consider it as an error occurs, just because one incomplete package will */
/* cause data corruption. In this situation, just close socket and let it reconnect.                */
/*     (4) If an error occurs, the SPIKG_Buffer will be null and he return code will be -1.         */
/*--------------------------------------------------------------------------------------------------*/
class KG_SocketStream : public IKG_SocketStream
{
public:
    KG_SocketStream();
    virtual ~KG_SocketStream();

public:
    bool Init(const SOCKET nSocket, const sockaddr_in &saAddress, const UINT32 uRecvBuffSize = 0, const UINT32 uSendBuffSize = 0);

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

SPIKG_SocketStream KG_GetSocketStreamFromMemoryPool(SOCKET &nSocket, const sockaddr_in &saAddress,
    const UINT32 uRecvBuffSize = 0, const UINT32 uSendBuffSize = 0);

class KG_AsyncSocketStream : public IKG_SocketStream
{
public: // constructor & destructor
    KG_AsyncSocketStream();
    virtual ~KG_AsyncSocketStream();

public: // public functions
    bool Init(const SOCKET nSocket, const sockaddr_in &saAddress, const UINT32 uRecvBuffSize = 0, const UINT32 uSendBuffSize = 0);
    bool IsCallbackNotified() const;
    void OnRecvCompleted(DWORD dwErrCode = 0, DWORD dwBytesTransfered = 0, LPOVERLAPPED lpOverlapped = NULL);
    void DoWaitCallbackNotified();

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    bool IsRecvCompleted()    const;
    bool IsDelayDestorying()  const;

#else                                                                   // linux   platform
#endif // KG_PLATFORM_WINDOWS

private: // private functions
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    bool _ActivateNextRecv();
#else                                                                   // linux   platform
#endif // KG_PLATFORM_WINDOWS

public: // override functions
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
    SOCKET              m_nSocket;                                      // Socket descriptor
    struct sockaddr_in  m_saAddress;                                    // Socket address
    int                 m_nConnIndex;                                   // connection index
    int                 m_nErrCode;                                     // Last error code

    xbuff::SPIKG_Buffer m_spRecvBuffer;                                 // recv buffer
    UINT32              m_uRecvHeadPos;                                 // recv head pos in buffer
    UINT32              m_uRecvTailPos;                                 // recv tail pos in buffer

    bool                m_bDelayDestorying;                             // delay destorying socket stream
    bool                m_bCallbackNotified;                            // 'IOCompletionCallBack' or 'EPOLL' callback notified?

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    WSABUF              m_wsaBuf;                                       // used in '::WSARecv' and '::WSASend' function.
    bool                m_bRecvCompleted;                               // 'IOCompletionCallBack' recv completed?
    int                 m_nRecvCompletedErrCode;                        // 'IOCompletionCallBack' recv completed error code.
    int                 m_nRecvCompletedDataSize;                       // 'IOCompletionCallBack' recv completed data size.
#else                                                                   // linux   platform
#endif // KG_PLATFORM_WINDOWS

public:
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    WSAOVERLAPPED       m_wsaOverlapped;                                // used in iocp
#endif // KG_PLATFORM_WINDOWS
};

typedef KG_AsyncSocketStream *                PKG_AsyncSocketStream;
typedef std::shared_ptr<KG_AsyncSocketStream> SPKG_AsyncSocketStream;

class KG_SocketEvent;
class KG_AsyncSocketStreamList : private xzero::KG_UnCopyable
{
private:
    xzero::KG_List      m_StreamList;                                   // stream list
    xzero::PKG_ListNode m_pLastProcessedNode;                           // pointer to last processed stream

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
#else                                                                   // linux   platform
    int                 m_nEpollHandle;                                 // windows不使用，linux使用
#endif // KG_PLATFORM_WINDOWS

public:
    KG_AsyncSocketStreamList();
    ~KG_AsyncSocketStreamList();

public:
    void Insert(SPIKG_SocketStream &spStream);
    void Remove(SPIKG_SocketStream &spStream);
    void Destroy();

    bool Activate(UINT32 uMaxCount, UINT32 &uCurCount, KG_SocketEvent * pEventList);

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
#else                                                                   // linux   platform
    void SetEpollHandle(int nEpollHandle);
#endif // KG_PLATFORM_WINDOWS

private:
    void _ProcessDestroy();
    bool _ProcessRecvOrClose(UINT32 uMaxCount, UINT32 &uCurCount, KG_SocketEvent * pEventList);

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
#else                                                                   // linux   platform
    bool _ProcessEpollEvents(UINT32 uMaxCount, UINT32 uCurCount);
#endif // KG_PLATFORM_WINDOWS
};

KG_NAMESPACE_END

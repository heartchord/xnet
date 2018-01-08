#include "acceptor.h"
#include "wrapper.h"

KG_NAMESPACE_BEGIN(xnet)

KG_SocketAcceptor::KG_SocketAcceptor()
{
    m_nSocket = KG_INVALID_SOCKET;
}

KG_SocketAcceptor::~KG_SocketAcceptor()
{
    KG_ASSERT(KG_INVALID_SOCKET == m_nSocket && "[ERROR] Forgot invoking KG_SocketAcceptor::Close()?");
}

bool KG_SocketAcceptor::Init(const char *pszIpAddr, USHORT uPort)
{
    int nResult  = false;
    int nRetCode = false;

#ifndef KG_PLATFORM_WINDOWS                                             // linux platform
    typedef void(*KG_SignalHandler)(int);
    KG_SignalHandler h = SIG_ERR;
#endif

    KG_PROCESS_ERROR(uPort > 0);
    KG_PROCESS_ERROR(KG_INVALID_SOCKET == m_nSocket && "[ERROR] It seems KG_SocketAcceptor has been initialized!");

#ifndef KG_PLATFORM_WINDOWS                                             // linux platform
    /*-------------------------------------------------------------------------------------------------------*/
    /*     When server closes one socket connection, server will receive a 'RST' response. If client sends   */
    /* one more data to server, the system will send a 'SIGPIPE' signal to the process to notify client      */
    /* connection is dead. By default, 'SIGPIPE' signal will let client exit. If u don't want client to exit,*/
    /* u can set 'SIGPIPE' to 'SIG_IGN', then the system will ignore 'SIGPIPE' signal.                       */
    /*-------------------------------------------------------------------------------------------------------*/
    h = ::signal(SIGPIPE, SIG_IGN);
    KG_PROCESS_ERROR(SIG_ERR != h);
#endif

    m_nSocket = KG_CreateListenSocket(pszIpAddr, uPort);
    KG_PROCESS_SOCKET_ERROR(m_nSocket);

    nRetCode = KG_SetSocketBlockMode(m_nSocket, false);
    KG_PROCESS_ERROR(nRetCode);

    nResult = true;
Exit0:
    if (!nResult)
    {
        if (KG_INVALID_SOCKET != m_nSocket)
        {
            KG_CloseSocketSafely(m_nSocket);
        }
    }
    return nResult;
}

bool KG_SocketAcceptor::Close()
{
    bool bResult  = false;
    int  nRetCode = false;

    nRetCode = KG_CloseSocketSafely(m_nSocket);
    KG_PROCESS_ERROR(nRetCode);

    bResult = true;
Exit0:
    return bResult;
}

int KG_SocketAcceptor::Accept(SPIKG_SocketStream &spSocketStream, const timeval *pcTimeout)
{
    int                nResult  = -1;
    int                nRetCode = false;
    SOCKET             nSocket  = KG_INVALID_SOCKET;
    socklen_t          nAddrLen = sizeof(sockaddr_in);
    struct sockaddr_in saAddress;

    KG_PROCESS_SOCKET_ERROR(m_nSocket);

    for (;;)
    {
        nRetCode = KG_CheckSocketRecvEx(m_nSocket, pcTimeout);
        KG_PROCESS_ERROR(nRetCode >= 0);                                // error
        KG_PROCESS_SUCCESS_RET_CODE(0 == nRetCode, 0);                  // timeout

        xzero::KG_ZeroMemory(&saAddress, sizeof(saAddress));
        nSocket = (SOCKET)::accept(m_nSocket, (struct sockaddr *)&saAddress, &nAddrLen);

        if (SOCKET_ERROR == nSocket)
        {
            nRetCode = KG_IsSocketInterrupted();
            if (nRetCode)
            {
                continue;
            }

            /*------------------------------------------------------------------*/
            /* WSA_IO_PENDING : overlapped operations will complete later.      */
            /* WSAEWOULDBLOCK : non waiting connection in listen-socket queue.  */
            /*------------------------------------------------------------------*/
            nRetCode = KG_IsSocketEWouldBlock();

            // accept success, try next loop, do not need assert here.
            KG_PROCESS_SUCCESS_RET_CODE(nRetCode, 0);                   // time out
            KG_PROCESS_SUCCESS_RET_CODE(true,    -1);                   // error
        }

        break;                                                          // success
    }

    spSocketStream = KG_GetSocketStreamFromMemoryPool(nSocket, saAddress);
    KG_PROCESS_ERROR(spSocketStream);                                   // error

    nSocket = KG_INVALID_SOCKET;                                        // transfer ownership
    nResult = 1;
Exit0:
    if (!nResult)
    {
        if (spSocketStream)
        {
            spSocketStream->Close();
            spSocketStream = SPIKG_SocketStream();
        }

        if (KG_INVALID_SOCKET != nSocket)
        {
            KG_CloseSocketSafely(nSocket);
        }
    }

    return nResult;
}

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform

static void WINAPI l_IOCompletionCallBack(DWORD dwErrCode, DWORD dwNumberOfBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    int                   nRetCode = false;
    KG_AsyncSocketStream *pStream  = NULL;

    pStream = KG_FetchAddressByField(lpOverlapped, KG_AsyncSocketStream, m_wsaOverlapped);
    KG_PROCESS_PTR_ERROR(pStream);

    if (0 == dwNumberOfBytesTransfered)
    {
        dwErrCode = WSAEDISCON;
    }

    pStream->OnRecvCompleted(dwErrCode, dwNumberOfBytesTransfered, lpOverlapped);

Exit0:
    return;
}

#else                                                                   // linux   platform
#endif // KG_PLATFORM_WINDOWS

static int l_AcceptToAsyncSocketStream(SOCKET nListen, PKG_AsyncSocketStream &pStream, UINT32 uRecvBuffSize = 0, UINT32 uSendBuffSize = 0, int nEpollHandle = -1)
{
    int                   nResult     = -1;
    int                   nRetCode    = false;
    bool                  bStreamInit = false;
    socklen_t             nAddrLen    = sizeof(struct sockaddr_in);
    SOCKET                nSocket     = KG_INVALID_SOCKET;
    PKG_AsyncSocketStream pTempStream = NULL;
    struct sockaddr_in    addr;

    KG_PROCESS_SOCKET_ERROR(nListen);
    KG_PROCESS_ERROR(uRecvBuffSize > 0);
    KG_PROCESS_ERROR(uSendBuffSize > 0);

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    KG_UNREFERENCED_PARAMETER(nEpollHandle);
#else                                                                   // linux   platform
    KG_PROCESS_SOCKET_ERROR(nEpollHandle);
#endif // KG_PLATFORM_WINDOWS

    pStream = NULL;

    for (;;)
    {
        xzero::KG_ZeroMemory(&addr, sizeof(addr));
        nSocket = (SOCKET)::accept(nListen, (struct sockaddr *)&addr, &nAddrLen);

        if (SOCKET_ERROR == nSocket)
        {
            nRetCode = KG_IsSocketInterrupted();
            if (nRetCode)
            {
                continue;
            }

            nRetCode = KG_IsSocketEWouldBlock();
            KG_PROCESS_SUCCESS_RET_CODE(nRetCode, 0);                   // time out
            KG_PROCESS_ERROR_RET_CODE  (false,   -1);                   // error
        }

        // create stream
        pTempStream = g_CreateAsyncSocketStream(nSocket, addr, uRecvBuffSize, uSendBuffSize);
        KG_PROCESS_ERROR_RET_CODE(NULL != pTempStream, -1);             // error

        // bind iocp
    #ifdef KG_PLATFORM_WINDOWS                                          // windows platform
        nRetCode = ::BindIoCompletionCallback((HANDLE)nSocket, l_IOCompletionCallBack, 0);
        KG_PROCESS_ERROR_RET_CODE(nRetCode, -1);                        // error
    #else                                                               // linux   platform
        {
            epoll_event ev;
            ev.events   = EPOLLIN | EPOLLET;
            ev.data.ptr = (void *)pTempStream;

            nRetCode = ::epoll_ctl(nEpollHandle, EPOLL_CTL_ADD, nSocket, &ev);
            KG_PROCESS_ERROR_RET_CODE(nRetCode >= 0, -1);
        }
    #endif // KG_PLATFORM_WINDOWS

        nRetCode = pTempStream->Open();
        KG_PROCESS_ERROR_RET_CODE(nRetCode, -1);                        // error

        // sucess here
        nSocket     = KG_INVALID_SOCKET;                                // transfer ownership
        pStream     = pTempStream;                                      // save result
        bStreamInit = true;
        break;
    }

    nResult = 1;
Exit0:
    if (nResult < 0)
    {
        if (bStreamInit)
        {
            nRetCode = pTempStream->Close();
            KG_ASSERT(nRetCode);
            bStreamInit = false;
        }
        xzero::KG_DeletePtrSafely(pTempStream);
        KG_CloseSocketSafely(nSocket);
    }

    return nResult;
}

KG_AsyncSocketAcceptor::KG_AsyncSocketAcceptor()
{
    m_nSocket                = KG_INVALID_SOCKET;
    m_uMaxAcceptEachActivate = 0;
    m_uRecvBuffSize          = 0;
    m_uSendBuffSize          = 0;

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
#else                                                                   // linux   platform
    m_nEpollHandle           = -1;
#endif // KG_PLATFORM_WINDOWS
}

KG_AsyncSocketAcceptor::~KG_AsyncSocketAcceptor()
{
    KG_ASSERT(KG_INVALID_SOCKET == m_nSocket && "[ERROR] Forgot invoking KG_AsyncSocketAcceptor::Close()?");

#ifndef WIN32
    KG_ASSERT(-1 == m_nEpollHandle && "[ERROR] Forgot invoking KG_AsyncSocketAcceptor::Close()?");
#endif
}

bool KG_AsyncSocketAcceptor::Init(const char *pszIpAddr, USHORT uPort, UINT32 uMaxAcceptEachActivate, UINT32 uRecvBuffSize, UINT32 uSendBuffSize)
{
    bool  bResult  = false;
    int   nRetCode = 0;

    KG_PROCESS_ERROR(uPort                  > 0);
    KG_PROCESS_ERROR(uRecvBuffSize          > 0);
    KG_PROCESS_ERROR(uSendBuffSize          > 0);
    KG_PROCESS_ERROR(uMaxAcceptEachActivate > 0);

    m_uMaxAcceptEachActivate = uMaxAcceptEachActivate;
    m_uRecvBuffSize = uRecvBuffSize;
    m_uSendBuffSize = uSendBuffSize;

    KG_PROCESS_ERROR(KG_INVALID_SOCKET == m_nSocket && "[ERROR] It seems KG_AsyncSocketAcceptor has been initialized!");
    m_nSocket = KG_CreateListenSocket(pszIpAddr, uPort);
    KG_PROCESS_SOCKET_ERROR(m_nSocket);

    nRetCode = KG_SetSocketBlockMode(m_nSocket, false);
    KG_PROCESS_ERROR(nRetCode);

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
#else                                                                   // linux   platform
    {
        typedef void(*PSignalHandler)(int);
        PSignalHandler pSignalHandler = SIG_ERR;

        pSignalHandler = ::signal(SIGPIPE, SIG_IGN);
        KG_PROCESS_ERROR(SIG_ERR != pSignalHandler);
    }

    //     The epoll_create() interface shall open an epoll file descriptor by allocating an event backing store of
    // approximately size size. The size parameter is a hint to the kernel about how large the event storage should
    // be, not a rigidly-defined maximum size.
    //     since Linux 2.6.8, the size argument of  epoll_create is ignored, but must be greater than zero.
    m_nEpollHandle = ::epoll_create(256);
    KG_PROCESS_ERROR(-1 != m_nEpollHandle);
    m_StreamList.SetEpollHandle(m_nEpollHandle);
#endif // KG_PLATFORM_WINDOWS

    bResult = true;
Exit0:
    if (!bResult)
    {
    #ifdef KG_PLATFORM_WINDOWS                                          // windows platform
    #else                                                               // linux   platform
        KG_CloseSocketSafely(m_nEpollHandle);
    #endif // KG_PLATFORM_WINDOWS

        KG_CloseSocketSafely(m_nSocket);
        m_uMaxAcceptEachActivate = 0;
    }

    return bResult;
}

bool KG_AsyncSocketAcceptor::Close()
{
    bool bResult  = false;
    int  nRetCode = 0;

    m_StreamList.Destroy();

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
#else                                                                   // linux   platform
    nRetCode = KG_CloseSocketSafely(m_nEpollHandle);
    KG_PROCESS_ERROR(nRetCode);
#endif // KG_PLATFORM_WINDOWS

    nRetCode = KG_CloseSocketSafely(m_nSocket);
    KG_PROCESS_ERROR(nRetCode);
 
    bResult = true;
Exit0:
    return bResult;
}

bool KG_AsyncSocketAcceptor::Activate(UINT32 uMaxCount, UINT32 &uCurCount, KG_SocketEvent *pEventList)
{
    bool bResult  = false;
    int  nRetCode = 0;

    KG_PROCESS_ERROR(uMaxCount > 0);
    KG_PROCESS_PTR_ERROR(pEventList);

    KG_ASSERT(0 == uCurCount);
    uCurCount = 0;

    // accept
    nRetCode = _ProcessAccept(uMaxCount, uCurCount, pEventList);
    KG_PROCESS_ERROR(nRetCode);
    KG_PROCESS_SUCCESS(uCurCount == uMaxCount);

    // recv
    nRetCode = m_StreamList.Activate(uMaxCount, uCurCount, pEventList);
    KG_PROCESS_ERROR(nRetCode);

Exit1:
    bResult = true;
Exit0:
    return bResult;
}

bool KG_AsyncSocketAcceptor::_ProcessAccept(UINT32 uMaxCount, UINT32 &uCurCount, KG_SocketEvent *pEventList)
{
    bool                  bResult       = false;
    int                   nRetCode      = 0;
    int                   nEpollHandle  = -1;
    unsigned              uProcessCount = 0;
    PKG_AsyncSocketStream pStream       = NULL;
    SPIKG_SocketStream    spSocketStream;

    KG_PROCESS_ERROR(uMaxCount > 0);
    KG_PROCESS_PTR_ERROR(pEventList);

    KG_ASSERT(0 == uCurCount);
    uCurCount = 0;

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    nEpollHandle = -1;
#else                                                                   // linux   platform
    nEpollHandle = m_nEpollHandle;
    KG_PROCESS_ERROR(nEpollHandle > 0);
#endif

    while (uCurCount < uMaxCount)
    {
        pStream  = NULL;
        nRetCode = l_AcceptToAsyncSocketStream(m_nSocket, pStream, m_uRecvBuffSize, m_uSendBuffSize, nEpollHandle);

        if (0 == nRetCode)
        { // time out, again
            break;
        }

        KG_PROCESS_ERROR(nRetCode > 0);
        KG_PROCESS_PTR_ERROR(pStream);

        spSocketStream = SPIKG_SocketStream(pStream);
        pStream        = NULL;                                          // transfer ownership
        m_StreamList.Insert(spSocketStream);

        // pStream has been initialized in l_AcceptToAsyncSocketStream()
        pEventList[uCurCount].m_uType    = KG_SOCKET_EVENT_ACCEPT;
        pEventList[uCurCount].m_spStream = spSocketStream;

        uCurCount++;
        uProcessCount++;

        if (uProcessCount >= m_uMaxAcceptEachActivate)
        {
            break;
        }
    }

    bResult = true;
Exit0:
    if (!bResult)
    {
        while (uProcessCount--)
        { // roll back
            uCurCount--;

            spSocketStream = pEventList[uCurCount].m_spStream;
            m_StreamList.Remove(spSocketStream);
            spSocketStream->Close();
            pEventList[uCurCount].Reset();
        }
    }

    return bResult;
}

KG_NAMESPACE_END

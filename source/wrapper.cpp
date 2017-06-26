#include "wrapper.h"

# pragma warning(disable: 4102)

KG_NAMESPACE_BEGIN(xnet)

SOCKET KG_CreateTcpSocket()
{
    SOCKET s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (s != KG_INVALID_SOCKET && s >= 0)
    {
        return s;
    }
    return KG_INVALID_SOCKET;
}

SOCKET KG_CreateUdpSocket()
{
    SOCKET s = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (s != KG_INVALID_SOCKET && s >= 0)
    {
        return s;
    }
    return KG_INVALID_SOCKET;
}

SOCKET KG_CreateListenSocket(const char * const cszIpAddr, const WORD nPort, const int nBackLog)
{
    int         nResult       = false;
    int         nRetCode      = 0;
    int         nOptVal       = 1;
    ULONG       ulAddress     = INADDR_ANY;
    SOCKET      hListenSocket = KG_INVALID_SOCKET;
    sockaddr_in saLocalAddr;

    KG_PROCESS_ERROR(nPort    > 0);
    KG_PROCESS_ERROR(nBackLog > 0);

    // create a tcp socket
    hListenSocket = KG_CreateTcpSocket();
    KG_PROCESS_SOCKET_ERROR(hListenSocket);

    // fill in sockaddr_in
    if (NULL != cszIpAddr && '\0' != cszIpAddr[0])
    {
        if (INADDR_NONE == (ulAddress = ::inet_addr(cszIpAddr)))
        {
            ulAddress = INADDR_ANY;
        }
    }

    saLocalAddr.sin_family      = AF_INET;
    saLocalAddr.sin_addr.s_addr = ulAddress;
    saLocalAddr.sin_port        = htons(nPort);

    /*----------------------------------------------------------------------------------------------*/
    /*     Usually if we don't set "SO_REUSEADDR" opt, the listen port used in server can't be used */
    /* immediately again in a certain probability, eg : restart a server. It often happens when we  */
    /* restart a server, the port will be used for a while. if set "SO_REUSEADDR" opt, the port will*/
    /* be used immediately when restarting servers.                                                 */
    nRetCode = ::setsockopt(hListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&nOptVal, sizeof(nOptVal));
    KG_PROCESS_ERROR(0 == nRetCode);

    nRetCode = ::bind(hListenSocket, (struct sockaddr *)&saLocalAddr, sizeof(saLocalAddr));
    KG_PROCESS_ERROR(0 == nRetCode);

    nRetCode = ::listen(hListenSocket, nBackLog);
    KG_PROCESS_ERROR(0 == nRetCode);

    nResult = true;
Exit0:
    if (!nResult)
    {
        nRetCode = KG_CloseSocketSafely(hListenSocket);
        KG_ASSERT(nRetCode);
    }

    return hListenSocket;
}

int KG_GetSocketErrCode()
{
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    return ::WSAGetLastError();
#else                                                                   // linux   platform
    return errno;
#endif // KG_PLATFORM_WINDOWS
}

int KG_CheckSocketSend(SOCKET nSocket, const timeval *pcTimeOut)
{
    int nResult  = -1;
    int nRetCode = 0;

    KG_PROCESS_SOCKET_ERROR(nSocket);

    for (;;)
    {
        fd_set fdSend; FD_ZERO(&fdSend); FD_SET(nSocket, &fdSend);
        /*----------------------------------------------------------------------------------------------------------*/
        /*     select operation can be used upon blocked and non-blocked socket both, to check if it can send data. */
        /* And the time-out is effective both. The difference between both situation is that:                       */
        /*     (1) ::send() operation will be blocked upon blocked socket.                                          */
        /*     (2) ::send() operation will be non-blocked upon non-blocked socket.                                  */
        /*----------------------------------------------------------------------------------------------------------*/

        nRetCode = ::select(nSocket + 1, 0, &fdSend, 0, pcTimeOut);
        KG_PROCESS_SUCCESS(nRetCode > 0);                               // success

        if (0 == nRetCode)
        {                                                               // time out
            nResult = 0; goto Exit0;
        }

        nRetCode = KG_IsSocketInterrupted();                            // interrupted, continue
        if (nRetCode)
        {
            continue;
        }

        nRetCode = KG_IsSocketEWouldBlock();                            // would block == time out
        if (nRetCode)
        {
            nResult = 0; goto Exit0;
        }

        nResult = -1; goto Exit0;                                       // error
    }

Exit1:
    nResult = 1;
Exit0:
    return nResult;
}

int KG_CheckSocketRecv(SOCKET nSocket, const timeval *pcTimeOut)
{
    int nResult  = -1;
    int nRetCode = 0;

    KG_PROCESS_SOCKET_ERROR(nSocket);

    for (;;)
    {
        fd_set fdRead; FD_ZERO(&fdRead); FD_SET(nSocket, &fdRead);

        nRetCode = ::select(nSocket + 1, &fdRead, 0, 0, pcTimeOut);
        KG_PROCESS_SUCCESS(nRetCode > 0);                               // success

        if (0 == nRetCode)
        {                                                               // time out
            nResult = 0; goto Exit0;
        }

        nRetCode = KG_IsSocketInterrupted();                            // interrupted, continue
        if (nRetCode)
        {
            continue;
        }

        nRetCode = KG_IsSocketEWouldBlock();                            // would block == time out
        if (nRetCode)
        {
            nResult = 0; goto Exit0;
        }

        nResult = -1; goto Exit0;                                       // error
    }

Exit1:
    nResult = 1;
Exit0:
    return nResult;
}

int KG_CheckSocketSendEx(SOCKET nSocket, const timeval *pcTimeOut)
{
    int           nResult  = -1;
    int           nRetCode = 0;
#ifndef KG_PLATFORM_WINDOWS                                             // linux   platform
    int           nTimeOut = -1;
    struct pollfd fdPoll;
#endif // KG_PLATFORM_WINDOWS

    KG_PROCESS_SOCKET_ERROR(nSocket);

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    nResult = KG_CheckSocketSend(nSocket, pcTimeOut);
    goto Exit0;
#else                                                                   // linux   platform
    if (pcTimeOut)
    {
        nTimeOut = (pcTimeOut->tv_sec * 1000) + (pcTimeOut->tv_usec / 1000);
    }

    for (;;)
    {
        fdPoll.fd      = nSocket;
        fdPoll.events  = POLLOUT;
        fdPoll.revents = 0;

        nRetCode = ::poll(&fdPoll, 1, nTimeOut);
        if (nRetCode > 0)
        {
            if ((fdPoll.revents & POLLERR) || (fdPoll.revents & POLLHUP) || (fdPoll.revents & POLLNVAL))
            {                                                           // fdPoll exceptional
                KG_ASSERT(false);
                continue;
            }

            KG_ASSERT((fdPoll.revents & POLLOUT) || (fdPoll.revents & POLLWRNORM) || (fdPoll.revents & POLLWRBAND));
            KG_PROCESS_SUCCESS(true);
        }

        if (0 == nRetCode)
        {                                                               // time out
            nResult = 0; goto Exit0;
        }

        nRetCode = KG_IsSocketInterrupted();                            // interrupted, continue
        if (nRetCode)
        {
            continue;
        }

        nRetCode = KG_IsSocketEWouldBlock();                            // would block == time out
        if (nRetCode)
        {
            nResult = 0; goto Exit0;
        }

        nResult = -1; goto Exit0;                                       // error
    }
#endif // KG_PLATFORM_WINDOWS

Exit1:
    nResult = 1;
Exit0:
    return nResult;
}

int KG_CheckSocketRecvEx(SOCKET nSocket, const timeval *pcTimeOut)
{
    int           nResult  = -1;
    int           nRetCode = 0;
#ifndef KG_PLATFORM_WINDOWS                                             // linux   platform
    int           nTimeOut = -1;
    struct pollfd fdPoll;
#endif // KG_PLATFORM_WINDOWS

    KG_PROCESS_SOCKET_ERROR(nSocket);

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    nResult = KG_CheckSocketRecv(nSocket, pcTimeOut);
    goto Exit0;
#else                                                                   // linux   platform
    if (pcTimeOut)
    {
        nTimeOut = (pcTimeOut->tv_sec * 1000) + (pcTimeOut->tv_usec / 1000);
    }

    for (;;)
    {
        fdPoll.fd      = nSocket;
        fdPoll.events  = POLLIN;
        fdPoll.revents = 0;

        nRetCode = ::poll(&fdPoll, 1, nTimeOut);
        if (nRetCode > 0)
        {
            if ((fdPoll.revents & POLLERR) || (fdPoll.revents & POLLHUP) || (fdPoll.revents & POLLNVAL))
            {                                                           // fdPoll exceptional
                KG_ASSERT(false);
                continue;
            }

            KG_ASSERT((fdPoll.revents & POLLIN) || (fdPoll.revents & POLLRDNORM) || (fdPoll.revents & POLLPRI) || (fdPoll.revents & POLLRDBAND));
            KG_PROCESS_SUCCESS(true);
        }

        if (0 == nRetCode)
        {                                                               // time out
            nResult = 0; goto Exit0;
        }

        nRetCode = KG_IsSocketInterrupted();                            // interrupted, continue
        if (nRetCode)
        {
            continue;
        }

        nRetCode = KG_IsSocketEWouldBlock();                            // would block == time out
        if (nRetCode)
        {
            nResult = 0; goto Exit0;
        }

        nResult = -1; goto Exit0;                                       // error
    }
#endif

Exit1:
    nResult = 1;
Exit0:
    return nResult;
}

int KG_CheckSendSocketData(SOCKET nSocket, const char * const cpcBuff, const UINT32 uBuffSize, const UINT32 uSendBytes, const timeval *pcTimeout)
{
    int          nResult    = -1;
    int          nRetCode   = 0;
    UINT32       uLeftBytes = uSendBytes;
    const char * pcCurrent  = cpcBuff;

    KG_PROCESS_PTR_ERROR(cpcBuff);
    KG_PROCESS_SOCKET_ERROR(nSocket);
    KG_PROCESS_ERROR(uBuffSize > 0 && uSendBytes > 0 && uSendBytes <= uBuffSize);

    while (uLeftBytes > 0)
    {
        // check can send
        nRetCode = KG_CheckSocketSendEx(nSocket, pcTimeout);
        KG_PROCESS_ERROR(nRetCode >= 0);                                // error

        if (0 == nRetCode)
        {
            nResult = 0; goto Exit0;                                    // timeout
        }

        // send data
        nRetCode = ::send(nSocket, pcCurrent, uLeftBytes, 0);
        KG_PROCESS_ERROR_Q(0 != nRetCode);                              // disconnected == error

        if (nRetCode > 0)                                               // success
        {
            pcCurrent  += nRetCode;
            uLeftBytes -= nRetCode;
            continue;
        }

        nRetCode = KG_IsSocketInterrupted();
        if (nRetCode)                                                   // interrupted
        {
            continue;
        }

        nRetCode = KG_IsSocketEWouldBlock();
        if (nRetCode)
        {
            nResult = 0; goto Exit0;                                    // timeout
        }

        nResult = -1; goto Exit0;                                       // error
    }

    nResult = 1;
Exit0:
    return nResult;
}

int KG_CheckRecvSocketData(SOCKET nSocket, char * const cpBuff, const UINT32 uBuffSize, UINT32 * const puRecvBytes, const timeval *pcTimeout)
{
    int    nResult    = -1;
    int    nRetCode   = 0;
    UINT32 uLeftBytes = uBuffSize;
    UINT32 uRecvBytes = 0;
    char * pCurrent   = cpBuff;

    *puRecvBytes = 0;

    KG_PROCESS_PTR_ERROR(cpBuff);
    KG_PROCESS_ERROR(uBuffSize > 0);
    KG_PROCESS_SOCKET_ERROR(nSocket);

    while (uLeftBytes > 0)
    {
        // check can recv
        nRetCode = KG_CheckSocketRecvEx(nSocket, pcTimeout);
        KG_PROCESS_ERROR(nRetCode >= 0);                                // error

        if (0 == nRetCode)
        {
            nResult = 0; goto Exit0;                                    // timeout
        }

        // recv
        nRetCode = ::recv(nSocket, pCurrent, uLeftBytes, 0);
        KG_PROCESS_ERROR_Q(0 != nRetCode);                              // disconnected == error

        if (nRetCode > 0)                                               // success
        {
            pCurrent    += nRetCode;
            uRecvBytes  += nRetCode;
            uLeftBytes  -= nRetCode;
            continue;
        }

        nRetCode = KG_IsSocketInterrupted();
        if (nRetCode)
        {
            continue;                                                   // interrupted
        }

        nRetCode = KG_IsSocketEWouldBlock();
        if (nRetCode)
        {
            nResult = 0; goto Exit0;                                    // timeout
        }

        nResult = -1; goto Exit0;                                       // error
    }

    nResult = 1;
Exit0:
    *puRecvBytes = uRecvBytes;
    return nResult;
}

bool KG_SetSocketBlockMode(SOCKET &nSocket, bool bBlocked)
{
    bool  bResult  = false;
    int   nRetCode = 0;
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    ULONG ulOption = 0;
#else                                                                   // linux   platform
    int   nOption  = 0;
#endif // KG_PLATFORM_WINDOWS

    KG_PROCESS_SOCKET_ERROR(nSocket);

    if (bBlocked)
    { // blocked mode
    #ifdef KG_PLATFORM_WINDOWS                                          // windows platform
        ulOption = 0;
        nRetCode = ::ioctlsocket(nSocket, FIONBIO, (unsigned long *)&ulOption);
        KG_PROCESS_ERROR(0 == nRetCode);
    #else                                                               // linux   platform
        nOption  = ::fcntl(nSocket, F_GETFL, 0);
        nRetCode = ::fcntl(nSocket, F_SETFL, nOption&~O_NONBLOCK);
        KG_PROCESS_ERROR(0 == nRetCode);
    #endif // KG_PLATFORM_WINDOWS
    }
    else
    { // non-blocked mode
    #ifdef KG_PLATFORM_WINDOWS                                          // windows platform
        ulOption = 1;
        nRetCode = ::ioctlsocket(nSocket, FIONBIO, (unsigned long *)&ulOption);
        KG_PROCESS_ERROR(0 == nRetCode);
    #else                                                               // linux   platform
        nOption  = ::fcntl(nSocket, F_GETFL, 0);
        nRetCode = ::fcntl(nSocket, F_SETFL, nOption | O_NONBLOCK);
        KG_PROCESS_ERROR(0 == nRetCode);
    #endif // KG_PLATFORM_WINDOWS
    }

    bResult = true;
Exit0:
    return bResult;
}

bool KG_IsSocketEWouldBlock()
{
    bool bResult  = false;
    int  nRetCode = 0;

    nRetCode = KG_GetSocketErrCode();

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    KG_PROCESS_ERROR_Q(WSAEWOULDBLOCK == nRetCode || WSA_IO_PENDING == nRetCode);
#else                                                                   // linux   platform
    KG_PROCESS_ERROR_Q(EAGAIN == nRetCode || EWOULDBLOCK == nRetCode);
#endif // KG_PLATFORM_WINDOWS

    bResult = true;
Exit0:
    return bResult;
}

bool KG_IsSocketInterrupted()
{
    bool bResult  = false;
    int  nRetCode = 0;

    nRetCode = KG_GetSocketErrCode();

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    KG_PROCESS_ERROR_Q(WSAEINTR == nRetCode);
#else                                                                   // linux   platform
    KG_PROCESS_ERROR_Q(EINTR == nRetCode);
#endif // KG_PLATFORM_WINDOWS

    bResult = true;
Exit0:
    return bResult;
}

bool KG_IsSocketClosedByRemote()
{
    bool bResult  = false;
    int  nRetCode = 0;

    nRetCode = KG_GetSocketErrCode();

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    KG_PROCESS_ERROR_Q(WSAECONNRESET == nRetCode);
#else                                                                   // linux   platform
    KG_PROCESS_ERROR_Q(ECONNRESET == nRetCode);
#endif // KG_PLATFORM_WINDOWS

    bResult = true;
Exit0:
    return bResult;
}

bool KG_CloseSocketSafely(SOCKET &nSocket)
{
    bool          bResult  = false;
    int           nRetCode = 0;
    struct linger li;

    KG_PROCESS_SUCCESS(KG_INVALID_SOCKET == nSocket || nSocket < 0);

    /*----------------------------------------------------------------------------------------------*/
    /* [ li.l_onoff = 0 ]                                                                           */
    /*     ::closesocket() will send any unsent data.                                               */
    /* [ li.l_onoff = 1, li.l_linger = 0 ]                                                          */
    /*     ::closesocket() will discard data and send "RST" to client to ignore "TIME_WAIT" state.  */
    /* [ li.l_onoff = 1, li.l_linger = 1 ]                                                          */
    /*     close operation will be delayed for some time. If there's some data in socket buffer, the*/
    /*  process will sleep until the data is received by client.                                    */
    /*----------------------------------------------------------------------------------------------*/
    li.l_onoff  = 1;
    li.l_linger = 0;
    nRetCode = ::setsockopt(nSocket, SOL_SOCKET, SO_LINGER, (const char *)&li, sizeof(li));
    KG_PROCESS_ERROR(0 == nRetCode);

    for (;;)
    {
    #ifdef KG_PLATFORM_WINDOWS                                          // windows platform
        nRetCode = ::closesocket(nSocket);
    #else                                                               // linux   platform
        nRetCode = ::close(nSocket);
    #endif // KG_PLATFORM_WINDOWS

        if (nRetCode < 0)
        { // error
            nRetCode = KG_IsSocketInterrupted();
            if (nRetCode)
            {
                continue;
            }

            KG_PROCESS_ERROR(false);
        }
        break;
    }

Exit1:
    nSocket = KG_INVALID_SOCKET;
    bResult = true;
Exit0:
    return bResult;
}

bool KG_SetSocketRecvBuff(SOCKET nSocket, const UINT32 uRecvBuffSize)
{
    bool bResult  = false;
    int  nRetCode = 0;

    KG_PROCESS_SOCKET_ERROR(nSocket);

    if (uRecvBuffSize > 0)
    {
    #ifdef KG_PLATFORM_WINDOWS                                          // windows platform
        nRetCode = ::setsockopt(nSocket, SOL_SOCKET, SO_RCVBUF,      (char *)&uRecvBuffSize, sizeof(uRecvBuffSize));
    #else                                                               // linux   platform
        nRetCode = ::setsockopt(nSocket, SOL_SOCKET, SO_RCVBUFFORCE, (char *)&uRecvBuffSize, sizeof(uRecvBuffSize));
    #endif // KG_PLATFORM_WINDOWS
        KG_PROCESS_ERROR(0 == nRetCode);
    }

    bResult = true;
Exit0:
    return bResult;
}

bool KG_SetSocketSendBuff(SOCKET nSocket, const UINT32 uSendBuffSize)
{
    bool bResult  = false;
    int  nRetCode = 0;

    KG_PROCESS_SOCKET_ERROR(nSocket);

    if (uSendBuffSize > 0)
    {
    #ifdef KG_PLATFORM_WINDOWS                                          // windows platform
        nRetCode = ::setsockopt(nSocket, SOL_SOCKET, SO_SNDBUF,      (char *)&uSendBuffSize, sizeof(uSendBuffSize));
    #else                                                               // linux   platform
        nRetCode = ::setsockopt(nSocket, SOL_SOCKET, SO_SNDBUFFORCE, (char *)&uSendBuffSize, sizeof(uSendBuffSize));
    #endif // KG_PLATFORM_WINDOWS
        KG_PROCESS_ERROR(0 == nRetCode);
    }

    bResult = true;
Exit0:
    return bResult;
}

bool KG_SetSocketSendTimeOut(SOCKET &nSocket, const timeval &tv)
{
    bool bResult       = false;
    int  nRetCode      = 0;
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    int  nMilliSeconds = 0;
#endif // KG_PLATFORM_WINDOWS

    KG_PROCESS_SOCKET_ERROR(nSocket);

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    nMilliSeconds = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    nRetCode      = ::setsockopt(nSocket, SOL_SOCKET, SO_SNDTIMEO, (const char *)&nMilliSeconds, sizeof(nMilliSeconds));
    KG_PROCESS_ERROR(0 == nRetCode);
#else                                                                   // linux   platform
    nRetCode = ::setsockopt(nSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
    KG_PROCESS_ERROR(0 == nRetCode);
#endif // KG_PLATFORM_WINDOWS

    bResult = true;
Exit0:
    return bResult;
}

bool KG_SetSocketRecvTimeOut(SOCKET &nSocket, const timeval &tv)
{
    bool bResult       = false;
    int  nRetCode      = 0;
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    int  nMilliSeconds = 0;
#endif // KG_PLATFORM_WINDOWS

    KG_PROCESS_SOCKET_ERROR(nSocket);

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    nMilliSeconds = tv.tv_sec * 1000 + tv.tv_usec / 1000;
    nRetCode      = ::setsockopt(nSocket, SOL_SOCKET, SO_RCVTIMEO, (const char *)&nMilliSeconds, sizeof(nMilliSeconds));
    KG_PROCESS_ERROR(0 == nRetCode);
#else
    nRetCode = ::setsockopt(nSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    KG_PROCESS_ERROR(0 == nRetCode);
#endif

    bResult = true;
Exit0:
    return bResult;
}

KG_NAMESPACE_END

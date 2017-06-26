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

bool KG_SocketAcceptor::Init(const char * const cpcIpAddr, const USHORT uPort)
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

    m_nSocket = KG_CreateListenSocket(cpcIpAddr, uPort);
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

int KG_SocketAcceptor::Accept(SPIKG_SocketStream &spSocketStream, const timeval *const cpcTimeout)
{
    int                nResult  = -1;
    int                nRetCode = false;
    SOCKET             nSocket  = KG_INVALID_SOCKET;
    socklen_t          nAddrLen = sizeof(sockaddr_in);
    struct sockaddr_in saAddress;

    KG_PROCESS_SOCKET_ERROR(m_nSocket);

    for (;;)
    {
        nRetCode = KG_CheckSocketRecvEx(m_nSocket, cpcTimeout);
        KG_PROCESS_ERROR(nRetCode >= 0);                                // error

        if (0 == nRetCode)
        {
            nResult = 0; goto Exit0;
        }

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
            if (nRetCode)
            { // accept success, try next loop, do not need assert here.
                nResult = 0; goto Exit0;                                // time out
            }

            nResult = -1; goto Exit0;
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
    }

    return nResult;
}

KG_NAMESPACE_END

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

KG_AsyncSocketStreamList::KG_AsyncSocketStreamList()
{
}

KG_AsyncSocketStreamList::~KG_AsyncSocketStreamList()
{
}

void KG_AsyncSocketStreamList::Insert(SPIKG_SocketStream spStream)
{
    if (!spStream)
    {
        return;
    }

    m_StreamList.push_back(spStream);
}

void KG_AsyncSocketStreamList::Remove(SPIKG_SocketStream spStream)
{
    if (!spStream)
    {
        return;
    }

    auto it = std::find(m_StreamList.begin(), m_StreamList.end(), spStream);
    if (m_StreamList.end() == it)
    {
        return;
    }

    m_StreamList.erase(it);
}

void KG_AsyncSocketStreamList::Destroy()
{
#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    return ProcessDestroy();
#else                                                                   // linux   platform
#endif // KG_PLATFORM_WINDOWS
}

bool KG_AsyncSocketStreamList::Activate(UINT32 uMaxCount, UINT32 &uCurCount, PKG_SocketEvent pEventList)
{

}

void KG_AsyncSocketStreamList::ProcessDestroy()
{
    int                   nRetCode = 0;
    PKG_AsyncSocketStream pStream  = NULL;

    for (;;)
    {
        for (auto it = m_StreamList.begin(); it != m_StreamList.end();)
        {
            pStream = static_cast<PKG_AsyncSocketStream>(it->get());

            if (!pStream)
            { // pStream is NULL
                KG_ASSERT(false);
                m_StreamList.erase(it++);
                continue;
            }

            nRetCode = pStream->IsValid();
            if (!nRetCode)
            {

            }


        }

        if (!m_StreamList.empty())
        {
            xzero::KG_MilliSleep(50);
        }
    }
}

KG_NAMESPACE_END

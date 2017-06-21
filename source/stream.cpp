#include "stream.h"
#include "wrapper.h"

KG_NAMESPACE_BEGIN(xnet)

KG_SocketStream::KG_SocketStream()
{
    m_nInitFlag     = false;
    m_nOpenFlag     = false;
    m_nSocket       = KG_INVALID_SOCKET;
    m_nErrCode      = 0;
    m_uRecvBuffSize = 0;
    m_uSendBuffSize = 0;
    xzero::KG_ZeroMemory(&m_saAddress, sizeof(sockaddr_in));
}

KG_SocketStream::~KG_SocketStream()
{
    KG_ASSERT(!m_nInitFlag                   && "[ERROR] Forgot invoking KG_SocketStream::Close()?");
    KG_ASSERT(!m_nOpenFlag                   && "[ERROR] Forgot invoking KG_SocketStream::Close()?");
    KG_ASSERT(KG_INVALID_SOCKET == m_nSocket && "[ERROR] Forgot invoking KG_SocketStream::Close()?");
}

int KG_SocketStream::Init(const SOCKET nSocket, const sockaddr_in &saAddress, const unsigned int uRecvBuffSize, const unsigned int uSendBuffSize)
{
    int nResult  = false;
    int nRetCode = false;

    KG_PROCESS_ERROR(!m_nInitFlag && "[ERROR] KG_SocketStream has been initialized!");
    KG_PROCESS_ERROR(!m_nOpenFlag && "[ERROR] KG_SocketStream has been opened!"     );
    KG_PROCESS_ERROR(KG_INVALID_SOCKET != nSocket && nSocket > 0);

    m_nSocket       = nSocket;
    m_saAddress     = saAddress;
    m_uRecvBuffSize = uRecvBuffSize;
    m_uSendBuffSize = uSendBuffSize;

    if (m_uRecvBuffSize > 0)
    {
        nRetCode = KG_SetSocketRecvBuff(m_nSocket, m_uRecvBuffSize);
        KG_PROCESS_ERROR(nRetCode);
    }

    if (m_uSendBuffSize > 0)
    {
        nRetCode = KG_SetSocketSendBuff(m_nSocket, m_uSendBuffSize);
        KG_PROCESS_ERROR(nRetCode);
    }

    m_nInitFlag = true;
    m_nOpenFlag = false;

    nResult = true;
Exit0:
    if (!nResult)
    { // do some clean up here
        m_nSocket       = KG_INVALID_SOCKET;
        m_nErrCode      = KG_GetSocketErrCode();
        m_uRecvBuffSize = 0;
        m_uSendBuffSize = 0;
        xzero::KG_ZeroMemory(&m_saAddress, sizeof(sockaddr_in));
    }

    return nResult;
}

KG_NAMESPACE_END

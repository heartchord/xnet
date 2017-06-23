#include "stream.h"
#include "wrapper.h"

KG_NAMESPACE_BEGIN(xnet)

bool KG_EncapsulatePacHead(char * const cpBuff, const UINT32 uBuffSize, UINT32 n)
{
    int bResult = false;

    KG_PROCESS_PTR_ERROR(cpBuff);
    KG_PROCESS_ERROR(uBuffSize > 0);

    for (UINT32 i = 0; i < uBuffSize; i++)
    {
        cpBuff[i] = n & 0xFF;
        n         = n >> 8;
    }

    bResult = true;
Exit0:
    return bResult;
}

KG_SocketStream::KG_SocketStream()
{
    m_nSocket  = KG_INVALID_SOCKET;
    m_nErrCode = 0;
    xzero::KG_ZeroMemory(&m_saAddress, sizeof(sockaddr_in));
}

KG_SocketStream::~KG_SocketStream()
{
    KG_ASSERT(KG_INVALID_SOCKET == m_nSocket && "[ERROR] Forgot invoking KG_SocketStream::Close()?");
}

int KG_SocketStream::Init(const SOCKET nSocket, const sockaddr_in &saAddress, const UINT32 uRecvBuffSize, const UINT32 uSendBuffSize)
{
    int nResult  = false;
    int nRetCode = false;

    KG_PROCESS_SOCKET_ERROR(nSocket);
    KG_PROCESS_ERROR(KG_INVALID_SOCKET == m_nSocket && "[ERROR] It seems KG_SocketStream has been initialized!");

    m_nSocket   = nSocket;
    m_saAddress = saAddress;

    if (uRecvBuffSize > 0)
    {
        nRetCode = KG_SetSocketRecvBuff(m_nSocket, uRecvBuffSize);
        KG_PROCESS_ERROR(nRetCode);
    }

    if (uSendBuffSize > 0)
    {
        nRetCode = KG_SetSocketSendBuff(m_nSocket, uSendBuffSize);
        KG_PROCESS_ERROR(nRetCode);
    }

    nResult = true;
Exit0:
    if (!nResult)
    { // Do some clean up here. If error occurs, don't close socket here, because the ownership wasn't  transferred.
        m_nSocket  = KG_INVALID_SOCKET;
        m_nErrCode = KG_GetSocketErrCode();
        xzero::KG_ZeroMemory(&m_saAddress, sizeof(sockaddr_in));
    }

    return nResult;
}

bool KG_SocketStream::Open()
{
    return true;
}

bool KG_SocketStream::Close()
{
    bool bResult  = false;
    int  nRetCode = false;

    nRetCode = KG_CloseSocketSafely(m_nSocket);
    KG_PROCESS_ERROR(nRetCode);

    m_nErrCode = 0;
    xzero::KG_ZeroMemory(&m_saAddress, sizeof(sockaddr_in));

    bResult = true;
Exit0:
    if (!bResult)
    {
        m_nErrCode = KG_GetSocketErrCode();
    }
    return bResult;
}

int KG_SocketStream::CheckSend(const timeval * const cpcTimeOut)
{
    int nResult  = -1;
    int nRetCode = 0;

    KG_PROCESS_SOCKET_ERROR(m_nSocket);

    nResult = KG_CheckSocketSendEx(m_nSocket, cpcTimeOut);
Exit0:
    if (-1 == nResult)
    {
        m_nErrCode = KG_GetSocketErrCode();
    }
    return nResult;
}

int KG_SocketStream::CheckRecv(const timeval * const cpcTimeOut)
{
    int nResult = -1;
    int nRetCode = 0;

    KG_PROCESS_SOCKET_ERROR(m_nSocket);

    nResult = KG_CheckSocketRecvEx(m_nSocket, cpcTimeOut);
Exit0:
    if (-1 == nResult)
    {
        m_nErrCode = KG_GetSocketErrCode();
    }
    return nResult;
}

int KG_SocketStream::Send(xbuff::SPIKG_Buffer &spBuffer, const UINT32 uPakHeadSize, const timeval *const cpcTimeOut)
{
    int    nResult   = -1;
    int    nRetCode  = 0;
    char * pBuff     = NULL;
    UINT32 uBuffSize = 0;
    UINT32 uReserved = 0;

    KG_PROCESS_ERROR(spBuffer);
    KG_PROCESS_SOCKET_ERROR(m_nSocket);
    KG_PROCESS_ERROR(uPakHeadSize > 0 && uPakHeadSize <= KG_MAX_PAK_HEAD_SIZE);

    uBuffSize = spBuffer->CurSize();
    KG_PROCESS_ERROR(uBuffSize > 0);

    pBuff = (char *)spBuffer->Buf();
    KG_PROCESS_PTR_ERROR(pBuff);

    uReserved = spBuffer->ResSize();
    KG_PROCESS_ERROR(uReserved >= uPakHeadSize);

    // encapsulate package head
    pBuff     -= uPakHeadSize;                                          // move pointer to package head position.
    uBuffSize += uPakHeadSize;                                          // total size = head size + body size.
    nRetCode = KG_EncapsulatePacHead(pBuff, uPakHeadSize, uBuffSize);
    KG_PROCESS_ERROR(nRetCode);

    nResult = KG_CheckSendSocketData(m_nSocket, pBuff, uBuffSize, uBuffSize, cpcTimeOut);
Exit0:
    if (-1 == nResult)
    {
        m_nErrCode = KG_GetSocketErrCode();
    }
    return nResult;
}

int KG_SocketStream::Recv(xbuff::SPIKG_Buffer &spBuffer, const UINT32 uPakHeadSize, const timeval *const cpcTimeOut)
{
    int    nResult    = -1;
    int    nRetCode   = 0;
    UINT32 uPakSize   = 0;
    UINT32 uRecvBytes = 0;

    KG_PROCESS_SOCKET_ERROR(m_nSocket);
    KG_PROCESS_ERROR(uPakHeadSize > 0 && uPakHeadSize <= KG_MAX_PAK_HEAD_SIZE);

    // recv pak head
    nRetCode = KG_CheckRecvSocketData(m_nSocket, (char * const)&uPakSize, uPakHeadSize, &uRecvBytes, cpcTimeOut);
    KG_PROCESS_ERROR(nRetCode >= 0);                                    // error

    if (0 == nRetCode)
    {
        if (0 == uRecvBytes)
        {
            nResult = 0; goto Exit0;                                    // timeout
        }
        KG_PROCESS_ERROR(false);                                        // error : no buffer, incomplete data
    }

    KG_PROCESS_ERROR(uPakHeadSize == uRecvBytes);
    KG_PROCESS_ERROR(uPakSize > 0 && uPakSize >= uPakHeadSize);

    // recv pak body
    uPakSize -= uPakHeadSize;                                           // exclude head
    spTempBuffer = KG_AllocateBufferSP((unsigned)uPakSize);
    KG_PROCESS_ERROR_RET_CODE(spTempBuffer, -1);                                    // error

                                                                                    // 接收包体
    nRetCode = KG_CheckRecvSocketData(
        m_hRemoteSocket,
        (char * const)spTempBuffer->GetData(),
        uPakSize,
        &uRecvBytes,
        pTimeOut,
        true
    );

    if (-1 == nRetCode)                                                             // error
    {
        m_nLastErrorCode = KG_GetSocketLastError();
    }

    KG_PROCESS_ERROR_RET_CODE(nRetCode > 0, -1);                         // error - 无缓冲接收必须成功
    KG_PROCESS_ERROR_RET_CODE(uRecvBytes >= uPakSize, -1);                         // error - 无缓冲接收必须成功

    spBuffer = spTempBuffer;
    nResult = 1;
Exit0:
    if (!nResult)
    {
    }
    return nResult;
}

KG_NAMESPACE_END

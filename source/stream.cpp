#include "stream.h"
#include "wrapper.h"

#include <algorithm>

KG_NAMESPACE_BEGIN(xnet)

// local variables
UINT32 l_uSocketMemBlockSizeArray[] =
{
    2  * 8,                                                             // 16    bytes        [00-01)
    4  * 8,                                                             // 32    bytes        [01-02)
    6  * 8,                                                             // 48    bytes        [02-03)
    8  * 8,                                                             // 64    bytes        [03-04)
    12 * 8,                                                             // 96    bytes        [04-05)
    16 * 8,                                                             // 128   bytes        [05-06)
    20 * 8,                                                             // 160   bytes        [06-07)
    24 * 8,                                                             // 192   bytes        [07-08)
    32 * 8,                                                             // 256   bytes        [08-09)
    40 * 8,                                                             // 320   bytes        [09-10)
    48 * 8,                                                             // 384   bytes        [10-11)
    56 * 8,                                                             // 448   bytes        [11-12)
    64 * 8,                                                             // 512   bytes        [12-13)
    96 * 8,                                                             // 768   bytes        [13-14)
    1  * 1024,                                                          // 1024  bytes(1  KB) [14-15)
    2  * 1024,                                                          // 2048  bytes(2  KB) [15-16)
    3  * 1024,                                                          // 3072  bytes(3  KB) [16-17)
    4  * 1024,                                                          // 4096  bytes(4  KB) [17-18)
    5  * 1024,                                                          // 5120  bytes(5  KB) [18-19)
    6  * 1024,                                                          // 6144  bytes(6  KB) [18-20)
    7  * 1024,                                                          // 7168  bytes(7  KB) [20-21)
    8  * 1024,                                                          // 8192  bytes(8  KB) [21-22)
    16 * 1024,                                                          // 16384 bytes(16 KB) [22-23)
    32 * 1024,                                                          // 32768 bytes(32 KB) [23-24)
    64 * 1024,                                                          // 65536 bytes(64 KB) [24-25)
};

const UINT32 l_uSocketMemBlockSizeArraySize = sizeof(l_uSocketMemBlockSizeArray) / sizeof(l_uSocketMemBlockSizeArray[0]);

typedef xzero::KG_MemoryPool<l_uSocketMemBlockSizeArraySize, l_uSocketMemBlockSizeArray> KG_SocketMemoryPool;
static KG_SocketMemoryPool l_SocketMemoryPool;

bool KG_EncapsulatePakHead(char * const cpBuff, const UINT32 uBuffSize, UINT32 n)
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
    m_nSocket    = KG_INVALID_SOCKET;
    m_nConnIndex = -1;
    m_nErrCode   = 0;
    xzero::KG_ZeroMemory(&m_saAddress, sizeof(sockaddr_in));
}

KG_SocketStream::~KG_SocketStream()
{
    KG_ASSERT(KG_INVALID_SOCKET == m_nSocket && "[ERROR] Forgot invoking KG_SocketStream::Close()?");
}

bool KG_SocketStream::Init(const SOCKET nSocket, const sockaddr_in &saAddress, const UINT32 uRecvBuffSize, const UINT32 uSendBuffSize)
{
    bool bResult  = false;
    int  nRetCode = false;

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

    bResult = true;
Exit0:
    if (!bResult)
    { // Do some clean up here. If an error occurs, don't close socket here, because the ownership isn't transferred.
        m_nSocket  = KG_INVALID_SOCKET;                                 // close it outside
        m_nErrCode = KG_GetSocketErrCode();
        xzero::KG_ZeroMemory(&m_saAddress, sizeof(sockaddr_in));
    }

    return bResult;
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
    nRetCode = KG_EncapsulatePakHead(pBuff, uPakHeadSize, uBuffSize);
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
    KG_PROCESS_ERROR_Q(nRetCode >= 0);                                  // error : include disconnection

    if (0 == nRetCode)
    {
        // no data in socket buffer
        KG_PROCESS_SUCCESS_RET_CODE(0 == uRecvBytes, 0);                // time out
        // incomplete data in socket buffer
        KG_PROCESS_ERROR(false);                                        // error : no buffer, incomplete data
    }

    KG_PROCESS_ERROR(uPakHeadSize == uRecvBytes);
    KG_PROCESS_ERROR(uPakSize > 0 && uPakSize >= uPakHeadSize);

    // recv pak body
    uPakSize -= uPakHeadSize;                                           // exclude head
    spBuffer  = xbuff::KG_GetSharedBuffFromMemoryPool(&l_SocketMemoryPool, uPakSize, KG_MAX_PAK_HEAD_SIZE);
    KG_PROCESS_ERROR(spBuffer);                                         // error

    nRetCode = KG_CheckRecvSocketData(m_nSocket, (char * const)spBuffer->Buf(), uPakSize, &uRecvBytes, cpcTimeOut);
    KG_PROCESS_ERROR(nRetCode > 0);                                     // error : no buffer, incomplete data
    KG_PROCESS_ERROR(uRecvBytes == uPakSize);

    nResult = 1;
Exit0:
    if (-1 == nResult)
    {
        if (spBuffer)
        { // error : drop data
            spBuffer = xbuff::SPIKG_Buffer();
        }
        
        m_nErrCode = KG_GetSocketErrCode();
    }
    return nResult;
}

void KG_SocketStream::SetConnIndex(const int nConnIndex)
{
    m_nConnIndex = nConnIndex;
}

int KG_SocketStream::GetConnIndex() const
{
    return m_nConnIndex;
}

bool KG_SocketStream::IsAlive()
{
    bool bResult  = false;
    int  nRetCode = false;
    int  nData    = 0;

    nRetCode = IsValid();
    KG_PROCESS_ERROR_Q(nRetCode);

    nRetCode = ::send(m_nSocket, (char *)&nData, 0, 0);                 // send 0-byte to test
    KG_PROCESS_ERROR_Q(-1 != nRetCode);

    bResult = true;
Exit0:
    return bResult;
}

bool KG_SocketStream::IsValid()
{
    return KG_INVALID_SOCKET != m_nSocket && m_nSocket >= 0;
}

int KG_SocketStream::GetErrCode() const
{
    return m_nErrCode;
}

void KG_SocketStream::GetAddress(struct in_addr *pIp, USHORT *pnPort) const
{
    *pIp    = m_saAddress.sin_addr;
    *pnPort = m_saAddress.sin_port;
}

class KG_SocketStreamDeleter
{
public:
    KG_SocketStreamDeleter(KG_SocketMemoryPool *pMP);
    ~KG_SocketStreamDeleter();

public:
    void operator()(PIKG_SocketStream piSocketStream);

private:
    KG_SocketMemoryPool *m_pMemoryPool;

};

inline KG_SocketStreamDeleter::KG_SocketStreamDeleter(KG_SocketMemoryPool *pMemoryPool) : m_pMemoryPool(pMemoryPool)
{
    KG_ASSERT(NULL != m_pMemoryPool);
}

inline KG_SocketStreamDeleter::~KG_SocketStreamDeleter()
{
}

inline void KG_SocketStreamDeleter::operator()(PIKG_SocketStream piSocketStream)
{
    int   nRetCode = false;
    void *pvBuff = NULL;

    KG_PROCESS_PTR_ERROR(piSocketStream);
    KG_PROCESS_PTR_ERROR(m_pMemoryPool);

    piSocketStream->Close();

    pvBuff = (void *)piSocketStream;
    KG_PROCESS_PTR_ERROR(pvBuff);

    piSocketStream->~IKG_SocketStream();                                // this will invoke KG_SocketStream::~KG_SocketStream() automatically
    nRetCode = m_pMemoryPool->Put(&pvBuff);
    KG_PROCESS_ERROR(nRetCode);

Exit0:
    return;
}

SPIKG_SocketStream KG_GetSocketStreamFromMemoryPool(SOCKET &nSocket, const sockaddr_in &saAddress, const UINT32 uRecvBuffSize, const UINT32 uSendBuffSize)
{
    bool             bResult = false;
    int              nRetCode = 0;
    void *           pvBuff = NULL;
    PKG_SocketStream pSocketStream = NULL;

    KG_PROCESS_SOCKET_ERROR(nSocket);

    nRetCode = l_SocketMemoryPool.Get(&pvBuff, sizeof(KG_SocketStream));
    KG_PROCESS_ERROR(nRetCode);
    KG_PROCESS_PTR_ERROR(pvBuff);

    pSocketStream = ::new(pvBuff)KG_SocketStream();                     // placement operator new
    KG_PROCESS_PTR_ERROR(pSocketStream);

    nRetCode = pSocketStream->Init(nSocket, saAddress, uRecvBuffSize, uSendBuffSize);
    KG_PROCESS_ERROR(nRetCode);
    nSocket = KG_INVALID_SOCKET;                                        // transfer ownership

    bResult = true;
Exit0:
    if (!bResult)
    {
        if (pSocketStream)
        {
            pSocketStream->Close();
            pSocketStream = NULL;
        }

        if (pvBuff)
        {
            l_SocketMemoryPool.Put(&pvBuff);
        }
    }
    return SPIKG_SocketStream(pSocketStream, KG_SocketStreamDeleter(&l_SocketMemoryPool));
}

KG_AsyncSocketStream::KG_AsyncSocketStream()
{
    m_nSocket            = KG_INVALID_SOCKET;
    m_nConnIndex         = -1;
    m_nErrCode           = 0;
    m_uRecvHeadPos       = 0;
    m_uRecvTailPos       = 0;
    xzero::KG_ZeroMemory(&m_saAddress, sizeof(sockaddr_in));

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform
    m_bCallBackFlag      = false;
    m_nCallBackErrCode   = 0;
    m_nCallBackDataSize  = 0;
    m_bRecvCompletedFlag = false;
#else                                                                   // linux   platform
#endif // KG_PLATFORM_WINDOWS
}

KG_AsyncSocketStream::~KG_AsyncSocketStream()
{
    KG_ASSERT(KG_INVALID_SOCKET == m_nSocket && "[ERROR] Forgot invoking KG_AsyncSocketStream::Close()?");
}

bool KG_AsyncSocketStream::Init(const SOCKET nSocket, const sockaddr_in &saAddress, const UINT32 uRecvBuffSize, const UINT32 uSendBuffSize)
{
    bool bResult  = false;
    int  nRetCode = false;

    KG_PROCESS_SOCKET_ERROR(nSocket);
    KG_PROCESS_ERROR(KG_INVALID_SOCKET == m_nSocket && "[ERROR] It seems KG_SocketStream has been initialized!");
    KG_PROCESS_ERROR(uRecvBuffSize > 0);
    KG_PROCESS_ERROR(uSendBuffSize > 0);

    m_nSocket   = nSocket;
    m_saAddress = saAddress;

    // set socket opts
    nRetCode = KG_SetSocketBlockMode(m_nSocket, false);
    KG_PROCESS_ERROR(nRetCode);

    nRetCode = KG_SetSocketRecvBuff(m_nSocket, uRecvBuffSize);
    KG_PROCESS_ERROR(nRetCode);

    nRetCode = KG_SetSocketSendBuff(m_nSocket, uSendBuffSize);
    KG_PROCESS_ERROR(nRetCode);

    // alloc recv data buffer
    m_spRecvBuffer = xbuff::KG_GetSharedBuffFromMemoryPool(&l_SocketMemoryPool, uRecvBuffSize);
    KG_PROCESS_ERROR(m_spRecvBuffer);

    bResult = true;
Exit0:
    if (!bResult)
    { // Do some clean up here. If an error occurs, don't close socket here, because the ownership isn't transferred.
        m_nSocket  = KG_INVALID_SOCKET;                                 // close it outside
        m_nErrCode = KG_GetSocketErrCode();

        m_spRecvBuffer.reset();
        xzero::KG_ZeroMemory(&m_saAddress, sizeof(sockaddr_in));
    }

    return bResult;
}

bool KG_AsyncSocketStream::GetClosedFlag() const
{
    return m_bClosedFlag;
}

void KG_AsyncSocketStream::SetClosedFlag(bool bClosedFlag)
{
    m_bClosedFlag = true;
}

#ifdef KG_PLATFORM_WINDOWS                                              // windows platform

bool KG_AsyncSocketStream::GetCallBackFlag() const
{
    return m_bCallBackFlag;
}

void KG_AsyncSocketStream::SetCallBackFlag(bool bCallBackFlag)
{
    m_bCallBackFlag = bCallBackFlag;
}

bool KG_AsyncSocketStream::GetRecvCompletedFlag() const
{
    return m_bRecvCompletedFlag;
}

void KG_AsyncSocketStream::SetRecvCompletedFlag(bool bRecvCompletedFlag)
{
    m_bRecvCompletedFlag = bRecvCompletedFlag;
}

void KG_AsyncSocketStream::OnRecvCompleted(DWORD dwErrCode, DWORD dwBytesTransfered, LPOVERLAPPED lpOverlapped)
{
    UNREFERENCED_PARAMETER(lpOverlapped);
    m_nCallBackErrCode   = dwErrCode;
    m_nCallBackDataSize  = dwBytesTransfered;
    m_bCallBackFlag      = true;
    m_bRecvCompletedFlag = true;
}

#else                                                                   // linux   platform
#endif // KG_PLATFORM_WINDOWS

KG_NAMESPACE_END

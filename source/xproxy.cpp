#include "xproxy.h"

KG_NAMESPACE_BEGIN(xnet)

bool IKG_ServerProxy::_OnClientClosed(SPIKG_SocketStream spStream)
{
    bool           bResult  = false;
    int            nRetCode = false;
    USHORT         uPort    = 0;
    struct in_addr addr;

    KG_PROCESS_ERROR(spStream);

    spStream->GetAddress(&addr, &uPort);
    xzero::KG_DebugPrintln("[MSG] IKG_ServerProxy - close an existed connection[ip - %s, port - %d]", ::inet_ntoa(addr), ntohs(uPort));

    bResult = true;
Exit0:
    return bResult;
}

bool IKG_ServerProxy::IKG_ServerProxy::_OnClientConnected(SPIKG_SocketStream spStream)
{
    bool           bResult  = false;
    int            nRetCode = false;
    USHORT         uPort    = 0;
    struct in_addr addr;

    KG_PROCESS_ERROR(spStream);

    spStream->GetAddress(&addr, &uPort);
    xzero::KG_DebugPrintln("[MSG] IKG_ServerProxy - accept a new connection[ip - %s, port - %d]", ::inet_ntoa(addr), ntohs(uPort));

    bResult = true;
Exit0:
    return bResult;
}

bool IKG_ServerProxy::_OnClientDataRecvd(SPIKG_SocketStream spStream, xbuff::SPIKG_Buffer spBuff)
{
    bool            bResult = false;
    int            nRetCode = false;
    USHORT         uPort = 0;
    struct in_addr addr;
    KG_PROCESS_ERROR(spStream);

    spStream->GetAddress(&addr, &uPort);
    xzero::KG_DebugPrintln("[MSG] IKG_ServerProxy - receive a package connection[ip - %s, port - %d]", ::inet_ntoa(addr), ntohs(uPort));
    xzero::KG_PrintlnInHex(spBuff->Buf(), spBuff->CurSize());

    bResult = true;
Exit0:
    return bResult;
}

KG_SingleClientServerProxy::KG_SingleClientServerProxy()
{
}

KG_SingleClientServerProxy::~KG_SingleClientServerProxy()
{
    KG_ASSERT(!m_spSocketStream   && "[ERROR] Forgot invoking KG_SingleClientServerProxy::Close()?");
    KG_ASSERT(!m_spSocketAcceptor && "[ERROR] Forgot invoking KG_SingleClientServerProxy::Close()?");
}

bool KG_SingleClientServerProxy::Init(const char * const cszIpAddr, const USHORT uPort)
{
    bool bResult  = false;
    int  nRetCode = 0;

    KG_PROCESS_ERROR(uPort > 0);
    KG_PROCESS_ERROR(!m_spSocketStream);
    KG_PROCESS_ERROR(!m_spSocketAcceptor);

    // start net service
    nRetCode = KG_SINGLETON_REF(KG_NetService).Open();
    KG_PROCESS_ERROR(nRetCode);

    // init socket acceptor
    m_spSocketAcceptor = SPKG_SocketAcceptor(::new KG_SocketAcceptor);
    KG_PROCESS_ERROR(m_spSocketAcceptor);

    nRetCode = m_spSocketAcceptor->Init(cszIpAddr, uPort);
    KG_PROCESS_ERROR(nRetCode);

    bResult = true;
Exit0:
    if (!bResult)
    {
        if (m_spSocketAcceptor)
        {
            m_spSocketAcceptor->Close();
            m_spSocketAcceptor.reset();
        }
    }

    return bResult;
}

bool KG_SingleClientServerProxy::Close()
{
    int nRetCode = 0;

    // release socket acceptor
    if (m_spSocketAcceptor)
    {
        nRetCode = m_spSocketAcceptor->Close();
        KG_ASSERT(nRetCode);
        m_spSocketAcceptor.reset();
    }

    // release socket stream
    if (m_spSocketStream)
    {
        nRetCode = m_spSocketStream->Close();
        KG_ASSERT(nRetCode);
        m_spSocketStream.reset();
    }

    return true;
}

bool KG_SingleClientServerProxy::Activate()
{
    ProcessAccept();
    ProcessPackage();

    return true;
}

void KG_SingleClientServerProxy::ProcessAccept()
{
    int     nResult  = -1;
    int     nRetCode = 0;
    timeval tm       = {0, 0};

    KG_PROCESS_SUCCESS(m_spSocketStream);                               // connected
    KG_PROCESS_ERROR(m_spSocketAcceptor);                               // not init?

    nRetCode = m_spSocketAcceptor->Accept(m_spSocketStream, &tm);
    KG_PROCESS_ERROR(nRetCode >= 0);                                    // error
    KG_PROCESS_SUCCESS_RET_CODE(0 == nRetCode, 0);                      // time out

    nRetCode = _OnClientConnected(m_spSocketStream);
    KG_PROCESS_ERROR(nRetCode);                                         // error

Exit1:
    nResult = 1;
Exit0:
    if (-1 == nResult)
    {
        CloseConnection();
    }
    return;
}

void KG_SingleClientServerProxy::ProcessPackage()
{
    int                 nResult        = -1;
    int                 nRetCode       = 0;
    static int          nPackageSerial = 0;
    timeval             tv             = {0, 0};
    xbuff::SPIKG_Buffer spBuff;

    KG_PROCESS_ERROR(m_spSocketAcceptor);                               // error
    KG_PROCESS_SUCCESS(!m_spSocketStream);                              // not connected

    for (;;)
    {
        nRetCode = m_spSocketStream->Recv(spBuff, 4, &tv);
        KG_PROCESS_ERROR_Q(nRetCode >= 0);                              // error
        KG_PROCESS_SUCCESS_RET_CODE(0 == nRetCode, 0);                  // time out

        xzero::KG_DebugPrintln("[MSG] KG_SingleClientServerProxy - Package Serial = %d", nPackageSerial++);

        nRetCode = _OnClientDataRecvd(m_spSocketStream, spBuff);
        KG_PROCESS_ERROR(nRetCode);                                     // error
    }

Exit1:
    nResult = 1;
Exit0:
    if (-1 == nResult)
    {
        CloseConnection();
    }
    return;
}

void KG_SingleClientServerProxy::CloseConnection()
{
    bool bResult  = false;
    int  nRetCode = false;

    KG_PROCESS_SUCCESS(!m_spSocketStream);                              // not connected

    nRetCode = _OnClientClosed(m_spSocketStream);
    KG_PROCESS_ERROR(nRetCode);

    nRetCode = m_spSocketStream->Close();
    KG_PROCESS_ERROR(nRetCode);
    m_spSocketStream.reset();

Exit1:
    bResult = true;
Exit0:
    return;
}

KG_MultiClientServerProxy::KG_MultiClientServerProxy()
{
}

KG_MultiClientServerProxy::~KG_MultiClientServerProxy()
{
    KG_ASSERT(m_SocketStreamList.empty() && "[ERROR] Forgot invoking KG_MultiClientServerProxy::Close()?");
    KG_ASSERT(!m_spSocketAcceptor        && "[ERROR] Forgot invoking KG_MultiClientServerProxy::Close()?");
}

bool KG_MultiClientServerProxy::Init(const char * const cszIpAddr, const USHORT uPort)
{
    bool bResult  = false;
    int  nRetCode = 0;

    KG_PROCESS_ERROR(uPort > 0);
    KG_PROCESS_ERROR(!m_spSocketAcceptor);

    // start net service
    nRetCode = KG_SINGLETON_REF(KG_NetService).Open();
    KG_PROCESS_ERROR(nRetCode);

    // init socket acceptor
    m_spSocketAcceptor = SPKG_SocketAcceptor(::new KG_SocketAcceptor);
    KG_PROCESS_ERROR(m_spSocketAcceptor);

    nRetCode = m_spSocketAcceptor->Init(cszIpAddr, uPort);
    KG_PROCESS_ERROR(nRetCode);

    bResult = true;
Exit0:
    if (!bResult)
    {
        if (m_spSocketAcceptor)
        {
            m_spSocketAcceptor->Close();
            m_spSocketAcceptor.reset();
        }
    }

    return bResult;
}

bool KG_MultiClientServerProxy::Close()
{
    int nRetCode = 0;

    // release socket acceptor
    if (m_spSocketAcceptor)
    {
        nRetCode = m_spSocketAcceptor->Close();
        KG_ASSERT(nRetCode);
        m_spSocketAcceptor.reset();
    }

    // release all socket streams
    for (auto it = m_SocketStreamList.begin(); it != m_SocketStreamList.end(); it++)
    {
        if (*it)
        {
            (*it)->Close();
        }
    }

    m_SocketStreamList.clear();
    return true;
}

bool KG_MultiClientServerProxy::Activate()
{
    ProcessAccept();
    ProcessPackage();

    return true;
}

void KG_MultiClientServerProxy::ProcessAccept()
{
    int                nResult  = -1;
    int                nRetCode = 0;
    timeval            tm       = {0, 0};
    SPIKG_SocketStream spSocketStream;
    KG_MySocketStreamList::iterator it;

    KG_PROCESS_ERROR(m_spSocketAcceptor);                               // not init?

    nRetCode = m_spSocketAcceptor->Accept(spSocketStream, &tm);
    KG_PROCESS_ERROR(nRetCode >= 0);                                    // error
    KG_PROCESS_SUCCESS_RET_CODE(0 == nRetCode, 0);                      // time out

    nRetCode = _OnClientConnected(spSocketStream);
    KG_PROCESS_ERROR(nRetCode);                                         // error

    it = std::find(m_SocketStreamList.begin(), m_SocketStreamList.end(), spSocketStream);
    KG_PROCESS_ERROR(m_SocketStreamList.end() == it);                   // error

    m_SocketStreamList.push_back(spSocketStream);

    nResult = 1;
Exit0:
    if (-1 == nResult)
    {
        if (spSocketStream)
        {
            spSocketStream->Close();
            spSocketStream.reset();
        }
    }
    return;
}

void KG_MultiClientServerProxy::ProcessPackage()
{
    int                 nResult        = -1;
    int                 nRetCode       = 0;
    static int          nPackageSerial = 0;
    timeval             tv             = {0, 0};
    xbuff::SPIKG_Buffer spBuff;

    KG_PROCESS_ERROR(m_spSocketAcceptor);                               // error

    for (auto it = m_SocketStreamList.begin(); it != m_SocketStreamList.end();)
    {
        nRetCode = ProcessOnePackage(*it);
        if (-1 == nRetCode)                                             // error
        {
            CloseConnection(*it);
            m_SocketStreamList.erase(it++);
            continue;
        }

        if (nRetCode > 0)                                               // success
        {
            xzero::KG_DebugPrintln("[MSG] KG_MultiClientServerProxy - Package Serial = %d", nPackageSerial++);
            nRetCode = _OnClientDataRecvd(*it++, spBuff);
            KG_PROCESS_ERROR(nRetCode);
            continue;
        }

        it++;
    }

    nResult = 1;
Exit0:
    return;
}

void KG_MultiClientServerProxy::CloseConnection(SPIKG_SocketStream spStream)
{
    bool bResult  = false;
    int  nRetCode = false;

    KG_PROCESS_SUCCESS(!spStream);

    nRetCode = _OnClientClosed(spStream);
    KG_PROCESS_ERROR(nRetCode);

    nRetCode = spStream->Close();
    KG_PROCESS_ERROR(nRetCode);
    spStream.reset();

Exit1:
    bResult = true;
Exit0:
    return;
}

int KG_MultiClientServerProxy::ProcessOnePackage(SPIKG_SocketStream spStream)
{
    int                 nResult = -1;
    int                 nRetCode = 0;
    static int          nPackageSerial = 0;
    timeval             tv = { 0, 0 };
    xbuff::SPIKG_Buffer spBuff;

    KG_PROCESS_ERROR(spStream);

    for (;;)
    {
        nRetCode = spStream->Recv(spBuff, 4, &tv);
        KG_PROCESS_ERROR_Q(nRetCode >= 0);                              // error
        KG_PROCESS_SUCCESS_RET_CODE(0 == nRetCode, 0);                  // time out

        xzero::KG_DebugPrintln("[MSG] KG_SingleClientServerProxy - Package Serial = %d", nPackageSerial++);

        nRetCode = _OnClientDataRecvd(spStream, spBuff);
        KG_PROCESS_ERROR(nRetCode);                                     // error
    }

    nResult = 1;
Exit0:
    return nResult;
}

KG_EventModelServerProxy::KG_EventModelServerProxy()
{
}

KG_EventModelServerProxy::~KG_EventModelServerProxy()
{
    KG_ASSERT(!m_spSocketAcceptor && "[ERROR] Forgot invoking KG_EventModelServerProxy::Close()?");
}

bool KG_EventModelServerProxy::Init(const char * const cszIpAddr, const USHORT uPort)
{
    bool                    bResult             = false;
    int                     nRetCode            = false;
    PKG_SocketEvent         pEventList          = NULL;
    bool                    bEventListInit      = false;
    PKG_AsyncSocketAcceptor pSocketAcceptor     = NULL;
    bool                    bSocketAcceptorInit = false;

    KG_PROCESS_ERROR(uPort > 0);
    KG_PROCESS_ERROR(!m_spSocketAcceptor);

    // start net service
    // start net service
    nRetCode = KG_SINGLETON_REF(KG_NetService).Open();
    KG_PROCESS_ERROR(nRetCode);

    // init socket event array
    pEventList = ::new KG_SocketEvent[KG_MAX_SOCKET_EVENT];
    KG_PROCESS_PTR_ERROR(pEventList);
    bEventListInit = true;

    // init socket server acceptor
    pSocketAcceptor = ::new KG_AsyncSocketAcceptor;
    KG_PROCESS_PTR_ERROR(pSocketAcceptor);

    nRetCode = pSocketAcceptor->Init(cszIpAddr, uPort, KG_MAX_SOCKET_ACCEPT_EVENT, 1024 * 4, 1024 * 16);
    KG_PROCESS_ERROR(nRetCode);
    bSocketAcceptorInit = true;

    // success here
    m_spEventList = SPKG_SocketEvent(pEventList, [](KG_SocketEvent *p) { delete[] p; });
    pEventList    = NULL;                                               // transfer owners

    m_spSocketAcceptor = SPKG_AsyncSocketAcceptor(pSocketAcceptor);
    pSocketAcceptor    = NULL;                                          // transfer owners

    bResult = true;
Exit0:
    if (!bResult)
    {
        if (bSocketAcceptorInit)
        {
            nRetCode = pSocketAcceptor->Close();
            KG_ASSERT(nRetCode);
        }

        xzero::KG_DeletePtrSafely(pSocketAcceptor);
        bSocketAcceptorInit = false;

        if (bSocketAcceptorInit)
        {
        }
        xzero::KG_DeleteArrayPtrSafely(pEventList);
        bEventListInit = false;
    }

    return bResult;
}

bool KG_EventModelServerProxy::Close()
{
    int nRetCode = 0;

    KG_ASSERT(m_spEventList);
    KG_ASSERT(m_spSocketAcceptor);

    if (m_spSocketAcceptor)
    {
        nRetCode = m_spSocketAcceptor->Close();
        KG_ASSERT(nRetCode);
        m_spSocketAcceptor.reset();
    }

    if (m_spEventList)
    {
        m_spEventList.reset();
    }

    return true;
}

bool KG_EventModelServerProxy::Activate()
{
    ProcessNetEvent();
    return true;
}

bool KG_EventModelServerProxy::ProcessNetEvent()
{
    bool   bResult   = false;
    int    nRetCode  = false;
    UINT32 uCurCount = 0;
    UINT32 uType     = KG_SOCKET_EVENT_INIT;
    SPIKG_SocketStream spSocketStream;

    for (;;)
    {
        uCurCount = 0;

        nRetCode = m_spSocketAcceptor->Activate(KG_MAX_SOCKET_EVENT, uCurCount, m_spEventList.get());
        KG_PROCESS_ERROR(nRetCode);
        KG_PROCESS_SUCCESS(0 == uCurCount);

        for (UINT32 i = 0; i < uCurCount; i++)
        {
            spSocketStream = m_spEventList.get()[i].m_spStream;
            uType          = m_spEventList.get()[i].m_uType;

            if (!spSocketStream)
            {
                KG_ASSERT(false);
                continue;
            }

            if (KG_SOCKET_EVENT_ACCEPT & uType)
            {
                nRetCode = _OnClientConnected(spSocketStream);
                if (!nRetCode)
                {
                    CloseConnection(spSocketStream);
                    m_spEventList.get()[i].Reset();
                }
                continue;
            }

            if (!(KG_SOCKET_EVENT_READ & uType))
            {
                xzero::KG_DebugPrintln("[Error] unexpected socket event : %u", uType);
                CloseConnection(spSocketStream);
                m_spEventList.get()[i].Reset();
                continue;
            }

            nRetCode = ProcessOnePackage(spSocketStream);
            if (!nRetCode)
            {
                CloseConnection(spSocketStream);
                m_spEventList.get()[i].Reset();
            }
        }
    }

Exit1:
    bResult = true;
Exit0:
    return bResult;
}

bool KG_EventModelServerProxy::ProcessOnePackage(SPIKG_SocketStream spStream)
{
    bool                bResult  = false;
    int                 nRetCode = 0;
    static int          nPackageSerial = 0;
    timeval             tv = { 0, 0 };
    xbuff::SPIKG_Buffer spBuff;

    KG_PROCESS_ERROR(spStream);

    for (;;)
    {
        nRetCode = spStream->Recv(spBuff, 4, &tv);
        KG_PROCESS_ERROR_Q(nRetCode >= 0);                              // error
        KG_PROCESS_SUCCESS(nRetCode == 0);                              // time out

        xzero::KG_DebugPrintln("[MSG] KG_SingleClientServerProxy - Package Serial = %d", nPackageSerial++);

        nRetCode = _OnClientDataRecvd(spStream, spBuff);
        KG_PROCESS_ERROR(nRetCode);                                     // error
    }

Exit1:
    bResult = true;
Exit0:
    return bResult;
}

void KG_EventModelServerProxy::CloseConnection(SPIKG_SocketStream spStream)
{
    bool bResult = false;
    int  nRetCode = false;

    KG_PROCESS_SUCCESS(!spStream);

    nRetCode = _OnClientClosed(spStream);
    KG_PROCESS_ERROR(nRetCode);

    nRetCode = spStream->Close();
    KG_PROCESS_ERROR(nRetCode);
    spStream.reset();

Exit1:
    bResult = true;
Exit0:
    return;
}

KG_NAMESPACE_END

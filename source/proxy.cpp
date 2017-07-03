#include "proxy.h"

KG_NAMESPACE_BEGIN(xnet)

bool IKG_ServerProxy::_OnClientClosed(SPIKG_SocketStream &spStream)
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

bool IKG_ServerProxy::IKG_ServerProxy::_OnClientConnected(SPIKG_SocketStream &spStream)
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

bool IKG_ServerProxy::_OnClientDataRecvd(SPIKG_SocketStream &spStream, xbuff::SPIKG_Buffer &spBuff)
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

KG_NAMESPACE_END

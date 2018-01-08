#include "event.h"

KG_NAMESPACE_BEGIN(xnet)

KG_SocketEvent::KG_SocketEvent()
{
    Reset();
}

KG_SocketEvent::~KG_SocketEvent()
{
}

void KG_SocketEvent::Reset()
{
    m_uType = KG_SOCKET_EVENT_INIT;
    m_spStream.reset();
}

KG_NAMESPACE_END

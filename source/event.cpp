#include "event.h"

KG_NAMESPACE_BEGIN(xnet)

KG_SocketEvent::KG_SocketEvent()
{
    m_uType = KG_SOCKET_EVENT_INIT;
}

KG_SocketEvent::~KG_SocketEvent()
{
}

KG_NAMESPACE_END

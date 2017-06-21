#include "public.h"

KG_NAMESPACE_BEGIN(xnet)

int KG_GetSocketErrCode();
int KG_CloseSocketSafely(SOCKET &nSocket);
int KG_SetSocketRecvBuff(SOCKET nSocket, const unsigned int uRecvBuffSize);
int KG_SetSocketSendBuff(SOCKET nSocket, const unsigned int uSendBuffSize);

KG_NAMESPACE_END

#include "public.h"

KG_NAMESPACE_BEGIN(xnet)

SOCKET KG_CreateTcpSocket();
SOCKET KG_CreateUdpSocket();
SOCKET KG_CreateListenSocket(const char * const cszIpAddr, const WORD nPort, const int nBackLog = KG_DEFAULT_ACCEPT_BACK_LOG);

int  KG_CheckSocketSend    (SOCKET nSocket, const timeval *pcTimeOut = NULL);
int  KG_CheckSocketRecv    (SOCKET nSocket, const timeval *pcTimeOut = NULL);
int  KG_CheckSocketSendEx  (SOCKET nSocket, const timeval *pcTimeOut = NULL);
int  KG_CheckSocketRecvEx  (SOCKET nSocket, const timeval *pcTimeOut = NULL);
int  KG_CheckSendSocketData(SOCKET nSocket, const char * const cpcBuff, const UINT32 uBuffSize, const UINT32 uSendBytes, const timeval *pcTimeout = NULL);
int  KG_CheckRecvSocketData(SOCKET nSocket, char * const cpBuff, const UINT32 uBuffSize, UINT32 * const puRecvBytes, const timeval *pcTimeout = NULL);

bool KG_IsSocketEWouldBlock();
bool KG_IsSocketInterrupted();
bool KG_IsSocketClosedByRemote();

int  KG_GetSocketErrCode();
bool KG_CloseSocketSafely(SOCKET &nSocket);

bool KG_SetSocketRecvBuff   (SOCKET nSocket, const UINT32 uRecvBuffSize);
bool KG_SetSocketSendBuff   (SOCKET nSocket, const UINT32 uSendBuffSize);
bool KG_SetSocketBlockMode  (SOCKET &nSocket, bool bBlocked = true);
bool KG_SetSocketSendTimeOut(SOCKET &nSocket, const timeval &tv);
bool KG_SetSocketRecvTimeOut(SOCKET &nSocket, const timeval &tv);

KG_NAMESPACE_END

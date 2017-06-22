#include "public.h"

KG_NAMESPACE_BEGIN(xnet)

SOCKET KG_CreateTcpSocket();
SOCKET KG_CreateUdpSocket();
SOCKET KG_CreateListenSocket(const char * const cszIpAddr, const WORD nPort, const int nBackLog = KG_DEFAULT_ACCEPT_BACK_LOG);
int KG_GetSocketErrCode();
int KG_CheckSocketSend(SOCKET nSocket, const timeval *pcTimeOut = NULL);
int KG_CheckSocketRecv(SOCKET nSocket, const timeval *pcTimeOut = NULL);
int KG_CheckSocketSendEx(SOCKET nSocket, const timeval *pcTimeOut = NULL);
int KG_CheckSocketRecvEx(SOCKET nSocket, const timeval *pcTimeOut = NULL);
int KG_CheckSendSocketData(SOCKET nSocket, const char * const cpcBuff, const unsigned int uBuffSize, const unsigned int uSendBytes, const timeval *pcTimeout = NULL);
int KG_CheckRecvSocketData(SOCKET nSocket, char * const cpBuff, const unsigned int uBuffSize, unsigned int * const puRecvBytes, const timeval *pcTimeout = NULL);
int KG_SetSocketBlockMode(SOCKET &nSocket, bool bBlocked = true);
int KG_IsSocketEWouldBlock();
int KG_IsSocketInterrupted();
int KG_IsSocketClosedByRemote();
int KG_CloseSocketSafely(SOCKET &nSocket);
int KG_SetSocketRecvBuff(SOCKET nSocket, const unsigned int uRecvBuffSize);
int KG_SetSocketSendBuff(SOCKET nSocket, const unsigned int uSendBuffSize);
int KG_SetSocketSendTimeOut(SOCKET &nSocket, const timeval &tv);
int KG_SetSocketRecvTimeOut(SOCKET &nSocket, const timeval &tv);

KG_NAMESPACE_END

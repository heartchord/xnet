#include "xpublic.h"

KG_NAMESPACE_BEGIN(xnet)

SOCKET KG_CreateTcpSocket();
SOCKET KG_CreateUdpSocket();
SOCKET KG_CreateListenSocket(const char *pszIpAddr, WORD nPort, int nBackLog = KG_DEFAULT_ACCEPT_BACK_LOG);

int  KG_CheckSocketSend    (SOCKET nSocket, const timeval *pcTimeOut = NULL);
int  KG_CheckSocketRecv    (SOCKET nSocket, const timeval *pcTimeOut = NULL);
int  KG_CheckSocketSendEx  (SOCKET nSocket, const timeval *pcTimeOut = NULL);
int  KG_CheckSocketRecvEx  (SOCKET nSocket, const timeval *pcTimeOut = NULL);
int  KG_CheckSendSocketData(SOCKET nSocket, const char * pcBuff, UINT32 uBuffSize, UINT32 uSendBytes,   const timeval *pcTimeout = NULL);
int  KG_CheckRecvSocketData(SOCKET nSocket, char * const cpBuff, UINT32 uBuffSize, UINT32 *puRecvBytes, const timeval *pcTimeout = NULL);

bool KG_IsSocketEWouldBlock();
bool KG_IsSocketInterrupted();
bool KG_IsSocketClosedByRemote();

int  KG_GetSocketErrCode();
bool KG_CloseSocketSafely(SOCKET &nSocket);

bool KG_SetSocketRecvBuff   (SOCKET nSocket, UINT32 uRecvBuffSize);
bool KG_SetSocketSendBuff   (SOCKET nSocket, UINT32 uSendBuffSize);
bool KG_SetSocketBlockMode  (SOCKET &nSocket, bool bBlocked = true);
bool KG_SetSocketSendTimeOut(SOCKET &nSocket, const timeval &tv);
bool KG_SetSocketRecvTimeOut(SOCKET &nSocket, const timeval &tv);

KG_NAMESPACE_END

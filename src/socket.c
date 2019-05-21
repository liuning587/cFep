/**
 ******************************************************************************
 * @file      socket.c
 * @brief     C Source file of socket.c.
 * @details   This file including all API functions's 
 *            implement of socket.c.	
 *
 * @copyright 
 ******************************************************************************
 */
 
/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
#ifdef _WIN32
  #define _WIN32_WINNT 0x501
  #define _CRT_SECURE_NO_WARNINGS
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <windows.h>
#else
//  #define _POSIX_C_SOURCE 200809L
  #ifdef __APPLE__
    #define _DARWIN_UNLIMITED_SELECT
  #endif
  #include <unistd.h>
  #include <netdb.h>
  #include <fcntl.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/time.h>
  #include <net/if.h>
  #include <netinet/in.h>
  #include <netinet/tcp.h>
  #include <arpa/inet.h>
  #include <sys/ioctl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>
#include <string.h>

#include "socket.h"

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
enum
{
    E_SOCKET_FD = 0x12,
    E_SOCKET_ADDR = 0x34,
} socket_fd_type_e;

typedef struct
{
    char type;  //socket_type_e
    int socket;
    socket_info_t info;
} new_socket_t;

/*-----------------------------------------------------------------------------
 Section: Constant Definitions
 ----------------------------------------------------------------------------*/
#ifdef _WIN32
  #define close(a) closesocket((SOCKET)a)
  #define getsockopt(a,b,c,d,e) getsockopt((a),(b),(c),(char*)(d),(e))
  #define setsockopt(a,b,c,d,e) setsockopt((a),(b),(c),(char*)(d),(e))

  #undef  errno
  #define errno WSAGetLastError()

  #undef  EWOULDBLOCK
  #define EWOULDBLOCK WSAEWOULDBLOCK

//  const char *inet_ntop(int af, const void *src, char *dst, socklen_t size) {
//    union { struct sockaddr sa; struct sockaddr_in sai;
//            struct sockaddr_in6 sai6; } addr;
//    int res;
//    memset(&addr, 0, sizeof(addr));
//    addr.sa.sa_family = af;
//    if (af == AF_INET6) {
//      memcpy(&addr.sai6.sin6_addr, src, sizeof(addr.sai6.sin6_addr));
//    } else {
//      memcpy(&addr.sai.sin_addr, src, sizeof(addr.sai.sin_addr));
//    }
//    res = WSAAddressToString(&addr.sa, sizeof(addr), 0, dst, (LPDWORD) &size);
//    if (res != 0) return NULL;
//    return dst;
//  }
#endif

#ifndef SOCKET
# define SOCKET	int
#endif

/*-----------------------------------------------------------------------------
 Section: Global Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Variables
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Local Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Global Function Prototypes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Function Definitions
 ----------------------------------------------------------------------------*/
/**
 ******************************************************************************
 * @brief   初始化socket模块
 * @retval   0  : 初始化成功
 * @retval  -1  : 初始化失败
 ******************************************************************************
 */
int
socket_init(void)
{
#ifdef __WIN32
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
    {
        return -1;
    }
#endif
    return 0;
}

/**
 ******************************************************************************
 * @brief      套接字初始化
 * @param[in]  *pHostName   : 主机名
 * @param[in]  port         : 端口号
 * @param[in]  type         : 0,tcp; 1,udp
 * @param[in]  *pdevice     : 网卡名
 *
 * @retval    -1: 初始化失败
 * @retval    >0: 初始化成功
 ******************************************************************************
 */
int
socket_connect(const char *pHostName,
        unsigned short port,
        char type,
        const char *pdevice)
{
#ifdef __WIN32
    SOCKET fd;
    SOCKADDR_IN server_addr;
    WSADATA wsaData;
    int time_out = 1000 * 15; //超时15s
#else
    int fd = -1;
    struct sockaddr_in server_addr;
    struct timeval timeout = {3, 0};
    struct ifreq sif;
#endif
    struct hostent *host;
    new_socket_t *pnew_socket = NULL;

    do
    {
#ifdef __WIN32
        // Initialize Winsock
        if (WSAStartup(MAKEWORD(2,2), &wsaData) != NO_ERROR)
        {
            printf("Error at WSAStartup()\n");
            break;
        }
#endif
        if ((host = gethostbyname(pHostName)) == NULL)
        {
            printf("%s gethostname:%s error\n", pdevice ? pdevice : "", pHostName);
            break;
        }

        if ((fd = socket(AF_INET, (type == E_SOCKET_UDP) ? SOCK_DGRAM : SOCK_STREAM, 0)) == -1)
        {
            printf("%s socket error\n", pdevice ? pdevice : "");
            break;
        }
#ifndef __WIN32
        if (pdevice)
        {
            //绑定本地网卡
            strncpy(sif.ifr_name, pdevice, sizeof(sif.ifr_name));
            if (setsockopt(fd, SOL_SOCKET, SO_BINDTODEVICE, &sif, sizeof(sif)) < 0)
            {
                close(fd);
                printf("%s setsockopt err:%s\n", pdevice, strerror(errno));
                break;
            }
        }
#endif

        memset(&server_addr, 0x00, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr = *((struct in_addr*)host->h_addr);

        //printf("%s try connect %s:%d...\n", pdevice, pHostName, port);
        if (type != E_SOCKET_UDP)
        {
#if 0   //超时连接
            fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
            if (!connect(fd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)))
            {
                close(fd);
                break;
            }

            if (errno != EINPROGRESS)
            {
                close(fd);
                break;
            }
            struct timeval tm = {30, 0};
            fd_set wset, rset;
            FD_ZERO(&wset);
            FD_ZERO(&rset);
            FD_SET(fd, &wset);
            FD_SET(fd, &rset);
            int res = select(fd + 1, &rset, &wset, NULL, &tm);
            if ((1 == res) && (FD_ISSET(fd, &wset)))
            {
                fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) & ~O_NONBLOCK);
            }
            else
            {
                close(fd);
                break;
            }
#else
            if (connect(fd, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr)) < 0)
            {
                close(fd);
                //printf("%s connect %s:%d error:%s\n", pdevice, pHostName,
                //        port, strerror(errno));
                break;
            }
#endif
        }

#ifdef __WIN32
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char*)&time_out, sizeof(int));
        int on = 1;
        setsockopt(fd, 6, TCP_NODELAY, &on, sizeof (on));
        u_long mode = 1;
        ioctlsocket((SOCKET)fd, FIONBIO, &mode);
#else
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval));
        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(struct timeval));
        unsigned int iMode = 1;  //non-blocking mode is enabled.
        ioctl(fd, FIONBIO, &iMode);
#endif

        if ((pnew_socket = malloc(sizeof(new_socket_t))))
        {
            memset(pnew_socket, 0x00, sizeof(new_socket_t));
            pnew_socket->type = (type == E_SOCKET_UDP) ? E_SOCKET_ADDR : E_SOCKET_FD;
            pnew_socket->socket = fd;

            pnew_socket->info.remote_ip = server_addr.sin_addr.s_addr;
            pnew_socket->info.remote_port = server_addr.sin_port;
        }
        else
        {
            close(fd);
            return -1;
        }

        return (int)pnew_socket;
    } while(0);

    return -1;
}

/**
 ******************************************************************************
 * @brief   socket监听
 * @param[in]  port : 监听端口号
 * @param[in]  type : 0,tcp; 1,udp
 *
 * @retval  listen fd
 ******************************************************************************
 */
int
socket_listen(unsigned short port,
        char type)
{
    int listen_fd;
    struct sockaddr_in servaddr;
    int flag = 1, len = sizeof(int);
    new_socket_t *pnew_socket = NULL;

    if (!port)
    {
        return -1;
    }

    listen_fd = socket(AF_INET, (type == E_SOCKET_UDP) ? SOCK_DGRAM : SOCK_STREAM, 0);
    if (listen_fd == -1)
    {
        perror("socket");
        return -1;
    }
#ifdef _WIN32
    u_long mode = 1;
    ioctlsocket((SOCKET)listen_fd, FIONBIO, &mode);

#define SIO_UDP_CONNRESET _WSAIOW(IOC_VENDOR, 12)
	BOOL bNewBehavior = FALSE;
	DWORD dwBytesReturned = 0;
	WSAIoctl((SOCKET)listen_fd, SIO_UDP_CONNRESET, &bNewBehavior, sizeof bNewBehavior, NULL, 0, &dwBytesReturned, NULL, NULL);
#else
    #if 0
    int flags = fcntl(listen_fd, F_GETFL);
    fcntl(listen_fd, F_SETFL,
            1 ? (flags | O_NONBLOCK) : (flags & ~O_NONBLOCK));
    #endif
    unsigned int iMode = 1;  //non-blocking mode is enabled.
    ioctl(listen_fd, FIONBIO, &iMode);
#endif
    memset(&servaddr, 0x00, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    if (bind((SOCKET)listen_fd, (struct sockaddr*) &servaddr, sizeof(servaddr)))
    {
        close(listen_fd);
        //perror("bind");
        return -1;
    }
    if (setsockopt((SOCKET)listen_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&flag, len) == -1)
    {
        perror("setsockopt");
        close(listen_fd);
        return -1;
    }
//    else
//    {
//        printf("bind port:%d OK!\n", port);
//    }
    if (type != E_SOCKET_UDP) //tcp
    {
        if (listen((SOCKET)listen_fd, 511))
        {
            perror("listen");
            close(listen_fd);
            return -1;
        }
    }
    if ((pnew_socket = malloc(sizeof(new_socket_t))))
    {
        pnew_socket->type = E_SOCKET_FD;
        pnew_socket->socket = listen_fd;
    }
    else
    {
        close(listen_fd);
        return -1;
    }

    return (int)pnew_socket;
}

/**
 ******************************************************************************
 * @brief   socket accept
 * @param[in]  listen_fd : 监听
 * @retval     > 0
 * @retval     -1 无
 ******************************************************************************
 */
int
socket_accept(int listen_fd)
{
    int cfd;
    new_socket_t *pnew_socket = NULL;
    new_socket_t *pnew_listen = (new_socket_t *)listen_fd;
    if (!pnew_listen || (pnew_listen->type == E_SOCKET_ADDR))
    {
        return -1;
    }

#ifdef __WIN32
    cfd = accept((SOCKET)pnew_listen->socket, (struct sockaddr*) NULL, NULL);
#else
    cfd = accept((SOCKET)pnew_listen->socket, (struct sockaddr*) NULL, NULL);

    unsigned int iMode = 1;  //non-blocking mode is enabled.

    if (cfd != -1)
    {
        ioctl(cfd, FIONBIO, &iMode);
    }
#endif

    if (cfd != -1)
    {
        if ((pnew_socket = malloc(sizeof(new_socket_t))))
        {
            memset(pnew_socket, 0x00, sizeof(new_socket_t));
            pnew_socket->type = E_SOCKET_FD;
            pnew_socket->socket = cfd;

            struct sockaddr_in sa;
            socklen_t len = sizeof(sa);
            if (!getpeername(cfd, (struct sockaddr *)&sa, &len))
            {
                pnew_socket->info.remote_ip = sa.sin_addr.s_addr;
                pnew_socket->info.remote_port = sa.sin_port;
            }
        }
        else
        {
            close(cfd);
            return -1;
        }

        return (int)pnew_socket;
    }
    return -1;
}

/**
 ******************************************************************************
 * @brief      套接字发送数据
 * @param[in]  socket   : 套接字句柄
 * @param[in]  *pbuf    : 发送数据首地址
 * @param[in]  size     : 发送数据长度
 *
 * @retval     -1   : 发送失败
 * @retval     size : 发送成功
 ******************************************************************************
 */
int
socket_send(int socket,
        const unsigned char *pbuf,
        int size)
{
    int len;
    new_socket_t *pnew_socket = (new_socket_t *)socket;

    if (pnew_socket->type == E_SOCKET_ADDR)
    {
        struct sockaddr_in remote = {0};
        remote.sin_family = SOCK_DGRAM;
        remote.sin_addr.S_un.S_addr = pnew_socket->info.remote_ip;
        remote.sin_port = pnew_socket->info.remote_port;
        len = sendto((SOCKET)pnew_socket->socket, (char *)pbuf, size, 0, (const struct sockaddr*)&remote, sizeof(remote));
    }
    else
    {
        len = send((SOCKET)pnew_socket->socket, (char *)pbuf, size, 0);
    }

    if (len <= 0)
    {
        if (errno == EWOULDBLOCK)
        {
            /* No more data can be written */
            return 0;
        }
        else
        {
            /* Handle disconnect */
            return -1;
        }
    }
    return len;
}

/**
 ******************************************************************************
 * @brief      套接字接收数据
 * @param[in]  socket   : 套接字句柄
 * @param[in]  *pbuf    : 接收数据首地址
 * @param[in]  size     : 希望接收数据长度
 *
 * @retval     -1   : 接收失败
 * @retval     size : 接收成功
 ******************************************************************************
 */
int
socket_recv(int socket,
        unsigned char *pbuf,
        int size)
{
    int err;
    int len;
    new_socket_t *pnew_socket = (new_socket_t *)socket;

#if 0 //UDP也通过recv函数读取数据
    if (pnew_socket->type == E_SOCKET_ADDR)
    {
        return 0;
    }
#endif
    len = recv((SOCKET)pnew_socket->socket, (char *)pbuf, size, 0);

    if (len <= 0)
    {
        err = errno;
        if (len == 0 || (!((err == EINTR || err == EWOULDBLOCK || err == EAGAIN))))
        {
            //printf("recv len:%d err:%d\n", len, err);
            return -1;
        }
        else
        {
            /* No more data */
            return 0;
        }
    }
    return len;
}

/**
 ******************************************************************************
 * @brief      套接字接收数据
 * @param[in]  socket   : 套接字句柄
 * @param[in]  *pbuf    : 接收数据首地址
 * @param[in]  size     : 希望接收数据长度
 * @param[out] *ip      : 对方IP
 * @param[out] *port    : 对方端口
 *
 * @retval     -1   : 接收失败
 * @retval     size : 接收成功
 ******************************************************************************
 */
int
socket_recvfrom(int socket,
        unsigned char *pbuf,
        int size,
        int *ip,
        unsigned short *port)
{
    struct sockaddr_in clt;
    socklen_t l = sizeof(struct sockaddr_in);
    int err;
    new_socket_t *pnew_socket = (new_socket_t *)socket;

    if (pnew_socket->type == E_SOCKET_ADDR)
    {
        return 0;
    }

    int len = recvfrom((SOCKET)pnew_socket->socket, (char *)pbuf, size, 0, (struct sockaddr *)&clt, &l);
    if (len <= 0)
    {
        err = errno;
        if (len == 0 || (!((err == EINTR || err == EWOULDBLOCK || err == EAGAIN || err == 10054)))) //WSAECONNRESET:10054
        {
            //printf("recvfrom len:%d err:%d\n", len, err);
            return -1;
        }
        else
        {
            /* No more data */
            return 0;
        }
    }

    if (ip) *ip = (int)clt.sin_addr.S_un.S_addr;
    if (port) *port = clt.sin_port;

    return len;
}

/**
 ******************************************************************************
 * @brief      套接字关闭
 * @param[in]  socket   : 套接字句柄
 *
 * @return  None
 ******************************************************************************
 */
void
socket_close(int socket)
{
    new_socket_t *pnew_socket = (new_socket_t *)socket;

//    if (pnew_socket->type != E_SOCKET_ADDR) //UDP也需要关闭
    {
        if (pnew_socket->socket > 0)
        {
#ifdef __WIN32
            closesocket((SOCKET) pnew_socket->socket);
            //WSACleanup(); //needed in winsock
#else
            close(pnew_socket->socket);
            pnew_socket->socket = -1;
#endif
        }
    }

    free(pnew_socket);
}

/**
 ******************************************************************************
 * @brief   获取套接字ip,端口
 * @param[in]  None
 * @param[out] None
 * @retval     None
 ******************************************************************************
 */
int
socket_get_ip_port(int sockfd)
{
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    new_socket_t *pnew_socket = (new_socket_t *)sockfd;

    if (pnew_socket->type == E_SOCKET_ADDR)
    {
        printf("对方IP：%d.%d.%d.%d",
                pnew_socket->info.ip[0],
                pnew_socket->info.ip[1],
                pnew_socket->info.ip[2],
                pnew_socket->info.ip[3]);
        printf("对方PORT：%d ", ntohs(pnew_socket->info.remote_port));
        return 0;
    }
    else
    {
        if (!getpeername((SOCKET)pnew_socket->socket, (struct sockaddr *) &sa, &len))
        {
            printf("对方IP：%s ", inet_ntoa(sa.sin_addr));
            printf("对方PORT：%d ", ntohs(sa.sin_port));
            return 0;
        }
    }
    return -1;
}

/**
 ******************************************************************************
 * @brief   获取套接字ip,端口字符串
 * @param[in]  None
 * @param[out] None
 * @retval     None
 ******************************************************************************
 */
const char *
socket_get_ip_port_str(int sockfd)
{
    static char str[16+6];
    struct sockaddr_in sa;
    socklen_t len = sizeof(sa);
    new_socket_t *pnew_socket = (new_socket_t *)sockfd;

    if (pnew_socket->type == E_SOCKET_ADDR)
    {
        sprintf(str, "%d.%d.%d.%d:%d", pnew_socket->info.ip[0],
                pnew_socket->info.ip[1],
                pnew_socket->info.ip[2],
                pnew_socket->info.ip[3],
                ntohs(pnew_socket->info.remote_port));
    }
    else
    {
        if (!getpeername((SOCKET)pnew_socket->socket, (struct sockaddr *)&sa, &len))
        {
            sprintf(str, "%s:%d", inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));
        }
        else
        {
            strcpy(str, "NULL");
        }
    }
    return str;
}

/**
 ******************************************************************************
 * @brief   获取socket信息
 * @param[in]  sockfd : socket句柄
 *
 * @retval  信息
 ******************************************************************************
 */
const socket_info_t *
socket_info_get(int sockfd)
{
    new_socket_t *pnew_socket = (new_socket_t *)sockfd;

    return &pnew_socket->info;
}

/**
 ******************************************************************************
 * @brief   设置socket info(UDP，才能设置)
 * @param[in]  ip   :
 * @param[in]  port :
 *
 * @retval  socketfd
 ******************************************************************************
 */
int
socket_info_set(int socket,
        int ip,
        unsigned short port)
{
    new_socket_t *pnew_socket;

    if ((pnew_socket = malloc(sizeof(new_socket_t))))
    {
        pnew_socket->type = E_SOCKET_ADDR;
        pnew_socket->socket = ((new_socket_t *)socket)->socket;
        pnew_socket->info.remote_ip = ip;
        pnew_socket->info.remote_port = port;
    }
    return (int)pnew_socket;
}

/**
 ******************************************************************************
 * @brief   获取socket type
 * @param[in]  sockfd : socket句柄
 *
 * @retval  socket type
 ******************************************************************************
 */
int
socket_type(int socketfd)
{
    new_socket_t *pnew_socket = (new_socket_t *)socketfd;

    if (pnew_socket->type == E_SOCKET_ADDR)
    {
        return E_SOCKET_UDP;
    }
    return E_SOCKET_TCP;
}

void
socket_exit(void)
{
#ifdef __WIN32
    WSACleanup();
#else
#endif
}

void
socket_msleep(int ms)
{
#ifdef __WIN32
    Sleep(ms);
#else
    usleep(ms * 1000);
#endif
}

/*--------------------------------socket.c-----------------------------------*/

/**
 ******************************************************************************
 * @file       socket.h
 * @brief      API include file of socket.h.
 * @details    This file including all API functions's declare of socket.h.
 * @copyright  
 *
 ******************************************************************************
 */
#ifndef SOCKET_H_
#define SOCKET_H_ 

/*-----------------------------------------------------------------------------
 Section: Includes
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Macro Definitions
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Type Definitions
 ----------------------------------------------------------------------------*/
typedef enum
{
    E_SOCKET_TCP = 0,
    E_SOCKET_UDP = 1,
} socket_type_e;

typedef struct
{
    union
    {
        int remote_ip;
        struct
        {
            unsigned char ip[4];
        };
    };
    unsigned short remote_port;
} socket_info_t;

/*-----------------------------------------------------------------------------
 Section: Globals
 ----------------------------------------------------------------------------*/
/* NONE */

/*-----------------------------------------------------------------------------
 Section: Function Prototypes
 ----------------------------------------------------------------------------*/
extern int
socket_init(void);

extern int
socket_connect(const char *pHostName,
        unsigned short port,
        char type,
        const char *pdevice);

extern int
socket_listen(unsigned short port,
        char type);

extern int
socket_accept(int listen_fd);

extern int
socket_send(int socket,
        const unsigned char *pbuf,
        int size);

extern int
socket_recv(int socket,
        unsigned char *pbuf,
        int size);

extern int
socket_recvfrom(int socket,
        unsigned char *pbuf,
        int size,
        int *ip,
        unsigned short *port);

extern void
socket_close(int socket);

extern void
socket_exit(void);

extern const char *
socket_get_ip_port_str(int sockfd);

extern const socket_info_t *
socket_info_get(int sockfd);

extern int
socket_info_set(int socket,
        int ip,
        unsigned short port);

extern int
socket_type(int socketfd);

extern void
socket_msleep(int ms);

#endif /* SOCKET_H_ */
/*--------------------------End of socket.h-----------------------------*/

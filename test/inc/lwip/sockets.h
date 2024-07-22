#ifndef _SOCKETS_H_
#define _SOCKETS_H_

#include <stdint.h>
#include <unistd.h>     //size_t, ssize_t

#define AF_INET         1

#define IPPROTO_IP      0
#define IPPROTO_ICMP    1
#define IPPROTO_TCP     6
#define IPPROTO_UDP     17

/* Socket protocol types (TCP/UDP/RAW) */
#define SOCK_STREAM     1
#define SOCK_DGRAM      2
#define SOCK_RAW        3

typedef uint32_t socklen_t;
typedef uint8_t  sa_family_t;

struct in_addr {
  uint32_t s_addr;
};

struct sockaddr_in {
  uint8_t         sin_len;
  uint8_t         sin_family;
  uint16_t        sin_port;
  struct in_addr  sin_addr;
#define SIN_ZERO_LEN 8
  char            sin_zero[SIN_ZERO_LEN];
};

struct sockaddr {
  uint8_t        sa_len;
  sa_family_t sa_family;
  char        sa_data[14];
};


static inline int socket(int domain,int type,int protocol){
    (void)domain;
    (void)type;
    (void)protocol;
    return 0;
}

static inline int bind(int s,const struct sockaddr *name, socklen_t namelen){
    (void)s;
    (void)name;
    (void)namelen;
    return 0;
}

static inline ssize_t recvfrom(int s,void *mem,size_t len,int flags,struct sockaddr *from,socklen_t *fromlen){
    (void)s;
    (void)mem;
    (void)len;
    (void)flags;
    (void)from;
    (void)fromlen;
    return 0;
}

static inline ssize_t sendto(int s,const void *dataptr,size_t size,int flags,const struct sockaddr *to,socklen_t tolen){
    (void)s;
    (void)dataptr;
    (void)size;
    (void)flags;
    (void)to;
    (void)tolen;
    return 0;
}

static inline int shutdown(int s,int how){
    (void)s;
    (void)how;
    return 0;
}

#endif // _SOCKETS_H_


#ifndef _IP4_ADDR_H_
#define _IP4_ADDR_H_

#include <stdint.h>


struct ip6_addr {
  uint32_t addr[4];
};
typedef struct ip6_addr ip6_addr_t;

/** This is the aligned version of ip4_addr_t,
   used as local variable, on the stack, etc. */
struct ip4_addr {
  uint32_t addr;
};
/** ip4_addr_t uses a struct for convenience only, so that the same defines can
 * operate both on ip4_addr_t as well as on ip4_addr_p_t. */
typedef struct ip4_addr ip4_addr_t;

typedef struct _ip_addr {
  union {
    ip6_addr_t ip6;
    ip4_addr_t ip4;
  } u_addr;
  uint8_t type;
} ip_addr_t;


#endif // _IP4_ADDR_H_


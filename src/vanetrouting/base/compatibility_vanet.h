#ifndef _VANET_COMPATIBILITY_H
#define _VANET_COMPATIBILITY_H

//#ifdef _WIN32
//#include <unistd.h>
//#include <sys/time.h>
//#include <sys/types.h>
//#endif
#include <omnetpp.h>
#include "VanetAddress.h"

#ifndef u_int8_t
typedef uint8_t u_int8_t;
#endif

#ifndef u_int16_t
typedef uint16_t u_int16_t;
#endif

#ifndef u_int32_t
typedef uint32_t u_int32_t;
#endif

#ifndef u_short
typedef unsigned short u_short;
#endif

#ifndef u_char
typedef unsigned char u_char;
#endif

#define ETH_ALEN    6       /* Octets in one ethernet addr   */

#ifndef in_addr_t
typedef uint32_t in_addr_t;
#endif

typedef unsigned short  sa_family_t;
#define MAXTTL      255

//struct in_addr
//  {
//    in_addr_t s_addr;
//  };

#ifndef in_addr_t
typedef uint32_t in_addr_t;
#endif

//#ifdef _WIN32
struct Vanet_In_addr
{
    VanetAddress S_addr;
#undef s_addr
#define s_addr S_addr
    Vanet_In_addr(const Vanet_In_addr & addr) {S_addr = addr.S_addr;}
    Vanet_In_addr() {S_addr = VanetAddress();}
    Vanet_In_addr & operator=(const Vanet_In_addr &other) {if (this==&other) return *this; S_addr = other.S_addr; return *this;}
};

struct Vanet_Sockaddr
{
    sa_family_t sa_family;  /* address family, AF_xxx   */
    char        sa_data[14];    /* 14 bytes of protocol address */
};

#undef in_addr
#undef sockaddr

#define in_addr Vanet_In_addr
#define sockaddr Vanet_Sockaddr

#endif              /* _DSR_H */

#ifndef DNS2TCP_NETUTILS_H
#define DNS2TCP_NETUTILS_H

#define _GNU_SOURCE
#include <stdint.h>
#include <netinet/in.h>
#undef _GNU_SOURCE

/* mtu(1500) - iphdr(20) - udphdr(8) */
#define UDP_PACKET_MAXSIZE 1472 /* bytes */

#define IP4STRLEN INET_ADDRSTRLEN
#define IP6STRLEN INET6_ADDRSTRLEN

#define PORTSTRLEN 6

typedef uint16_t portno_t;

typedef struct sockaddr     skaddr_t;
typedef struct sockaddr_in  skaddr4_t;
typedef struct sockaddr_in6 skaddr6_t;

/* AF_INET or AF_INET6 or -1(invalid) */
int get_ipstr_addrfamily(const char *ipstr);

/* before calling, please memset(skaddr) to 0 */
void build_skaddr4_from_ipport(skaddr4_t *skaddr, const char *ipstr, portno_t portno);
void build_skaddr6_from_ipport(skaddr6_t *skaddr, const char *ipstr, portno_t portno);

void parse_ipport_from_skaddr4(char *ipstr, portno_t *portno, const skaddr4_t *skaddr);
void parse_ipport_from_skaddr6(char *ipstr, portno_t *portno, const skaddr6_t *skaddr);

void make_socket_non_blocking(int sockfd);
void set_socketopt_ipv6_v6only(int sockfd);
void set_socketopt_tcp_nodelay(int sockfd);

#endif

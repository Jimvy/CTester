/*
 * Wrapper for getaddrinfo, freeaddrinfo, gai_strerror and getnameinfo
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __WRAP_NETWORK_DNS_H_
#define __WRAP_NETWORK_DNS_H_

#include <netdb.h>
#include <stdbool.h>

// getaddrinfo parameters structures and statistics structure

struct params_getaddrinfo_t {
    const char *node;
    const char *service;
    const struct addrinfo *hints;
    struct addrinfo **res;
};

struct addrinfo_node_t;

struct stats_getaddrinfo_t {
    int called; // number of times the getaddrinfo call has been issued
    struct params_getaddrinfo_t last_params; // parameters for the last monitored call
    struct addrinfo_node_t *addrinfo_list; // list of addrinfo lists "returned" by getaddrinfo
    int last_return; // return value of the last monitored call issued
};

// getnameinfo parameters structures and statistics structure

struct params_getnameinfo_t {
    const struct sockaddr *addr;
    socklen_t addrlen;
    char *host;
    socklen_t hostlen;
    char *serv;
    socklen_t servlen;
    int flags;
};

struct stats_getnameinfo_t {
    int called; // number of calls
    struct params_getnameinfo_t last_params; // parameters of the last monitored call
    int last_return; // return value of the last monitored call issued
};

// freeaddrinfo parameters structures and statistics structure

struct stats_freeaddrinfo_t {
    int called; // number of calls
    struct addrinfo *last_param; // parameter of the last monitored call issued
    /** status of the last monitored call:
     * if check_freeaddrinfo is true
     *     and last_param was not returned by getaddrinfo during the monitoring,
     *     then status is 1;
     * else, status is zero.
     */
    int status; 
};

// gai_strerror parameters structures and statistics structure

struct stats_gai_strerror_t {
    int called; // number of calls
    int last_params; // parameter of the last monitored call
    const char *last_return; // return value of the last monitored call
};

// TODO Maybe add some init, clean and resetstats methods to these calls ?

/**
 * When check is true, freeaddrinfo will check if its argument has been "returned" by an earlier call to getaddrinfo directly, and will report if it is not the case.
 */
void set_check_freeaddrinfo(bool check);


/**
 * Functions of this type should return in *res a valid list of struct addrinfo,
 * suitable to be freed by freeaddrinfo
 */
typedef int (*getaddrinfo_method_t)(const char *node, const char *service,
        const struct addrinfo *hints, struct addrinfo **res);

/**
 * if method is not NULL, replaces __real_getaddrinfo by the function "method"
 * as the real function executed by the wrapper.
 * If method is NULL, resets the method to __real_getaddrinfo.
 */
void set_getaddrinfo_method(getaddrinfo_method_t method);

/**
 * Example replacement function for getaddrinfo.
 * This function will call getaddrinfo, and will then remove from res
 * all the entries that fail when using socket or connect.
 */
int filtered_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);


typedef int (*getnameinfo_method_t)(const struct sockaddr *addr,
        socklen_t addrlen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);

/**
 * If method is not NULL, replaces __real_getnameinfo by the function "method"
 * as the real function executed by the wrapper.
 * If method is NULL, resets the method to __real_getnameinfo.
 */
void set_getnameinfo_method(getnameinfo_method_t method);


typedef const char *(*gai_strerror_method_t)(int);

/**
 * If method is not NULL, replaces __real_gai_strerror by the function "method"
 * as the real function called by the wrapper.
 * If method is NULL, resets the method to __real_gai_strerror.
 * This allows specifying other error messages than the standard ones.
 */
void set_gai_strerror_method(gai_strerror_method_t method);


typedef void (*freeaddrinfo_badarg_report_t)();

/**
 * Sets the method to be called if check_freeaddrinfo is true
 * and the argument passed to freeaddrinfo was not returned by getaddrinfo.
 * This is useful if you want to use the push_info_msg method
 * for your feedback, for example. Default is NULL.
 */
void set_freeaddrinfo_badarg_report(freeaddrinfo_badarg_report_t reporter);

#endif // __WRAP_NETWORK_DNS_H_


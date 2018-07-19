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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include "wrap.h"

int __real_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);
int __real_getnameinfo(const struct sockaddr *addr, socklen_t addrlen, char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags);
void __real_freeaddrinfo(struct addrinfo *res);
const char *__real_gai_strerror(int errcode);

extern bool wrap_monitoring;
extern struct wrap_stats_t stats;
extern struct wrap_monitor_t monitored;
extern struct wrap_fail_t failures;

bool check_freeaddrinfo = false;
getaddrinfo_method_t getaddrinfo_method = NULL;
getnameinfo_method_t getnameinfo_method = NULL;
gai_strerror_method_t gai_strerror_method = NULL;
freeaddrinfo_badarg_report_t freeaddrinfo_badarg_reporter = NULL;

// Used to record the addrinfo lists "returned" by getaddrinfo, in order to check for their deallocation via freeaddrinfo.
// TODO maybe it would be better placed in the .c file ?
struct addrinfo_node_t {
    struct addrinfo *addr_list;
    struct addrinfo_node_t *next;
};

void add_result(struct addrinfo *res)
{
    struct addrinfo_node_t *new_node = malloc(sizeof(*new_node));
    if (new_node == NULL) return;
    new_node->addr_list = res;
    new_node->next = stats.getaddrinfo.addrinfo_list;
    stats.getaddrinfo.addrinfo_list = new_node;
}

/**
 * Returns 0 if res was successfully removed of the list,
 * -1 if res was not present in the list,
 *  -2 if there was some other errors.
 */
int remove_result(struct addrinfo *res)
{
    if (stats.getaddrinfo.addrinfo_list != NULL) {
        struct addrinfo_node_t *runner = stats.getaddrinfo.addrinfo_list;
        if (runner->addr_list == res) {
            stats.getaddrinfo.addrinfo_list = runner->next;
            free(runner);
            // runner->addr_list will be freed by freeaddrinfo
            return 0;
        }
        for (; runner->next != NULL; runner = runner->next) {
            if (runner->next->addr_list == res) {
                struct addrinfo_node_t *old = runner->next;
                runner->next = runner->next->next;
                free(old);
                // old->addr_list will be freed by freeaddrinfo when we return
                return 0;
            }
        }
        return -1;
    } else {
        return -1;
    }
}


int __wrap_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    if (! (wrap_monitoring && monitored.getaddrinfo)) {
        return __real_getaddrinfo(node, service, hints, res);
    }
    stats.getaddrinfo.called++;
    stats.getaddrinfo.last_params = (struct params_getaddrinfo_t) {
        .node = node,
        .service = service,
        .hints = hints,
        .res = res
    };
    if (FAIL(failures.getaddrinfo)) {
        failures.getaddrinfo = NEXT(failures.getaddrinfo);
        errno = failures.getaddrinfo_errno;
        stats.getaddrinfo.last_return = failures.getaddrinfo_ret;
        return failures.getaddrinfo_ret;
    }
    failures.getaddrinfo = NEXT(failures.getaddrinfo);
    int ret;
    if (getaddrinfo_method == NULL) {
        ret = __real_getaddrinfo(node, service, hints, res);
    } else {
        ret = (*getaddrinfo_method)(node, service, hints, res);
    }
    stats.getaddrinfo.last_return = ret;
    /*
     * At this point, we may assume that *res is a valid memory location,
     * as the program will segfault if it is not.
     */
    if (check_freeaddrinfo) {
        add_result(*res);
    }
    return ret;
}

int __wrap_getnameinfo(const struct sockaddr *addr, socklen_t addrlen,
        char *host, socklen_t hostlen, char *serv, socklen_t servlen, int flags)
{
    if (! (wrap_monitoring && monitored.getnameinfo)) {
        return __real_getnameinfo(addr, addrlen, host, hostlen, serv, servlen, flags);
    }
    stats.getnameinfo.called++;
    stats.getnameinfo.last_params = (struct params_getnameinfo_t) {
        .addr = addr,
        .addrlen = addrlen,
        .host = host,
        .hostlen = hostlen,
        .serv = serv,
        .servlen = servlen,
        .flags = flags
    };
    if (FAIL(failures.getnameinfo)) {
        failures.getnameinfo = NEXT(failures.getnameinfo);
        errno = failures.getnameinfo_errno;
        stats.getnameinfo.last_return = failures.getnameinfo_ret;
        return failures.getnameinfo_ret;
    }
    failures.getnameinfo = NEXT(failures.getnameinfo);
    getnameinfo_method_t met = &__real_getnameinfo;
    if (getnameinfo_method != NULL) {
        met = getnameinfo_method;
    }
    int ret = (*met)(addr, addrlen, host, hostlen, serv, servlen, flags);
    stats.getnameinfo.last_return = ret;
    return ret;
}

/**
 * The student should call freeaddrinfo on all the result structures that have been initialized by getaddrinfo; failing to do so leads to memory leak.
 * If the parameter check_freeaddrinfo is true, then this function will check if res was actually returned by getaddrinfo, and is not either an invalid pointer, or a non-first addrinfo in one of the lists.
 */
void __wrap_freeaddrinfo(struct addrinfo *res)
{
    if (! (wrap_monitoring && monitored.freeaddrinfo)) {
        __real_freeaddrinfo(res);
        return;
    }
    stats.freeaddrinfo.called++;
    stats.freeaddrinfo.last_param = res;
    if (check_freeaddrinfo) {
        if (remove_result(res) == 0) {
            // Okay, it is valid
            stats.freeaddrinfo.status = 0;
        } else {
            // Not okay: it is invalid (e.g. this is the second item of the list instead of the first).
            // TODO report the error, as it is still invalid
            stats.freeaddrinfo.status = 1;
            if (freeaddrinfo_badarg_reporter != NULL) {
                (*freeaddrinfo_badarg_reporter)();
            }
        }
    } else {
        stats.freeaddrinfo.status = 0; // will not check
    }
    __real_freeaddrinfo(res);
}

const char * __wrap_gai_strerror(int ecode)
{
    if (! (wrap_monitoring && monitored.getaddrinfo)) {
        return __real_gai_strerror(ecode);
    }
    stats.gai_strerror.called++;
    stats.gai_strerror.last_params = ecode;
    gai_strerror_method_t met = (gai_strerror_method == NULL ? &__real_gai_strerror : gai_strerror_method);
    const char *ret = (*met)(ecode);
    stats.gai_strerror.last_return = ret;
    return ret;
}


bool check_validity(struct addrinfo *a, bool bindable)
{
    int sfd = socket(a->ai_family, a->ai_socktype, a->ai_protocol);
    if (sfd == -1)
        return 1;
    int rep = 0;
    if (bindable) {
        // Only case where bind(2) is the right syscall to call
        rep = bind(sfd, a->ai_addr, a->ai_addrlen);
    } else {
        rep = connect(sfd, a->ai_addr, a->ai_addrlen);
    }
    if (rep == -1) {
        close(sfd);
        return 1;
    }
    close(sfd); // No actual need to check its return value, as we have written nothing to it.
    return 0;
}

int filtered_getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res)
{
    struct addrinfo *real_res;
    int rep = getaddrinfo(node, service, hints, &real_res);
    if (rep != 0) {
        *res = real_res;
        return rep;
    }
    bool bindable = (node == NULL) && ((hints->ai_flags & AI_PASSIVE) != 0);
    struct addrinfo *real_p = real_res;
    char *canonname_cpy = NULL;
    if (real_res != NULL && real_res->ai_canonname != NULL) {
        canonname_cpy = strdup(real_res->ai_canonname);
        if (canonname_cpy == NULL) {
            *res = real_res;
            return EAI_MEMORY;
        }
    }
    while (real_p != NULL && real_p->ai_next != NULL) {
        // Test if admissible
        if (check_validity(real_p->ai_next, bindable) == 0) {
            // Admissible
            real_p = real_p->ai_next;
            continue;
        }
        // Not admissible
        struct addrinfo *old = real_p->ai_next;
        real_p->ai_next = real_p->ai_next->ai_next;
        old->ai_next = NULL;
        freeaddrinfo(old); // Let glibc handle his queetzlies for us.

        /*
        struct addrinfo *p = (struct addrinfo*) malloc(sizeof(*p)
                + ((real_p->ai_family == AF_INET)
                    ? sizeof(struct sockaddr_in)
                    : sizeof(struct sockaddr_in6))
                );
        if (p == NULL) {
            *res = real_res;
            goto free_and_return;
        }
        p->ai_flags = real_p->ai_flags;
        p->ai_family = real_p->ai_family;
        p->ai_socktype = real_p->ai_socktype;
        p->ai_protocol = real_p->ai_protocol;
        p->ai_addrlen = real_p->ai_addrlen;
        if (real_p->ai_canonname != NULL) {
            p->ai_canonname = strdup(real_p->ai_canonname);
            if (p->ai_canonname == NULL) {
                *res = real_res;
                goto free_and_return;
            }
        }
        p->ai_addr = (void*) (p + 1);
        p->ai_addr->sa_family = real_p->ai_addr->sa_family;
        p->ai_addr->sa_data = //
        if (rp == NULL) {
            rp = p;
            *res = p;
        } else {
            rp->ai_next = p;
            rp = p;
        }
        */
    }
    // Check if head is valid
    if (real_res != NULL && check_validity(real_res, bindable) == 1) {
        *res = real_res->ai_next;
    } else { // real_res == NULL or real_res is admissible
        *res = real_res;
    }
    if ((*res)->ai_canonname == NULL && canonname_cpy != NULL) {
        (*res)->ai_canonname = canonname_cpy;
    } else {
        free(canonname_cpy);
    }
    return 0;

    /*
free_and_return:
    for (struct addrinfo *p = *res; p != NULL;) {
        struct addrinfo *old = p;
        p = p->ai_next;
        free(old);
    }
    return EAI_MEMORY;
    */
}

void set_getaddrinfo_method(getaddrinfo_method_t method)
{
    getaddrinfo_method = method;
}

void set_getnameinfo_method(getnameinfo_method_t method)
{
    getnameinfo_method = method;
}

void set_gai_strerror_method(gai_strerror_method_t method)
{
    gai_strerror_method = method;
}


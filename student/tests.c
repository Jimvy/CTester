// CTester template

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "student_code.h"
#include <arpa/inet.h>
#include "CTester/CTester.h"

int dummy_getaddrinfo(const char *node, const char *serv, const struct addrinfo *hints, struct addrinfo **res)
{
    // deactivate the monitoring before calling the true getaddrinfo
    bool old = monitored.getaddrinfo;
    monitored.getaddrinfo = false;
    int rep = getaddrinfo(node, serv, hints, res);
    monitored.getaddrinfo = old;
    return rep;
}

const char *__real_gai_strerror(int errcode);

const char *my_gai_strerror(int errcode)
{
#define A(X) case X: \
    return #X; \
    break

    switch (errcode) {
        A(EAI_AGAIN);
        A(EAI_BADFLAGS);
        A(EAI_FAIL);
        A(EAI_FAMILY);
        A(EAI_MEMORY);
        A(EAI_NONAME);
        A(EAI_SERVICE);
        A(EAI_SOCKTYPE);
        A(EAI_SYSTEM);
        default:
        return __real_gai_strerror(errcode);
    };
}

void test_print_address1() {
    set_test_metadata("print_address", _("Coucou"), 1);
    int ret1 = 3, ret2 = 3, ret3 = 3, ret4 = 3, ret5 = 3, ret6 = 3, ret7 = 3, ret8 = 3, ret9 = 3, ret10 = 3;
    monitored.getaddrinfo = true;
    monitored.freeaddrinfo = true;
    monitored.malloc = true;
    monitored.free = true;
    set_getaddrinfo_method(dummy_getaddrinfo);
    set_gai_strerror_method(my_gai_strerror);
    //FILE *my_stdout = fdopen(stdout_cpy, "r");
    //FILE *my_stderr = fdopen(stderr_cpy, "r");

    SANDBOX_BEGIN;

    ret1 = print_address("google.be", "http");
    ret2 = print_address("uclouvain.be", "tcp"); // Error 1: socktype
    ret3 = print_address("uclouvain.be", NULL);
    ret4 = print_address(NULL, "ssh");
    ret5 = print_address("127.0.0.1", "http");
    ret6 = print_address("::1", "http");
    ret7 = print_address("example.net", "22"); // Error 2: service
    ret8 = print_address("nonexistentwebsitename.lol", "http"); // Error 3: no name

    SANDBOX_END;

    CU_ASSERT_EQUAL(ret1, 0);
    CU_ASSERT_EQUAL(ret2, EAI_SERVICE);
    CU_ASSERT_EQUAL(ret3, 0);
    CU_ASSERT_EQUAL(ret4, 0);
    CU_ASSERT_EQUAL(ret5, 0);
    CU_ASSERT_EQUAL(ret6, 0);
    CU_ASSERT_EQUAL(ret7, 0);
    CU_ASSERT_EQUAL(ret8, EAI_NONAME);
    CU_ASSERT_EQUAL(ret9, 3);
    CU_ASSERT_EQUAL(ret10, 3);
    //bool haserror = false;
    set_getaddrinfo_method(NULL);
    set_gai_strerror_method(NULL);

}

void test_print_address2() {
    set_test_metadata("print_address", _("Kikoo"), 1);
    int ret1 = 3, ret2 = 3, ret3 = 3, ret4 = 3, ret5 = 3;
    monitored.getaddrinfo = true;
    monitored.freeaddrinfo = true;
    set_gai_methods(simple_getaddrinfo, simple_freeaddrinfo);
    SANDBOX_BEGIN;
    ret1 = print_address("123.123.123.123", "42");
    ret2 = print_address("2001:6a8::1234:1234", "42");
    ret3 = print_address(NULL, "17");
    ret4 = print_address("Lolcat", "42");
    ret5 = print_address("127:0:0:1", "22");
    SANDBOX_END;
    CU_ASSERT_EQUAL(ret1, 0);
    CU_ASSERT_EQUAL(ret2, 0);
    CU_ASSERT_EQUAL(ret3, 0);
    CU_ASSERT_EQUAL(ret4, EAI_NONAME);
    CU_ASSERT_EQUAL(ret5, EAI_NONAME);
    set_gai_methods(NULL, NULL);
}

void test_verbose()
{
    set_test_metadata("verb", _("Checks if the verbose function fails correctly"), 1);
    monitored.write = true;
    
    SANDBOX_BEGIN;
    SANDBOX_END;
    //CU_ASSERT_EQUAL(stats.write.last_params.fd, STDOUT_FILENO);
    //CU_ASSERT_EQUAL(stats.write.last_errno, EAGAIN);
    //CU_ASSERT_EQUAL(stats.write.last_return, -1);
}

void test_lol()
{
    set_test_metadata("lol", _("lol"), 1);
    SANDBOX_BEGIN;
    lol();
    SANDBOX_END;
}

void test_myfunc_ret() {
    set_test_metadata("myfunc", _("Brief description of the test"), 1);

    int ret1 = 2, ret2 = 2, ret3 = 2, errno3 = 0, ret4 = 2, ret5 = 2, errno5 = 0;
    struct addrinfo *answer1, *answer2, *answer3, *answer4, *answer5;
    monitored.getaddrinfo = true;
    monitored.freeaddrinfo = true;

    SANDBOX_BEGIN;
    ret1 = report_address_info("google.be", "80", &answer1, 0, 0);
    ret2 = report_address_info("216.58.212.195", NULL, &answer2, 1, 1);
    ret3 = report_address_info("google.be", NULL, &answer3, 1, 1);
    errno3 = errno;
    ret4 = report_address_info("2a00:1450:400e:802::2003", "80", &answer4, 1, 1);
    ret5 = report_address_info(NULL, NULL, &answer5, 0, 0);
    errno5 = errno;
    freeaddrinfo(answer1);
    freeaddrinfo(answer2);
    //freeaddrinfo(answer3);
    freeaddrinfo(answer4);
    //freeaddrinfo(answer5);
    SANDBOX_END;

    CU_ASSERT_EQUAL(ret1, 0);
    CU_ASSERT_EQUAL(ret2, 0);
    CU_ASSERT_EQUAL(ret3, -2);
    CU_ASSERT_EQUAL(errno3, EAI_NONAME);
    CU_ASSERT_EQUAL(ret4, 0);
    CU_ASSERT_EQUAL(ret5, -2);
    CU_ASSERT_EQUAL(errno5, EAI_NONAME);
    CU_ASSERT_EQUAL(stats.getaddrinfo.called, 5);
    CU_ASSERT_EQUAL(stats.freeaddrinfo.called, 3);
}

int main(int argc,char** argv)
{
    BAN_FUNCS();
    RUN(test_myfunc_ret, test_print_address1, test_print_address2);
}


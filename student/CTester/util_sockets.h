#ifndef __CTESTER_UTIL_SOCKETS_H__
#define __CTESTER_UTIL_SOCKETS_H__

#include <stdint.h>

/**
 * Constants used when reporting the output of the mock server or client.
 * Currenly, they may be combined with any of them;
 * maybe we should exclude some combination.
 */
#define OK 0
#define TOO_MUCH 1 // And the amount should follow
#define TOO_FEW 2 // And the amount should follow
#define NOT_SAME 4 // And the recvd tab should follow
#define NOTHING_RECV 8
#define RECV_ERROR 16
#define SEND_ERROR 32
#define EXIT_PROCESS 64 // The child process exits: beware of SIGPIPE!
#define EXTEND_MSG 128 // Next octet gives the status
#define U8SZ (sizeof(uint8_t))
#define U16SZ (sizeof(uint16_t))

/**
 * Creates a socket with the specified domain, type, a nought protocol,
 * and which is either bound to the specified host name (which should be NULL)
 * and service name (if do_bind is true and flags has AI_PASSIVE) or which is
 * connected to the specified host name and service name (if do_bind is false).
 * The call to getaddrinfo has flags AI_NUMERICHOST and AI_NUMERICSERV
 * set, in order to disable DNS; additionnal flags are then passed with it.
 * If do_bind, it is important to note that the socket will NOT listen
 * for incoming connections: it is up to the caller to activate it.
 */
int create_socket(const char *host, const char *serv, int domain, int type, int flags, int do_bind);

/**
 * Creates a socket with the specified domain (AF_INET or AF_INET6),
 * type (either SOCK_STREAM or SOCK_SEQPACKET), a nought protocol,
 * and which is listening for incoming connections on port specified in serv.
 * This calls create_socket, and thus requires serv to be numeric.
 */
int create_tcp_server_socket(const char *serv, int domain, int type);

/**
 * Creates a socket with the specified domain (AF_INET or AF_INET6),
 * type (either SOCK_STREAM or SOCK_SEQPACKET), a nought protocol,
 * and connected to the specified address and port (host and serv).
 * This calls create_socket, and thus requires host and serv to be numeric.
 */
int create_tcp_client_socket(const char *host, const char *serv, int domain, int type);

/**
 * Creates a UDP socket with the specified domain (AF_INET or AF_INET6),
 * a nought protocol, and which accepts UDP packets on port specified in serv.
 * This calls create_socket, and thus requires serv to be numeric.
 */
int create_udp_server_socket(const char *serv, int domain);

/**
 * Creates a UDP socket with the specified domain (AF_INET or AF_INET6), a zero
 * protocol, and connected to the specified address and port (host and serv).
 * This calls create_socket, and thus requires host and serv to be numeric.
 */
int create_udp_client_socket(const char *host, const char *serv, int domain);

#define RECV_CHUNK 1
#define SEND_CHUNK 2

/**
 * A chunk of data to be recv or send by the server/client.
 * type can be
 * - RECV_CHUNK, in which case the server/client will try to receive it
 *   and will compare the received bytes with the provided data
 *   (no byte order conversion are done!)
 * - SEND_CHUNK, in which case the server/client will send to the remote end
 *   the data, and will report any transmission error.
 * Warning, the chunks should have the network byte order, as the server/client
 * will not try to reverse the bytes.
 */
struct cs_network_chunk {
    void *data;
    size_t data_length;
    int type;
};

/**
 * A transaction, which is a list of chunks exchanged between the server/client
 * and the remote end.
 * A transaction is successful if all RECV_CHUNK chunks have been received
 * correctly, and if the server/client hasn't been asked to quit.
 */
struct cs_network_transaction {
    struct cs_network_chunk *chunks;
    size_t nchunks;
};

/**
 * A list of transactions to be tested one after another, using one instance
 * of the server/client.
 */
struct cs_network_transactions {
    struct cs_network_transaction *transactions;
    size_t ntransactions;
};

/**
 * Launches a TCP-listening and accepting server, on port specified by serv,
 * in a separate process. In order to communicate with the caller,
 * server_in is filled with the writing end of a pipe transfering toward
 * the server (for example, stop condition), server_out is filled with
 * the reading end of a pipe written by the server with the results of
 * the requested transactions, and spid is filled with the process pid,
 * so that the caller can wait on it.
 * Returns 0 on successful launch of the server, 1 otherwise.
 * The server exits with code 3 if it receives something on server_in,
 * with code 2 if there was an error, and with 1 if it couldn't create the socket.
 */
int launch_test_tcp_server(struct cs_network_transactions *transactions, const char *serv, int domain, int *server_in, int *server_out, int *spid);

/**
 * Launches a TCP client, connecting and sending to host host on port serv,
 * in a separate process. To communicate with the caller, client_in is filled
 * with the writing end of a pipe transfering toward the client (for example,
 * stop condition), client_out is filled with the reading end of a pipe written
 * by the client with the results of the requested transactions, and cpid
 * is filled with the process pid, so that the caller can wait on it.
 * Returns 0 on successful launch of the client, 1 otherwise.
 */
int launch_test_tcp_client(struct cs_network_transactions *transactions, const char *host, const char *serv, int domain, int *client_in, int *client_out, int *cpid);

/**
 * Launches a UDP server, accepting messages on port specified by serv,
 * in a separate process. To communicate with the caller, server_in is filled
 * with the writing end of a pipe transfering toward the server (for example,
 * stop condition), server_out is filled with the reading end of a pipe
 * written by the server with the results of the requested transactions,
 * and spid is filled with the process pid, so that the caller can wait on it.
 * Returns 0 on successful launch of the server, 1 otherwise.
 */
int launch_test_udp_server(struct cs_network_transactions *transactions, const char *serv, int domain, int *server_in, int *server_out, int *spid);

/**
 * Launches a UDP client, sending messages to host host on port serv,
 * in a separate process. To communicate with the caller, client_in is filled
 * with the writing end of a pipe transfering toward the client (for example,
 * stop condition), client_out is filled with the reading end of a pipe written
 * by the client with the results of the requested transactions, and cpid
 * is filled with the process pid, so that the caller can wait on it.
 * Returns 0 on successful launch of the server, 1 otherwise.
 */
int launch_test_udp_client(struct cs_network_transactions *transactions, const char *host, const char *serv, int domain, int *client_in, int *client_out, int *cpid);

#endif // __CTESTER_UTIL_SOCKETS_H__


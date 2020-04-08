#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/kthread.h>
#include <linux/memory.h>
#include <linux/sched/signal.h>
#include <linux/string.h>
#include <linux/tcp.h>

#include "fibonacci/bn.h"
#include "http_parser.h"
#include "http_server.h"

#define CRLF "\r\n"

#define HTTP_RESPONSE_200                                      \
    ""                                                         \
    "HTTP/1.1 200 OK" CRLF "Server: " KBUILD_MODNAME CRLF      \
    "Content-Type: text/plain" CRLF "Content-Length: %lu" CRLF \
    "Connection: %s" CRLF CRLF "%s" CRLF
#define HTTP_RESPONSE_200_DUMMY                               \
    ""                                                        \
    "HTTP/1.1 200 OK" CRLF "Server: " KBUILD_MODNAME CRLF     \
    "Content-Type: text/plain" CRLF "Content-Length: 12" CRLF \
    "Connection: Close" CRLF CRLF "Hello World!" CRLF
#define HTTP_RESPONSE_200_KEEPALIVE_DUMMY                     \
    ""                                                        \
    "HTTP/1.1 200 OK" CRLF "Server: " KBUILD_MODNAME CRLF     \
    "Content-Type: text/plain" CRLF "Content-Length: 12" CRLF \
    "Connection: Keep-Alive" CRLF CRLF "Hello World!" CRLF
#define HTTP_RESPONSE_501                                              \
    ""                                                                 \
    "HTTP/1.1 501 Not Implemented" CRLF "Server: " KBUILD_MODNAME CRLF \
    "Content-Type: text/plain" CRLF "Content-Length: 21" CRLF          \
    "Connection: Close" CRLF CRLF "501 Not Implemented" CRLF
#define HTTP_RESPONSE_501_KEEPALIVE                                    \
    ""                                                                 \
    "HTTP/1.1 501 Not Implemented" CRLF "Server: " KBUILD_MODNAME CRLF \
    "Content-Type: text/plain" CRLF "Content-Length: 21" CRLF          \
    "Connection: KeepAlive" CRLF CRLF "501 Not Implemented" CRLF

#define RECV_BUFFER_SIZE 4096
#define MAX_URL_SIZE 128

struct http_request {
    struct socket *socket;
    enum http_method method;
    char request_url[MAX_URL_SIZE];
    int complete;
};

static int http_server_recv(struct socket *sock, char *buf, size_t size)
{
    struct kvec iov = {.iov_base = (void *) buf, .iov_len = size};
    struct msghdr msg = {.msg_name = 0,
                         .msg_namelen = 0,
                         .msg_control = NULL,
                         .msg_controllen = 0,
                         .msg_flags = 0};
    return kernel_recvmsg(sock, &msg, &iov, 1, size, msg.msg_flags);
}

static int http_server_send(struct socket *sock, const char *buf, size_t size)
{
    struct msghdr msg = {.msg_name = NULL,
                         .msg_namelen = 0,
                         .msg_control = NULL,
                         .msg_controllen = 0,
                         .msg_flags = 0};
    int done = 0;
    while (done < size) {
        struct kvec iov = {
            .iov_base = (void *) ((char *) buf + done),
            .iov_len = size - done,
        };
        int length = kernel_sendmsg(sock, &msg, &iov, 1, iov.iov_len);
        if (length < 0) {
            pr_err("write error: %d\n", length);
            break;
        }
        done += length;
    }
    return done;
}

static size_t my_log10(size_t N)
{
    unsigned int vals[] = {
        1UL,      10UL,      100UL,      1000UL,      10000UL,
        100000UL, 1000000UL, 10000000UL, 100000000UL, 1000000000UL,
    };
    size_t i;
    for (i = 0; i < 9; ++i) {                   // 9
        if (N >= vals[i] && N < vals[i + 1]) {  // 8
            break;                              // 1
        }
    }
    return i;
}

static char *generate_response(char *url, int keep_alive)
{
    // TODO: use macro in http_parser.c
    char *connection = keep_alive ? "Keep-Alive" : "Close";

    // parse URL
    char *content = url;  // assign to fibonacci iff url == "fib/[0-9]*"

    // first character is "/", ignore it
    ++url;
    char *path = strsep(&url, "/");

    if (!strcmp(path, "fib") && (path = strsep(&url, "/"))) {
        long k = 1;
        bn_t fib = BN_INITIALIZER;
        if (kstrtol(path, 10, &k) == 0) {
            fibonacci(k, fib);
            content = bn_return(fib);
            if (!content) {
                pr_info("Get fibonacci error\n");
            }
        }
    }


    // use log10 to count length of number
    size_t response_buf_size = strlen(HTTP_RESPONSE_200) +
                               my_log10(strlen(content)) + 1 +
                               strlen(connection) + strlen(content);
    char *response = MALLOC(response_buf_size);

    snprintf(response, response_buf_size, HTTP_RESPONSE_200, strlen(content),
             connection, content);
    return response;
}
static int http_server_response(struct http_request *request, int keep_alive)
{
    char *response;

    pr_info("requested_url = %s\n", request->request_url);
    if (request->method != HTTP_GET)
        response = keep_alive ? HTTP_RESPONSE_501_KEEPALIVE : HTTP_RESPONSE_501;
    else {
        response = generate_response(request->request_url, keep_alive);
    }
    http_server_send(request->socket, response, strlen(response));
    return 0;
}

static int http_parser_callback_message_begin(http_parser *parser)
{
    struct http_request *request = parser->data;
    struct socket *socket = request->socket;
    memset(request, 0x00, sizeof(struct http_request));
    request->socket = socket;
    return 0;
}

static int http_parser_callback_request_url(http_parser *parser,
                                            const char *p,
                                            size_t len)
{
    struct http_request *request = parser->data;
    strncat(request->request_url, p, len);
    return 0;
}

static int http_parser_callback_header_field(http_parser *parser,
                                             const char *p,
                                             size_t len)
{
    return 0;
}

static int http_parser_callback_header_value(http_parser *parser,
                                             const char *p,
                                             size_t len)
{
    return 0;
}

static int http_parser_callback_headers_complete(http_parser *parser)
{
    struct http_request *request = parser->data;
    request->method = parser->method;
    return 0;
}

static int http_parser_callback_body(http_parser *parser,
                                     const char *p,
                                     size_t len)
{
    return 0;
}

static int http_parser_callback_message_complete(http_parser *parser)
{
    struct http_request *request = parser->data;
    http_server_response(request, http_should_keep_alive(parser));
    request->complete = 1;
    return 0;
}

static int http_server_worker(void *arg)
{
    char *buf;
    struct http_parser parser;
    struct http_parser_settings setting = {
        .on_message_begin = http_parser_callback_message_begin,
        .on_url = http_parser_callback_request_url,
        .on_header_field = http_parser_callback_header_field,
        .on_header_value = http_parser_callback_header_value,
        .on_headers_complete = http_parser_callback_headers_complete,
        .on_body = http_parser_callback_body,
        .on_message_complete = http_parser_callback_message_complete};
    struct http_request request;
    struct socket *socket = (struct socket *) arg;

    allow_signal(SIGKILL);
    allow_signal(SIGTERM);

    buf = kmalloc(RECV_BUFFER_SIZE, GFP_KERNEL);
    if (!buf) {
        pr_err("can't allocate memory!\n");
        return -1;
    }

    request.socket = socket;
    http_parser_init(&parser, HTTP_REQUEST);
    parser.data = &request;
    while (!kthread_should_stop()) {
        int ret = http_server_recv(socket, buf, RECV_BUFFER_SIZE - 1);
        if (ret <= 0) {
            if (ret)
                pr_err("recv error: %d\n", ret);
            break;
        }
        http_parser_execute(&parser, &setting, buf, ret);
        if (request.complete && !http_should_keep_alive(&parser))
            break;
    }
    kernel_sock_shutdown(socket, SHUT_RDWR);
    sock_release(socket);
    kfree(buf);
    return 0;
}

int http_server_daemon(void *arg)
{
    struct socket *socket;
    struct task_struct *worker;
    struct http_server_param *param = (struct http_server_param *) arg;

    allow_signal(SIGKILL);
    allow_signal(SIGTERM);

    while (!kthread_should_stop()) {
        int err = kernel_accept(param->listen_socket, &socket, 0);
        if (err < 0) {
            if (signal_pending(current))
                break;
            pr_err("kernel_accept() error: %d\n", err);
            continue;
        }
        worker = kthread_run(http_server_worker, socket, KBUILD_MODNAME);
        if (IS_ERR(worker)) {
            pr_err("can't create more worker process\n");
            continue;
        }
    }
    return 0;
}

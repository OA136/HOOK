#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <amqp.h>
#include <amqp_tcp_socket.h>
#include <amqp_framing.h>
#include "utils.c"
//#include <mlcLog.h>
//#include <iniparser.h>
typedef enum __bool {false = 0, true =1, }bool;

int constumer(int channel, char const *exchange, char const *bindingkey) {
    printf("call recvMessage()\n");
    amqp_connection_state_t conn;
    amqp_socket_t *socket = NULL;
    amqp_bytes_t queuename;
    //创建连接
    conn = amqp_new_connection();
    //打开socket
    socket = amqp_tcp_socket_new(conn);
    if(!socket){
        printf("new socket failed!\n");
        return 0;
    }
    if(amqp_socket_open(socket,"localhost",5672) != AMQP_STATUS_OK){
        printf("open socket failed!\n");
    }
    //登录rabbitMQ
    die_on_amqp_error(amqp_login(conn,"/",0,524288,0,AMQP_SASL_METHOD_PLAIN,"guest","guest"), "Logging in");

    amqp_channel_open(conn, channel);
    die_on_amqp_error(amqp_get_rpc_reply(conn ), "Opening channel");
    {
        amqp_queue_declare_ok_t *r = amqp_queue_declare(conn, 1, amqp_empty_bytes, 0, 0, 0, 1,
                                                        amqp_empty_table);
        die_on_amqp_error(amqp_get_rpc_reply(conn), "Declaring queue");
        queuename = amqp_bytes_malloc_dup(r->queue);
        if (queuename.bytes == NULL) {
            fprintf(stderr, "Out of memory while copying queue name");
            return 0;
        }
        printf("queue_name:%s\n", (char *)(r->queue.bytes));
    }

    amqp_queue_bind(conn, channel, queuename, amqp_cstring_bytes(exchange), amqp_cstring_bytes(bindingkey),
                                                                                    amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Binding queue");

    amqp_basic_consume(conn, channel, queuename, amqp_empty_bytes, 0, 1, 0, amqp_empty_table);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Consuming");

    {
        for (;;) {
            amqp_rpc_reply_t res;
            amqp_envelope_t envelope;

            amqp_maybe_release_buffers(conn);

            res = amqp_consume_message(conn, &envelope, NULL, 0);

            if (AMQP_RESPONSE_NORMAL != res.reply_type) {
                break;
            }

//            printf("Delivery %u, exchange %.*s routingkey %.*s\n",
//            (unsigned) envelope.delivery_tag,
//            (int) envelope.exchange.len, (char *) envelope.exchange.bytes,
//            (int) envelope.routing_key.len, (char *) envelope.routing_key.bytes);
//
//            if (envelope.message.properties._flags & AMQP_BASIC_CONTENT_TYPE_FLAG) {
//                printf("Content-type: %.*s\n",
//                (int) envelope.message.properties.content_type.len,
//                (char *) envelope.message.properties.content_type.bytes);
//            }
//            printf("----\n");
//
//            amqp_dump(envelope.message.body.bytes, envelope.message.body.len);
            int len = (int)envelope.message.body.len;
            char *dest = (char *)malloc(sizeof(char)*(len+1));
            printf("len:%d\ncontent:%s\n", len, str_sub(dest, (unsigned char *) (envelope.message.body.bytes), len));
            amqp_destroy_envelope(&envelope);
            free(dest);
        }
    }

    die_on_amqp_error(amqp_channel_close(conn, 1, AMQP_REPLY_SUCCESS), "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");
    return 0;
}

int main() {
    char const *exchange = "amq.direct";
    char const *bindingkey = "test";
    int channel = 1;
    constumer(channel, exchange, bindingkey);
    return 0;
}

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

typedef enum __bool {false = 0, true =1, }bool;

int producer(int channel, char const *exchange, char const *routingkey, char const *messagebody){
    bool f_false = false, f_true = true;
    amqp_connection_state_t conn;
    amqp_socket_t *socket = NULL;
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
    die_on_amqp_error(amqp_login(conn,"/",0,1310172,0,AMQP_SASL_METHOD_PLAIN,"guest","guest"), "Logging in");

    //打开隧道
    amqp_channel_open(conn, channel);
    die_on_amqp_error(amqp_get_rpc_reply(conn), "Opening channel");
    //amqp_confirm_select_ok_t *acsot = amqp_confirm_select(conn , 1);
    //printf("amqp_confirm_select_ok_t= %i",acsot->dummy);
    //amqp_get_rpc_reply(conn);
    /*创建线程*/
    /*
    pthread_t id;
    int thread_result;
    if ((thread_result = pthread_create(&id, NULL, recvMessage, NULL)) != 0){
    printf("can't create thread:%s\n",strerror(thread_result));
    return -1;
    }*/
    /*发送消息*/

    {
        amqp_basic_properties_t props;
        props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
        props.content_type = amqp_cstring_bytes("text/plain");
        props.delivery_mode = 2; /* persistent delivery mode */
        die_on_error(amqp_basic_publish(conn,
                                        channel,
                                        amqp_cstring_bytes(exchange),
                                        amqp_cstring_bytes(routingkey),
                                        0,
                                        0,
                                        &props,
                                        amqp_cstring_bytes(messagebody)),
                                        "Publishing");
    }
    //pthread_join(id, NULL);
    die_on_amqp_error(amqp_channel_close(conn, channel, AMQP_REPLY_SUCCESS), "Closing channel");
    die_on_amqp_error(amqp_connection_close(conn, AMQP_REPLY_SUCCESS), "Closing connection");
    die_on_error(amqp_destroy_connection(conn), "Ending connection");
    return 0;
}

int main() {
    char const *messagebody = "hello sq\0";
    char const *exchange = "amq.direct";
    char const *routingkey = "test";
    int channel = 1;
    producer(channel, exchange, routingkey, messagebody);
    return 0;
}

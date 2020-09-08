
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>

#include <mosquitto.h>

#define mqtt_host           "127.0.0.1"
#define mqtt_port           (1883)

static bool run = true;

static void Handle_signal(int s)
{
    switch (s) {
        case SIGINT:
            printf("\nInterrupt signal\n");
            break;
        case SIGTERM:
            printf("\nTermination signal\n");
            break;
        default:
            printf("\nUnknown signal\n");
    }
    run = false;
}

static void Connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("Connected to broker %s:%d\n", mqtt_host, mqtt_port);
}

static void Message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{

    printf("message '%.*s' form topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

}

int main(int argc, char *argv[])
{
    char clientid[15];
    int rc = 0;
    struct mosquitto *mosq;

    signal(SIGINT, Handle_signal); //  Interrupt signal
    signal(SIGTERM, Handle_signal); // Termination signal

    mosquitto_lib_init();

    sprintf(clientid, "PID=%d", getpid()); // for example client_id - use pid process
    printf("%s\n",clientid);
    mosq = mosquitto_new(clientid, true, 0);

    if(mosq){
        mosquitto_connect_callback_set(mosq, Connect_callback); // callback is called when connecting to the broker
        mosquitto_message_callback_set(mosq, Message_callback); // callback for message

        rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);

        mosquitto_subscribe(mosq, NULL, "/test_topic", 0);

        while(run){
            rc = mosquitto_loop(mosq, -1, 1);
            if(rc){
                printf("connection error!\n");
                sleep(10);
                mosquitto_reconnect(mosq);
            }
        }
        mosquitto_destroy(mosq);
        printf("Destroy resource\n");
    }

    mosquitto_lib_cleanup();
    return rc;
}
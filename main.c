#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#if defined(ENABLE_LOG)
#include <syslog.h>
#endif

#include <mosquitto.h>
#include "tiny-json.h"


#define mqtt_host           "127.0.0.1"
#define mqtt_port           (1883)

/** for example global variable  */
static enum {
    SSLMode0,
    SSLMode1,
    SSLMode2

}SSLMode_e; /** 0:disable, 1:CA Mode, 2: PSK Mode */

static char strTLCertSPW[37] = "05155853"; // SSL/TLS provate key or pre-shared-key
static char strDeviceID[37] = "00000001-0000-0000-0000-305A3A770020"; //Target device unique ID
static char strConnID[256] = "0e95b665-3748-46ce-80c5-bdd423d7a8a5:631476df-31a5-4b66-a4c6-bd85228b9d27"; //broker connection ID
static char strConnPW[64] = "f3a2342t4oejbefc78cgu080ia"; //MQTT broker connection password

static bool run = true;
static bool print_dump = false;

static bool Json_parser_process(char *payload);
static bool Json_parser_settings(char *payload);
static void dump( json_t const* json );


static int Password_check_callback(char *buf, int size, int rwflag, void *userdata);


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


static void Mosquitto_log_callback(struct mosquitto *mosq, void *obj, int level, const char* str){

    printf(" Mosquitto LOG: %s\n", str);
#if defined(ENABLE_LOG)
    syslog(LOG_DEBUG, "%s", str);
#endif
}


static void Connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("Connected to broker %s:%d\n", mqtt_host, mqtt_port);
}

static void Message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{

    mosquitto_publish(mosq, 0, "/test_topic_2", message->payloadlen,  (char*) message->payload, 0,true);
    //printf("message '%.*s' form topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

    if (strstr (message->topic,"/settings") != NULL){
        if (Json_parser_settings((char *) message->payload)) {
            /*** ***/
            /*** ***/
            /*** ***/
        }
    }

    if (strstr (message->topic,"/test_topic") != NULL) {
        if (Json_parser_process((char *) message->payload)) {
            /*** ***/
            /*** ***/
            /*** ***/
        }
    }

}


int main(int argc, char *argv[])
{
    char clientid[15];
    int rc = 0;
    struct mosquitto *mosq;
    SSLMode_e  = SSLMode0;
    bool authentication = false;

#if defined(ENABLE_LOG)
    openlog("MQTT", 0, LOG_USER);
#endif

    signal(SIGINT, Handle_signal); //  Interrupt signal
    signal(SIGTERM, Handle_signal); // Termination signal

    mosquitto_lib_init();

    sprintf(clientid, "PID=%d", getpid()); // for example client_id - use pid process
    printf("%s\n",clientid);
    mosq = mosquitto_new(clientid, true, 0);
    mosquitto_log_callback_set(mosq, Mosquitto_log_callback);


    /** Setup connection SSL/TLS,
     * SSLMode=0 disable the SSL/TLS.
     * SSLMode=1 certificate based SSL/TLS.
     * If the server you are connecting to requires clients to provide a
     * certificate, define certfile and keyfile with your client certificate and
     * private key. If your private key is encrypted, provide a password callback
     * function or you will have to enter the password at the command line.
     * SSLMode=2  pre-shared-key based TLS.
     * If the server you are connecting to provide a pre-shared-key, define the pre-shared-key and an ID with your client.
     * private key.
     */
    switch (SSLMode_e) {
        case SSLMode1:
            mosquitto_tls_insecure_set(mosq, true);
            mosquitto_tls_set(mosq, "server.crt", NULL, "ca.crt", "ca.key", Password_check_callback);
            break;
        case SSLMode2:
            mosquitto_tls_psk_set(mosq, strTLCertSPW, strDeviceID, NULL);
            break;
        default:
            break;
    }

    /** Setup MQTT connection ID and Password */
    if (authentication) {
        if (strlen(strConnID) > 0 && strlen(strConnPW) > 0)
            mosquitto_username_pw_set(mosq, strConnID, strConnPW);
        /*reset will message*/
        mosquitto_will_clear(mosq);
    }


    if(mosq){
        mosquitto_connect_callback_set(mosq, Connect_callback); // callback is called when connecting to the broker
        mosquitto_message_callback_set(mosq, Message_callback); // callback for message

        rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);

        mosquitto_subscribe(mosq, NULL, "/test_topic", 0);
        mosquitto_subscribe(mosq, NULL, "/settings", 0);

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
#if defined(ENABLE_LOG)
    closelog();
#endif
    return rc;
}


static bool Json_parser_settings(char *payload){
/**
 *    {
 *      "dump":  "true"
 *    }
*/
    json_t mem[32];
    json_t const* json = json_create( payload, mem, sizeof mem / sizeof *mem );
    if ( !json ) {
        printf("Wrong json\n");
        return false;
    }

    json_t const* dump = json_getProperty( json, "dump" );
    if ( !dump ) {
        printf("Error, the dump is not found\n");
        return false;
    }
    bool const dumpVal = json_getBoolean( dump );
    print_dump = dumpVal;
    printf( "dump: %s.\n", dumpVal == true ? "true" : "false" );

    return true;
}


static bool Json_parser_process(char *payload){
/**
 *    {
 *      "user":  "Johndoe",
 *      "admin":  "true",
 *      "uid":  "1234567654321",
 *      "groups":  "admin"
 *    }
*/

    json_t mem[32];
    json_t const* json = json_create( payload, mem, sizeof mem / sizeof *mem );
    if ( !json ) {
        printf("Wrong json\n");
        return false;
    }

    if (print_dump)
        dump( json );

    printf(" \n");
    json_t const* user = json_getProperty( json, "user" );
    if ( !user || JSON_TEXT != json_getType( user ) ) {
        printf("Error, the user property is not found\n");
        return false;
    }
    char const* firstNameVal = json_getValue( user );
    printf( "user: %s.\n", firstNameVal );



    json_t const* admin = json_getProperty( json, "admin" );
    if ( !admin ) {
        printf("Error, the admin is not found\n");
        return false;
    }
    bool const adminVal = json_getBoolean( admin );
    printf( "admin: %s.\n", adminVal == true ? "true" : "false" );



    json_t const* uid = json_getProperty( json, "uid" );
    if ( !uid ) {
        printf("Error, the uid property is not found\n");
        return false;
    }
    long int const uidVal = json_getInteger( uid );
    printf( "uid: %lu.\n", uidVal );



    json_t const* groups = json_getProperty( json, "groups" );
    if ( !groups || JSON_TEXT != json_getType( groups ) ) {
        printf("Error, the groups property is not found\n");
        return false;
    }

    char const* groupsVal = json_getValue( groups );
    printf( "groups: %s.\n", groupsVal );

    return true;

}


static void dump( json_t const* json ) {

    jsonType_t const type = json_getType( json );
    if ( type != JSON_OBJ && type != JSON_ARRAY ) {
        printf("error\n");
        return;
    }

    printf( "%s\n", type == JSON_OBJ? " {": " [" );

    json_t const* child;
    for( child = json_getChild( json ); child != 0; child = json_getSibling( child ) ) {

        jsonType_t propertyType = json_getType( child );
        char const* name = json_getName( child );
        if ( name ) printf(" \"%s\": ", name );

        if ( propertyType == JSON_OBJ || propertyType == JSON_ARRAY )
            dump( child );

        else {
            char const* value = json_getValue( child );
            if ( value ) {
                bool const text = JSON_TEXT == json_getType( child );
                char const* fmt = text? " \"%s\"": " %s";
                printf( fmt, value );
                bool const last = !json_getSibling( child );
                if ( !last ) putchar(',');
            }
        }
    }

    printf( "%s", type == JSON_OBJ? " }": " ]" );

}


/** password check callback function to handle SSL/TLS private password */
static int Password_check_callback(char *buf, int size, int rwflag, void *userdata)
{
    int length = 0;
    if(!buf)
        return 0;
    length = strlen(strTLCertSPW);
    memset(buf, 0, size);
    if(length+1 >= size)
    {
        strncpy(buf, strTLCertSPW, size);
        return size;
    }
    else
    {
        strncpy(buf, strTLCertSPW, length + 1);
        return length;
    }
}
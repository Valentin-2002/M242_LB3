/** MQTT Publish von Sensordaten */
#include "mbed.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>


#include "PinNames.h"
#include "mbed_thread.h"


#if MBED_CONF_IOTKIT_BMP180_SENSOR == true
#include "BMP180Wrapper.h"
#endif
#include "MFRC522.h"

#if MBED_CONF_IOTKIT_HTS221_SENSOR == true
#include "HTS221Sensor.h"
#endif


#include <MQTTClientMbedOs.h>
#include <MQTTNetwork.h>
#include <MQTTClient.h>
#include <MQTTmbed.h> // Countdown

// Sensoren wo Daten fuer Topics produzieren
static DevI2C devI2c( MBED_CONF_IOTKIT_I2C_SDA, MBED_CONF_IOTKIT_I2C_SCL );
#if MBED_CONF_IOTKIT_HTS221_SENSOR == true
static HTS221Sensor hum_temp(&devI2c);
#endif
#if MBED_CONF_IOTKIT_BMP180_SENSOR == true
static BMP180Wrapper hum_temp( &devI2c );
#endif


// Assign Pins for Stepper Motor
DigitalOut s1( D8 );
DigitalOut s2( D9 );
DigitalOut s3( D12 );
DigitalOut s4( D13 );

DigitalOut en1( D10 );
DigitalOut en2( D11 );

// Topic's publish
char* TOPIC_PUB_mTemp = (char*) "CKWLB3/mTemp";
char* TOPIC_PUB_wPos = (char*) "CKWLB3/wPos";

// Topic's subscribe
char* TOPIC_SUB_pTemp = (char*) "CKWLB3/pTemp";

// MQTT Brocker
char* hostname = (char*) "cloud.tbz.ch";
int port = 1883;
// MQTT Message
MQTT::Message message;
// I/O Buffer
char buf[100];

int mTemp;
int pTemp = 25; 
float wPos;

// Time to wait between every pole switch in the stepper motor
static int stepSleepMS = 1;

// Current Position of the Stepper Motor
static int stepperPosition = 0;

// Maximal allowed Position of the Stepper Motor
static int maxPosition = 2000;

// Minutes to wait after executing the loop
static int sleepForMins = 1;

// Amount of Positions for the Window
static int stepAmount = 5;

void stepCounterClockWise () {
    en1 = 1; en2 = 1;
    s1 = 1; s2 = 0; s3 = 1; s4 = 0;
    thread_sleep_for( stepSleepMS );
    s3 = 0; s4 = 1;                 
    thread_sleep_for( stepSleepMS );
    s1 = 0; s2 = 1;                
    thread_sleep_for( stepSleepMS );
    s3 = 1; s4 = 0;                 
    thread_sleep_for( stepSleepMS );
    stepperPosition--;
}

// Helper Method wich performs a single Step of the Stepper Motor clockwise
void stepClockwise() {
    en1 = 1; en2 = 1;
    s1 = 0; s2 = 1; s3 = 1; s4 = 0;
    thread_sleep_for( stepSleepMS );
    s3 = 0; s4 = 1;                 
    thread_sleep_for( stepSleepMS );
    s1 = 1; s2 = 0;                 
    thread_sleep_for( stepSleepMS );
    s3 = 1; s4 = 0;                 
    thread_sleep_for( stepSleepMS );
    stepperPosition++;
}

/** Hilfsfunktion zum Publizieren auf MQTT Broker */
void publish( MQTTNetwork &mqttNetwork, MQTT::Client<MQTTNetwork, Countdown> &client, char* topic )
{
    MQTT::Message message;    
    printf( "Push: %s\n", buf );
    message.qos = MQTT::QOS0;
    message.retained = false;
    message.dup = false;
    message.payload = (void*) buf;
    message.payloadlen = strlen(buf)+1;
    client.publish( topic, message);  
}

/** Daten empfangen von MQTT Broker */
void tempValueArrived( MQTT::MessageData& md )
{

    MQTT::Message &message = md.message;

    ((char*) message.payload)[message.payloadlen] = '\0';

    sscanf( (char*) message.payload, "%d", &pTemp );

    printf("pTemp: %d\n", pTemp);
            
}

/** Hauptprogramm */
int main()
{
    uint8_t id;
    
    printf( "MQTTPublish\r\n" );
    printf( "host: %s:%d\r\n", hostname, port );

    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    printf( "SSID: %s\r\n", MBED_CONF_APP_WIFI_SSID );
    
    // Connect to the network with the default networking interface
    // if you use WiFi: see mbed_app.json for the credentials
    WiFiInterface *wifi = WiFiInterface::get_default_instance();
    if ( !wifi ) 
    {
        printf("ERROR: No WiFiInterface found.\n");
        return -1;
    }
    printf("\nConnecting to %s...\n", MBED_CONF_APP_WIFI_SSID);
    int ret = wifi->connect( MBED_CONF_APP_WIFI_SSID, MBED_CONF_APP_WIFI_PASSWORD, NSAPI_SECURITY_WPA_WPA2 );
    if ( ret != 0 ) 
    {
        printf("\nConnection error: %d\n", ret);
        return -1;
    }    

    // TCP/IP und MQTT initialisieren (muss in main erfolgen)
    MQTTNetwork mqttNetwork( wifi );
    MQTT::Client<MQTTNetwork, Countdown> client(mqttNetwork);

    printf("Connecting to %s:%d\r\n", hostname, port);
    int rc = mqttNetwork.connect(hostname, port);
    if (rc != 0)
        printf("rc from TCP connect is %d\r\n", rc); 

    // Zugangsdaten - der Mosquitto Broker ignoriert diese
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char*) wifi->get_mac_address(); // muss Eindeutig sein, ansonsten ist nur 1ne Connection moeglich
    data.username.cstring = (char*) wifi->get_mac_address(); // User und Password ohne Funktion
    data.password.cstring = (char*) "password";
    if ((rc = client.connect(data)) != 0)
        printf("rc from MQTT connect is %d\r\n", rc);           

    // MQTT Subscribe!
    client.subscribe(TOPIC_SUB_pTemp , MQTT::QOS0, tempValueArrived );
    printf("MQTT subscribe %s\n", TOPIC_SUB_pTemp );
    
    /* Init all sensors with default params */
    hum_temp.init(NULL);
    hum_temp.enable();     

    while ( 1 ) 
    {

        if(stepperPosition < 0) {
            printf("%i", abs(stepperPosition));
            for(int i  = 0; i <= abs(stepperPosition); i++) {
                printf("+\n");
                stepClockwise();
            }
        } else if(stepperPosition > maxPosition) {
            for(int i  = 0; i <= stepperPosition - maxPosition; i++) {
                printf("-\n");
                stepCounterClockWise();
            }
        }

        float rawmTemp;

        // Temperator und Luftfeuchtigkeit
        hum_temp.read_id(&id);
        hum_temp.get_temperature((&rawmTemp));

        mTemp = (int) rawmTemp;
        
        sprintf( buf, "%d", mTemp ); 
        publish( mqttNetwork, client, TOPIC_PUB_mTemp );

        sprintf( buf, "%f", wPos ); 
        publish( mqttNetwork, client, TOPIC_PUB_wPos );

        printf("pTemp: %d\n", pTemp);

        if(mTemp != pTemp) {
            if(mTemp < pTemp) {
                if(stepperPosition <= maxPosition && stepperPosition > 0) {
                    for(int i = 0; i <= (maxPosition / stepAmount); i++) {
                        stepCounterClockWise();
                    }
                }
            } else {
                if(stepperPosition >= 0 && stepperPosition < maxPosition) {
                    for(int i = 0; i <= (maxPosition / stepAmount); i++) {
                        stepClockwise();
                    }
                }
            }
        }
        
        client.yield    ( 5000 );                   // MQTT Client darf empfangen
        thread_sleep_for( sleepForMins * 1000 * 1 );

    }

    // Verbindung beenden
    if ((rc = client.disconnect()) != 0) {
        printf("rc from disconnect was %d\r\n", rc);
        mqttNetwork.disconnect();   
    }
}

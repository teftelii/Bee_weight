#include <HX711.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WiFi.h>  // Enables the ESP8266 to connect to the local network (via WiFi)
#include <PubSubClient.h> // Connect and publish to the MQTT broker

// DS18B20
const int oneWireBus = D2; // pin 4
// Setup a oneWire instance to communicate with any OneWire devices  
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// HX711
HX711 scale(D7, D6);    // DOUT - pin #D7 SCK - pin #D6
float rawAvg = 0;
float scaleRaw = 0;
float scale_kg = 0;
float scaleTare = 3.54; // 128 - gain 3.470; 64 -gain 3.54;
float scaleCalib = 11703.29805; // 

// WiFi
//const char* ssid = "MikroTik_2GHz";                 // network SSID
const char* ssid = "MikroTik-2GHz";                 // network SSID
const char* wifi_password = ""; // network password

// MQTT
const char* mqtt_server = "";  // IP of the MQTT broker
const char* weight_topic = "home/bee/weight";
const char* temperature_topic = "home/bee/temperature";
const char* voltage_topic = "home/bee/voltage";
const char* mqtt_username = ""; // MQTT username
const char* mqtt_password = ""; // MQTT password
const char* clientID = "client_bee"; // MQTT client ID

// Analog in 
const int analogInPin = A0;  // ESP8266 Analog Pin ADC0 = A0
int voltValue = 0;  // value read from the res divider 
float adcValue = 0.0;
float vout = 0;
float Vout = 0;
float vin = 0;
float R1 = 1000000; // first resistor 1MEG  
float R2 = 85000; // second resistor  116.2k 

// Initialise the WiFi and MQTT Client objects
WiFiClient wifiClient;
// 1883 is the listener port for the Broker
PubSubClient client(mqtt_server, 1883, wifiClient); 


//connet to the MQTT broker
void connect_MQTT(){
  //Serial.print("Connecting to ");
  //Serial.println(ssid);

  // Connect to the WiFi
  WiFi.begin(ssid, wifi_password);

  // Wait until the connection has been confirmed before continuing
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //Serial.print(".");
  }

  //IP Address of the ESP8266
  //Serial.println("WiFi connected");
  //Serial.print("IP address: ");
  //Serial.println(WiFi.localIP());

  // Connect to MQTT Broker
  if (client.connect(clientID, mqtt_username, mqtt_password)) {
    //Serial.println("Connected to MQTT Broker!");
  }
  else {
    //Serial.println("Connection to MQTT Broker failed...");
  }

}


void setup() {
  //Serial.begin(9600);
  sensors.begin(); // Start the DS18B20 sensor
}

void loop() {
  scale.power_up();
  connect_MQTT();
  //weight
  rawAvg = scale.read_average(20);
  scaleRaw = rawAvg /scaleCalib;
  scale_kg = scaleRaw - scaleTare;
  
  //temperature
  sensors.requestTemperatures(); 
  float temp = sensors.getTempCByIndex(0);
  
  //voltage
  voltValue  = analogRead(analogInPin);
 
  vout = (voltValue * 3.3) / 1024.0; // see text
  vout = vout - 0.045; // correction 
  Vout = (R2/(R1+R2))/vout;
  vin = 1/Vout;

  // MQTT transmit strings
  String ws="Weight: "+String((float)scale_kg)+" kg ";
  String ts="Temp: "+String((float)temp)+" C ";
  String vs="Volt: "+String((float)vin)+" V ";

  // PUBLISH to the MQTT Broker (topic = Temperature, defined at the beginning)
  if (client.publish(temperature_topic, String(temp).c_str())) {
    //Serial.println("Temperature sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    //Serial.println("Temperature failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(temperature_topic, String(temp).c_str());
  }

  // PUBLISH to the MQTT Broker (topic = Weight, defined at the beginning)
  if (client.publish(weight_topic, String(scale_kg).c_str())) {
    //Serial.println("Weight sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    //Serial.println("Weight failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(weight_topic, String(scale_kg).c_str());
  }

  // PUBLISH to the MQTT Broker (topic = Voltage, defined at the beginning)
  if (client.publish(voltage_topic, String(vin).c_str())) {
    //Serial.println("Voltage sent!");
  }
  // Again, client.publish will return a boolean value depending on whether it succeded or not.
  // If the message failed to send, we will try again, as the connection may have broken.
  else {
    //Serial.println("Voltage failed to send. Reconnecting to MQTT Broker and trying again");
    client.connect(clientID, mqtt_username, mqtt_password);
    delay(10); // This delay ensures that client.publish doesn't clash with the client.connect call
    client.publish(voltage_topic, String(vin).c_str());
  }
  
  scale.power_down();
  delay(100);
  ESP.deepSleep(930e6,WAKE_RF_DEFAULT); // Sleep for 900 seconds 
  delay(100);
}

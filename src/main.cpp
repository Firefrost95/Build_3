#include <Arduino.h>
#include <OneWire.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "config.h"

#define DS18S20_Pin 2
float temperature = 0;
float receivedValue = 0;
unsigned long previousMillis = 0;
const long interval = 6000; // Time interval in milliseconds

OneWire ds(DS18S20_Pin); // on digital pin 2
WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi()
{
 WiFi.begin(ssid, password);
 while (WiFi.status() != WL_CONNECTED)
 {
  delay(2000);
  Serial.println("Connecting to WiFi...");
 }
 Serial.println("Connected to WiFi");
}

void callback(char *topic, byte *payload, unsigned int length)
{
 Serial.println("Message arrived in topic: " + String(topic));
 Serial.print("Message: ");
 for (int i = 0; i < length; i++)
 {
  Serial.print((char)payload[i]);
 }
 Serial.println();

 // Convert the payload to a float and store it
 receivedValue = atof((char*)payload);
}

void reconnect()
{
 while (!client.connected())
 {
  Serial.println("Attempting MQTT connection...");
  if (client.connect("ESP32Client", mqttUser, mqttPassword))
  {
    Serial.println("Connected to MQTT broker");
    client.subscribe(mqttTopic1);
  }
  else
  {
    Serial.println("Failed to connect to MQTT broker. Retrying in 5 seconds...");
    delay(5000);
  }
 }
}

void setup()
{
 Serial.begin(115200);
 setup_wifi();
 client.setServer(mqttServer, mqttPort);
 client.setCallback(callback);
}

float getTemp()
{
 byte data[12];
 byte addr[8];

 if (!ds.search(addr))
 {
  ds.reset_search();
  return -1000;
 }

 if (OneWire::crc8(addr, 7) != addr[7])
 {
  Serial.println("CRC is not valid!");
  return -1000;
 }

 if (addr[0] != 0x10 && addr[0] != 0x28)
 {
  Serial.print("Device is not recognized");
  return -1000;
 }

 ds.reset();
 ds.select(addr);
 ds.write(0x44, 1);

 byte present = ds.reset();
 ds.select(addr);
 ds.write(0xBE); 

 for (int i = 0; i < 9; i++)
 { 
  data[i] = ds.read();
 }

 ds.reset_search();

 byte MSB = data[1];
 byte LSB = data[0];

 float tempRead = ((MSB << 8) | LSB); 
 float TemperatureSum = tempRead / 16;

 return TemperatureSum;
}

void loop()
{
 unsigned long currentMillis = millis();

 if (currentMillis - previousMillis >= interval)
 {
  previousMillis = currentMillis; 

  temperature = getTemp();
  Serial.print("The current temperature is: ");
  Serial.print(temperature);
  Serial.println(" degrees.");
  Serial.print(receivedValue);

  if (!client.connected())
  {
    reconnect();
  }
  client.loop();

  if (temperature >= receivedValue)
  {
    char payload[10];
    dtostrf(temperature, 4, 2, payload);
    client.publish(mqttTopic, payload);
  }
 }
}

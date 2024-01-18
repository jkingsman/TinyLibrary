/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp32-web-server-microsd-card/

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include "FS.h"
#include "SD.h"
#include "esp32-hal-cpu.h"
#include "esp_wifi.h"

#include <WiFi.h>
#include <DNSServer.h>

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>


#include <Arduino.h>

// Replace with your network credentials
const char* ssid = "Free eBooks -- Connect to Read";
const char* password;

// set addressing
const IPAddress localIP(192, 168, 4, 1);
const IPAddress gatewayIP(192, 168, 4, 1);
const IPAddress subnetMask(255, 255, 255, 0);
const String localIPURL = "http://192.168.4.1/";

AsyncWebServer server(80);
DNSServer dnsServer;

void initSDCard(){
  if(!SD.begin()){
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
    Serial.println("No SD card attached");
    return;
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void initWiFi() {
  WiFi.mode(WIFI_MODE_AP);
  WiFi.softAPConfig(localIP, gatewayIP, subnetMask);
  WiFi.softAP(ssid, password);

  // Disable AMPDU RX on the ESP32 WiFi to fix a bug on Android per github.com/CDFER/Captive-Portal-ESP32
	esp_wifi_stop();
	esp_wifi_deinit();
	wifi_init_config_t my_config = WIFI_INIT_CONFIG_DEFAULT();
	my_config.ampdu_rx_enable = false;
	esp_wifi_init(&my_config);
	esp_wifi_start();
	vTaskDelay(100 / portTICK_PERIOD_MS);  // Add a small delay
  Serial.print("Hosting WIFI AP '");
  Serial.print(ssid);
  Serial.print("' with gateway at ");
  Serial.println(localIPURL);
}

void setUpDNSServer() {
	// Set the TTL for DNS response and start the DNS server
	dnsServer.setTTL(3600);
	dnsServer.start(53, "*", localIP);
  Serial.println("DNS server is up");
}

void setUpWebServer() {
  server.on("/connecttest.txt", [](AsyncWebServerRequest *request) { request->redirect("http://logout.net"); });	// windows 11 captive portal workaround
  server.on("/wpad.dat", [](AsyncWebServerRequest *request) { request->send(404); });								// Honestly don't understand what this is but a 404 stops win 10 keep calling this repeatedly and panicking the esp32 :)

  // github.com/CDFER/Captive-Portal-ESP32
  // Background responses: Probably not all are Required, but some are. Others might speed things up?
  // A Tier (commonly used by modern systems)
  server.on("/generate_204", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });		   // android captive portal redirect
  server.on("/redirect", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			   // microsoft redirect
  server.on("/hotspot-detect.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });  // apple call home
  server.on("/canonical.html", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });	   // firefox captive portal call home
  server.on("/success.txt", [](AsyncWebServerRequest *request) { request->send(200); });					   // firefox captive portal call home
  server.on("/ncsi.txt", [](AsyncWebServerRequest *request) { request->redirect(localIPURL); });			   // windows call home

  // 404 handler
	server.onNotFound([](AsyncWebServerRequest *request) {
		request->redirect(localIPURL);
	});

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SD, "/index.html", "text/html");
  });

  server.serveStatic("/", SD, "/");
  server.begin();

  Serial.println("Web server is up");
}

void setup() {
  Serial.setTxBufferSize(1024);
	Serial.begin(115200);

  while (!Serial);

  // do some power savings
  setCpuFrequencyMhz(80);
  btStop();
  Serial.println("Frequency clamped; bluetooth disabled");

  // boot the good stuff
  initWiFi();
  initSDCard();

  setUpDNSServer();
  setUpWebServer();

  Serial.println("Boot complete.");
}

void loop() {
	dnsServer.processNextRequest();
	delay(50);
}

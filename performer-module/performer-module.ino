// Synesthetic Performer Module
// Charles P. Martin 2017
// ESP8266 program to activate wearable lighting, gather movement data, and
// communicate via OSC with other performance systems. 

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <Adafruit_NeoPixel.h>

// Example of zeroconf service announcement:
// https://github.com/esp8266/Arduino/blob/master/libraries/ESP8266mDNS/examples/mDNS-SD_Extended/mDNS-SD_Extended.ino


char ssid[] = "deepthought";          // your network SSID (name)
char pass[] = "zarniwoop";                    // your network password

// A UDP instance to let us send and receive packets over UDP
WiFiUDP Udp;
const IPAddress outIp(10, 40, 10, 105);     // remote IP (not needed for receive)
char hostString[16] = {0};
const unsigned int localPort = 8888;        // local port to listen for UDP packets (here's where we send the packets)

#define NEOPIN D4 // USE D4 on d1minilite

// Parameter 1 = number of pixels in strip
// Parameter 2 = pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs) - ticks
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products) - tick
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, NEOPIN, NEO_GRB + NEO_KHZ800);

OSCErrorCode error;
unsigned int ledState = LOW;              // LOW means led is *on*

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, ledState);    // turn *on* led
  pixels.begin(); // This initializes the NeoPixel library.
  pixels.setBrightness(25);
  pixels.setPixelColor(0, 100, 0, 100);
  pixels.show();
  Serial.begin(115200);
  delay(100);

  sprintf(hostString, "ESP_%06X", ESP.getChipId());
  Serial.print("\nHostname: ");
  Serial.println(hostString);
  WiFi.hostname(hostString);

  // Connect to WiFi network
  Serial.print("\n\nConnecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());

  if (!MDNS.begin(hostString)) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started");
  MDNS.addService("osc", "udp", localPort); // Announce udp service on localPort
}

void led(OSCMessage &msg) {
  ledState = msg.getInt(0);
  digitalWrite(BUILTIN_LED, ledState);
  Serial.print("/led: ");
  Serial.println(ledState);
}

void pixel(OSCMessage &msg) {
  int r = msg.getInt(0);
  int g = msg.getInt(1);
  int b = msg.getInt(2);
  pixels.setBrightness(msg.getInt(3));
  pixels.setPixelColor(0, r, g, b); // Moderately bright green color.
  pixels.show(); // This sends the updated pixel color to the hardware.
  Serial.printf("/pixel: %d %d %d\n", r,g, b);
}

void loop() {
  OSCMessage msg;
  int size = Udp.parsePacket();

  if (size > 0) {
    while (size--) {
      msg.fill(Udp.read());
    }
    if (!msg.hasError()) {
      msg.dispatch("/pixel", pixel);
      msg.dispatch("/led", led);
    } else {
      error = msg.getError();
      Serial.print("error: ");
      Serial.println(error);
    }
  }
}




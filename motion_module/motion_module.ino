#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>
#include <SparkFun_ADXL345.h>         // SparkFun ADXL345 Library


ADXL345 adxl = ADXL345();             // USE FOR I2C COMMUNICATION
int interruptPin = D3;                 // Setup pin 2 to be the interrupt pin (for most Arduino Boards)

char ssid[] = "NETGEAR50";          // your network SSID (name)
char pass[] = "zanyocean960";                    // your network password

WiFiUDP Udp;
// 192.168.1.4 determinist
// 192.168.1.9 boot wifi
// 192.168.1.7 boot ethernet
// 192.168.1.6 heimskringla
const IPAddress outIp(192, 168, 1, 7);     // remote IP (not needed for receive)
const unsigned int outPort = 9999;          // remote port to receive OSC
char hostString[16] = {0};
const unsigned int localPort = 8888;        // local port to listen for UDP packets (here's where we send the packets)
OSCErrorCode error;

void setup(){
  Serial.begin(9600);                 // Start the serial terminal
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

  
  adxl.powerOn();                     // Power on the ADXL345

  adxl.setRangeSetting(16);           // Give the range settings
                                      // Accepted values are 2g, 4g, 8g or 16g
                                      // Higher Values = Wider Measurement Range
                                      // Lower Values = Greater Sensitivity

  adxl.setSpiBit(0);                  // Configure the device to be in 4 wire SPI mode when set to '0' or 3 wire SPI mode when set to 1
                                      // Default: Set to 1
                                      // SPI pins on the ATMega328: 11, 12 and 13 as reference in SPI Library 
   
  adxl.setActivityXYZ(1, 0, 0);       // Set to activate movement detection in the axes "adxl.setActivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setActivityThreshold(200);      // 62.5mg per increment   // Set activity   // Inactivity thresholds (0-255)
 
  adxl.setInactivityXYZ(1, 0, 0);     // Set to detect inactivity in all the axes "adxl.setInactivityXYZ(X, Y, Z);" (1 == ON, 0 == OFF)
  adxl.setInactivityThreshold(75);    // 62.5mg per increment   // Set inactivity // Inactivity thresholds (0-255)
  adxl.setTimeInactivity(10);         // How many seconds of no activity is inactive?

  adxl.setTapDetectionOnXYZ(0, 0, 1); // Detect taps in the directions turned ON "adxl.setTapDetectionOnX(X, Y, Z);" (1 == ON, 0 == OFF)
 
  // Set values for what is considered a TAP and what is a DOUBLE TAP (0-255)
  adxl.setTapThreshold(50);           // 62.5 mg per increment
  adxl.setTapDuration(15);            // 625 μs per increment
  adxl.setDoubleTapLatency(80);       // 1.25 ms per increment
  adxl.setDoubleTapWindow(100);       // 1.25 ms per increment
 
  // Set values for what is considered FREE FALL (0-255)
  adxl.setFreeFallThreshold(7);       // (5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(30);       // (20 - 70) recommended - 5ms per increment
 
  // Setting all interupts to take place on INT1 pin
  //adxl.setImportantInterruptMapping(1, 1, 1, 1, 1);     // Sets "adxl.setEveryInterruptMapping(single tap, double tap, free fall, activity, inactivity);" 
                                                        // Accepts only 1 or 2 values for pins INT1 and INT2. This chooses the pin on the ADXL345 to use for Interrupts.
                                                        // This library may have a problem using INT2 pin. Default to INT1 pin.
  
  // Turn on Interrupts for each mode (1 == ON, 0 == OFF)
  adxl.InactivityINT(1);
  adxl.ActivityINT(1);
  adxl.FreeFallINT(1);
  adxl.doubleTapINT(1);
  adxl.singleTapINT(1);
  
  pinMode(interruptPin, INPUT_PULLUP);
  //attachInterrupt(digitalPinToInterrupt(interruptPin), ADXL_ISR, RISING);   // Attach Interrupt

}

/****************** MAIN CODE ******************/
/*     Accelerometer Readings and Interrupt    */
void loop(){
  
  // Accelerometer Readings
  int x,y,z;   
  adxl.readAccel(&x, &y, &z);         // Read the accelerometer values and store them in variables declared above x,y,z
  /* UNCOMMENT TO VIEW X Y Z ACCELEROMETER VALUES */  
  // Serial.print(x);
  // Serial.print(", ");
  // Serial.print(y);
  // Serial.print(", ");
  // Serial.println(z); 

  OSCMessage msg("/accel");
  msg.add(x);
  msg.add(y);
  msg.add(z);
  Udp.beginPacket(outIp, outPort);
  msg.send(Udp);
  Udp.endPacket();
  msg.empty();
  
  ADXL_ISR();
}

/********************* ISR *********************/
/* Look for Interrupts and Triggered Action    */
void ADXL_ISR() {
  
  // getInterruptSource clears all triggered actions after returning value
  // Do not call again until you need to recheck for triggered actions
  byte interrupts = adxl.getInterruptSource();
  
  // Free Fall Detection
  if(adxl.triggered(interrupts, ADXL345_FREE_FALL)){
    Serial.println("*** FREE FALL ***");
    //add code here to do when free fall is sensed
  } 
  
  // Inactivity
  if(adxl.triggered(interrupts, ADXL345_INACTIVITY)){
    Serial.println("*** INACTIVITY ***");
     //add code here to do when inactivity is sensed
  }
  
  // Activity
  if(adxl.triggered(interrupts, ADXL345_ACTIVITY)){
    Serial.println("*** ACTIVITY ***"); 
     //add code here to do when activity is sensed
  }
  
  // Double Tap Detection
  if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)){
    Serial.println("*** DOUBLE TAP ***");
    OSCMessage msg("/doubletap");
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
  }
  
  // Tap Detection
  if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
    Serial.println("*** TAP ***");
    OSCMessage msg("/tap");
    Udp.beginPacket(outIp, outPort);
    msg.send(Udp);
    Udp.endPacket();
    msg.empty();
  } 
}


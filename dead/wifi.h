

#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#define HOSTNAME "LineStorm"
#define PASSWORD "phage4life"

IPAddress ip(192,168,4,1);

//========================================================================== UDP / OSC
// A UDP instance to let us send and receive packets over UDP
#include <WiFiUdp.h>
WiFiUDP Udp;
//const IPAddress outIp(192,168,1,35);  // remote IP (not needed for receive)

#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

#define OSC_PORT_OUT 9999
#define OSC_PORT_IN  8888
OSCErrorCode error;



void setupWiFi() {
  WiFi.mode(WIFI_AP);
  //char* AP_NameString = "Wave";
  //WiFi.softAP(HOSTNAME, WiFiAPPSK);
  
  boolean result = WiFi.softAP(HOSTNAME, PASSWORD);
  if(result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }
 
  //---------------------------------------------------------------------------------- UDP
  Serial.println("Starting UDP");
  Udp.begin(OSC_PORT_IN);
  Serial.print("Local port: ");
  #ifdef ESP32
    Serial.println(OSC_PORT_IN);
  #else
    Serial.println(Udp.localPort());
  #endif  
    

  
  Serial.printf("Connect to Wi-Fi access point: %s\n", HOSTNAME);
  Serial.println("and open http://192.168.4.1 in your browser");
}






void setThrottleLeftOSC(OSCMessage &msg) {Line::setThrottleLeft(msg.getFloat(0));}
void setThrottleRightOSC(OSCMessage &msg) {Line::setThrottleRight(msg.getFloat(0));}

void setHuePerOSC(OSCMessage &msg) { Strobe::setHuePer(msg.getFloat(0)); }
void setSatPerOSC(OSCMessage &msg) { Strobe::setSatPer(msg.getFloat(0)); }
void setBrtPerOSC(OSCMessage &msg) { Strobe::setBrtPer(msg.getFloat(0)); }

void setHueFrqOSC(OSCMessage &msg) { Strobe::setHueFrq(msg.getFloat(0)); }
void setSatFrqOSC(OSCMessage &msg) { Strobe::setSatFrq(msg.getFloat(0)); }
void setBrtFrqOSC(OSCMessage &msg) { Strobe::setBrtFrq(msg.getFloat(0)); }

void setHueModOSC(OSCMessage &msg) { Strobe::setHueMod(msg.getFloat(0)); }
void setSatModOSC(OSCMessage &msg) { Strobe::setSatMod(msg.getFloat(0)); }
void setBrtModOSC(OSCMessage &msg) { Strobe::setBrtMod(msg.getFloat(0)); }


void checkOSC() {

  OSCBundle bundle;
  std::string buf = "";
  int size = Udp.parsePacket();
  if (size == 0) { 
    //Serial.println("Zero size OSC Packet");
    return; 
  }
  Serial.printf("Reading %d bytes: ", size);
  while (size--) {
    uint8_t c = Udp.read();
    buf += c;
    Serial.printf(" %X ", c);
    bundle.fill(c); 
    if (bundle.hasError()) {
      Serial.print("x");
    }
  }
  Serial.println();
  
  if (!bundle.hasError()) {}
    Serial.printf("OSC: %s\n", buf.c_str());
    bundle.dispatch("/ttl",           setThrottleLeftOSC);
    bundle.dispatch("/ttr",           setThrottleRightOSC);
    bundle.dispatch("/hf",            setHueFrqOSC);
    bundle.dispatch("/ht",            setHuePerOSC);
    bundle.dispatch("/hs",            setHueModOSC);
    bundle.dispatch("/sf",            setBrtFrqOSC);
    bundle.dispatch("/st",            setBrtPerOSC);
    bundle.dispatch("/sd",            setBrtModOSC);
    //bundle.dispatch("/palette",       setPalette);
    //bundle.dispatch("/paletteDown",   setPaletteDown);
    //bundle.dispatch("/paletteUp",     setPaletteUp);
    //bundle.dispatch("/csync",         setHueSync);
  /*} else {
    error = bundle.getError();
    Serial.printf("OSC: %s\n", buf.c_str());
    Serial.print("error:  ");
    Serial.println(error);
  }*/
}

void sendStateOSC() {}

#define HOSTNAME "LineStorm"
#define PASSWORD "phage4life"


#define CONFIG_PORTAL_TIMEOUT 30

#include <WiFi.h>          
#include <WiFiClient.h>
#include <WiFiAP.h>

WiFiServer server(80);

IPAddress ip(192,168,4,1);

#define LED_BUILTIN 2

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
  /*
  oled.setConnected(false);
  oled.refreshIcons();
  oled.clearMsgArea();
  oled.println("Scanning ...");
  oled.display();
  */

  
  //WiFi.mode(WIFI_AP);
  //WiFi.config(ip);
  WiFi.softAP(HOSTNAME, PASSWORD);
  WiFi.softAPConfig(ip, ip, IPAddress(255,255,255,0));
  Serial.printf("Running in Stand-Alone Mode\n");
  Serial.printf("Connect to Wi-Fi access point: %s\n", HOSTNAME);
  Serial.println("and open http://192.168.4.1 in your browser");    
  
  /*
  oled.setConnected(false);
  oled.refreshIcons();
  oled.clearMsgArea();
  oled.println("Scan Failed!");
  oled.print("Hosting: ");
  oled.println(wifiManager.getConfigPortalSSID());
  oled.display();
  */

  /*
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  */


  //---------------------------------------------------------------------------------- UDP
  Serial.println("Starting UDP");
  Udp.begin(OSC_PORT_IN);
  Serial.print("Local port: ");
  #ifdef ESP32
    Serial.println(OSC_PORT_IN);
  #else
    Serial.println(Udp.localPort());
  #endif  
    
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


void setPalette(int a) {
  int b;
  if (a <= 0) { b = gCurrentPaletteNumber - 1; }
  else        { b = gCurrentPaletteNumber + 1; }
  changePalette(b);
}


void setPalette(    OSCMessage &msg) { setPalette(msg.getInt(0)); }
void setPaletteDown(OSCMessage &msg) { setPalette(0); }
void setPaletteUp(  OSCMessage &msg) { setPalette(1); }

/*
void setHueSync(bool a) {
  hueSync = a;
  setHueFreq(0.0);
}
void setHueSync(OSCMessage &msg) {
  setHueSync(msg.getInt(0) == 1);
}
*/

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
    Serial.printf("OSC: %s\n\n", buf.c_str());
    bundle.dispatch("/ttl",           setThrottleLeftOSC);
    bundle.dispatch("/ttr",           setThrottleRightOSC);
    bundle.dispatch("/hf",            setHueFrqOSC);
    bundle.dispatch("/ht",            setHuePerOSC);
    bundle.dispatch("/hs",            setHueModOSC);
    bundle.dispatch("/sf",            setBrtFrqOSC);
    bundle.dispatch("/st",            setBrtPerOSC);
    bundle.dispatch("/sd",            setBrtModOSC);
    bundle.dispatch("/palette",       setPalette);
    bundle.dispatch("/paletteDown",   setPaletteDown);
    bundle.dispatch("/paletteUp",     setPaletteUp);
    //bundle.dispatch("/csync",         setHueSync);
  /*} else {
    error = bundle.getError();
    Serial.printf("OSC: %s\n", buf.c_str());
    Serial.print("error:  ");
    Serial.println(error);
  }*/
}

void sendStateOSC() {}

/*
 * An example that demonstrates most capabilities of Espalexa v2.4.0
 */ 
#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
//#define ESPALEXA_ASYNC            //uncomment for async operation (can fix empty body issue)
//#define ESPALEXA_NO_SUBPAGE       //disable /espalexa status page
#define ESPALEXA_DEBUG            //activate debug serial logging
//#define ESPALEXA_MAXDEVICES 15    //set maximum devices add-able to Espalexa
#include <Espalexa.h>

#include <IRremoteESP8266.h>
#include <IRsend.h>

#include <credentials.h>
#include <ir_codes.h>


const char* ssid = WIFI_SSID;
const char* password = WIFI_PASS;

const uint16_t kIrLed = 4;  // ESP8266 GPIO pin to use. Recommended: 4 (D2).
IRsend irsend(kIrLed);  // Set the GPIO to be used to sending the message.

// global variables
bool wifiConnected = false;

Espalexa espalexa;

uint8_t lastVol;
bool volReset = false;
uint32_t volResetTimer;


// preallocate functions
bool connectWifi();


//callback functions
void handleStereo(EspalexaDevice* dev);
void handleStereo_volume(EspalexaDevice* dev);


//create devices yourself
EspalexaDevice* Stereo_volume;



void setup()
{
  Serial.begin(115200);
  irsend.begin();
  // Initialise wifi connection
  wifiConnected = connectWifi();
  
  if(!wifiConnected){
    while (1) {
      Serial.println("Cannot connect to WiFi. Please check data and reset the ESP.");
      delay(2500);
    }
  }
  
  // Define your devices here. 

  espalexa.addDevice("Stereoanlage", handleStereo, EspalexaDeviceType::onoff);

  Stereo_volume = new EspalexaDevice("Sony Lautstärke", handleStereo_volume, EspalexaDeviceType::dimmable, 127); // Lautstärke
  espalexa.addDevice(Stereo_volume);

  Stereo_volume->setPercent(50);
  lastVol = 50;


  espalexa.begin();
}
 
void loop()
{
  espalexa.loop();
  delay(1);

  if ((millis() > volResetTimer + 5000) & volReset) {
    Serial.println("Volume Reset");
    Stereo_volume->setPercent(50);
    lastVol = 50;
    volReset = false;
  }
}

//our callback functions

//void handleTV_main(EspalexaDevice* d) {
//  if (d == nullptr) return; //this is good practice, but not required
//
//  bool status = d->getValue();
//
//  switch (d->getId()) {
//    case 0:   // TV on/off
//      if(status) {
//        irsend.sendPronto(powON, 104);
//      }
//      else
//      {
//        irsend.sendPronto(powOFF, 104);
//      }
//      
//      Serial.printf("TV switched %s \n",status ? "ON" : "OFF");
//      break;
//
//    case 1:   // HDMI 1
//      if(status) {
//        irsend.sendPronto(HDMI1, 104);
//      }
//
//      Serial.printf("HDMI 1 switched %s \n",status ? "ON" : "OFF");
//      break;
//    case 2:   // HDMI 2
//      if(status) {
//        irsend.sendPronto(HDMI2, 104);
//      }
//
//      Serial.printf("HDMI 2 switched %s \n",status ? "ON" : "OFF");
//      break;
//    case 3:   // HDMI 3
//      if(status) {
//        irsend.sendPronto(HDMI3, 104);
//      }
//
//      Serial.printf("HDMI 3 switched %s \n",status ? "ON" : "OFF");
//      break;
//    case 4:   // Media Playback
//       if(status) {
//        irsend.sendPronto(mediaPlay, 104);
//      }
//      else
//      {
//        irsend.sendPronto(mediaPause, 104);
//      }
//
//      Serial.printf("Media Playback switched %s \n",status ? "ON" : "OFF");
//      break;
//    default: // Default
//      Serial.println("Device not defined");
//      break;
//  }
//}


void handleStereo_volume(EspalexaDevice* d) {
  if (d == nullptr) return; //this is good practice, but not required

  uint8_t value = d->getValue();
  uint8_t percent = round(value/255.0*100.0);

  if (percent - lastVol > 0) {
    for (int i = 1; i <= abs(percent - lastVol); i++) {
      irsend.sendSony(0x481, 12, 2);
      Serial.println(i);
      delay(200);
    }
  }
  else if (percent - lastVol < 0){
    for (int i = 1; i <= abs(percent - lastVol); i++) {
      irsend.sendSony(0xc81, 12, 2);
      Serial.println(i);
      delay(200);
    }
  }

  Serial.printf("Changed volume to %i %% \n", percent);
  Serial.printf("Value %i \n", value);

  volReset = true;
  volResetTimer = millis();
  lastVol = percent;
}

void handleStereo(EspalexaDevice* d) {
  if (d == nullptr) return; //this is good practice, but not required

  bool status = d->getValue();

  if(status) {
    irsend.sendSony(0xa81, 12, 2);    // Lautsprecher einschalten
    delay(700);

    irsend.sendPronto(AUX, 30,2);
  }
  else
  {
    irsend.sendSony(0xa81, 12, 2);    // Lautsprecher ausschalten
  }


  Serial.printf("Stereo switched %s \n",status ? "ON" : "OFF");

}



// connect to wifi – returns true if successful or false if not
bool connectWifi(){
  bool state = true;
  int i = 0;
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20){
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state){
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println();
  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}
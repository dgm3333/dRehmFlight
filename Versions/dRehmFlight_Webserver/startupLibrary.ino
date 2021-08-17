#ifdef ESP

#include <ArduinoOTA.h>   // required for OTA updates

const char* ssid = "BTHub6-FZPS"; //replace this with your WiFi network name
const char* password = "RKfMeQHPngN9"; //replace this with your WiFi network password
void startWiFi() {
  Serial.println("Activating Wifi");

//  WiFi.persistent(false);
//  WiFi.mode(WIFI_STA);      // This is station mode (can connect to another server/router
  WiFi.begin(ssid,password); //WiFi connection
  Serial.print("Connecting");    
  while (WiFi.status() != WL_CONNECTED) { //Wait for the WiFI connection completion
    delay(500);
    Serial.print(".");    
  }
//  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
//    Serial.println("Connection Failed! Rebooting...");
//    delay(5000);
//    ESP.restart();
//  }

  
  Serial.print("\nConnected");
//  client.connect(server, 80);   // Connection to the server
//  while (WiFi.status() != WL_CONNECTED) { //Wait for the WiFI connection completion
//    delay(500);
//    Serial.print(".");    
//  }
  Serial.println("");

  Serial.print("Local IP address: ");
  Serial.println(WiFi.localIP());  //Print the local IP

}

void startOTA () {

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);




  // Hostname defaults to esp8266-[ChipID]
//  String hostName = sketchName();
//  hostName = hostName + (String)ESP.getEfuseMac(); // chipID
//  Serial.println("Using " + hostName + " for OTA port");
//  ArduinoOTA.setHostname("ESP8622");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_SPIFFS
      type = "filesystem";
    }

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready\r\n");
}

#endif


/*
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}
*/


String sketchName(String the_path) {
  int slash_loc = the_path.lastIndexOf('/');
  String the_cpp_name = the_path.substring(slash_loc+1);
  int dot_loc = the_cpp_name.lastIndexOf('.');
  String the_sketchname = the_cpp_name.substring(0, dot_loc);
//  Serial.println("sketchName =" + the_sketchname);
  
  return the_sketchname;
}

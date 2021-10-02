/*********
    2021 - David Mckenzie
    dRehmFlight_Webserver

    This code can be run on a temporarily connected ESP8266 or ESP32 (or other wifi enabled devboard) to provide 
    an on-the-fly method of changing settings in dRehmFlight.
    You will need to connect the client device (phone/tablet/pc) wifi to the "dRehmFlight webserver" hotspot to link to it

    The board running the webserver can be disconnected when the settings are finalised
    although dRehmFlight does NOT save these settings beyond reboot, so you will have to manually record them and 
    update/reflash the sketch when you have identified you desired settings.

    To remove all webserver code from the dRehmFlight sketch just comment out SERIAL_WEBSERVER in that sketch

    To add a new modifiable dRehmFlight variable you will need to
    1. In the dRehmFlight sketch
        a. Ensure the variable is global
        b. In the setupFromSerial() routine add appropriate variable update options (GET,SET,PLUS,MINUS)

    2. In the dRehmFlight_Webserver sketch
        a. Edit handleRoot() under ADD NEW BUTTONS add appropriate variable options (GET,SET,PLUS,MINUS)

NB Depending on your ESP32 type, you may have to hold down the boot button on the ESP32 to upload your sketch

To connect to the ESP32 webserver from your phone/tablet or PC
// Search in your wifi settings for "dRehmFlight Webserver" and connect to it
// Then in your browser type the root IP address: 192.168.1.1


//Hardware Connections (Serial pins)
//Teensy has numerous Serial/UART pin options https://www.pjrc.com/teensy/td_uart.html 
// By default dRehmFlight uses the pins for Serial 1-2 for motor and servos, Serial 3-5 for PWM channels and Serial5 for SBUS
// I use SBUS so use Serial3 for the webserver connections
// If you use PWM then just do a global replace of Serial3 for (eg) Serial6 in the Teensy sketch and use the appropriate pins
// ESP32 has 3 ports available: https://circuits4you.com/2018/12/31/esp32-hardware-serial2-example/
ESP - Teensy
TX2 -    RX3        - Teensy can also use Rx6, RX7 or RX8. Teensy Pins: RX3=Pin15, 6=25, 7=28, 8=34
RX2 -    TX3        - Teensy can also use RX6, RX7 or RX8. Teensy Pins: TX3=Pin14, 6=24, 7=29, 8=35


TODO:
Code the client to hold the connection open even when the webserver is out of range, but to display an error to indicate this is the case.

*********/

#define ESP32             // just comment this out if using ESP8266
//#define SOFTAP            // soft Access Point - enables connection without a wireless router, but is less stable
#define WEBSERVER_SERIAL_SPEED 115000
#define WEBSERVER_PING_RETRIES 50             // delay of 100ms between retries
#define MAX_DREHM_VARIABLES 100

#define U2_RXD 25     // GPIO16
#define U2_TXD 27     // GPIO17

#include <Arduino.h>

// Load Wi-Fi library
#ifdef ESP32
#include <WiFi.h>
#include <WebServer.h>
// WiFi config
const char *SSID = "BTHub6-FZPS";
const char *PWD = "RKfMeQHPngN9";

//AsyncWebServer requires the following libraries.
// Download them using the links then install from Arduino under Sketch > Include Library > Add .zip Library
//https://github.com/me-no-dev/ESPAsyncWebServer/archive/master.zip
//https://github.com/me-no-dev/AsyncTCP/archive/master.zip
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>


//#include <ArduinoOTA.h>     // required for OTA updates (but this sketch is too big on an ESP8266

#else
#include <ESP8266WiFi.h>
#include <WiFiServer.h>
#endif


// Replace with your network credentials
const char* ssid         = "dRehmFlight Webserver";
const char* password = "123456789";

// Create AsyncWebServer object on port 80
//AsyncWebServer server(80);

/* Put IP Address details */
IPAddress local_IP(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,0,0);

WebServer server(80);

// Variable to store the HTTP request
String header;
String message;

char dRehmVarType[MAX_DREHM_VARIABLES];
String dRehmVarName[MAX_DREHM_VARIABLES];
int dRehmIntValues[MAX_DREHM_VARIABLES];
float dRehmFloatValues[MAX_DREHM_VARIABLES];
float dRehmIncrementValues[MAX_DREHM_VARIABLES];
String dRehmStrValues[MAX_DREHM_VARIABLES];
int dRehmVariableCnt = 0;
bool webserverOK = false;

void handleRootTst() {
    server.send(200, "text/html", "<h1>You are connected</h1>");
}

// Create the html for a webpage which is served to anything connecting to the url
void handleRoot() {
    
    message = "";

    // This is standard html response - if you don't return status 200 (ie OK) then the flutter app will thow an exception
    //     and the returned json won't be parsed
//    message += "HTTP/1.1 200 OK";
//    message += "Content-Type: text/html";
//    message += "Connection: close\n";

    // <--- This is the html header code - Apart from the title, you probably won't need to change this --->
    message += "<!DOCTYPE html>\n";
    message += "<html lang=\"en\">\n";
    message += "    <head>\n";
    message += "        <!-- Required meta tags -->\n";
    message += "        <meta charset=\"utf-8\">\n";
    message += "        <meta name=\"viewport\" content=\"width=device-width, initial-scale=1, shrink-to-fit=yes\">\n";
    //    message += "        <!-- Bootstrap CSS -->";
    // bootstrap has a number of icons and useful snippets. These should be cached after first load so 
    // should still function even if the phone/tablet/pc subsequently goes offline. 
    message += "        <link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\" integrity=\"sha384-ggOyR0iXCbMQv3Xipma34MD+dH/1fQ784/j6cY/iJTQUOhcWr7x9JvoRxT2MZw1T\" crossorigin=\"anonymous\">\n";
    // if you want to include specific javascript widgets, you can put them here (eg this is a colour picker from another project)
    //message += "        <script src=\"https://cdn.jsdelivr.net/npm/@jaames/iro/dist/iro.min.js\"></script>\n";
    // a widget will also need <script> code included (eg see https://github.com/jaames/iro.js for how this one would work)
    
    message += "        <title>" + sketchName(__FILE__) + "</title>\n";
    message += "    </head>\n";
    message += "    <body>\n";
    // <--- end of the header code --->


    // <--- start of the text and buttons on the webpage --->
    // This code is provided to be run by the client phone/tablet/pc
    // when a button is clicked the client sends the html string to the webserver (running this sketch) which will then
    // parse and appropriate process it.
    // the javascript function bClk(btn) will request the "page" at \btn of the webserver
    // When received by the webserver this requested can then be used to trigger whichever desired sketch function
    // is defined in void setup(). However at a minimum that function should return a plaintext 'OK' response eg:
    //        server.send(200, "text/plain", "OK");             //Response to the HTTP request
    //    You are unlikely to want to include the full IP address to btnClk. However
    //    if full IP desired then insert <a href=\"http://" + WiFi.localIP().toString() + "
    // The following could be optomised to decrease bandwidth, but for sake of readability has not been.


    // <--- ADD NEW BUTTONS --->
    message += "        <p>Please select a function below.</p>\n";
    message += "     <div\">\n";
    for (int i = 0; i < dRehmVariableCnt; i++) {
        if (dRehmVarType[i] == 'D') {                // end the div and add the "variable name" as the text to display
            message += "     </div>\n";                
            message += "     <div\">\n";
            message += "        <p>" + dRehmVarName[i] + "</p>\n";
        } else if (dRehmVarType[i] == 'c') { 
            message += "        <p>" + dRehmVarName[i] + ":\n";
            message += "        document.getElementById('" + dRehmVarName[i] + "').innerHTML = " + dRehmStrValues[i] + "\n";
            message += "        <button onclick=\"bClk('GET', " + String(i) + ")\" class=\"btn btn-light center-block\">GET</button>\n";
            message += "        <button onclick=\"bClk('SET', " + String(i) + ", document.getElementById('" + dRehmVarName[i] + "').value)\" class=\"btn btn-dark center-block\">SET</button>\n";
            message += "        <input type='text' id='" + dRehmVarName[i] + "' value=document.getElementById('" + dRehmVarName[i] + "').value>\n";
        } else if (dRehmVarType[i] == 'i' or dRehmVarType[i] == 'f') { 
            message += "        <p>" + dRehmVarName[i] + ":\n";
            message += "        document.getElementById('" + dRehmVarName[i] + "').innerHTML = " + dRehmFloatValues[i] + "\n";
            message += "        <button onclick=\"bClk('GET', " + String(i) + "'G')\" class=\"btn btn-light center-block\">GET</button>\n";
            message += "        <button onclick=\"bClk('SET', " + String(i) + ", document.getElementById('" + dRehmVarName[i] + "').value)\" class=\"btn btn-dark center-block\">SET</button>\n";
            message += "        <input type='text' id='" + dRehmVarName[i] + "' value=document.getElementById('" + dRehmVarName[i] + "').value>\n";
            message += "        <button onclick=\"bClk('PLUS', " + String(i) + ", '+')\" class=\"btn btn-success center-block\">+" + dRehmIncrementValues[i] + "</button>\n";
            message += "        <button onclick=\"bClk('MINUS', " + String(i) + ", '-')\" class=\"btn btn-success center-block\">-" + dRehmIncrementValues[i] + "</button>\n";
            message += "        </p></br>\n";
        }
    }
    message += "        </br></br>\n";
    message += "     </div>\n";

    message += "        <p id=\"var1\">var1</p>\n";
    message += "        <p id=\"var2\">var2</p>\n";
    message += "        <p id=\"var3\">var3</p>\n";
    message += "\n";
    message += "        <script>\n";
                                        // This is a basic debounce : if the interval between button clicks is too short then ignore them (
    message += "            var oldTime = new Date().getTime();\n";
    message += "            var nextInterval = 0;\n";
    message += "\n";
    message += "            // buttonClick event\n";
    message += "            function bClk(btn, argID, argVal='') {\n";
    message += "                var now = new Date().getTime();\n";
    message += "                var interval = now - oldTime;\n";             // Find the interval between now and the count down date
    message += "                oldTime = now;\n";
    message += "                if (interval < nextInterval) return;\n";
    message += "                nextInterval = 1000;\n";
    message += "\n";
    // end of debounce code
                                        
    message += "                var url = \"/btn?id=argID?val=argVal\";\n";

    message += "                var request = new XMLHttpRequest();\n";
    message += "                request.open('GET', url, true);\n";
    message += "                request.onload = function() { if (request.status == 200) nextInterval = 0; };\n";
    message += "                request.onerror = function() { /* request failed */ };\n";
    message += "                request.send(new FormData(event.target)); // create FormData from form that triggered event\n";
    message += "                event.preventDefault();\n";
    message += "            }\n";
    message += "\n";
    message += "\n";
    message += "        </script>\n";
    message += "    </body>\n";
    message += "</html>\n";

    server.send(200, "text/html", message);             //Response to the HTTP request
}



// Callback when a 'Val' button is pushed
void handleGET() {
    Serial2.print("G");
    Serial2.print(server.arg("id"));     // This is the variable name
    Serial2.print("=");
    Serial2.print('\n');                        // End the packet
    handleRoot();
    //server.send(200, "text/plain", "OK");             //Response to the HTTP request
}
void handleSET() {
    Serial2.print("S");
    Serial2.print(server.arg("id"));     // This is the variable name
    Serial2.print("=");
    Serial2.print(server.arg("val"));     // This is the value to update it with
    Serial2.print('\n');                        // End the packet
    handleRoot();
//    server.send(200, "text/plain", "OK");             //Response to the HTTP request
}
void handlePLUS() {
    Serial2.print("+");
    Serial2.print(server.arg("id"));     // This is the variable name
    Serial2.print("=");
    Serial2.print(server.arg("val"));     // This is the value to update it with
    Serial2.print('\n');                        // End the packet
    handleRoot();
//    server.send(200, "text/plain", "OK");             //Response to the HTTP request
}
void handleMINUS() {
    Serial2.print("-");
    Serial2.print(server.arg("var"));     // This is the variable name
    Serial2.print("=");
    Serial2.print(server.arg("val"));     // This is the value to update it with
    Serial2.print('\n');                        // End the packet
    handleRoot();
//    server.send(200, "text/plain", "OK");             //Response to the HTTP request
}


// This listens to the Teensy and sets up the buttons for the variables it requests
void linkToTeensy() {
        byte serialBuffer[255];
        int endChr = 0;
        String valStr = "";        
        int pingCount = 0;
        webserverOK = true;
        Serial.println("Attempting to link to Teensy");
        Serial.print("Pinging: ");
        while (Serial2.read() != '.') {
                Serial.print('.');
                Serial2.print('.');
                pingCount++;
                if (pingCount > WEBSERVER_PING_RETRIES) {
                        webserverOK = false;
                        Serial.println();
                        Serial.println("Failed to connect to dRehmFlight Teensy via Serial2. Please check connections");
                        return;
                }
                delay(500);         // wait 100ms before next try
        }
        Serial.println(" Response Recieved...");
        while (Serial2.read() != '\n') delay(10);
        while (true) {
                while (Serial2.available() < 1) delay(10);            // wait until there is at least one byte to be read 
                dRehmVarType[dRehmVariableCnt] = Serial2.read();
                if (dRehmVarType[dRehmVariableCnt]    == 'X')
                        break;     // we're done with setup
                char c = '\0';
                bool commaNotFound = true;

                while (true) {             
                        while (Serial2.available() < 1) delay(10);
                        if (c == '\n') {
                                break;
                        } else if (c == ',') {
                                commaNotFound = false;
                        } else if (commaNotFound) {
                                dRehmVarName[dRehmVariableCnt] += c;
                        } else {
                                valStr += c;
                        }
                }
        }
        if (dRehmVarType[dRehmVariableCnt] == 'i') {
                dRehmFloatValues[dRehmVariableCnt] = valStr.toFloat();            // don't think any benefit in using dRehmIntValues
        } else if (dRehmVarType[dRehmVariableCnt] == 'f') {
                dRehmFloatValues[dRehmVariableCnt] = valStr.toFloat();
        } else {
                dRehmStrValues[dRehmVariableCnt] = valStr;
        }
        dRehmVariableCnt++;
}

void connectToWiFi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);
  
  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    // we can even make the ESP32 to sleep
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
}

void setup() {
    Serial.begin(115200);
    Serial2.begin(WEBSERVER_SERIAL_SPEED, SERIAL_8N1, U2_RXD, U2_TXD);


    // Connect to Wi-Fi network with SSID and password
    Serial.println();
    Serial.print("Initialising ");
    Serial.println(ssid);
    #ifdef SOFTAP
        WiFi.softAP(ssid, password);
            // Configures static IP address
        if (!WiFi.config(local_IP, gateway, subnet)) {        //, primaryDNS, secondaryDNS)) {
            Serial.println("STA Failed to configure");
        }
    #else
        connectToWiFi();
    #endif
    // Print local IP address and start web server
    Serial.println();
    Serial.print(ssid); Serial.println(" should now be visible to connect to in your phone/tablet/pc wifi network list");
    Serial.print("Current password is "); Serial.println(password);
    Serial.println();

    // HTTP server setup
    Serial.println("Starting HTTP server using IP address: ");
    Serial.println(local_IP);
    Serial.println("Enter this IP address in your browser's address bar");
    
    // <--- SET UP SERVER HANDLERS --->>>
    server.on("/", handleRoot);
    server.on("/GET", handleGET); //Associate the handler function to the path
    server.on("/SET", handleSET); //Associate the handler function to the path
    server.on("/PLUS", handlePLUS); //Associate the handler function to the path
    server.on("/MINUS", handleMINUS); //Associate the handler function to the path
//    server.on("/test", handleTest); //Associate the handler function to the path

    server.begin(); //Start the server
    Serial.println("Server now live and listening for clients on:");
    Serial.println(WiFi.localIP());
    
    linkToTeensy();
}

void loop(){
    server.handleClient();
}

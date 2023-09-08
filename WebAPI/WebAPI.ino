#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>
#include <DHT.h>

#define LEDPIN 2
#define DHTPIN 4
#define DHTTYPE DHT11

String deviceParameters[] = {"temp", "led"};
WebServer server(80);

struct ResponseStruct{
  int statusCode;
  String device;
  String parameter;
  String value;
};

DynamicJsonDocument GetResponseJsonDoc (ResponseStruct resData){
  DynamicJsonDocument doc(512);

  doc["statusCode"] = resData.statusCode;
  doc["parameter"] = resData.parameter;
  doc["value"] = resData.value;

return doc;
}


//Check if header is present and correct
bool is_authentified() {
  Serial.println("Enter is_authentified");
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
    if (cookie.indexOf("ESPSESSIONID=1") != -1) {
      Serial.println("Authentification Successful");
      return true;
    }
  }
  Serial.println("Authentification Failed");
  return false;
}

//login page, also called for disconnect
void handleLogin() {
  String msg;
  if (server.hasHeader("Cookie")) {
    Serial.print("Found cookie: ");
    String cookie = server.header("Cookie");
    Serial.println(cookie);
  }
  if (server.hasArg("DISCONNECT")) {
    Serial.println("Disconnection");
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.sendHeader("Set-Cookie", "ESPSESSIONID=0");
    server.send(301);
    return;
  }
  if (server.hasArg("USERNAME") && server.hasArg("PASSWORD")) {
    if (server.arg("USERNAME") == "admin" &&  server.arg("PASSWORD") == "admin") {
      server.sendHeader("Location", "/");
      server.sendHeader("Cache-Control", "no-cache");
      server.sendHeader("Set-Cookie", "ESPSESSIONID=1");
      server.send(301);
      Serial.println("Log in Successful");
      return;
    }
    msg = "Wrong username/password! try again.";
    Serial.println("Log in Failed");
  }
  String content = "<html><body><form action='/login' method='POST'>To log in, please use : admin/admin<br>";
  content += "User:<input type='text' name='USERNAME' placeholder='user name'><br>";
  content += "Password:<input type='password' name='PASSWORD' placeholder='password'><br>";
  content += "<input type='submit' name='SUBMIT' value='Submit'></form>" + msg + "<br>";
  content += "You also can go <a href='/inline'>here</a></body></html>";
  server.send(200, "text/html", content);
}

//root page can be accessed only if authentification is ok
void handleRoot() {
  Serial.println("Enter handleRoot");
  String header;

  // Redirect on no login
  if (!is_authentified()) {
    server.sendHeader("Location", "/login");
    server.sendHeader("Cache-Control", "no-cache");
    server.send(301);
    return;
  }
  String content = "<html><body><H2>hello, you successfully connected to esp8266!</H2><br>";
  if (server.hasHeader("User-Agent")) {
    content += "the user agent used is : " + server.header("User-Agent") + "<br><br>";
  }
  content += "You can access this page until you <a href=\"/login?DISCONNECT=YES\">disconnect</a></body></html>";
  server.send(200, "text/html", content);
}

//no need authentification
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//
// sending results to client(mobile)
void handleFetch(){
  DynamicJsonDocument reqDataDoc = ParseReqData();
  String parameter = reqDataDoc["parameter"];

  String jsonRes = "";
  if(parameter == "temp"){
    jsonRes = ParseResJson(parameter, GetTemp());
  }else if(parameter == "led"){
    String ledStatus = "";

    if(digitalRead(LEDPIN) == HIGH){
      ledStatus = "on";
    }else{
      ledStatus = "off";
    }
    jsonRes = ParseResJson(parameter, ledStatus);
  }

  server.send(200,"application/json",jsonRes);
}

// Updating the device states on client(mobile) call
void handleUpdate(){
  DynamicJsonDocument reqDataDoc = ParseReqData();

  String parameter = reqDataDoc["parameter"];
  String value = reqDataDoc["value"];
  value.toLowerCase();

  if(parameter == "temp"){
    server.send(400, "application/json", "Temperature is read only");
    return;
  }else if(parameter == "led"){
    if(value != "on" && value != "off") {
      server.send(400, "application/json", "value should be on OR off");
      return;
    }
    digitalWrite(LEDPIN, (value == "on"));
    server.send(200,"application/json","LED turned ON");
  }
}









float GetTemp(){
  DHT dht(DHTPIN, DHTTYPE);
  dht.begin();
  float t = dht.readTemperature();
  return t;
}

void SetLed(bool state){
  if(state){
    digitalWrite(LEDPIN, HIGH);
  }else{
    digitalWrite(LEDPIN, LOW);
  }
}

DynamicJsonDocument ParseReqData(){
  String data = server.arg("plain");
  DynamicJsonDocument doc(512);
  deserializeJson(doc, data);

  return doc;
}

String ParseResJson(String parameter, String value){
  DynamicJsonDocument doc(512);

  doc["parameter"] = parameter;
  doc["value"] = value;

  String jsonRes ="";
  serializeJson(doc, jsonRes);

  return jsonRes;
}

String ParseResJson(String parameter, float value){
  DynamicJsonDocument doc(512);

  doc["parameter"] = parameter;
  doc["value"] = value;

  String jsonRes ="";
  serializeJson(doc, jsonRes);

  return jsonRes;
}















void setup(void) {
  Serial.begin(115200);
  
  pinMode(LEDPIN, OUTPUT);
  
  // WiFi Manager settings
  WiFiManager wm;
  wm.resetSettings(); // Only for testing. Comment this in production

  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  bool res = wm.autoConnect("IoT_Wifi_config_AP"); // password protected ap

  if(!res) {
    Serial.println("Failed to connect");
    ESP.restart();
  } 
  else {
    //if you get here you have connected to the WiFi    
    Serial.println("connected...yeey :)");
  }

  // Wait for 
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }


  server.on("/", handleRoot);
  server.on("/login", handleLogin);
  server.on("/inline", []() {
    server.send(200, "text/plain", "this works without need of authentification");
  });
  server.on("/fetch", handleFetch);
  server.on("/update", handleUpdate);

  server.onNotFound(handleNotFound);
  //here the list of headers to be recorded
  const char * headerkeys[] = {"User-Agent", "Cookie"} ;
  size_t headerkeyssize = sizeof(headerkeys) / sizeof(char*);
  //ask server to track these headers
  server.collectHeaders(headerkeys, headerkeyssize);
  server.begin();
  Serial.println("HTTP server started");
}

void loop(void) {
  server.handleClient();
  delay(2);//allow the cpu to switch to other tasks

  // Serial.println(GetTemp());
  // delay(1000);
}

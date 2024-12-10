#include "WiFi.h"
#include <HTTPClient.h>
#include "time.h"
#include <AHT10.h>


//Definitions
#define uS_TO_S_FACTOR 1000000  // Converting micro seconds to seconds
#define TIME_TO_SLEEP  30        // Time ESP32 will go to sleep (in seconds)


const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 19800;
const int   daylightOffset_sec = 0;
// WiFi information
const char* ssid = "DaRouter";         // change SSID
const char* password = "ConnectedtotheWired";    // change password


String GOOGLE_SCRIPT_ID = "AKfycbyQxcJRzE701FJAvWf-334y6mon_xuHALj_i_iaUv0TgILz2h-Xzb9l4J7jhi7T9zqN"; // Google script ID




RTC_DATA_ATTR int bootCount = 0;
uint8_t readStatus = 0;


AHT10 myAHT10(AHT10_ADDRESS_0X38); //Memory address of the aht10




void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  // connect to WiFi
  Serial.println();
  Serial.print("Connecting to wifi: ");
  Serial.println(ssid);
  Serial.flush();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) { //Print a dot for each half a second when the aht10 is connecting to wifi
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");


  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);


  while (myAHT10.begin() != true) //Checks to see if there is any issue with the AHT10
  {
    Serial.println(F("AHT10 not connected"));
    delay(5000);
  }
  Serial.println(F("AHT10 OK!"));


  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));


  loop();


  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR); // Using the deep sleep timer for every minute
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) +
  " Seconds");


  Serial.println("Going to sleep now");
  delay(1000);
  Serial.flush();
  esp_deep_sleep_start();
}
void loop() {
   if (WiFi.status() == WL_CONNECTED) {
    static bool flag = false;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      return;
    }


     Serial.print(F("Temperature: ")); Serial.println(myAHT10.readTemperature()); //Temperature
    Serial.print(F("Humidity: ")); Serial.println(myAHT10.readHumidity()); //Humidity
    char timeStringBuff[50]; //50 chars should be enough
    strftime(timeStringBuff, sizeof(timeStringBuff), "%A, %B %d %Y %H:%M:%S", &timeinfo);
    String asString(timeStringBuff);
    asString.replace(" ", "-");
    Serial.print("Time:");
    Serial.println(asString);
    String urlFinal = "https://script.google.com/macros/s/"+GOOGLE_SCRIPT_ID+"/exec?"+"humidity=" + String(myAHT10.readHumidity()) + "&temperature=" + String(myAHT10.readTemperature());
    Serial.print("POST data to spreadsheet:");
    Serial.println(urlFinal);
    HTTPClient http;
    http.begin(urlFinal.c_str());
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    int httpCode = http.GET();
    Serial.print("HTTP Status Code: ");
    Serial.println(httpCode);
    //---------------------------------------------------------------------
    //getting response from google sheet
    String payload;
    if (httpCode > 0) {
        payload = http.getString();
        Serial.println("Payload: "+payload);    
    }
    //---------------------------------------------------------------------
    http.end();
  }
}

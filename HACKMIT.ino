#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>

#define SCREEN_WIDTH 128       // OLED display width, in pixels
#define SCREEN_HEIGHT 64       // OLED display height, in pixels
#define BASEURL "192.168.4.2"  // client URL

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET -1  // Reset pin
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiServer server(80);

// Add/Subtract people
int addPerson = D8;
int subPerson = D4;
int numPeople = 1;

// Add/Subtract hours
int addHour = D5;
int subHour = D6;
int numHours = 1;

String displayString = "";

// Generate buttons & animation pins
int genButton = D7;
// int RLED = 2
// int GLED = 1
// int BLED = 0

// Wi-Fi configuration
const char *ssid = "IDEATRON";
const char *password = "IDEATRON3000";

void animation() {
  // empty unless we have extra time to make it,
  // LEDs, buzzer, etc
}

String prepareJson() {
  JsonDocument doc;

  String htmlPage;
  String jsonHolder;  // need to hold for the stream class
  htmlPage.reserve(1024);

  // create Json
  doc["people"] = numPeople;
  doc["hours"] = numHours;

  // send to stream
  serializeJson(doc, jsonHolder);

  // create string
  htmlPage = F("HTTP/1.1 200 OK\r\n"
               "Content-Type: text/html\r\n"
               "Connection: close\r\n"  // the connection will be closed after completion of the response
               "Refresh: 5\r\n"         // refresh the page automatically every 5 sec
               "\r\n"
               "<!DOCTYPE HTML>"
               "<html>");
  htmlPage += jsonHolder;  // TODO: add the arduino JSON
  htmlPage += F("</html>"
                "\r\n");

  return htmlPage;
}

void handleClient() {
  WiFiClient client = server.accept();
  // wait for a client (web browser) to connect
  if (client) {
    Serial.println("\n[Client connected]");
    while (client.connected()) {
      // read line by line what the client (web browser) is requesting
      if (client.available()) {
        String line = client.readStringUntil('\r');
        Serial.print(line);
        // wait for end of client's request, that is marked with an empty line
        if (line.length() == 1 && line[0] == '\n') {
          client.println(prepareJson());
          break;
        }
      }
    }

    while (client.available()) {
      // but first, let client finish its request
      // that's diplomatic compliance to protocols
      // (and otherwise some clients may complain, like curl)
      // (that is an example, prefer using a proper webserver library)
      client.read();
    }

    // close the connection:
    client.stop();
    Serial.println("[Client disconnected]");
  }
}

void scrapePage() {
}

void displayIdea(String idea) {
  // Clear the buffer.
  display.clearDisplay();

  // Display Text
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 28);
  display.println("Hello world!");
  display.display();
  delay(2000);
  display.clearDisplay();
  delay(500);
}

void displayStats() {
  // display current amount of hours and people
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 28);
  display.print("Number of people:");
  display.println(numPeople);
  display.print("Number of hours:");
  display.println(numHours);
  display.display();
  delay(100);
  display.clearDisplay();
}


void setup() {
  Serial.begin(115200);
  Serial.println();
  WiFi.softAP(ssid, password);
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" connected");

  server.begin();
  Serial.printf("Web server started, open %s in a web browser\n", WiFi.localIP().toString().c_str());

  // initialize IO
  pinMode(addPerson, INPUT);
  pinMode(subPerson, INPUT);
  pinMode(addHour, INPUT);
  pinMode(subHour, INPUT);
  pinMode(genButton, INPUT);
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (digitalRead(addPerson) == HIGH) {
    numPeople++;
    delay(300);
  } else if (digitalRead(subPerson) == HIGH && numPeople > 0) {
    numPeople--;
    delay(300);
  } else if (digitalRead(addHour) == HIGH) {
    numHours++;
    delay(300);
  } else if (digitalRead(subHour) == HIGH && numHours > 0) {
    numHours--;
    delay(300);
  } else if (digitalRead(genButton) == HIGH) {
    digitalWrite(LED_BUILTIN, HIGH);
    animation();
    delay(300);
    while (displayString == "") {
      handleClient();
    }
    displayString = "";
    numHours = 1;
    numPeople = 1;
  } else {
    digitalWrite(LED_BUILTIN, LOW);
  }
  // handleClient();
  displayStats();
}
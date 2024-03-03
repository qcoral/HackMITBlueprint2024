#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <SPI.h>
#include <Wire.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define GPT_DELAY 10000; // 10 seconds, just to be safe
#define TONE_TIMING 50 // timing between beeps

// Declaration for SSD1306 display connected using I2C
#define OLED_RESET -1 // Reset pin
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
int speaker = D3;

// Wi-Fi configuration
const char *ssid = "SM-A526W8121";
const char *password = "dtwg0081";

void animation()
{
    tone(speaker, 800);
    delay(TONE_TIMING);
    noTone(speaker);
    delay(TONE_TIMING);
    tone(speaker, 800);
    delay(TONE_TIMING);
    noTone(speaker);
    delay(TONE_TIMING);
    tone(speaker, 800);
    delay(TONE_TIMING);
    noTone(speaker);
    delay(TONE_TIMING);
    tone(speaker, 800);
    delay(TONE_TIMING);
    noTone(speaker);
}

String prepareJson()
{
    JsonDocument doc;

    String htmlPage;
    String jsonHolder; // need to hold for the stream class
    htmlPage.reserve(1024);

    // create Json
    doc["people"] = numPeople;
    doc["hours"] = numHours;

    // send to stream
    serializeJson(doc, jsonHolder);

    // create string
    htmlPage = F("HTTP/1.1 200 OK\r\n"
                 "Content-Type: text/html\r\n"
                 "Connection: close\r\n" // the connection will be closed after completion of the response
                 "Refresh: 5\r\n"        // refresh the page automatically every 5 sec
                 "\r\n"
                 "<!DOCTYPE HTML>"
                 "<html>");
    htmlPage += jsonHolder; // TODO: add the arduino JSON
    htmlPage += F("</html>"
                  "\r\n");

    return htmlPage;
}

void scrapePage()
{

    WiFiClient client;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");

    // Stall until found
    while (true)
    {
        if (http.begin(client, "http://192.168.112.227/api/idea"))
        { // HTTP
            Serial.print("[HTTP] GET...\n");
            // start connection and send HTTP header
            int httpCode = http.GET();

            // httpCode will be negative on error
            if (httpCode > 0)
            {
                // HTTP header has been send and Server response header has been handled
                Serial.printf("[HTTP] GET... code: %d\n", httpCode);

                // file found at server
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                {
                    displayString = http.getString();

                    // continue if blank page
                    if (displayString == "")
                    {
                        continue;
                    }
                    Serial.println(displayString);
                    break;
                }
            }
            else
            {
                displayString = "Server error, please try again";
                Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }

            http.end();
        }
        else
        {
            Serial.println("[HTTP] Unable to connect");
        }
        delay(200);
    }
    animation();
    displayIdea();
}

void displayIdea()
{
    // Clear the buffer.
    display.clearDisplay();

    // Display Text
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("Your idea is:");
    display.println(displayString);
    display.setCursor(0, 50);
    display.println("Press to continue");
    display.display();
    delay(500);
    while (true)
    {
        if (digitalRead(genButton) == HIGH)
        {
            Serial.println("genbutton was pressed");
            displayString = "STOPCODE";
            delay(200);
            display.clearDisplay();
            break;
        }
        delay(100);
    }
}

void displayStats()
{
    // display current amount of hours and people
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(2);
    display.setCursor(0, 0);
    display.println("IDEABOARD v1.0");
    display.setTextSize(1);

    // display.setCursor(0, 20);
    display.print("Number of people:");
    display.println(numPeople);
    display.print("Number of hours:");
    display.println(numHours);
    // display.setCursor(0, 32);
    // display.println("Current IP address: ");
    // display.println(WiFi.localIP());
    display.setCursor(0, 54);
    display.println("Press to send data");
    display.display();
    delay(100);
}

void sendingMsg()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 28);
    display.println("Sending data...");
    display.display();
    delay(100);
}

void processingMsg()
{
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 28);
    display.println("Processing...");
    display.display();
    delay(100);
}

void handleClient()
{
    WiFiClient client = server.accept();
    // wait for a client (web browser) to connect
    if (client)
    {
        Serial.println("\n[Client connected]");
        while (client.connected())
        {
            // read line by line what the client (web browser) is requesting
            if (client.available())
            {
                String line = client.readStringUntil('\r');
                Serial.print(line);
                // wait for end of client's request, that is marked with an empty line
                if (line.length() == 1 && line[0] == '\n')
                {
                    client.println(prepareJson());
                    Serial.println("Json Sent");
                    break;
                }
            }
        }

        while (client.available())
        {
            // but first, let client finish its request
            // that's diplomatic compliance to protocols
            // (and otherwise some clients may complain, like curl)
            // (that is an example, prefer using a proper webserver library)
            client.read();
        }

        // close the connection:
        client.stop();
        Serial.println("[Client disconnected]");
        processingMsg();
        scrapePage();
    }
}

void setup()
{
    Serial.begin(9600);

    // initialize the OLED object
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
    {
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    //Clear display
    display.display();
    delay(500);
    display.clearDisplay();
    display.display();
    delay(500);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 28);
    display.print("Connecting to wifi....");
    display.display();

    WiFi.begin(ssid, password); // Connect to your WiFi router
    Serial.println("Connecting to WiFi");
    // Wait for connection
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    server.begin();

    // Set IO
    pinMode(addPerson, INPUT);
    pinMode(subPerson, INPUT);
    pinMode(addHour, INPUT);
    pinMode(subHour, INPUT);
    pinMode(genButton, INPUT);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    if (digitalRead(addPerson) == HIGH)
    {
        numPeople++;
        delay(300);
    }
    else if (digitalRead(subPerson) == HIGH && numPeople > 1)
    {
        numPeople--;
        delay(300);
    }
    else if (digitalRead(addHour) == HIGH)
    {
        numHours++;
        delay(300);
    }
    else if (digitalRead(subHour) == HIGH && numHours > 1)
    {
        numHours--;
        delay(300);
    }
    else if (digitalRead(genButton) == HIGH)
    {
        Serial.println("genbutton was pressed");
        digitalWrite(LED_BUILTIN, HIGH);
        animation();
        while (displayString == "")
        {
            sendingMsg();
            handleClient();
            if (displayString == "STOPCODE")
            {
                Serial.println("BROKE THE LOOP");
                break;
            }
        }
        displayString = "";
        numHours = 1;
        numPeople = 1;
    }
    else
    {
        digitalWrite(LED_BUILTIN, LOW);
    }
    displayStats();
}
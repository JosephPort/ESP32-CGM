#include <SPI.h>

#include <TFT_eSPI.h> 

#include <WiFi.h>

#include <HTTPClient.h>

#include <ArduinoJson.h>

#include <TFT_eWidget.h>              

// Invoke custom library for the display
TFT_eSPI tft = TFT_eSPI();

// Graph widget gr instance with pointer to tft
GraphWidget gr = GraphWidget(&tft);
// Graph trace tr with pointer to gr
TraceWidget tr = TraceWidget(&gr);

int prevVals[10];

void setup(void) {
  // Inits display on start
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);

  connectWiFi();

  tft.fillScreen(TFT_BLACK);

  initGraph();

  // Begins loop after 1 second
  delay(1000);
}

void connectWiFi(){
  // Sets cursor to the top left for printing purposes
  tft.setCursor(0, 4, 4);
  tft.setTextColor(TFT_WHITE);

  const char* ssid = "Wi-Fi Username";
  const char* password = "Wi-Fi Password";

  Serial.begin(115200);
  delay(2000);

  WiFi.mode(WIFI_STA); //Optional
  WiFi.begin(ssid, password);
  tft.println("\nConnecting");

  while(WiFi.status() != WL_CONNECTED){
    tft.print(".");
    delay(100);
  }

  tft.println("\nConnected to the WiFi network");
  tft.print("Local ESP32 IP: ");
  tft.println(WiFi.localIP());

  delay(1000);
}

void initGraph(){
  gr.createGraph(239, 150, tft.color565(5, 5, 5));

  gr.setGraphScale(0, 9, 0, 300);

  gr.setGraphGrid(0, 9, 0, 300, TFT_WHITE);
}

void loop() {
  // Resets screen so newly written information does not overlap
  tft.fillScreen(TFT_BLACK);

  getRequest();

  drawContent();

  // Repeats loop every 60000ms or every minute
  delay(60000);
}

void getRequest(){
  String url = "{Your Nightscout URL}/api/v1/entries.json";

  HTTPClient http;

  //Begin the HTTP request for the URL
  http.begin(url);

  // Send GET request and store as the code
  int httpCode = http.GET();  

  tft.setCursor(210, 220, 2);
  tft.println(httpCode);

  String response = "";

  if (httpCode == 200) {
    response = http.getString(); 
  }
  else {
    Serial.print("Request failed with status code: ");
    Serial.println(httpCode);
    return;
  }

  // Close the connection
  http.end();

  // Stream & input;
  JsonDocument doc;

  DeserializationError error = deserializeJson(doc, response);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Init variable to keep track of the number of sgv values stored
  int sgvCount = 0;

  // Loop through each JSON object in the array
  for (JsonObject item : doc.as<JsonArray>()) {
      int sgv = item["sgv"];
      // Store value in array and increment sgvCount so next loop iteration 
      // will not overwrite index
      prevVals[sgvCount++] = sgv;
  }
}

void drawContent(){
  gr.drawGraph(0, 0);
  
  tft.setCursor(15, 180, 6);
  tft.println(prevVals[0]);

  tr.startTrace(TFT_RED);
  // Add a trace point at 0.0,0.0 on graph
  tr.addPoint(0.0, 70.0);
  // Line represents a low glucose value on the graph
  tr.addPoint(9.0, 70.0);

  tr.startTrace(TFT_YELLOW);
  // Add a trace point at 0.0,0.0 on graph
  tr.addPoint(0.0, 200.0);
    // Line represents a high glucose value on the graph
  tr.addPoint(9.0, 200.0);

  tr.startTrace(TFT_WHITE);

  for (int i = 0; i < 10; i++){
    // Draws in decreasing order since increasing order will flip the graph
    // where most recent is (0, Y) on the graph
    tr.addPoint(i, prevVals[9 - i]);
  }
}

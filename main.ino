#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Preferences.h>

#define LED_PIN 2 //built in
#define RELAY_PIN 13 // P13
#define RELAY_PIN_A 23 // P23

const char *ap_ssid = "ESP32-Access-Point";
const char *ap_password = "12345678";

AsyncWebServer server(80);
Preferences preferences;

const char *PARAM_SSID = "ssid";
const char *PARAM_PASSWORD = "password";

void startServer();
void startConfigPortal();
bool tryConnectWiFi(unsigned long timeout);

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  pinMode(RELAY_PIN_A, OUTPUT);
  digitalWrite(RELAY_PIN_A, LOW);
  Serial.begin(115200);

  preferences.begin("wifi-creds", false);

  // Check if credentials are already saved
  String savedSSID = preferences.getString(PARAM_SSID, "");
  String savedPassword = preferences.getString(PARAM_PASSWORD, "");

  if (savedSSID != "" && savedPassword != "") {
    // Try to connect to the saved network
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());

    Serial.print("Connecting to ");
    Serial.println(savedSSID);

    // Try to connect for 60 seconds
    if (!tryConnectWiFi(60000)) {  
      Serial.println("Failed to connect to saved WiFi network.");
      ESP.restart();
    } else {
      Serial.println("Connected to WiFi network");
      Serial.print("IP Address: ");
      Serial.println(WiFi.localIP());

      // Start the web server for LED control
      startServer();
    }
  } else {
    startConfigPortal();
  }
}

void loop() {
  
}

bool tryConnectWiFi(unsigned long timeout) {
  unsigned long startTime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - startTime >= timeout) {
      preferences.clear();
      return false;  // Failed to connect within the timeout
    }
    delay(500);
    Serial.print(".");
  }
  return true;  // Connected to WiFi
}

void startConfigPortal() {
  // Set up as an access point
  WiFi.softAP(ap_ssid, ap_password);
  Serial.println("Setting up AP");
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>WiFi Configuration</title>
        <style>
          body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
          .container { display: inline-block; padding: 20px; border: 1px solid #ccc; border-radius: 10px; width: 300px; }
          h1 { color: #333; }
          input[type=text], input[type=password] { width: 100%; padding: 10px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; }
          input[type=submit] { padding: 10px 20px; font-size: 16px; cursor: pointer; background-color: #4CAF50; color: white; border: none; border-radius: 5px; }
          .error { color: red; }
        </style>
      </head>
      <body>
        <div class="container">
          <h1>ESP32 WiFi Configuration</h1>
          <form action='/get' method='GET'>
            <input type='text' name='ssid' placeholder='SSID' required>
            <input type='password' name='password' placeholder='Password' required>
            <input type='submit' value='Submit'>
          </form>
          <p class='error' id='error'></p>
        </div>
      </body>
      </html>
    )rawliteral");
  });

  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request) {
    String newSSID;
    String newPassword;
    if (request->hasParam(PARAM_SSID) && request->hasParam(PARAM_PASSWORD)) {
      newSSID = request->getParam(PARAM_SSID)->value();
      newPassword = request->getParam(PARAM_PASSWORD)->value();

      WiFi.begin(newSSID.c_str(), newPassword.c_str());

      // Save credentials and redirect to control page
      preferences.putString(PARAM_SSID, newSSID);
      preferences.putString(PARAM_PASSWORD, newPassword);

      String redirectPage = String(R"rawliteral(
          <!DOCTYPE html>
          <html>
          <head>
            <title>WiFi Configuration</title>
            <style>
              body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
              .container { display: inline-block; padding: 20px; border: 1px solid #ccc; border-radius: 10px; }
              h1 { color: #333; }
            </style>
          </head>
          <body>
            <div class="container">
              <h1>WiFi credentials saved</h1>
              <p>The device will restart and connect to the network.</p>
            </div>
          </body>
          </html>
        )rawliteral");

      request->send(200, "text/html", redirectPage);
      delay(2000);
      ESP.restart();
    } else {
    request->send(200, "text/html", "<h1>Please provide both SSID and password.</h1>");
    }
});

server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
  preferences.clear();
  request->send(200, "text/html", "<h1>Preferences cleared. Device will restart.</h1>");
  delay(1000);
  ESP.restart();
});

server.begin();
}

void startServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", R"rawliteral(
      <!DOCTYPE html>
      <html>
      <head>
        <title>Control</title>
        <style>
          body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; }
          .container { display: inline-block; padding: 20px; border: 1px solid #ccc; border-radius: 10px; }
          h1 { color: #333; }
          button { padding: 10px 20px; font-size: 16px; cursor: pointer; }
          .on { background-color: #4CAF50; color: white; }
          .off { background-color: #f44336; color: white; }
          .reset { margin-top: 20px; padding: 10px 20px; font-size: 16px; cursor: pointer; background-color: #ff9800; color: white; border: none; border-radius: 5px; }
        </style>
      </head>
      <body>
        <div class="container">
          <h1>Devices Control Panel</h1>
          <p>RGB LED : </p><button id="toggleButton" class="off" onclick="toggleLED()">OFF</button><br><br>
          <p>FAN :</p><button id="toggleButtonrelay" class="off" onclick="toggleRelay()">OFF</button><br><br>
          <p>Heater:</p><button id="toggleButtonrelay_a" class="off" onclick="toggleRelayA()">OFF</button><br><br>
          <button class="reset" onclick="resetPreferences()">Reset WiFi</button>
        </div>
        <script>
          function toggleLED() {
            fetch('/toggle')
              .then(response => response.text())
              .then(state => {
                const button = document.getElementById('toggleButton');
                if (state === 'ON') {
                  button.textContent = 'ON';
                  button.className = 'on';
                } else {
                  button.textContent = 'OFF';
                  button.className = 'off';
                }
              });
          }

          function toggleRelay() {
            fetch('/relayon')
              .then(response => response.text())
              .then(state => {
                const button = document.getElementById('toggleButtonrelay');
                if (state === 'ON') {
                  button.textContent = 'OFF';
                  button.className = 'off';
                } else {
                  button.textContent = 'ON';
                  button.className = 'on';
                }
              });
          }

          function toggleRelayA() {
            fetch('/relayon_a')
              .then(response => response.text())
              .then(state => {
                const button = document.getElementById('toggleButtonrelay_a');
                if (state === 'ON') {
                  button.textContent = 'OFF';
                  button.className = 'off';
                } else {
                  button.textContent = 'ON';
                  button.className = 'on';
                }
              });
          }

          function resetPreferences() {
            fetch('/clear')
              .then(() => {
                alert("Preferences cleared. Device will restart.");
              });
          }
        </script>
      </body>
      </html>
    )rawliteral");
  });

  server.on("/toggle", HTTP_GET, [](AsyncWebServerRequest *request) {
    int ledState = digitalRead(LED_PIN);
    int relayState = digitalRead(RELAY_PIN);
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
    String state = ledState ? "ON" : "OFF";
    request->send(200, "text/plain", state);
  });

  server.on("/relayon", HTTP_GET, [](AsyncWebServerRequest *request) {
    int relayState = digitalRead(RELAY_PIN);
    relayState = !relayState;
    digitalWrite(RELAY_PIN, relayState);
    String state = relayState ? "ON" : "OFF";
    request->send(200, "text/plain", state);
  });

  server.on("/relayon_a", HTTP_GET, [](AsyncWebServerRequest *request) {
    int relayState_A = digitalRead(RELAY_PIN_A);
    relayState_A = !relayState_A;
    digitalWrite(RELAY_PIN_A, relayState_A);
    String state_A = relayState_A ? "ON" : "OFF";
    request->send(200, "text/plain", state_A);
  });

  server.on("/clear", HTTP_GET, [](AsyncWebServerRequest *request) {
    preferences.clear();
    request->send(200, "text/html", "<h1>Preferences cleared. Device will restart.</h1>");
    delay(2000);
    ESP.restart();
  });

  server.begin();
}

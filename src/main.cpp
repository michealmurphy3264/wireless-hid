#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include "esp_netif.h"
#include "BleHid.h"
#include "web_ui.h"
#include "secrets.h"   // AP_SSID / AP_PASS — gitignored; copy from secrets.h.example

BleHid           hid;
WebServer        server(80);
WebSocketsServer ws(81);
DNSServer        dns;
Preferences      prefs;

// Keyboard layout (0=US, 1=UK) — persisted in NVS
uint8_t kbLayout = 0;

// Jiggler state (persisted in NVS)
bool     jiggling     = false;
int      jiggleMinMs  = 20000;
int      jiggleMaxMs  = 60000;
int      jiggleStep   = 10;
bool     jiggleRand   = false;
uint32_t lastJiggle   = 0;
uint32_t nextJiggleIn = 20000; // randomised each cycle
uint32_t jiggleCount  = 0;
uint32_t jiggleHistory[10] = {};
uint8_t  jiggleHistIdx = 0;

void saveJiggle() {
    prefs.begin("jiggle", false);
    prefs.putBool("on",    jiggling);
    prefs.putInt("msMin",  jiggleMinMs);
    prefs.putInt("msMax",  jiggleMaxMs);
    prefs.putInt("step",   jiggleStep);
    prefs.putBool("rand",  jiggleRand);
    prefs.end();
}

// Commands:
//   TYPE <text>           - type a string (\n and \t are parsed)
//   KEY <keycode>         - press a raw HID keycode (decimal)
//   COMBO <mod> <key>     - modifier bits + keycode (e.g. COMBO 1 6 = Ctrl+C)
//   MOVE <x> <y>          - relative mouse move (-127..127)
//   DRAG <x> <y>          - move with left button held
//   SCROLL <n>            - scroll wheel
//   CLICK <1|2|3>         - click left/right/middle
//   PRESS <1|2|3>         - hold mouse button
//   RELEASE               - release all buttons/keys
//   STATUS                - connection status
//   LAYOUT US|UK          - set keyboard layout (saved to flash)
//   JIGGLE ON|OFF         - enable/disable jiggler (saved to flash)
//   JIGGLE MIN <ms>       - set minimum jiggle interval in ms
//   JIGGLE MAX <ms>       - set maximum jiggle interval in ms
//   JIGGLE STEP <px>      - set jiggle step size
//   JIGGLE RAND <0|1>     - randomise direction

String handleCommand(const String& line) {
    // LAYOUT command — no BLE connection required
    if (line.startsWith("LAYOUT ")) {
        String sub = line.substring(7);
        if (sub == "US") { kbLayout = 0; hid.setLayout(0); prefs.begin("kb", false); prefs.putUChar("layout", 0); prefs.end(); return "OK:LAYOUT_US"; }
        if (sub == "UK") { kbLayout = 1; hid.setLayout(1); prefs.begin("kb", false); prefs.putUChar("layout", 1); prefs.end(); return "OK:LAYOUT_UK"; }
        return "ERR:UNKNOWN_LAYOUT";
    }

    // JIGGLE commands — no BLE connection required
    if (line.startsWith("JIGGLE ")) {
        String sub = line.substring(7);
        if (sub == "ON")  { jiggling = true;  nextJiggleIn = random(jiggleMinMs, jiggleMaxMs + 1); saveJiggle(); return "OK:JIGGLE_ON"; }
        if (sub == "OFF") { jiggling = false; saveJiggle(); return "OK:JIGGLE_OFF"; }
        if (sub.startsWith("MIN "))  { jiggleMinMs = sub.substring(4).toInt(); saveJiggle(); return "OK:JIGGLE_MIN"; }
        if (sub.startsWith("MAX "))  { jiggleMaxMs = sub.substring(4).toInt(); saveJiggle(); return "OK:JIGGLE_MAX"; }
        if (sub.startsWith("STEP ")) { jiggleStep  = sub.substring(5).toInt(); saveJiggle(); return "OK:JIGGLE_STEP"; }
        if (sub.startsWith("RAND ")) { jiggleRand  = sub.substring(5).toInt() != 0; saveJiggle(); return "OK:JIGGLE_RAND"; }
        return "ERR:UNKNOWN_JIGGLE";
    }

    if (line == "STATUS") {
        return hid.isConnected() ? "CONNECTED" : "ADVERTISING";
    }

    if (line.startsWith("TYPE ")) {
        String text = line.substring(5);
        text.replace("\\n", "\n");
        text.replace("\\t", "\t");
        hid.typeString(text.c_str());
        return "OK:TYPE";

    } else if (line.startsWith("KEY ")) {
        uint8_t keycode = (uint8_t)line.substring(4).toInt();
        hid.keyPress(keycode);
        return "OK:KEY";

    } else if (line.startsWith("COMBO ")) {
        int sp = line.indexOf(' ', 6);
        uint8_t mod = (uint8_t)line.substring(6, sp).toInt();
        uint8_t key = (uint8_t)line.substring(sp + 1).toInt();
        hid.keyPress(key, mod);
        return "OK:COMBO";

    } else if (line.startsWith("MOVE ")) {
        int sp = line.indexOf(' ', 5);
        int8_t x = (int8_t)line.substring(5, sp).toInt();
        int8_t y = (int8_t)line.substring(sp + 1).toInt();
        hid.mouseMove(x, y);
        return "OK:MOVE";

    } else if (line.startsWith("DRAG ")) {
        int sp = line.indexOf(' ', 5);
        int8_t x = (int8_t)line.substring(5, sp).toInt();
        int8_t y = (int8_t)line.substring(sp + 1).toInt();
        hid.mousePress(MOUSE_LEFT);
        hid.mouseMove(x, y);
        return "OK:DRAG";

    } else if (line.startsWith("SCROLL ")) {
        int8_t w = (int8_t)line.substring(7).toInt();
        hid.mouseMove(0, 0, w);
        return "OK:SCROLL";

    } else if (line.startsWith("CLICK ")) {
        uint8_t btn = (uint8_t)line.substring(6).toInt();
        hid.mouseClick(btn);
        return "OK:CLICK";

    } else if (line.startsWith("PRESS ")) {
        uint8_t btn = (uint8_t)line.substring(6).toInt();
        hid.mousePress(btn);
        return "OK:PRESS";

    } else if (line == "RELEASE") {
        hid.mouseRelease();
        hid.releaseKeys();
        return "OK:RELEASE";
    }

    return "ERR:UNKNOWN:" + line;
}

// Serial handler
void handleSerial() {
    if (!Serial.available()) return;
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    // JIGGLE and STATUS work without BLE
    bool needsConn = !line.startsWith("JIGGLE") && line != "STATUS";
    if (needsConn && !hid.isConnected()) {
        Serial.println("ERR:NOT_CONNECTED");
        return;
    }
    Serial.println(handleCommand(line));
}

// Web handlers
void handleRoot() {
    server.send_P(200, "text/html", PAGE_HTML);
}

void handleStatus() {
    bool staConn = (WiFi.status() == WL_CONNECTED);
    String json = "{";
    json += "\"connected\":"  + String(hid.isConnected() ? "true" : "false") + ",";
    json += "\"layout\":\""   + String(kbLayout == 1 ? "UK" : "US") + "\",";
    json += "\"jiggling\":"   + String(jiggling   ? "true" : "false") + ",";
    json += "\"jiggleMinMs\":" + String(jiggleMinMs) + ",";
    json += "\"jiggleMaxMs\":" + String(jiggleMaxMs) + ",";
    json += "\"jiggleStep\":"  + String(jiggleStep)  + ",";
    json += "\"jiggleRand\":"  + String(jiggleRand ? "true" : "false") + ",";
    int32_t msLeft = (jiggling && hid.isConnected())
        ? max(0L, (long)nextJiggleIn - (long)(millis() - lastJiggle))
        : -1;
    json += "\"msUntilJiggle\":" + String(msLeft) + ",";
    json += "\"jiggleCount\":"   + String(jiggleCount) + ",";
    uint8_t n = (uint8_t)min((int)jiggleHistIdx, 10);
    json += "\"jiggleHistory\":[";
    for (uint8_t i = 0; i < n; i++) {
        uint8_t slot = (jiggleHistIdx - n + i) % 10;
        if (i) json += ",";
        json += String(jiggleHistory[slot] / 1000);
    }
    json += "],";
    json += "\"staIP\":\""    + (staConn ? WiFi.localIP().toString() : String("")) + "\",";
    json += "\"staSSID\":\""  + (staConn ? WiFi.SSID() : String("")) + "\"";
    json += "}";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

void handleCmd() {
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"ok\":false,\"error\":\"NO_BODY\"}");
        return;
    }
    String cmd = server.arg("plain");
    cmd.trim();

    bool needsConn = !cmd.startsWith("JIGGLE") && cmd != "STATUS";
    if (needsConn && !hid.isConnected()) {
        server.send(200, "application/json", "{\"ok\":false,\"error\":\"NOT_CONNECTED\"}");
        return;
    }

    String result = handleCommand(cmd);
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"ok\":true,\"result\":\"" + result + "\"}");
}

// Helper: MAC bytes -> "AA:BB:CC:DD:EE:FF"
String macToStr(const uint8_t* addr) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
        addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    return String(buf);
}

// Helper: "AA:BB:CC:DD:EE:FF" -> NVS key "AABBCCDDEEFF"
String macToKey(const String& mac) {
    String k = mac; k.replace(":", ""); return k;
}

void handleBleList() {
    int num = NimBLEDevice::getNumBonds();
    String json = "{\"count\":" + String(num) + ",\"active\":" +
                  String(hid.isConnected() ? "true" : "false") + "}";
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", json);
}

void handleBleConnect() {
    server.sendHeader("Access-Control-Allow-Origin", "*");
    if (hid.isConnected()) {
        hid.disconnect();
        Serial.println("BLE: disconnect requested for switch");
    } else {
        NimBLEDevice::startAdvertising();
        Serial.println("BLE: not connected, restarted advertising");
    }
    server.send(200, "application/json", "{\"ok\":true,\"wasConnected\":" +
                String(hid.isConnected() ? "true" : "false") + "}");
}

void handleBleForget() {
    String mac = server.arg("mac");
    if (mac.length() > 0) {
        NimBLEDevice::deleteBond(NimBLEAddress(mac.c_str()));
        prefs.begin("blenames", false);
        prefs.remove(macToKey(mac).c_str());
        prefs.end();
    } else {
        NimBLEDevice::deleteAllBonds();
    }
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"ok\":true}");
}

void handleBleName() {
    String mac  = server.arg("mac");
    String name = server.arg("name");
    prefs.begin("blenames", false);
    prefs.putString(macToKey(mac).c_str(), name);
    prefs.end();
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.send(200, "application/json", "{\"ok\":true}");
}

void handleWifi() {
    String ssid = server.arg("ssid");
    String pass = server.arg("pass");

    server.sendHeader("Access-Control-Allow-Origin", "*");

    if (ssid.length() == 0) {
        // Clear saved credentials and disconnect
        prefs.begin("wifi", false);
        prefs.remove("ssid");
        prefs.remove("pass");
        prefs.end();
        WiFi.disconnect();
        server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Disconnected\"}");
        return;
    }

    // Save and attempt connection
    prefs.begin("wifi", false);
    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    prefs.end();

    WiFi.begin(ssid.c_str(), pass.c_str());
    server.send(200, "application/json", "{\"ok\":true,\"msg\":\"Connecting...\"}");
}

void wsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    if (type != WStype_TEXT) return;
    String cmd = String((char*)payload);
    cmd.trim();
    if (cmd.length() == 0) return;

    bool needsConn = !cmd.startsWith("JIGGLE") && cmd != "STATUS";
    if (needsConn && !hid.isConnected()) {
        ws.sendTXT(num, "{\"ok\":false,\"error\":\"NOT_CONNECTED\"}");
        return;
    }
    String result = handleCommand(cmd);
    // Skip response for MOVE to avoid flooding back to client
    if (!cmd.startsWith("MOVE ")) {
        ws.sendTXT(num, "{\"ok\":true,\"result\":\"" + result + "\"}");
    }
}

void setup() {
    Serial.begin(115200);
    delay(2000);
    Serial.println("Booting...");

    // Load keyboard layout from NVS
    prefs.begin("kb", true);
    kbLayout = prefs.getUChar("layout", 0);
    prefs.end();
    hid.setLayout(kbLayout);

    // Load jiggler settings from NVS
    prefs.begin("jiggle", false);
    jiggling     = prefs.getBool("on",    false);
    jiggleMinMs  = prefs.getInt("msMin",  20000);
    jiggleMaxMs  = prefs.getInt("msMax",  60000);
    jiggleStep   = prefs.getInt("step",   10);
    jiggleRand   = prefs.getBool("rand",  false);
    prefs.end();
    nextJiggleIn = random(jiggleMinMs, jiggleMaxMs + 1);

    // WiFi first — must init before BLE on ESP32-C3 for coexistence
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(AP_SSID, AP_PASS);

    // Clear gateway from DHCP so phones keep using mobile data for internet.
    {
        esp_netif_t* ap = esp_netif_get_handle_from_ifkey("WIFI_AP_DEF");
        if (ap) {
            esp_netif_dhcps_stop(ap);
            esp_netif_ip_info_t info;
            esp_netif_get_ip_info(ap, &info);
            info.gw.addr = 0;
            esp_netif_set_ip_info(ap, &info);
            esp_netif_dhcps_start(ap);
        }
    }

    WiFi.setTxPower(WIFI_POWER_8_5dBm); // reduce interference with BLE
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

    // Try to connect to saved home WiFi
    prefs.begin("wifi", true);
    String staSSID = prefs.getString("ssid", "");
    String staPass = prefs.getString("pass", "");
    prefs.end();

    if (staSSID.length() > 0) {
        Serial.printf("Connecting to WiFi: %s\n", staSSID.c_str());
        WiFi.begin(staSSID.c_str(), staPass.c_str());
        for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
            delay(500);
        }
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("STA IP: %s\n", WiFi.localIP().toString().c_str());
        } else {
            Serial.println("STA: could not connect, AP only");
        }
    }

    MDNS.begin("hid"); // accessible at http://hid.local on both AP and home network

    // BLE HID
    hid.begin("Wireless HID");

    // Web server routes
    server.on("/",       HTTP_GET,  handleRoot);
    server.on("/status", HTTP_GET,  handleStatus);
    server.on("/cmd",    HTTP_POST, handleCmd);
    server.on("/wifi",        HTTP_POST, handleWifi);
    server.on("/ble",         HTTP_GET,  handleBleList);
    server.on("/ble/connect", HTTP_POST, handleBleConnect);
    server.on("/ble/forget",  HTTP_POST, handleBleForget);
    server.on("/ble/name",    HTTP_POST, handleBleName);
    // Captive portal redirect — only for requests arriving on the AP interface
    server.onNotFound([]() {
        if (server.client().localIP() == WiFi.softAPIP()) {
            server.sendHeader("Location", "http://192.168.4.1/");
            server.send(302, "text/plain", "");
        } else {
            server.send(404, "text/plain", "Not found");
        }
    });
    server.begin();

    ws.begin();
    ws.onEvent(wsEvent);

    // DNS: catch all domains and return our IP (captive portal)
    dns.start(53, "*", WiFi.softAPIP());
    Serial.println("Web server started");

    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    Serial.println("READY");

    // Discard anything in the RX buffer so boot messages
    // don't get processed as commands in loop()
    delay(100);
    while (Serial.available()) Serial.read();
}

void loop() {
    dns.processNextRequest();
    server.handleClient();
    ws.loop();
    handleSerial();

    // Firmware jiggler — runs independently of browser
    if (jiggling && hid.isConnected()) {
        if (millis() - lastJiggle >= nextJiggleIn) {
            int8_t dx = jiggleRand ? (int8_t)(random(-jiggleStep, jiggleStep + 1)) : (int8_t)jiggleStep;
            int8_t dy = jiggleRand ? (int8_t)(random(-jiggleStep, jiggleStep + 1)) : 0;
            hid.mouseMove(dx, dy);
            jiggleHistory[jiggleHistIdx % 10] = nextJiggleIn;
            jiggleHistIdx++;
            jiggleCount++;
            lastJiggle   = millis();
            nextJiggleIn = random(jiggleMinMs, jiggleMaxMs + 1);
        }
    }
}

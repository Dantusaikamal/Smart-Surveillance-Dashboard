// esp32_cam_server.ino
// Production-grade ESP32-CAM firmware with CORS, heat mitigation, flash control, concurrency lock

#include "esp_camera.h"
#include <WiFi.h>

// ======================== Wi-Fi Configuration ========================
const char* ssid = "YOUR_WIFI_USERNAME";
const char* password = "YOUR_WIFI_PASSWORD";

// ===================== ESP32-CAM AI Thinker Pin Mapping =====================
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#define FLASH_LED_PIN 4

// ============================ Globals ============================
WiFiServer server(80);
bool flashOn = false;
bool isStreaming = false;
unsigned long lastFlashToggleTime = 0;
const unsigned long FLASH_TIMEOUT_MS = 10000;  // Auto turn off after 10s

// ============================ Camera Init ============================
void initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_QVGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[ERROR] Camera init failed: 0x%x\n", err);
    while (true)
      ;
  }
  Serial.println("[INFO] Camera initialized");
}

// ============================ HTTP Handlers ============================
void handleStream(WiFiClient& client) {
  if (isStreaming) {
    client.println("HTTP/1.1 503 Busy\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
    return;
  }
  isStreaming = true;
  Serial.println("[INFO] /stream started");

  client.println("HTTP/1.1 200 OK");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println();

  while (client.connected()) {
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("[WARN] Failed to get frame");
      continue;
    }
    client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
    client.write(fb->buf, fb->len);
    client.println();
    esp_camera_fb_return(fb);

    delay(100);  // Reduce FPS to lower heat
    yield();

    if (flashOn && millis() - lastFlashToggleTime > FLASH_TIMEOUT_MS) {
      flashOn = false;
      digitalWrite(FLASH_LED_PIN, LOW);
      Serial.println("[INFO] Flash auto OFF");
    }
  }

  Serial.println("[INFO] /stream ended");
  isStreaming = false;
}

void handleCapture(WiFiClient& client) {
  if (isStreaming) {
    client.println("HTTP/1.1 503 Busy\r\nAccess-Control-Allow-Origin: *\r\n\r\nESP32 is streaming");
    return;
  }

  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb) {
    client.println("HTTP/1.1 500 Internal Server Error\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
    Serial.println("[ERROR] Capture failed");
    return;
  }

  client.println("HTTP/1.1 200 OK");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Content-Type: image/jpeg");
  client.println("Content-Disposition: inline; filename=\"capture.jpg\"");
  client.printf("Content-Length: %u\r\n\r\n", fb->len);
  client.write(fb->buf, fb->len);
  esp_camera_fb_return(fb);
  Serial.println("[INFO] /capture served");
}

void handleFlash(WiFiClient& client) {
  flashOn = !flashOn;
  digitalWrite(FLASH_LED_PIN, flashOn ? HIGH : LOW);
  lastFlashToggleTime = millis();

  client.println("HTTP/1.1 200 OK");
  client.println("Access-Control-Allow-Origin: *");
  client.println("Content-Type: text/plain\r\n\r\n");
  client.println(flashOn ? "Flash ON" : "Flash OFF");
  Serial.printf("[INFO] Flash toggled: %s\n", flashOn ? "ON" : "OFF");
}

// ============================ Setup ============================
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(1000);

  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, LOW);

  Serial.println("[INFO] Connecting to WiFi...");
  WiFi.begin(ssid, password);
  Serial.println("[INFO] Connecting to WiFi...");
  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n❌ Failed to connect to WiFi.");
  }
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.printf("\n[INFO] WiFi connected. IP: %s\n", WiFi.localIP().toString().c_str());

  initCamera();
  server.begin();
  Serial.println("[INFO] HTTP server started");
}

// ============================ Loop ============================
void loop() {
  WiFiClient client = server.available();
  if (!client) return;

  String req = client.readStringUntil('\r');
  client.flush();

  Serial.print("[INFO] HTTP Request: ");
  Serial.println(req);

  if (req.indexOf("GET /stream") >= 0) {
    handleStream(client);
  } else if (req.indexOf("GET /capture") >= 0) {
    handleCapture(client);
  } else if (req.indexOf("GET /flash") >= 0) {
    handleFlash(client);
  } else {
    client.println("HTTP/1.1 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\n\r\n");
    Serial.println("[WARN] Unknown endpoint requested");
  }

  delay(1);
}

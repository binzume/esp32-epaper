#include <Arduino.h>
#include <GIFDecoder.h>
#include <GxEPD.h>
#include <GxGDEW075Z08/GxGDEW075Z08.h>  // 7.5" b/w/r 800x480
#include <GxIO/GxIO.h>
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include "config.h"
#ifdef ENABLE_MATTER
#include "matter_ble_server.h"
#endif
#define LOG Serial
struct PixelWriter {
  PixelWriter(GxEPD_Class* d) : display(d) {}
  GxEPD_Class* display;
  int width, height, pos;
  void setSize(uint16_t w, uint16_t h) {
    width = w;
    height = h;
    pos = 0;
  }
  void setPalette(uint8_t c, uint8_t r, uint8_t g, uint8_t b) {}
  void writePixel(uint8_t c) {
    uint16_t p[] = {GxEPD_BLACK, GxEPD_RED, GxEPD_WHITE};
    display->drawPixel(pos % width, pos / width, p[c % 3]);
    pos++;
  }
};

struct ByteReader {
  WiFiClient* stream;
  int size;
  ByteReader(WiFiClient* s, int sz) : stream(s), size(sz) {}
  bool eof() { return size <= 0; }
  uint8_t readByte() {
    int l;
    while (size > 0 && ((l = stream->available()) || stream->connected())) {
      if (l == 0) {
        delay(1);
        continue;
      }
      size--;
      return stream->read();
    }
    size = 0;  // set eof
    return 0;
  }
};

void loadBitmap(GxEPD_Class* display, WiFiClient* stream, int size) {
  int l, p = 0;
  int16_t x = 0, y = 0;
  while (((l = stream->available()) || stream->connected()) && p < size) {
    if (l == 0) {
      delay(1);
      continue;
    }
    int b = stream->read();
    p++;
    if (p <= 0x36 + 4 * 2) {
      continue;
    }
    for (int i = 0; i < 8; i++) {
      if (((b << i) & 0x80) == 0) {
        display->drawPixel(x, y, GxEPD_BLACK);
      } else {
        display->drawPixel(x, y, GxEPD_WHITE);
      }
      x++;
      if (x >= 800) {
        x = 0;
        y++;
      }
    }
  }

  LOG.print("Received: ");
  LOG.println(p);
}

void loadGif(GxEPD_Class* display, WiFiClient* stream, int size) {
  ByteReader reader(stream, size);
  PixelWriter writer(display);

  auto decoder = new GIFDecoder<ByteReader, PixelWriter>(reader, writer);
  decoder->decode();
  delete decoder;
}

int loadImage(GxEPD_Class* display) {
  HTTPClient* client = new HTTPClient();
  client->begin(IMG_URL);
  const char* headerKeys[] = {"refresh"};
  client->collectHeaders(headerKeys, 1);
  client->GET();
  long startTime = millis();
  WiFiClient* stream = client->getStreamPtr();
  int size = client->getSize();
  LOG.print("Size: ");
  LOG.println(size);
  LOG.print("Refresh: ");
  int refresh = client->header("refresh").toInt();
  LOG.println(refresh);

  if (stream->peek() == 'B') {
    loadBitmap(display, stream, size);
  } else {
    loadGif(display, stream, size);
  }
  LOG.println("Loaded");
  delete client;

  // disable wifi ASAP
  WiFi.enableSTA(false);

  display->update();
  LOG.println("Updated");

  long elapsedTime = (int)((millis() - startTime) / 1000);
  return refresh == 0 ? 3600
                      : constrain(refresh - elapsedTime, 300, 86400 + 3600);
}

void connect() {
#ifdef ENABLE_MATTER
  if (WiFi.begin() == WL_CONNECT_FAILED) {
    LOG.println("Starting Matter PASE");
    wait_for_commissioning_complete();
  } else {
    LOG.println("Already configured");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      LOG.print(".");
    }
    LOG.println();
  }
#else
  delay(5000);
  Serial.printf("Connecting to %s ", SSID);
  // Wait for connection
  // wl_status_t wl_status = WiFi.begin();
  // if (wl_status == WL_CONNECT_FAILED || wl_status == WL_DISCONNECTED) {
  // WiFi.begin(SSID, PASSWORD);
  //}
  WiFi.begin(SSID, PASSWORD);
  int count = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    LOG.print(".");
    if (++count > 100) {
      LOG.println("timeout");
      esp_sleep_enable_timer_wakeup(3600 * 3 * 1000000LL);
      esp_deep_sleep_start();
      delay(500);
      return;
    }
  }
  LOG.println();
#endif

  LOG.print("Connected to ");
  LOG.println(SSID);
  LOG.print("IP address: ");
  LOG.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

#if ARDUINO_XIAO_ESP32C3
  SPI.begin(3, -1, 2, 4);
#elif ARDUINO_XIAO_ESP32C5
  SPI.begin(0, -1, 1, 25);
#elif ARDUINO_M5Stack_ATOM
  SPI.begin(23, -1, 19, 5);
#endif

#if ARDUINO_XIAO_ESP32C3
  GxIO_Class io(SPI, /*CS=*/4, /*DC=*/5, /*RST=*/6);
  GxEPD_Class* display = new GxEPD_Class(io, /*RST=*/6, /*BUSY=*/7);
#elif ARDUINO_XIAO_ESP32C5
  GxIO_Class io(SPI, /*CS=*/25, /*DC=*/7, /*RST=*/23);
  GxEPD_Class* display = new GxEPD_Class(io, /*RST=*/23, /*BUSY=*/24);
#elif ARDUINO_M5Stack_ATOM
  GxIO_Class io(SPI, /*CS=*/33, /*DC=*/21, /*RST=*/22);
  GxEPD_Class* display = new GxEPD_Class(io, /*RST=*/22, /*BUSY=*/25);
#else
  // NodeMCU ESP-32S
  GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/17, /*RST=*/16);
  GxEPD_Class* display = new GxEPD_Class(io, /*RST=*/16, /*BUSY=*/4);
#endif
  display->init(115200);

  connect();

  int t = loadImage(display);

  delete display;

  esp_sleep_enable_timer_wakeup(t * 1000000LL);
  esp_deep_sleep_start();
}

void loop() {}

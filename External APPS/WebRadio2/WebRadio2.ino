#include "pin_config.h"
#include <WiFi.h>
#include <Wire.h>
#include "AudioGeneratorMP3.h"
#include "AudioFileSourceICYStream.h"
#include "AudioOutputESP32I2S.h"
#include "AudioManager.h"
#include "HWCDC.h"

#define EXAMPLE_SAMPLE_RATE 44100
#define EXAMPLE_VOICE_VOLUME 80                  // 0 - 100
#define EXAMPLE_MIC_GAIN (es8311_mic_gain_t)(3)  // 0 - 7
#define EXAMPLE_RECV_BUF_SIZE (10000)

const char* ssid = "ID525_F2F1B7";
const char* password = "4NDX5MKTWHo5x";
const char* streamURL = "http://stream.srg-ssr.ch/m/rsj/mp3_128";

AudioGeneratorMP3 mp3;
AudioFileSourceICYStream icyStream;
AudioOutputESP32I2S out;
I2SClass i2s;
HWCDC USBSerial;

void setup() {
  USBSerial.begin(115200);

  //USBSerial.println("Startup!");

  if (!i2s.begin(I2S_MODE_STD, EXAMPLE_SAMPLE_RATE,
               I2S_DATA_BIT_WIDTH_16BIT,
               I2S_SLOT_MODE_STEREO,
               I2S_STD_SLOT_BOTH)) {
    USBSerial.println("Failed to initialize I2S bus!");
    return;
  }
  Wire.begin(IIC_SDA, IIC_SCL);

  // 🔊 init audio (USA IL TUO STACK)
  AudioManager::init();

  // 🌐 WiFi
  WiFi.begin(ssid, password);
  USBSerial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    USBSerial.print(".");
    delay(500);
  }
  USBSerial.println("\nConnected");

  // 🎧 stream
  icyStream.open(streamURL);
  mp3.begin(&icyStream, &out);

  USBSerial.println("Streaming started");
}

void loop() {
  if (mp3.isRunning()) {
    if (!mp3.loop()) {
      USBSerial.println("MP3 loop error");
      mp3.stop();
    }
  } else {
    USBSerial.println("Restart stream");
    icyStream.close();
    icyStream.open(streamURL);
    mp3.begin(&icyStream, &out);
  }
}
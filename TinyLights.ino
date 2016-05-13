#include <FastLED.h>
#include <SimpleTimer.h>
#include <EEPROM.h>
#include <SimpleList.h>

#include <StandardCplusplus.h>
#include <vector>
#include <iterator>

#define LED_PIN     PD5
#define NUM_LEDS    5
#define BRIGHTNESS  200

#define UPDATES_PER_SECOND 25

#define LED_TYPE    WS2812
#define COLOR_ORDER GRB

#define LINE_FEED 10


CRGB leds[NUM_LEDS];

SimpleTimer timer;

std::vector<String> parseFrame(String frame) {

  std::vector<String> arguments;
  
  int indexBegin = frame.indexOf('>');
  if(indexBegin == -1) {
    return arguments;
  }

  frame = frame.substring(indexBegin + 1);
  
  int indexOfDelimiter = 0, offset = 0;
  while((indexOfDelimiter = frame.indexOf(',', offset)) != -1) {
    arguments.push_back(frame.substring(offset, indexOfDelimiter));
    offset = indexOfDelimiter + 1;
  }

  arguments.push_back(frame.substring(offset));
  return arguments;
}

String buildFrame(std::vector<String> arguments) {
  String result = ">";
  
  for(String argument : arguments) {
    result += argument + ",";
  }

  result = result + "\n";
  result.replace(",\n", "\n");

  return result;
}


void setup() {
  
  Serial.begin(115200);
  
  std::vector<String> output;
  output.push_back(String("HI"));

  Serial.print(buildFrame(output));
  
  delay(500); // power-up safety delay
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(BRIGHTNESS);


  if(EEPROM.read(0) == 42) {
    loadLedValues();
  } else {
    for(uint16_t i = 0; i < NUM_LEDS; i++) {
      leds[i] = CRGB(255, 0, 0);
    }
  }

  timer.setInterval(1000 / UPDATES_PER_SECOND, updateLeds);
}

void loadLedValues() { 
  for(int i = 0; i < NUM_LEDS; i++) {
    int offset = i * 3;
    leds[i] = CRGB(EEPROM.read(offset + 1), EEPROM.read(offset + 2), EEPROM.read(offset + 3));
  }
}

void storeLedValues() {
  EEPROM.write(0, 42);

  for(int i = 0; i < NUM_LEDS; i++) {
    int offset = i * 3;
    
    EEPROM.write(offset + 1, leds[i].red);
    EEPROM.write(offset + 2, leds[i].green);
    EEPROM.write(offset + 3, leds[i].blue);
  }
}

void sendOk() {
  std::vector<String> output;
  output.push_back("OK");
  Serial.print(buildFrame(output));
}

void loop() {

  if(Serial.available() > 0) {
      String inputBuffer = Serial.readStringUntil(LINE_FEED);

      std::vector<String> output = parseFrame(inputBuffer);
      if(output.size() > 0) {

        if(output[0] == "SET" && output.size() == 5) {
          setLedColor(output[1].toInt(), output[2].toInt(), output[3].toInt(), output[4].toInt());
          sendOk();
        } else if(output[0] == "GET" && output.size() == 2) {
          getLedColor(output[1].toInt());
        } else if(output[0] == "FLASH" && output.size() == 1) {
          storeLedValues();
          sendOk();
        } else if(output[0] == "NUM" && output.size() == 1) {
          std::vector<String> output;
          output.push_back("NUM");
          output.push_back(String(NUM_LEDS));

          Serial.print(buildFrame(output));
        }
      }

      inputBuffer[0] = '\0';
  }
  
  timer.run();
}

void setLedColor(int index, int r, int g, int b) {
  
  if(index > NUM_LEDS) {
    return;
  }

  leds[index] = CRGB(r, g, b);
}

void getLedColor(int index) {
 
  if(index > NUM_LEDS) {
    return;
  }

  std::vector<String> frame;
  
  frame.push_back("GET");
  frame.push_back(String(index));
  frame.push_back(String(leds[index].red));
  frame.push_back(String(leds[index].green));
  frame.push_back(String(leds[index].blue));

  Serial.println(buildFrame(frame));
}

void updateLeds() {
  FastLED.show();
}


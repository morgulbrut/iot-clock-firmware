#include <math.h>

#include <algorithm>
#include <iterator>

#define PIXEL_PIN D2
#define PIXEL_COUNT 60
#define PIXEL_TYPE WS2812B

#include "libs/neopixel.h"


// thanks stackoverflow
#define CHECK_BIT(var,pos) !!((var) & (1<<(static_cast<int>(pos))))

Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIXEL_PIN, PIXEL_TYPE);

/*
Handsmode:

Binary        | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 0 0 0 0 | 1
Reverse       | 0 0 0 0 | 0 0 0 0 | 1 1 1 1 | 1 1 1 1 | X
Trail         | 0 0 0 0 | 1 1 1 1 | 0 0 0 0 | 1 1 1 1 | X
random Secs   | 0 0 1 1 | 0 0 1 1 | 0 0 1 1 | 0 0 1 1 | X
rainbow Secs  | 0 1 0 1 | 0 1 0 1 | 0 1 0 1 | 0 1 0 1 | X

*/

int handsMode = 16;

enum class modesMask{
  RAINBOW_SECS,RAND_SECS,TRAIL,REVERSE,BINARY
};

int hourHue = 60;
int minuteHue = 180;
int secondHue = 300;
int timeShift = 1;
int brightness = 60;

String wifi;
String config;

int isConnected;

bool hourPxl[PIXEL_COUNT], minPxl[PIXEL_COUNT],secPxl[PIXEL_COUNT],hMarkPxl[PIXEL_COUNT];
float handCorr = 0.0;
int hourPos,minutePos,secondPos;

bool leds[8];

void setup(){

  Particle.function("tellTime",tellTime);
  //Particle.function("setTShift",setTimeShift);
  Particle.function("setHaMode",setHandsMode);
  Particle.function("setBright",setBright);
  Particle.function("setHHue",setHourHue);
  Particle.function("setMHue",setMinHue);
  Particle.function("setSHue",setSecHue);
  Particle.function("rndMode",randomMode);
  Particle.function("alert",alert);

  Particle.variable("getHaMode",handsMode);
  Particle.variable("getHHue",hourHue);
  Particle.variable("getMHue",minuteHue);
  Particle.variable("getSHue",secondHue);
  Particle.variable("getBright",brightness);
  Particle.variable("getWifi",wifi);
  Particle.variable("getConfig",config);

  wifi = WiFi.SSID();

  strip.begin();
  strip.clear();
  strip.setBrightness(brightness);

  Time.zone(timeShift);
  isConnected = 2;

  randomSeed(Time.second());
  randomMode("");
  RGB.control(true);
  RGB.color(0,0,0);
}

void loop(){

  if(isConnected == 2 && Particle.connected()){
    isConnected = 1;
  }
  handCorr=Time.minute()/60.0;
  calcHandsPos();
  drawClock();
  strip.show();
  config = getConfig();

  delay(100);
}


void calcHandsPos(){

  clearStrip();

  if(!CHECK_BIT(handsMode,modesMask::BINARY)){

    // set the hourmarks
    for(int i = 0; i<PIXEL_COUNT; i++){
      if(i%5==0){
        hMarkPxl[i]=true;
      }
    }

    // set the time pos

    if(CHECK_BIT(handsMode,modesMask::RAND_SECS)){
      hourPos = int(5*(Time.hour()%12+handCorr));
      minutePos = Time.minute();
      if(random(50)>40){
        secondPos = Time.second();
      }
    }
    else{
      hourPos = int(5*(Time.hour()%12+handCorr));
      minutePos = Time.minute();
      secondPos = Time.second();
    }

    if(CHECK_BIT(handsMode,modesMask::TRAIL)){
      for( int i = 0;i <=hourPos;i++){
        hourPxl[i]=true;
      }
      for( int i = 0;i <=minutePos;i++){
        minPxl[i]=true;
      }
      for( int i = 0;i <=secondPos;i++){
        secPxl[i]=true;
      }
    }
    else {
      hourPxl[hourPos]=true;
      minPxl[minutePos]=true;
      secPxl[secondPos]=true;
    }



    if(CHECK_BIT(handsMode,modesMask::REVERSE)){
      // reverse arrays;
      std::reverse(std::begin(hourPxl), std::end(hourPxl));
      std::reverse(std::begin(minPxl), std::end(minPxl));
      std::reverse(std::begin(secPxl), std::end(secPxl));
    }
  }

  else{  // BINARY MODE
    intToBin(leds,Time.second());
    strip.clear();
    for (int i=0; i<7; i++) {

      if(leds[i]==1){
        secPxl[i*2] = true;
      }
      else{
        hMarkPxl[i*2] = true;
      }
    }
    intToBin(leds,Time.minute());
    for (int i=0; i<7; i++) {
      if(leds[i]==1){
        minPxl[PIXEL_COUNT/3+i*2]=true;
      }
      else{
        hMarkPxl[PIXEL_COUNT/3+i*2]=true;
      }
    }
    intToBin(leds,Time.hour());
    for (int i=0; i<7; i++) {
      if(leds[i]==1){
        hourPxl[PIXEL_COUNT*2/3+i*2]=true;
      }
      else{
        hMarkPxl[PIXEL_COUNT*2/3+i*2]=true;
      }
    }
  }
}

void drawClock(){

  strip.clear();

  drawHMarks();

  if(secondPos>=minutePos && minutePos>=hourPos ){
    drawSec();
    drawMin();
    drawHour();
  }
  else if(secondPos>=hourPos && hourPos>=minutePos ){
    drawSec();
    drawHour();
    drawMin();
  }

  else if(minutePos>=secondPos && secondPos>=hourPos ){
    drawMin();
    drawSec();
    drawHour();
  }
  else if(minutePos>=hourPos && hourPos>=secondPos ){
    drawMin();
    drawHour();
    drawSec();
  }

  else if(hourPos>=minutePos && minutePos>=secondPos ){
    drawHour();
    drawMin();
    drawSec();
  }
  else if(hourPos>=secondPos && secondPos>=minutePos ){
    drawHour();
    drawSec();
    drawMin();
  }
}

void drawHMarks(){
  for(int i = 0; i<PIXEL_COUNT; i++){
    if (hMarkPxl[i]) strip.setPixelColor(i,strip.Color(20,20,20));
  }
}

void drawSec(){
  for(int i = 0; i<PIXEL_COUNT; i++){
    if (secPxl[i]){
      if(CHECK_BIT(handsMode,modesMask::RAINBOW_SECS)){
        strip.setPixelColor(i,ColorHSV(i*6,255,255));
      }else{
        strip.setPixelColor(i,ColorHSV(secondHue,255,255));
      }
    }
  }
}

void drawMin(){
  for(int i = 0; i<PIXEL_COUNT; i++){
    if (minPxl[i]) strip.setPixelColor(i,ColorHSV(minuteHue,255,255));
  }
}

void drawHour(){
  for(int i = 0; i<PIXEL_COUNT; i++){
    if (hourPxl[i]) strip.setPixelColor(i,ColorHSV(hourHue,255,255));
  }
}

void clearStrip(){
  for(int i = 0; i<PIXEL_COUNT; i++){
    hourPxl[i]=false;
    minPxl[i]=false;
    secPxl[i]=false;
    hMarkPxl[i]=false;
  }
}

uint32_t ColorHSV(uint16_t hue,uint8_t sat, uint8_t val){
  // hue: 0..360
  // sat,val: 0..255
  uint16_t r,g,b;

  uint16_t H_accent = hue/60;
  //unsigned int bottom = ((255 - sat) * val)>>8;
  uint16_t bottom = 0;
  uint16_t top = val;
  uint16_t rising  = ((top-bottom)  *(hue%60   )  )  /  60  +  bottom;
  uint16_t falling = ((top-bottom)  *(60-hue%60)  )  /  60  +  bottom;

  switch(H_accent) {
    case 0:
    r = top;
    g = rising;
    b = bottom;
    break;

    case 1:
    r = falling;
    g = top;
    b = bottom;
    break;

    case 2:
    r = bottom;
    g = top;
    b = rising;
    break;

    case 3:
    r = bottom;
    g = falling;
    b = top;
    break;

    case 4:
    r = rising;
    g = bottom;
    b = top;
    break;

    case 5:
    r = top;
    g = bottom;
    b = falling;
    break;
  }
  return strip.Color(r,g,b);
}

void intToBin(bool *binArr,int val){
  for (int i=0; i<8; i++) {
    if (val%2){
      binArr[i]=1;
    }
    else{
      binArr[i]=0;
    }
    val/=2;
  }
}

String getConfig(){
  String ret = "{ 'mode' : ";
  ret += static_cast<String>(handsMode);
  ret += " , 'hourHue' : ";
  ret += static_cast<String>(hourHue);
  ret += " , 'minuteHue' : ";
  ret += static_cast<String>(minuteHue);
  ret += " , 'secondHue' : ";
  ret += static_cast<String>(secondHue);
  ret += " , 'brightness' : ";
  ret += static_cast<String>(brightness);
  ret += " , 'wifi' : '" + wifi + "' }";
  return ret;
}



/* Callback functions */

int setHandsMode(String command){
  handsMode = command.toInt();
  return handsMode;
}

int tellTime(String command){
  if (command == "hour"){
    return Time.hour();
  } else if (command == "minute"){
    return Time.minute();
  } else if (command == "second"){
    return Time.second();
  } else if (command == "now"){
    return Time.now();
  } else{
    return -1;
  }
}

int setHourHue(String command){
  hourHue = constrain(command.toInt(),0,359);
  return hourHue;
}

int setMinHue(String command){
  minuteHue = constrain(command.toInt(),0,359);
  return minuteHue;
}

int setSecHue(String command){
  secondHue = constrain(command.toInt(),0,359);
  return secondHue;
}

int setBright(String command){
  brightness = constrain(command.toInt(),1,150);
  strip.setBrightness(brightness);
  return brightness;
}

int alert(String command){
  int col = constrain(command.toInt(),0,359);
  for(int i = 0; i<PIXEL_COUNT; i++){
    strip.setPixelColor(i,ColorHSV(col,255,255));
    strip.show();
    delay(20);
  }
  delay(100);
  for(int i = PIXEL_COUNT; i>=0; i--){
    strip.setPixelColor(i,strip.Color(0,0,0));
    strip.show();
    delay(20);
  }
  return col;
}

int randomMode(String command){
  handsMode = random(17);
  hourHue = random(359);
  minuteHue = random(359);
  secondHue = random(359);
  return handsMode;
}

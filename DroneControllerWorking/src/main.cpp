#include <Arduino.h>
#include "WiFi.h"
#include "AsyncUDP.h"

int BUTTON = 22;
int LED_PIN = 5;

int JOYX_PIN = 35;
int JOYY_PIN = 34;
int JOYBTN_PIN = 32;
bool commandSent;
bool takeoffSent;
bool landSent;
String lastMessage = "no last message";

// state is used to define if the button should function as a "takeoff"-button or as a "land"-button
int state = 0;


//const char * ssid = "TELLO-FE2F25";
// We know this shouldn't be available for everyone but screw it
const char * ssid2 = "KoebenhavnKommune";
const char * password = "ringtilreinert";

AsyncUDP udp;


void setup()
{
  pinMode(LED_PIN, OUTPUT);
  commandSent=false;
  takeoffSent=false;
  landSent=false;
state = 0;
pinMode(BUTTON, INPUT);
Serial.begin(9600);


WiFi.mode(WIFI_STA);
WiFi.begin(ssid2, password);
//WiFi.begin(ssid);
if (WiFi.waitForConnectResult() != WL_CONNECTED) {
Serial.println("WiFi Failed");
while(1) {
delay(1000);
}
}
}

// this method will measure the output of the joystic and send the relevant udp message to the emulator
// it has a natural "deadzone" that is called if the joystick is centered
void formatPrintJoystickValues(std::pair<int,int> joystick) {
if (joystick.first > 3000) {
Serial.println("up");
udp.writeTo((const uint8_t*)"up", 2, IPAddress(192,168,0,75), 6000);
lastMessage="up";
}

else if (joystick.first<1300) {
Serial.println("down");
udp.writeTo((const uint8_t*)"down", 4, IPAddress(192,168,0,75), 6000);
lastMessage="down";
}


if (joystick.second>2000) {
Serial.println("right");
udp.writeTo((const uint8_t*)"right", 5, IPAddress(192,168,0,75), 6000);
lastMessage="right";

} else if (joystick.second<1400) {
Serial.println("left");
udp.writeTo((const uint8_t*)"left", 4, IPAddress(192,168,0,75), 6000);
} 

// could be written only using "else" instead of "else if" but for some reason it didn't work aswell
else if (joystick.first>1300 && joystick.first<3000 && joystick.second<2000 && joystick.second>1400){
  // this extra condition is used to make sure the ESP isn't spamming "not moving" since it will trigger an animation in the emulator
  // it doesn't really matter though, it's just kinda annoying
  if (!lastMessage.equals("notmoving")) {
  Serial.println("not moving");
  udp.writeTo((const uint8_t*)"notmoving", 9, IPAddress(192,168,0,75), 6000);
  }
  lastMessage="notmoving";
}

}


void formatPrintJoystickButton(int BTN) {
String s = "Button: " + String(BTN);
Serial.println(s);
}


// combining the joystick's sensors
std::pair<int, int> getJoystickValues() {
int xVal = analogRead(JOYX_PIN);
int yVal = analogRead(JOYY_PIN);
return std::make_pair(xVal, yVal);
}

int getButtonValue() {
  int ButtonState = digitalRead(BUTTON);
  return ButtonState;
}

// takeoff and land functions that are triggered by the button
void takeoff() {
  udp.writeTo((const uint8_t*)"takeoff", 7, IPAddress(192,168,0,75), 6000);
  lastMessage="takeoff";
  takeoffSent=true;
}

void land() {
  udp.writeTo((const uint8_t*)"land", 4, IPAddress(192,168,0,75), 6000);
  lastMessage="land";
  landSent=true;
}

// the method that triggers methods above
void buttonStuff(int ButtonState) {
  if (state==0) {
    digitalWrite(LED_PIN, LOW);
    if (ButtonState==HIGH && takeoffSent==false) {
    takeoff();
    Serial.println("takeoff has been initiated");
    // we were trying to avoid using delay completely since it will freeze the whole program
    // but the wouldn't work cleanly if we didn't
    delay(500);
    }
    // this is the attempt we made to avoid delay:
    // the state should only change once the button was released, therefor not triggering the land and takeoff function simultaneously
    // but for some reason it kept calling the other method after the release of the button.
    // at some point we decided to stop trying to debug this and just add the delay
    if (ButtonState==LOW && takeoffSent) {
      state=1;
    }
  }
  
  // same as above but opposite :)
  if (state==1) {
    digitalWrite(LED_PIN, HIGH);
    if (ButtonState==HIGH && landSent==false) {
    land();
    Serial.println("land has been initiated");
    delay(500);
    }
    if (ButtonState==LOW && landSent) {
      state=0;
    }
  }
}

void loop() {
  int ButtonState = getButtonValue();
  buttonStuff(ButtonState);
  if (takeoffSent && landSent) {
    takeoffSent=false;
    landSent=false;
  }
std::pair<int, int> joystick = getJoystickValues();
formatPrintJoystickValues(joystick);

//udp.onPacket([](AsyncUDPPacket packet) {
  //Serial.write(packet.data(), packet.length());
//});

}
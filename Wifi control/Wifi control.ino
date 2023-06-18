/*
 A simple web server that lets you blink the onboard RGB LED via the web.

 Run on ESP32-S2-Saola-1 (ESP32-S2),
 ast as access point, setup web server,
 and turn ON/OFF onboard RGB LED (NexPixel) accordingly.

 The onboard RGB LED of ESP32-S2-Saola-1 is connected to GPIO18.

 http://yourAddress/H turns the LED on
 http://yourAddress/L turns it off

reference examples in Arduino:
Examples > WiFi > WiFiAccessPoint
Examples > WiFi > SimpleWiFiServer
Examples > Adafruit NeoPixel > simple
 
 */
#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <Esp.h>
#include "HCPCA9685.h"

/* I2C slave address for the device/module. For the HCMODU0097 the default I2C address
   is 0x40 */
#define  I2CAdd 0x40


//Onboard RGB LED (NeoPixel)
#define PIXELPIN  18
#define NUMPIXELS 1

HCPCA9685 HCPCA9685(I2CAdd);

int TibiaAngle[] = {270,180,90,180};
int FemurAngle[] = {270,180,90,180};

int j;
int k;
int l;

unsigned long previousMillis = 0;
unsigned long interval = 1500;


enum State {
  STORE,
  LEFT,
  RIGHT,
  WALK,
  REVERSE,
  STAND,
  CRAWL,
  REVERSECRAWL,
  PUSHUP,
  SHAKE,
  NUM_STATES  // always keep this one last
};
bool isTransitioning = false;
State currentState = STORE;
State nextState = STORE; 

unsigned long stateStartMillis;
unsigned long mainMillis = 0; 
// const long mainInterval = 1000;  // interval at which to do something (milliseconds)
unsigned long stateDuration = 0;

bool isFuncRunning = false; // a flag to check if the function is running
unsigned long funcStartMillis = 0; // timestamp when the function started
unsigned long funcDuration = 5000; // function duration, e.g., 5000 milliseconds (5 seconds)
int mode = 1;


const char* ssid     = "esp32s2";
const char* password = "password";

const char* html="<html>"
      "<head>"
      "<meta charset=\"utf-8\">"
      "<title>ESP32-S2 Web control NeoPixel</title>"
      "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1, maximum-scale=1\">"
      "</head>"
      "<body>"
                   "Click <a href=\"/S\">here</a> to Sleep.<br>"
                   "Click <a href=\"/L\">here</a> to Left.<br>"
                   "Click <a href=\"/R\">here</a> to Right.<br>"
                   "Click <a href=\"/W\">here</a> to Walk.<br>"
                   "Click <a href=\"/U\">here</a> to Reverse.<br>"
                   "Click <a href=\"/T\">here</a> to Stand.<br>"
                   "Click <a href=\"/C\">here</a> to Crawl.<br>"
                   "Click <a href=\"/V\">here</a> to ReverseCrawl.<br>"
                   "Click <a href=\"/P\">here</a> to Pushups.<br>"
                   "Click <a href=\"/H\">here</a> to Shake.<br>"
      "</body>"
      "</html>";

Adafruit_NeoPixel pixel(NUMPIXELS, PIXELPIN, NEO_GRB + NEO_KHZ800);
WiFiServer server(80);


double mapValues(int input_value) {
  const double input_min = 30.0;
  const double input_max = 330.0;
  const double output_min = 0.0;
  const double output_max = 470.0;
  
  double input_range = input_max - input_min;
  double output_range = output_max - output_min;
  double ratio = (input_value - input_min) / input_range;
  
  double output_value = (ratio * output_range) + output_min;
  
  if(output_value <= 30){
    output_value = 50;
  }
  if(output_value >= 470){
    output_value = 450;
  }

  return output_value;

}



void setup()
{
    Serial.begin(115200);
    pixel.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
    pixel.clear(); // Set pixel colors to 'off'
    pixel.show();
      /* Initialise the library and set it to 'servo mode' */ 
  HCPCA9685.Init(SERVO_MODE);
HCPCA9685.Sleep(false);
    delay(1);
    Serial.printf("\n\n---Start---\n");
    Serial.print("Chip Model: ");
    Serial.print(ESP.getChipModel());
    Serial.print("\nChip Revision: ");
    Serial.print(ESP.getChipRevision());
    Serial.println();
  
    Serial.println();
    Serial.println("Configuring access point...");

    WiFi.softAP(ssid, password);

    IPAddress myIP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(myIP);
    server.begin();
    
    Serial.println("Server started");

}

int value = 0;

void loop(){
 WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");          // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character

          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            client.print(html);
            // break out of the while loop:
            break;
          } else {    // if you got a newline, then clear currentLine:
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }

        // Check to see if the client request was "GET /H" or "GET /L":
              if (currentLine.endsWith("GET /S")) {
          nextState = STORE;
          Store();
        }
        if (currentLine.endsWith("GET /L")) {
          nextState = LEFT;
          Left();
        }
        if (currentLine.endsWith("GET /R")) {
          nextState = RIGHT;
          Right();
        }
        if (currentLine.endsWith("GET /W")) {
          nextState = WALK;
          Walk();
        }
        if (currentLine.endsWith("GET /U")) {
          nextState = REVERSE;
          ReverseWalk();
        }
        if (currentLine.endsWith("GET /P")) {
          nextState = PUSHUP;
          PushUps();
        }
        if (currentLine.endsWith("GET /H")) {
          nextState = SHAKE;
          Shake();
        }
        if (currentLine.endsWith("GET /T")) {
          nextState = STAND;
          Stand();
        }
        if (currentLine.endsWith("GET /C")) {
          nextState = CRAWL;
          Crawl();
        }
        if (currentLine.endsWith("GET /V")) {
          nextState = REVERSECRAWL;
          ReverseCrawl();
        }
        pixel.show();
      }
    }
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
    // If we're not transitioning, check if we need to switch state
    // if (!isTransitioning) {
    //     if (currentState != nextState) {
    //         currentState = nextState;
    //         isTransitioning = true;
    //     }
    // }

    // // Only run the function if we're transitioning
    // if (isTransitioning) {
    //     unsigned long stateStart = millis();

    //     // ... existing switch-case here ...

    //     // If the function has run for long enough, stop transitioning
    //     if (millis() - stateStart >= stateDuration) {
    //         isTransitioning = false;
    //     }
    // }
  

    switch (nextState) {
    case STORE:
      Store();
      stateDuration = 1000;  // run for 1 seconds
      break;
    case LEFT:
      Left();
      stateDuration = 800;  // run for 0.8 seconds
      break;
    case RIGHT:
      Right();
      stateDuration = 2500;  // run for 2.5 seconds
      break;
    case WALK:
      Walk();
      stateDuration = 5000;  // run for 10 seconds
      break;
    case REVERSE:
      ReverseWalk();
      stateDuration = 5000;  // run for 5 seconds
      break;
      case STAND:
      Stand();
      stateDuration = 5000;  // run for 10 seconds
      break;
      case CRAWL:
      Crawl();
      stateDuration = 5000;  // run for 10 seconds
      break;
      case REVERSECRAWL:
      ReverseCrawl();
      stateDuration = 5000;  // run for 10 seconds
      break;
      case PUSHUP:
      PushUps();
      stateDuration = 8000;  // run for 10 seconds
      break;
      case SHAKE:
      Shake();
      stateDuration = 5000;  // run for 10 seconds
      break;
      // break;
    default:
      break;
  }
}

void Shake() {
int TibiaAngle[] = {180,180,180,180};
// int FemurAngle[] = {180,180,180,180};

int BRTibiaAngle[] = {135,135,135,135};
int BRFemurAngle[] = {135,135,135,135};
int BLTibiaAngle[] = {225,225,225,225};
int BLFemurAngle[] = {240,240,240,240};

int FLTibiaAngle[] = {135,135,135,135};
int FLFemurAngle[] = {135,135,135,135};


int FRFemurAngle[] = {30,30,30,30};
int FRTibiaAngle[] = {200,160,200,160};

// int FFemurAngle[] = {280,280,280,280};
// int BFemurAngle[] = {90,90,90,90};
interval = 250;

  static int i = 0;
  if (i >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    i = 0;
  }
  
  /* Set the position of the servos */
  //Back Left
  HCPCA9685.Servo(0, mapValues(BLTibiaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BLFemurAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(4, mapValues(BRTibiaAngle[i]-4));  
  HCPCA9685.Servo(3, mapValues(BRFemurAngle[i]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FLTibiaAngle[i]));  
  HCPCA9685.Servo(7, mapValues(FLFemurAngle[i]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FRTibiaAngle[i]-10));
  HCPCA9685.Servo(10, mapValues(FRFemurAngle[i])+18);

  /* Update the loop variables */
  i++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}

void Stand() {
int TibiaAngle[] = {180,180,180,180};
int FemurAngle[] = {180,180,180,180};
interval = 150;

  static int i = 0;
  if (i >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    i = 0;
  }
  
  /* Set the position of the servos */
  //Front right
  HCPCA9685.Servo(0, mapValues(TibiaAngle[i])-4);  
  HCPCA9685.Servo(1, mapValues(FemurAngle[i])-4);  
  //Front left
  HCPCA9685.Servo(3, mapValues(TibiaAngle[i]-4));  
  HCPCA9685.Servo(4, mapValues(FemurAngle[i]-3));  
  //Back right
  HCPCA9685.Servo(6, mapValues(TibiaAngle[i]));  
  HCPCA9685.Servo(7, mapValues(FemurAngle[i]));  
  //Back left
  HCPCA9685.Servo(9, mapValues(TibiaAngle[i]-10));
  HCPCA9685.Servo(10, mapValues(FemurAngle[i])+10);

  /* Update the loop variables */
  i++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}


void PushUps() {
int TibiaAngle[] = {180,180,180,180};
// int FemurAngle[] = {180,180,180,180};
int FFemurAngle[] = {180,180,90,90};
int FTibiaAngle[] = {180,180,60,60};


int BFemurAngle[] = {270,270,270,270};
int BTibiaAngle[] = {270,270,270,270};


// int FFemurAngle[] = {280,280,280,280};
// int BFemurAngle[] = {90,90,90,90};
interval = 400;

  static int i = 0;
  if (i >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    i = 0;
  }
  
  /* Set the position of the servos */
  //Back Left
  HCPCA9685.Servo(0, mapValues(BTibiaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BFemurAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(4, mapValues(BTibiaAngle[i]-4));  
  HCPCA9685.Servo(3, mapValues(BFemurAngle[i]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FTibiaAngle[i]));  
  HCPCA9685.Servo(7, mapValues(FFemurAngle[i]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FTibiaAngle[i]-10));
  HCPCA9685.Servo(10, mapValues(FFemurAngle[i])+18);

  /* Update the loop variables */
  i++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}
void Crawl() {

int FemurAngle[] = {229,131,131,144,161,180,199,216,229};
int TibiaAngle[] = {262,98,98,129,156,180,204,231,262};


interval = 350;

  static int i = 0, j = 0.25*sizeof(TibiaAngle)/sizeof(TibiaAngle[0]), k = 0.5*sizeof(TibiaAngle)/sizeof(TibiaAngle[0]), l = 0.75*sizeof(TibiaAngle)/sizeof(TibiaAngle[0]);
  if (i >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    i = 0;
  }
  if (j >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    j = 0;
  }
  if (k >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    k = 0;
  }
  if (l >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    l = 0;
  }
  
  /* Set the position of the servos */
  //Front right
  HCPCA9685.Servo(0, mapValues(TibiaAngle[i])-4);  
  HCPCA9685.Servo(1, mapValues(FemurAngle[i])-4);  
  //Front left
  HCPCA9685.Servo(3, mapValues(TibiaAngle[k]-4));  
  HCPCA9685.Servo(4, mapValues(FemurAngle[k]-3));  
  //Back right
  HCPCA9685.Servo(6, mapValues(TibiaAngle[l]));  
  HCPCA9685.Servo(7, mapValues(FemurAngle[l]));  
  //Back left
  HCPCA9685.Servo(9, mapValues(TibiaAngle[j]-10));
  HCPCA9685.Servo(10, mapValues(FemurAngle[j])+10);

  /* Update the loop variables */
  i++;
  j++;
  k++;
  l++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}

void ReverseCrawl() {
int FemurAngle[] = {229,216,199,180,161,144,131,131,229};
int TibiaAngle[] = {262,231,204,180,156,129,98,98,262};


interval = 350;

  static int i = 0, j = 0.25*sizeof(TibiaAngle)/sizeof(TibiaAngle[0]), k = 0.5*sizeof(TibiaAngle)/sizeof(TibiaAngle[0]), l = 0.75*sizeof(TibiaAngle)/sizeof(TibiaAngle[0]);
  if (i >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    i = 0;
  }
  if (j >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    j = 0;
  }
  if (k >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    k = 0;
  }
  if (l >= sizeof(TibiaAngle)/sizeof(TibiaAngle[0])) {
    l = 0;
  }
  
  /* Set the position of the servos */
  //Front right
  HCPCA9685.Servo(0, mapValues(TibiaAngle[i])-4);  
  HCPCA9685.Servo(1, mapValues(FemurAngle[i])-4);  
  //Front left
  HCPCA9685.Servo(3, mapValues(TibiaAngle[k]-4));  
  HCPCA9685.Servo(4, mapValues(FemurAngle[k]-3));  
  //Back right
  HCPCA9685.Servo(6, mapValues(TibiaAngle[l]));  
  HCPCA9685.Servo(7, mapValues(FemurAngle[l]));  
  //Back left
  HCPCA9685.Servo(9, mapValues(TibiaAngle[j]-10));
  HCPCA9685.Servo(10, mapValues(FemurAngle[j])+10);

  /* Update the loop variables */
  i++;
  j++;
  k++;
  l++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}



void Store() {
int FFemerAngle[] = {280,280,280,280};
int BFemerAngle[] = {90,90,90,90};

int FTibeaAngle[] = {300,300,300,300};
int BTibeaAngle[] = {60,60,60,60};
interval = 150;

  static int i = 0;
  if (i >= sizeof(FTibeaAngle)/sizeof(FTibeaAngle[0])) {
    i = 0;
  }
  
    /* Set the position of the servos */
  //Back Left
  HCPCA9685.Servo(0, mapValues(BTibeaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BFemerAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(3, mapValues(BTibeaAngle[i]-4));  
  HCPCA9685.Servo(4, mapValues(BFemerAngle[i]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FTibeaAngle[i]));  
  HCPCA9685.Servo(7, mapValues(FFemerAngle[i]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FTibeaAngle[i]-10));
  HCPCA9685.Servo(10, mapValues(FFemerAngle[i])+18);

  /* Update the loop variables */
  i++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}

void Right() {
// int RFemerAngle[] = {177,177,174,169,165,165,157,149,143,139,139,140,144,151,153,153,153,154,154,155,156,156,157,158,158,159,160,160,161,161,162,162,163,163,163,163,164,164,165,165,165,166,166,167,167,168,168,169,169,170,171,171,172,173,174,175,175,176,177,178};
// int RTibeaAngle[] = {158,145,123,105,98,96,97,101,109,122,122,132,149,169,167,167,167,166,166,167,167,167,167,167,167,167,167,167,167,166,166,165,164,163,162,161,159,158,158,157,156,155,154,154,154,153,153,153,153,153,154,154,155,156,156,157,158,160,161,163};

// int LFemerAngle[] = {177,177,174,169,165,165,160,156,152,148,148,147,149,152,155,155,156,157,157,158,159,159,160,161,161,162,162,163,164,164,165,165,166,166,167,167,167,168,168,169,169,169,170,170,170,171,171,172,172,172,173,174,174,175,175,176,176,177,178,178};
// int LTibeaAngle[] = {158,145,123,105,98,96,96,97,99,102,102,108,119,129,131,131,132,133,135,136,137,138,139,140,142,143,144,145,146,147,148,148,149,149,149,150,150,149,150,150,150,150,150,150,151,151,151,152,152,153,153,154,155,156,156,158,159,160,161,163};
// int RFemerAngle[] = {172,168,159,159,148,140,140,144,152,152,153,154,155,157,158,159,161,162,162,163,163,163,164,164,164,165,165,166,167,167,168,169,170,171,172,174};
// int RTibeaAngle[] = {158,121,97,96,102,119,119,144,159,159,160,162,163,165,167,169,170,171,171,170,168,166,163,162,160,159,158,157,157,157,157,157,158,159,160,162};

// int LFemerAngle[] = {172,168,159,159,151,144,144,146,152,152,153,155,156,157,158,159,160,161,162,163,164,164,164,165,166,166,166,167,168,168,169,170,171,172,173,174};
// int LTibeaAngle[] = {158,121,97,96,100,108,108,127,139,139,141,143,145,147,149,151,153,154,155,156,156,156,155,156,155,155,154,154,155,155,156,157,158,159,160,162};


// int BLFemerAngle[] = {180,225,225,135,135,135,135,180,180};
// int BLTibeaAngle[] =  {180,225,225,135,135,135,135,180,180};

int FLFemerAngle[] = {180,225,225,135,135,135,135,180,180};
int FLTibeaAngle[] =  {180,225,225,135,135,135,135,180,180};

int BLFemerAngle[] = {135,135,180,180,180,225,225,225,225};
int BLTibeaAngle[] = {135,135,180,180,180,225,225,225,225};

int BRFemerAngle[] = {180, 135, 135, 225, 225, 225, 225, 180, 180};
int BRTibeaAngle[] = {180, 135, 135, 225, 225, 225, 225, 180, 180};

int FRFemerAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 135};
int FRTibeaAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 135};




interval = 420;

  static int i = 0, j = 0.25*sizeof(BRTibeaAngle)/sizeof(BRTibeaAngle[0]), k = 0.5*sizeof(BRTibeaAngle)/sizeof(BRTibeaAngle[0]), l = 0.75*sizeof(BRTibeaAngle)/sizeof(BRTibeaAngle[0]);
  if (i >= sizeof(BRTibeaAngle)/sizeof(BRTibeaAngle[0])) {
    i = 0;
  }
  if (j >= sizeof(BRTibeaAngle)/sizeof(BRTibeaAngle[0])) {
    j = 0;
  }
  if (k >= sizeof(BRTibeaAngle)/sizeof(BRTibeaAngle[0])) {
    k = 0;
  }
  if (l >= sizeof(BRTibeaAngle)/sizeof(BRTibeaAngle[0])) {
    l = 0;
  }
  

  /* Set the position of the servos */
  //Back Left
  HCPCA9685.Servo(0, mapValues(BLTibeaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BLFemerAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(3, mapValues(BRTibeaAngle[j]-4));  
  HCPCA9685.Servo(4, mapValues(BRFemerAngle[j]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FLTibeaAngle[k]));  
  HCPCA9685.Servo(7, mapValues(FLFemerAngle[k]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FRTibeaAngle[l]-10));
  HCPCA9685.Servo(10, mapValues(FRFemerAngle[l])+18);
      /* Set the position of the servos */
  //Back Left
  HCPCA9685.Servo(0, mapValues(BLTibeaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BLFemerAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(3, mapValues(BRTibeaAngle[i]-4));  
  HCPCA9685.Servo(4, mapValues(BRFemerAngle[i]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FLTibeaAngle[i]));  
  HCPCA9685.Servo(7, mapValues(FLFemerAngle[i]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FRTibeaAngle[i]-10));
  HCPCA9685.Servo(10, mapValues(FRFemerAngle[i])+18);

  /* Update the loop variables */
  i++;
  j++;
  k++;
  l++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}


void Left() {
// int LFemerAngle[] = {174,174,170,166,161,161,154,148,143,139,139,140,144,151,153,153,153,154,155,155,156,157,158,159,159,160,161,161,162,162,163,163,163,163,163,163,163,164,164,164,164,165,165,165,166,166,167,167,168,168,169,169,170,171,171,172,173,174,174,175};
// int LTibeaAngle[] = {157,143,121,104,97,96,97,102,110,122,122,132,149,169,167,167,167,168,168,169,170,170,171,172,173,174,174,174,174,174,173,172,170,169,167,166,164,162,162,161,160,158,158,157,156,156,156,155,155,155,156,156,156,157,157,158,158,159,160,161};

//asss
// int RFemerAngle[] = {174,174,170,166,161,161,156,151,147,143,143,142,145,149,152,152,153,153,154,155,155,156,157,158,158,159,160,160,161,161,162,162,163,163,164,164,164,165,165,165,166,166,166,167,167,167,168,168,169,169,170,170,171,171,172,173,173,174,174,175};
// int RTibeaAngle[] = {157,143,121,104,97,96,97,99,104,110,110,118,130,141,142,142,143,144,145,146,147,148,149,150,151,152,153,154,155,155,156,156,156,156,156,156,155,155,156,155,154,154,154,154,153,153,154,154,154,154,155,155,155,156,157,158,158,159,160,161};
// int LFemerAngle[] = {172,168,159,159,148,140,140,144,152,152,153,154,155,157,158,159,161,162,162,163,163,163,164,164,164,165,165,166,167,167,168,169,170,171,172,174};
// int LTibeaAngle[] = {158,121,97,96,102,119,119,144,159,159,160,162,163,165,167,169,170,171,171,170,168,166,163,162,160,159,158,157,157,157,157,157,158,159,160,162};

// int RFemerAngle[] = {172,168,159,159,151,144,144,146,152,152,153,155,156,157,158,159,160,161,162,163,164,164,164,165,166,166,166,167,168,168,169,170,171,172,173,174};
// int RTibeaAngle[] = {158,121,97,96,100,108,108,127,139,139,141,143,145,147,149,151,153,154,155,156,156,156,155,156,155,155,154,154,155,155,156,157,158,159,160,162};

int RFemerAngle[] = {180,225,225,225,135,135,135,180,180};
int RTibeaAngle[] =  {180,225,225,225,135,135,135,180,180};

int BRFemerAngle[] = {180,225,225,225,135,135,135,180,180};
int BRTibeaAngle[] =  {180,225,225,225,135,135,135,180,180};

// int FLFemerAngle[] = {180,225,225,135,135,135,135,180,180};
// int FLTibeaAngle[] =  {180,225,225,135,135,135,135,180,180};


// int BLFemerAngle[] = {135,135,180,180,180,225,225,225,225};
// int BLTibeaAngle[] = {135,135,180,180,180,225,225,225,225};
int FRFemerAngle[] = {135,135,180,180,180,225,225,225,135};
int FRTibeaAngle[] = {135,135,180,180,180,225,225,225,135};


int FLFemerAngle[] = {180, 135, 135, 135, 225, 225, 225, 180, 180};
int FLTibeaAngle[] = {180, 135, 135, 135, 225, 225, 225, 180, 180};

// int BRFemerAngle[] = {180, 135, 135, 225, 225, 225, 225, 180, 180};
// int BRTibeaAngle[] = {180, 135, 135, 225, 225, 225, 225, 180, 180};

// int FRFemerAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 135};
// int FRTibeaAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 135};

int BLFemerAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 225};
int BLTibeaAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 225};


interval = 420;

  static int i = 0, j = 0.25*sizeof(RTibeaAngle)/sizeof(RTibeaAngle[0]), k = 0.5*sizeof(RTibeaAngle)/sizeof(RTibeaAngle[0]), l = 0.75*sizeof(RTibeaAngle)/sizeof(RTibeaAngle[0]);
  if (i >= sizeof(RTibeaAngle)/sizeof(RTibeaAngle[0])) {
    i = 0;
  }
  if (j >= sizeof(RTibeaAngle)/sizeof(RTibeaAngle[0])) {
    j = 0;
  }
  if (k >= sizeof(RTibeaAngle)/sizeof(RTibeaAngle[0])) {
    k = 0;
  }
  if (l >= sizeof(RTibeaAngle)/sizeof(RTibeaAngle[0])) {
    l = 0;
  }
  
  /* Set the position of the servos */
  //Back Left
  HCPCA9685.Servo(0, mapValues(BLTibeaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BLFemerAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(3, mapValues(BRTibeaAngle[j]-4));  
  HCPCA9685.Servo(4, mapValues(BRFemerAngle[j]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FLTibeaAngle[k]));  
  HCPCA9685.Servo(7, mapValues(FLFemerAngle[k]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FRTibeaAngle[l]-10));
  HCPCA9685.Servo(10, mapValues(FRFemerAngle[l])+18);

      /* Set the position of the servos */
  //Back Left
  HCPCA9685.Servo(0, mapValues(BLTibeaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BLFemerAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(3, mapValues(BRTibeaAngle[i]-4));  
  HCPCA9685.Servo(4, mapValues(BRFemerAngle[i]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FLTibeaAngle[i]));  
  HCPCA9685.Servo(7, mapValues(FLFemerAngle[i]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FRTibeaAngle[i]-10));
  HCPCA9685.Servo(10, mapValues(FRFemerAngle[i])+18);
  /* Update the loop variables */
  i++;
  j++;
  k++;
  l++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}


void Walk() {
  interval = 420;
int TibeaAngle[] = {180,225,225,225,135,135,135,180,180};

int BRFemerAngle[] = {180,225,225,225,135,135,135,180,180};
int BRTibeaAngle[] =  {180,225,225,225,135,135,135,180,180};

int FLFemerAngle[] = {180,225,225,135,135,135,135,180,180};
int FLTibeaAngle[] =  {180,225,225,135,135,135,135,180,180};


int BLFemerAngle[] = {135,135,180,180,180,225,225,225,225};
int BLTibeaAngle[] = {135,135,180,180,180,225,225,225,225};
int FRFemerAngle[] = {135,135,180,180,180,225,225,225,135};
int FRTibeaAngle[] = {135,135,180,180,180,225,225,225,135};

  static int i = 0;
  if (i >= sizeof(TibeaAngle)/sizeof(TibeaAngle[0])) {
    i = 0;
  }
  
  /* Set the position of the servos */
 HCPCA9685.Servo(0, mapValues(BLTibeaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BLFemerAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(3, mapValues(BRTibeaAngle[i]-4));  
  HCPCA9685.Servo(4, mapValues(BRFemerAngle[i]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FLTibeaAngle[i]));  
  HCPCA9685.Servo(7, mapValues(FLFemerAngle[i]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FRTibeaAngle[i]-10));
  HCPCA9685.Servo(10, mapValues(FRFemerAngle[i])+18);

  /* Update the loop variables */
  i++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}

void ReverseWalk() {
int TibeaAngle[] = {180,225,225,225,135,135,135,180,180};
interval = 420;
int FLFemerAngle[] = {180, 135, 135, 135, 225, 225, 225, 180, 180};
int FLTibeaAngle[] = {180, 135, 135, 135, 225, 225, 225, 180, 180};

int BRFemerAngle[] = {180, 135, 135, 225, 225, 225, 225, 180, 180};
int BRTibeaAngle[] = {180, 135, 135, 225, 225, 225, 225, 180, 180};

int FRFemerAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 135};
int FRTibeaAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 135};

int BLFemerAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 225};
int BLTibeaAngle[] = {225, 225, 180, 180, 180, 135, 135, 135, 225};

  static int i = 0;
  if (i >= sizeof(TibeaAngle)/sizeof(TibeaAngle[0])) {
    i = 0;
  }
  
  /* Set the position of the servos */
 HCPCA9685.Servo(0, mapValues(BLTibeaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BLFemerAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(3, mapValues(BRTibeaAngle[i]-4));  
  HCPCA9685.Servo(4, mapValues(BRFemerAngle[i]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FLTibeaAngle[i]));  
  HCPCA9685.Servo(7, mapValues(FLFemerAngle[i]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FRTibeaAngle[i]-10));
  HCPCA9685.Servo(10, mapValues(FRFemerAngle[i])+18);

  /* Update the loop variables */
  i++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}



void setIntervalSpeed(int speed) {
  interval = speed;
}
// unsigned long McurrentMillis = millis();
//     unsigned long stateDuration = 0;

//     switch (currentState) {
//         case STORE:
//             Store();
//             stateDuration = 1000;  // run for 2 seconds
//             break;
//         case LEFT:
//             Left();
//             stateDuration = 1800;  // run for 1.8 seconds
//             break;
//         case RIGHT:
//             Right();
//             stateDuration = 1800;  // run for 1.8 seconds
//             break;
//         case WALK:
//             Walk();
//             stateDuration = 2800;  // run for 2.8 seconds
//             break;
//         case REVERSE:
//             ReverseWalk();
//             stateDuration = 2800;  // run for 2.8 seconds
//             break;
//         default:
//             break;
//     }

//     // check if current state's time is up
//     if (McurrentMillis - stateStartMillis >= stateDuration) {
//         // move to next state
//         currentState = static_cast<State>((currentState + 1) % NUM_STATES);
//         // reset timer
//         stateStartMillis = McurrentMillis;
//     }



// HEYO WALKING GAIT
void Walking() {
int FemerAngle[] = {200,160,100,300};

int TibeaAngle[] = {180,225,220,300};

// int FemerAngle[] = {180,180,180,180};
int FFemerAngle[] = {180,180,270,270};
int FTibeaAngle[] = {160,160,280,280};


int BFemerAngle[] = {270,270,270,270};
int BTibeaAngle[] = {270,270,270,270};
interval = 450;

  static int i = 0, j = 0.25*sizeof(TibeaAngle)/sizeof(TibeaAngle[0]), k = 0.5*sizeof(TibeaAngle)/sizeof(TibeaAngle[0]), l = 0.75*sizeof(TibeaAngle)/sizeof(TibeaAngle[0]);
  if (i >= sizeof(TibeaAngle)/sizeof(TibeaAngle[0])) {
    i = 0;
  }
  if (j >= sizeof(TibeaAngle)/sizeof(TibeaAngle[0])) {
    j = 0;
  }
  if (k >= sizeof(TibeaAngle)/sizeof(TibeaAngle[0])) {
    k = 0;
  }
  if (l >= sizeof(TibeaAngle)/sizeof(TibeaAngle[0])) {
    l = 0;
  }
  
  /* Set the position of the servos */
  //Back Left
  HCPCA9685.Servo(0, mapValues(BTibeaAngle[i]));  
  HCPCA9685.Servo(1, mapValues(BFemerAngle[i])-14);  
  //Back Right
  HCPCA9685.Servo(3, mapValues(BTibeaAngle[i]-4));  
  HCPCA9685.Servo(4, mapValues(BFemerAngle[i]-3));  
  //Front Left
  HCPCA9685.Servo(6, mapValues(FTibeaAngle[i]));  
  HCPCA9685.Servo(7, mapValues(FFemerAngle[i]));  
  //Front Right
  HCPCA9685.Servo(9, mapValues(FTibeaAngle[i]-10));
  HCPCA9685.Servo(10, mapValues(FFemerAngle[i])+18);


  /* Update the loop variables */
  i++;
  j++;
  k++;
  l++;

  /* Wait for the motor to receive the command */
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis < interval) {
    delay(interval - (currentMillis - previousMillis));
  }
  previousMillis = millis();
}



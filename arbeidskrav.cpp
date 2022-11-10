#include <SPI.h>

#include <IRremote.h>
#define IR_RECEIVE 5
#define IR_BUTTON_OFF 69

#include <time.h>
#include <Wire.h>
#include <RtcDS3231.h>

RtcDS3231<TwoWire> Rtc(Wire);

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include <Thread.h>
#include <ThreadController.h>

#define TFT_CS        10
#define TFT_RST        -1
#define TFT_DC         8

#define countof(a) (sizeof(a) / sizeof(a[0]))

#define ONE_SECOND 1000
#define BUZZER 7
#define MOTION_SENSOR 12
#define RED_LED 3

bool ALARM_ACTIVATED = false;

//declaring pointers of type Thread
Thread* displayThread = new Thread(); 
Thread* alarmThread   = new Thread();

ThreadController controll = ThreadController();

Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Remote controller
void checkForRemoteOff() {
  if (!IrReceiver.decode()) return; // No IR signal received

  int command = IrReceiver.decodedIRData.command;
  if (command == IR_BUTTON_OFF ) ALARM_ACTIVATED = false;

  IrReceiver.resume();
}

// Motion sensor
void checkForMotion() {
  ALARM_ACTIVATED = digitalRead(MOTION_SENSOR) == HIGH;
  if (!ALARM_ACTIVATED) return; //No need to continue if alarm is not activated

  digitalWrite(RED_LED, HIGH);
  digitalWrite(BUZZER, HIGH);

  while (ALARM_ACTIVATED) {
      checkForRemoteOff();
  }

  digitalWrite(RED_LED, LOW);
  digitalWrite(BUZZER, LOW);
}

// Time
void displayClock() {
    RtcDateTime now = Rtc.GetDateTime();
    char datestring[20];

    //Screen
    tft.fillScreen(ST77XX_BLACK);
    snprintf_P(datestring,
              countof(datestring),
              PSTR("%02u:%02u"),
              now.Hour(),
              now.Minute());
    
    tft.setCursor(50, 50);
    tft.setTextColor(ST77XX_WHITE);
    tft.print(datestring);
}


void setup() {
  IrReceiver.begin(IR_RECEIVE);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(MOTION_SENSOR, INPUT);

  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
  Rtc.SetDateTime(compiled);
  Rtc.Enable32kHzPin(false);
  Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);

  tft.init(135, 240);
  tft.fillScreen(ST77XX_BLACK);
  tft.setRotation(45);
  tft.setTextSize(5);
  tft.setTextWrap(true);

  displayThread->onRun(displayClock);
  displayThread->setInterval(ONE_SECOND);

  alarmThread->onRun(checkForMotion);
  alarmThread->setInterval(ONE_SECOND);

  controll.add(displayThread);
  controll.add(alarmThread);
}

void loop() {
  controll.run();
}
// SPDX-FileCopyrightText: Copyright (c) 2022 esp32beans@gmail.com
//
// SPDX-License-Identifier: MIT

/* Convert Thrustmaster T.16000M Joystick to XAC joystick */

#include <usbhid.h>
#include <hiduniversal.h>
#include <usbhub.h>

// Satisfy the IDE, which needs to see the include statment in the ino too.
#ifdef dobogusinclude
#include <spi4teensy3.h>
#endif
#include <SPI.h>

#include <Joystick.h>

// X and Y axes with 8 buttons
Joystick_ Joystick(JOYSTICK_DEFAULT_REPORT_ID,
  JOYSTICK_TYPE_JOYSTICK, 8, 0,
  true, true, false, false, false, false,
  false, false, false, false, false);

#define T16K_MIN_ADC  (0)
#define T16K_MID_ADC  (8192)
#define T16K_MAX_ADC  (16383)

// 1=lots of debug output, 0=fastest performance
#define JS_DEBUG 0

// MAYBE some day
// Reverse X axis left and right
#define X_REVERSE (0)
// Reverse Y axis up and down
#define Y_REVERSE (0)

// Thrustmaster T.16000M HID report
struct GamePadEventData
{
  uint16_t  buttons;
  uint8_t   hat;
  uint16_t  x;
  uint16_t  y;
  uint8_t   twist;
  uint8_t   slider;
}__attribute__((packed));

class JoystickEvents
{
public:
  virtual void OnGamePadChanged(const GamePadEventData *evt);
};

#define RPT_GAMEPAD_LEN sizeof(GamePadEventData)

class JoystickReportParser : public HIDReportParser
{
  JoystickEvents  *joyEvents;

public:
  JoystickReportParser(JoystickEvents *evt);

  virtual void Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf);
};

JoystickReportParser::JoystickReportParser(JoystickEvents *evt) :
  joyEvents(evt)
{}

void JoystickReportParser::Parse(USBHID *hid, bool is_rpt_id, uint8_t len, uint8_t *buf)
{
  if (len != sizeof(GamePadEventData)) return;
  // Calling Game Pad event handler
  if (joyEvents) {
    joyEvents->OnGamePadChanged((const GamePadEventData*)buf);
  }
}

void JoystickEvents::OnGamePadChanged(const GamePadEventData *evt)
{

  Joystick.setXAxis(evt->x);
  Joystick.setYAxis(evt->y);

  uint16_t t16k_buttons = evt->buttons;
  for (size_t i = 0; i < 8; i++) {
    Joystick.setButton(i, t16k_buttons & 1);
    t16k_buttons >>= 1;
  }
  Joystick.sendState();

  if (JS_DEBUG) {
    Serial.print("X: ");
    Serial.print(evt->x, HEX);
    Serial.print(" Y: ");
    Serial.print(evt->y, HEX);
    Serial.print(" Hat Switch: ");
    Serial.print(evt->hat, HEX);
    Serial.print(" Twist: ");
    Serial.print(evt->twist, HEX);
    Serial.print(" Slider: ");
    Serial.print(evt->slider, HEX);
    Serial.print(" Buttons: ");
    Serial.println(evt->buttons, HEX);
  }
}

USB                   Usb;
USBHub                Hub(&Usb);
HIDUniversal          Hid(&Usb);
JoystickEvents        JoyEvents;
JoystickReportParser  Joy(&JoyEvents);

void setup()
{
  Serial.begin( 115200 );
  if (JS_DEBUG) {
#if !defined(__MIPSEL__)
    while (!Serial); // Wait for serial port to connect - used on Leonardo, Teensy and other boards with built-in USB CDC serial connection
#endif
  }
  Serial.println("Start");

  // Set Range Values
  Joystick.setXAxisRange(T16K_MIN_ADC, T16K_MAX_ADC);
  Joystick.setYAxisRange(T16K_MIN_ADC, T16K_MAX_ADC);
  Joystick.begin(false);

  if (Usb.Init() == -1)
      Serial.println("USB shield did not start.");

  delay( 200 );

  if (!Hid.SetReportParser(0, &Joy))
      ErrorMessage<uint8_t>(PSTR("SetReportParser"), 1  );
}

void loop()
{
  Usb.Task();
}

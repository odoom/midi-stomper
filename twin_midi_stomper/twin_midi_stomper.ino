
#include <Bounce2.h>
#include <EEPROM.h>
#include <MIDIUSB.h>
#include "pitchToNote.h"

const uint8_t CHANNEL = 10 - 1;
const uint8_t BUTTON_0_PITCH = pitchC2;
const uint8_t BUTTON_1_PITCH = pitchD2b;
const uint8_t MIDI_NOTE_ON_VELOCITY = 0x7F;
const uint8_t MIDI_NOTE_OFF_VELOCITY = 0x00;
const uint8_t MIDI_NOTE_ON = 0x90;
const uint8_t MIDI_NOTE_OFF = 0x80;
const uint8_t USB_CABLE_NUMBER = 0x00;

typedef struct
{
  uint8_t channel;
  uint8_t pitch;
} ButtonSetting;

Bounce buttons[2];
ButtonSetting buttonSettings[2];
unsigned long holdStart = 0;

void setup() {
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);

  buttons[0] = Bounce();
  buttons[0].attach(2);
  buttons[0].interval(1);

  buttons[1] = Bounce();
  buttons[1].attach(3);
  buttons[1].interval(1);

  delay(1000);
  buttons[0].update();
  buttons[1].update();

  if (!buttons[0].read() && !buttons[1].read()) {
    resetToDefault();
  } else {
    EEPROM.get(0, buttonSettings[0]);
    EEPROM.get(sizeof(buttonSettings[0]), buttonSettings[1]);
    if (!(buttonSettings[0].channel < 16 &&
          buttonSettings[1].channel < 16 &&
          buttonSettings[0].pitch < 128 &&
          buttonSettings[1].pitch < 128)) {
      resetToDefault();
    }
  }
}

void resetToDefault() {
  buttonSettings[0].channel = CHANNEL;
  buttonSettings[1].channel = CHANNEL;
  buttonSettings[0].pitch = BUTTON_0_PITCH;
  buttonSettings[1].pitch = BUTTON_1_PITCH;
  saveButtonSettings();
}

void saveButtonSettings() {
  EEPROM.put(0, buttonSettings[0]);
  EEPROM.put(sizeof(buttonSettings[0]), buttonSettings[1]);
}

void loop() {
  buttons[0].update();
  buttons[1].update();

  if (holdStart != 0 && (millis() - holdStart) > 2000) {
    // buttons have been held for 2 seconds
    holdStart = 0;
    reprogramPitches();
  } else if ((buttons[0].fell() && !buttons[1].read()) || (buttons[1].fell() && !buttons[0].read())) {
    // start waiting period
    holdStart = millis();
  } else if (!buttons[0].read() && !buttons[1].read()) {
    // buttons are depressed, just wait
    ;
  } else {
    // set counter to 0, one or none buttons pressed
    holdStart = 0;
  }

  if (buttons[0].fell()) {
    noteOn(buttonSettings[0]);
  } else if (buttons[0].rose()) {
    noteOff(buttonSettings[0]);
  }

  if (buttons[1].fell()) {
    noteOn(buttonSettings[1]);
  } else if (buttons[1].rose()) {
    noteOff(buttonSettings[1]);
  }

  clearBuffer();
}

void reprogramPitches() {
  noteOff(buttonSettings[0]);
  noteOff(buttonSettings[1]);
  clearBuffer();
  midiEventPacket_t packets[2];
  packets[0].header = 0;
  packets[1].header = 0;
  waitForPackets(packets, 2);
  if (packets[0].header != 0 && packets[1].header != 0) {
    buttonSettings[0].channel = packets[0].byte1 & 0x0F;
    buttonSettings[0].pitch = packets[0].byte2;
    buttonSettings[1].channel = packets[1].byte1 & 0x0F;
    buttonSettings[1].pitch = packets[1].byte2;
    saveButtonSettings();
  }
  playTestNotes();
}

void playTestNotes() {
  delay(500);
  noteOn(buttonSettings[0]);
  delay(250);
  noteOn(buttonSettings[1]);
  delay(250);
}

void waitForPackets(midiEventPacket_t packets[], int arrayLength) {
  unsigned long start = millis();
  delay(5);
  int index = 0;
  do {
    delay(5);
    midiEventPacket_t rx = MidiUSB.read();
    if (rx.header != 0 && (rx.byte1 & 0xF0) == MIDI_NOTE_ON && rx.byte3 != 0x00) {
      packets[index] = rx;
      index++;
    }
  } while (millis() - start < 30000L && index < arrayLength);
}

void clearBuffer() {
  while (MidiUSB.read().header != 0) {
    ;
  }
}

void noteOn(ButtonSetting buttonSetting) {
  midiEventPacket_t noteOnPacket;
  noteOnPacket.header = USB_CABLE_NUMBER | (MIDI_NOTE_ON >> 4);
  noteOnPacket.byte1 = MIDI_NOTE_ON | buttonSetting.channel;
  noteOnPacket.byte2 = buttonSetting.pitch;
  noteOnPacket.byte3 = MIDI_NOTE_ON_VELOCITY;
  MidiUSB.sendMIDI(noteOnPacket);
  MidiUSB.flush();
}

void noteOff(ButtonSetting buttonSetting) {
  midiEventPacket_t noteOffPacket;
  noteOffPacket.header = USB_CABLE_NUMBER | (MIDI_NOTE_OFF >> 4);
  noteOffPacket.byte1 = MIDI_NOTE_OFF | buttonSetting.channel;
  noteOffPacket.byte2 = buttonSetting.pitch;
  noteOffPacket.byte3 = MIDI_NOTE_OFF_VELOCITY;
  MidiUSB.sendMIDI(noteOffPacket);
  MidiUSB.flush();
}


// Copyright (c) 2017 Jonathan S. Fisher
// Subject to terms and conditions provided in LICENSE.md

const String COPYRIGHT_NOTICE = "Copyright (c) 2017 Jonathan S. Fisher. Subject to terms and conditions provided in https://github.com/odoom/midi-stomper/edit/master/LICENSE.md";

#include <Bounce2.h>
#include <EEPROM.h>
#include <MIDIUSB.h>
#include "pitchToNote.h"

const byte CHANNEL = 0x09;
const byte MIDI_NOTE_ON_VELOCITY = 0x7F;
const byte MIDI_NOTE_OFF_VELOCITY = 0x00;
const byte MIDI_NOTE_ON = 0x90;
const byte MIDI_NOTE_OFF = 0x80;
const byte USB_CABLE_NUMBER = 0x00;
const byte BUTTON_0_PITCH = pitchC2;
const byte BUTTON_0_PIN = 2;
const byte NUMBER_OF_BUTTONS = 3;
const byte BUTTON_DEBOUNCE_INTERVAL = 2;

typedef struct
{
  byte channel;
  byte pitch;
} ButtonSetting;

Bounce buttons[NUMBER_OF_BUTTONS];
ButtonSetting buttonSettings[NUMBER_OF_BUTTONS];
unsigned long holdStart = 0;
bool firstLoop = true;

void setup() {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    pinMode(i + BUTTON_0_PIN, INPUT_PULLUP);
    buttons[i] = Bounce();
    buttons[i].attach(i + BUTTON_0_PIN);
    buttons[i].interval(BUTTON_DEBOUNCE_INTERVAL);
  }

  loadFromEEPROM();
  if (!validSettings()) {
    resetToDefault();
  }
}

void loadFromEEPROM() {
  int location = 0;
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    EEPROM.get(location, buttonSettings[i]);
    location += sizeof(buttonSettings[0]);
  }
}

bool validSettings() {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    if (buttonSettings[i].channel > 0x0F || // MIDI Max Channel is 16
        buttonSettings[i].pitch > 0x7F) { // MIDI Max Velocity is 127
      return false;
    }
  }
  return true;
}

void resetToDefault() {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    buttonSettings[i].channel = CHANNEL;
    buttonSettings[i].pitch = BUTTON_0_PITCH + i;
  }
}

void loop() {
  updateButtons();

  if (firstLoop && allButtonsLow()) {
    resetToDefault();
    saveButtonSettings();
    while (allButtonsLow()) {
      updateButtons();
      delay(100);
    }
  } else {
    if (allButtonsLow()) {
      if (holdStart == 0) {
        //start waiting
        holdStart = millis();
      } else if ((millis() - holdStart) > 2000) {
        // waited long enough
        holdStart = 0;
        reprogramButtons();
      } else {
        // wait longer
      }
    } else {
      // set counter to 0, not all buttons pressed
      holdStart = 0;
    }

    for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
      if (buttons[i].fell()) {
        noteOn(buttonSettings[i]);
      } else if (buttons[i].rose()) {
        noteOff(buttonSettings[i]);
      }
    }
  }

  firstLoop = false;
  clearBuffer();
}

void updateButtons() {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    buttons[i].update();
  }
}

bool allButtonsLow() {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    if (buttons[i].read()) {
      return false;
    }
  }
  return true;
}

void saveButtonSettings() {
  int location = 0;
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    EEPROM.put(location, buttonSettings[i]);
    location += sizeof(buttonSettings[0]);
  }
}

void reprogramButtons() {
  midiEventPacket_t packets[NUMBER_OF_BUTTONS];
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    noteOff(buttonSettings[i]);
    packets[i].header = 0;
  }
  clearBuffer();
  waitForPackets(packets);
  if (allPacketsHaveNonZeroHeader(packets)) {
    copyToSettings(packets);
    saveButtonSettings();
  }
  delay(500);
  playTestNotes();
}

void clearBuffer() {
  while (MidiUSB.available() > 0) {
    MidiUSB.read();
  }
}

void waitForPackets(midiEventPacket_t packets[]) {
  unsigned long start = millis();
  byte index = 0;
  do {
    delay(5); //not sure why this is needed but it helps
    midiEventPacket_t rx = MidiUSB.read();
    if (rx.header != 0 && // non zero header
        (rx.byte1 & 0xF0) == MIDI_NOTE_ON && // first nibble is a note on
        rx.byte3 != 0x00 && // velocity is not zero
        rx.byte2 < 128) { // pitch is valid
      packets[index] = rx;
      index++;
    }
  } while (millis() - start < 30000L && index < NUMBER_OF_BUTTONS);
}

bool allPacketsHaveNonZeroHeader(midiEventPacket_t packets[]) {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    if (packets[i].header == 0) {
      return false;
    }
  }
  return true;
}

void copyToSettings(midiEventPacket_t packets[]) {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    buttonSettings[i].channel = packets[i].byte1 & 0x0F;
    buttonSettings[i].pitch = packets[i].byte2;
  }
}

void playTestNotes() {
  for (byte i = 0; i < NUMBER_OF_BUTTONS; i++) {
    noteOn(buttonSettings[i]);
    delay(350);
    noteOff(buttonSettings[i]);
    delay(100);
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


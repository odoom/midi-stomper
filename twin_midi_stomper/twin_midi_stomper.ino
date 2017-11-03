
#include <Bounce2.h>
#include <EEPROM.h>
#include <MIDIUSB.h>
#include "pitchToNote.h"

const uint8_t CHANNEL = 10 - 1;
const uint8_t BUTTON_0_PITCH = pitchC2;
const uint8_t MIDI_NOTE_ON_VELOCITY = 0x7F;
const uint8_t MIDI_NOTE_OFF_VELOCITY = 0x00;
const uint8_t MIDI_NOTE_ON = 0x90;
const uint8_t MIDI_NOTE_OFF = 0x80;
const uint8_t USB_CABLE_NUMBER = 0x00;
const unsigned int NUMBER_OF_BUTTONS = 2;
const unsigned int FIRST_PIN = 2;

typedef struct
{
  uint8_t channel;
  uint8_t pitch;
} ButtonSetting;

Bounce buttons[NUMBER_OF_BUTTONS];
ButtonSetting buttonSettings[NUMBER_OF_BUTTONS];
unsigned long holdStart = 0;

void setup() {
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    pinMode(i + FIRST_PIN, INPUT_PULLUP);
    buttons[i] = Bounce();
    buttons[i].attach(i + FIRST_PIN);
    buttons[i].interval(1);
  }

  delay(1000);
  updateButtons();

  if (allButtonsLow()) {
    resetToDefault();
  } else {
    loadFromEEPROM();
    if (!validSettings()) {
      resetToDefault();
      saveButtonSettings();
    }
  }
}

void updateButtons() {
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    buttons[i].update();
  }
}

bool allButtonsLow() {
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    if (buttons[i].read()) {
      return false;
    }
  }
  return true;
}

void loadFromEEPROM() {
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    EEPROM.get(sizeof(buttonSettings[i]) * i, buttonSettings[i]);
  }
}

bool validSettings() {
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    if (buttonSettings[i].channel > 16 && buttonSettings[i].pitch > 127) {
      return false;
    }
  }
  return true;
}

void resetToDefault() {
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    buttonSettings[i].channel = CHANNEL;
    buttonSettings[i].pitch = BUTTON_0_PITCH + i;
  }
}

void saveButtonSettings() {
  EEPROM.put(0, buttonSettings[0]);
  EEPROM.put(sizeof(buttonSettings[0]), buttonSettings[1]);
}

void loop() {
  updateButtons();

  if (allButtonsLow()) {
    if (holdStart == 0) {
      //start waiting
      holdStart = millis();
    } else if ((millis() - holdStart) > 2000) {
      // waited long enough
      holdStart = 0;
      reprogramPitches();
    } else {
      // wait longer
    }
  } else {
    // set counter to 0, not all buttons pressed
    holdStart = 0;
  }

  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    if (buttons[i].fell()) {
      noteOn(buttonSettings[i]);
    } else if (buttons[i].rose()) {
      noteOff(buttonSettings[i]);
    }
  }

  clearBuffer();
}

void reprogramPitches() {
  midiEventPacket_t packets[NUMBER_OF_BUTTONS];
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    noteOff(buttonSettings[i]);
    packets[i].header = 0;
  }
  clearBuffer();
  waitForPackets(packets);
  if (allPacketsHaveNonZeroHeader(packets)) {
    copyToSettings(packets);
    saveButtonSettings();
  }
  playTestNotes();
}

void copyToSettings(midiEventPacket_t packets[]) {
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    buttonSettings[i].channel = packets[i].byte1 & 0x0F;
    buttonSettings[i].pitch = packets[i].byte2;
  }
}

void playTestNotes() {
  delay(500);
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    noteOn(buttonSettings[0]);  delay(250);
  }
  delay(250);
}

bool allPacketsHaveNonZeroHeader(midiEventPacket_t packets[]) {
  for (int i = 0; i < NUMBER_OF_BUTTONS; i++) {
    if (packets[i].header == 0) {
      return false;
    }
  }
  return true;
}

void waitForPackets(midiEventPacket_t packets[]) {
  unsigned long start = millis();
  delay(5);
  int index = 0;
  do {
    delay(5);
    midiEventPacket_t rx = MidiUSB.read();
    if (rx.header != 0 && (rx.byte1 & 0xF0) == MIDI_NOTE_ON && (rx.byte1 & 0x0F) < 16 && rx.byte3 != 0x00 && rx.byte3 < 128) {
      packets[index] = rx;
      index++;
    }
  } while (millis() - start < 30000L && index < NUMBER_OF_BUTTONS);
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


#include <Bounce2.h>
#include <MIDIUSB.h>
#include "pitchToNote.h"

// Configuration
const uint8_t CHANNEL = 10 - 1;
const uint8_t BUTTON0_PITCH = pitchC2;
const uint8_t BUTTON1_PITCH = pitchD2b;

// Less common configuration
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

// Buttons
Bounce button0 = Bounce();
Bounce button1 = Bounce();

ButtonSetting buttonSetting0;
ButtonSetting buttonSetting1;
unsigned long holdStart = 0;

void setup() {
  pinMode(2, INPUT_PULLUP);
  button0.attach(2);
  button1.interval(1);

  pinMode(3, INPUT_PULLUP);
  button1.attach(3);
  button1.interval(1);

  buttonSetting0.channel = CHANNEL;
  buttonSetting1.channel = CHANNEL;
  buttonSetting0.pitch = BUTTON0_PITCH;
  buttonSetting1.pitch = BUTTON1_PITCH;
}

void loop() {
  button0.update();
  button1.update();

  if (holdStart != 0 && (millis() - holdStart) > 2000) {
    // buttons have been held for 2 seconds
    holdStart = 0;
    reprogramPitches();
  } else if ((button0.fell() && !button1.read()) || (button1.fell() && !button0.read())) {
    // start waiting period
    holdStart = millis();
  } else if (!button0.read() && !button1.read()) {
    // buttons are depressed, just wait
    ;
  } else {
    holdStart = 0;
    if (button0.fell()) {
      noteOn(buttonSetting0);
    } else if (button0.rose()) {
      noteOff(buttonSetting0);
    }

    if (button1.fell()) {
      noteOn(buttonSetting1);
    } else if (button1.rose()) {
      noteOff(buttonSetting1);
    }
  }

  clearBuffer();
}

void reprogramPitches() {
  noteOff(buttonSetting0);
  noteOff(buttonSetting1);
  clearBuffer();
  midiEventPacket_t packet0 = waitForPacket();
  clearBuffer();
  midiEventPacket_t packet1 = waitForPacket();
  buttonSetting0.channel = packet0.byte1 & 0x0F;
  buttonSetting0.pitch = packet0.byte2;
  buttonSetting1.channel = packet1.byte1 & 0x0F;
  buttonSetting1.pitch = packet1.byte2;
}

midiEventPacket_t waitForPacket() {
  midiEventPacket_t rx;
  while (rx.header == 0 || (rx.byte1 & 0xF0) != MIDI_NOTE_ON || rx.byte3 == 0x00) {
    rx = MidiUSB.read();
  }
  return rx;
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


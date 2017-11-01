#include <Bounce2.h>
#include <MIDIUSB.h>
#include "pitchToNote.h"

// Configuration
const uint8_t CHANNEL = 10 - 1;
const uint8_t BUTTON1_PITCH = pitchD2b;
const uint8_t BUTTON2_PITCH = pitchD2;

// Less common configuration
const uint8_t MIDI_NOTE_ON_VELOCITY = 0x7F;
const uint8_t MIDI_NOTE_OFF_VELOCITY = 0x00;
const uint8_t MIDI_NOTE_ON = 0x90;
const uint8_t MIDI_NOTE_OFF = 0x80;
const uint8_t USB_CABLE_NUMBER = 0x00;

// Buttons
Bounce button1 = Bounce();
Bounce button2 = Bounce();

void setup() {
  pinMode(2, INPUT_PULLUP);
  button1.attach(2);
  button2.interval(1);

  pinMode(3, INPUT_PULLUP);
  button2.attach(3);
  button2.interval(1);
}

void loop() {
  button1.update();
  button2.update();

  if (button1.fell()) {
    noteOn(BUTTON1_PITCH);
  } else if (button1.rose()) {
    noteOff(BUTTON1_PITCH);
  }

  if (button2.fell()) {
    noteOn(BUTTON2_PITCH);
  } else if (button2.rose()) {
    noteOff(BUTTON2_PITCH);
  }

  MidiUSB.flush();
}

void noteOn(uint8_t pitch) {
  midiEventPacket_t noteOnPacket;
  noteOnPacket.header = USB_CABLE_NUMBER | (MIDI_NOTE_ON >> 4);
  noteOnPacket.byte1 = MIDI_NOTE_ON | CHANNEL;
  noteOnPacket.byte2 = pitch;
  noteOnPacket.byte3 = MIDI_NOTE_ON_VELOCITY;
  MidiUSB.sendMIDI(noteOnPacket);
}

void noteOff(uint8_t pitch) {
  midiEventPacket_t noteOffPacket;
  noteOffPacket.header = USB_CABLE_NUMBER | (MIDI_NOTE_OFF >> 4);
  noteOffPacket.byte1 = MIDI_NOTE_OFF | CHANNEL;
  noteOffPacket.byte2 = pitch;
  noteOffPacket.byte3 = MIDI_NOTE_OFF_VELOCITY;
  MidiUSB.sendMIDI(noteOffPacket);
}


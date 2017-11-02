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
}ButtonSetting;

// Buttons
Bounce button0 = Bounce();
Bounce button1 = Bounce();

ButtonSetting buttonSetting0;
ButtonSetting buttonSetting1;

void setup() {
  pinMode(2, INPUT_PULLUP);
  button0.attach(2);
  button1.interval(1);

  pinMode(3, INPUT_PULLUP);
  button1.attach(3);
  button1.interval(1);

  buttonSetting0.channel = CHANNEL;
  buttonSetting0.channel = BUTTON0_PITCH;
  buttonSetting1.channel = CHANNEL;
  buttonSetting1.channel = BUTTON1_PITCH;
}

void loop() {
  button0.update();
  button1.update();

  if (button0.fell() && button1.fell()) {
    reprogramPitches();
  } else {
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
  MidiUSB.flush();
}

void reprogramPitches() {
  midiEventPacket_t packet0 = waitForPacket();
  midiEventPacket_t packet1 = waitForPacket();
  if (packet0.header != 0 && packet1.header != 0) {
    buttonSetting0.channel = packet0.byte1 << 4;
    buttonSetting0.channel = packet0.byte2;
    buttonSetting1.channel = packet1.byte1 << 4;
    buttonSetting1.channel = packet1.byte2;
  }
}

midiEventPacket_t waitForPacket() {
  unsigned long start = millis();
  midiEventPacket_t rx;
  do {
    rx = MidiUSB.read();
  } while (rx.header != 0 && (millis() - start) < 10000);
  return rx;
}

void clearBuffer() {
  midiEventPacket_t rx;
  do {
    rx = MidiUSB.read();
  } while (rx.header != 0);
}

void noteOn(ButtonSetting buttonSetting) {
  midiEventPacket_t noteOnPacket;
  noteOnPacket.header = USB_CABLE_NUMBER | (MIDI_NOTE_ON >> 4);
  noteOnPacket.byte1 = MIDI_NOTE_ON | buttonSetting.channel;
  noteOnPacket.byte2 = buttonSetting.pitch;
  noteOnPacket.byte3 = MIDI_NOTE_ON_VELOCITY;
  MidiUSB.sendMIDI(noteOnPacket);
}

void noteOff(ButtonSetting buttonSetting) {
  midiEventPacket_t noteOffPacket;
  noteOffPacket.header = USB_CABLE_NUMBER | (MIDI_NOTE_OFF >> 4);
  noteOffPacket.byte1 = MIDI_NOTE_OFF | buttonSetting.channel;
  noteOffPacket.byte2 = buttonSetting.pitch;
  noteOffPacket.byte3 = MIDI_NOTE_OFF_VELOCITY;
  MidiUSB.sendMIDI(noteOffPacket);
}


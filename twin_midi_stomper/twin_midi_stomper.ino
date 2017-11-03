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

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ;
  }

  Serial.println("Setup start");
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

  printStatus();
  Serial.println("Setup Complete");
}

void printStatus() {
  Serial.println("buttonSetting0.channel: 0x" + String(buttonSetting0.channel, HEX));
  Serial.println("buttonSetting0.pitch: 0x" + String(buttonSetting0.pitch, HEX));
  Serial.println("buttonSetting1.channel: 0x" + String(buttonSetting1.channel, HEX));
  Serial.println("buttonSetting1.pitch: 0x" + String(buttonSetting1.pitch, HEX));
  Serial.println("button0.read():" + String(button0.read()));
  Serial.println("button1.read():" + String(button1.read()));
}


void loop() {
  button0.update();
  button1.update();

  if ((button0.fell() && !button1.read()) || (button1.fell() && !button0.read())) {
    Serial.println("reprogramPitches");
    noteOff(buttonSetting0);
    noteOff(buttonSetting1);
    reprogramPitches();
    Serial.println("reprogramPitches done");
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
}

void reprogramPitches() {
  clearBuffer();
  midiEventPacket_t packet0 = waitForPacket();
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
  Serial.println("waitForPacket()   byte1: 0x" + String(rx.byte1, HEX));
  Serial.println("waitForPacket() channel: 0x" + String(rx.byte1 & 0x0F, HEX));
  Serial.println("waitForPacket()   pitch: 0x" + String(rx.byte2, HEX));
  return rx;
}

void clearBuffer() {
  unsigned long loops = 0;
  while (MidiUSB.read().header != 0) {
    loops++;
  }
  Serial.println("clearBuffer() loops:" + String(loops));
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


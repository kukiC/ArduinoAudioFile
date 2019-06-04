#include "ArduinoAudioFile.h"
#include <SPI.h>
#include <SD.h>

#define CS 53

void setup() {
  ArduinoAudioFile<double> song = ArduinoAudioFile<double>();
  song.load("Blue December.wav");
}

void loop() {
  // put your main code here, to run repeatedly:
  song.printSummary();
}

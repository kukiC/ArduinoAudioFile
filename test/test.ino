#include "ArduinoAudioFile.h"

void setup() {
  ArduinoAudioFile<double> song = ArduinoAudioFile<double>();
  song.load("Blue December.wav");
}

void loop() {
  // put your main code here, to run repeatedly:
  song.printSummary();
}

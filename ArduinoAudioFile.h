#ifndef ARDUINOAUDIOFILE_H
#define RDUINOAUDIOFILE_H

#include "Arduino.h"
#include <SD.h>
#include <ArduinoComponents.h>
using namespace components;
#define blueToothSerial Serial2


template <class T>
class ArduinoAudioFile
{
public:
  /* Constructor */
  ArduinoAudioFile();
  /* Deconstructor */
  ~ArduinoAudioFile();

  bool load (char* filename);

  uint32_t getSampleRate() const;
  int getNumChannels() const;
  int getBitDepth() const;
  int getNumSamplesPerChannel() const;
  double getLengthInSeconds() const;
  void printSummary() const;

  Vector<T> *samples;

private:
  bool decodeWaveFile (File *file);
  int getIndexOfString (File *file, char* stringToSearchFor);
  int16_t twoBytesToInt (File *file, int startIndex);
  int32_t fourBytesToInt (File *file, int startIndex);
  void clearAudioBuffer();
  T singleByteToSample (uint8_t sample);
  T sixteenBitIntToSample (int16_t sample);

  /* Variables */
  uint32_t sampleRate;
  int bitDepth;


};



#endif /* AudioFile_h */

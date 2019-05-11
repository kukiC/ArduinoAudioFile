#include "ArduinoAudioFile.h"
#include <ArduinoComponents.h>
using namespace components;

template <class T>
ArduinoAudioFile<T>::ArduinoAudioFile(){
  bitDepth = 16;
  sampleRate = 44100;
}



template <class T>
bool ArduinoAudioFile<T>::load (char* filename)
{
    // read-only mode
    File file = SD.open(filename, FILE_READ);

    // check the file exists
    if (!file)
    {
        blueToothSerial.print("\r\nERROR: File doesn't exist or otherwise can't load file ");
        blueToothSerial.print(file);
        blueToothSerial.print("\r\n");
        return false;
    }

    return decodeWaveFile(&file);
}

//=============================================================
template <class T>
uint32_t ArduinoAudioFile<T>::getSampleRate() const
{
    return sampleRate;
}

//=============================================================
template <class T>
int ArduinoAudioFile<T>::getNumChannels() const
{
    return (int)samples->size();
}


//=============================================================
template <class T>
int ArduinoAudioFile<T>::getBitDepth() const
{
    return bitDepth;
}

//=============================================================
template <class T>
int ArduinoAudioFile<T>::getNumSamplesPerChannel() const
{
  return (int) samples->size();

}

//=============================================================
template <class T>
double ArduinoAudioFile<T>::getLengthInSeconds() const
{
    return (double)getNumSamplesPerChannel() / (double)sampleRate;
}

//=============================================================
template <class T>
void ArduinoAudioFile<T>::printSummary() const
{
    blueToothSerial.print("\r\n|======================================|\r\n");
    blueToothSerial.print("\r\nNum Channels: ");
    blueToothSerial.print(getNumChannels());
    blueToothSerial.print("\r\n");

    blueToothSerial.print("\r\nNum Samples Per Channel: ");
    blueToothSerial.print(getNumSamplesPerChannel());
    blueToothSerial.print("\r\n");

    blueToothSerial.print("\r\nSample Rate: ");
    blueToothSerial.print(sampleRate());
    blueToothSerial.print("\r\n");

    blueToothSerial.print("\r\nBit Depth: ");
    blueToothSerial.print(bitDepth());
    blueToothSerial.print("\r\n");

    blueToothSerial.print("\r\nLength in Seconds: ");
    blueToothSerial.print(getLengthInSeconds());
    blueToothSerial.print("\r\n");
    blueToothSerial.print("\r\n|======================================|\r\n");
}


/* =============== Private functions ===============*/

template <class T>
bool ArduinoAudioFile<T>::decodeWaveFile (File *file)
{

  // -----------------------------------------------------------
  // try and find the start points of key chunks
  int indexOfDataChunk = getIndexOfString (file, "data");
  int indexOfFormatChunk = getIndexOfString (file, "fmt");

  // if we can't find the data or format chunks, or the IDs/formats don't seem to be as expected
  // then it is unlikely we'll able to read this file, so abort
  if (indexOfDataChunk == -1 || indexOfFormatChunk == -1)
  {
    blueToothSerial.print("\r\nERROR: this doesn't seem to be a valid .WAV file\r\n");
    return false;
  }

    // -----------------------------------------------------------
    // FORMAT CHUNK
    int f = indexOfFormatChunk;
    // std::string formatChunkID (fileData.begin() + f, fileData.begin() + f + 4);
    //int32_t formatChunkSize = fourBytesToInt (fileData, f + 4);
    int16_t audioFormat = twoBytesToInt (file, f + 8);
    int16_t numChannels = twoBytesToInt (file, f + 10);
    sampleRate = (uint32_t) fourBytesToInt (file, f + 12);
    int32_t numBytesPerSecond = fourBytesToInt (file, f + 16);
    int16_t numBytesPerBlock = twoBytesToInt (file, f + 20);
    bitDepth = (int) twoBytesToInt (file, f + 22);

    int numBytesPerSample = bitDepth / 8;

    // check that the audio format is PCM
    if (audioFormat != 1)
    {
        blueToothSerial.print("ERROR: this is a compressed .WAV file and this library does not support decoding them at present");
        return false;
    }

    // check the number of channels is mono or stereo
    if (numChannels < 1 ||numChannels > 2)
    {
        blueToothSerial.print("ERROR: this WAV file seems to be neither mono nor stereo (perhaps multi-track, or corrupted?)");
        return false;
    }

    // check header data is consistent
    if ((numBytesPerSecond != (numChannels * sampleRate * bitDepth) / 8) || (numBytesPerBlock != (numChannels * numBytesPerSample)))
    {
        blueToothSerial.print("ERROR: the header data in this WAV file seems to be inconsistent");
        return false;
    }

    // check bit depth is either 8, 16 or 24 bit
    if (bitDepth != 8 && bitDepth != 16 && bitDepth != 24)
    {
        blueToothSerial.print("ERROR: this file has a bit depth that is not 8, 16 or 24 bits");
        return false;
    }

    // -----------------------------------------------------------
    // DATA CHUNK
    int d = indexOfDataChunk;
    // std::string dataChunkID (fileData.begin() + d, fileData.begin() + d + 4);
    int32_t dataChunkSize = fourBytesToInt (file, d + 4);

    int numSamples = dataChunkSize / (numChannels * bitDepth / 8);
    int samplesStartIndex = indexOfDataChunk + 8;

    clearAudioBuffer();
    samples = new Vector<T>[numChannels];
    
    for (int i = 0; i < numSamples; i++)
    {
        for (int channel = 0; channel < numChannels; channel++)
        {
            int sampleIndex = samplesStartIndex + (numBytesPerBlock * i) + channel * numBytesPerSample;

            if (bitDepth == 8)
            {
                file->seek(sampleIndex);
                char* temp = new char[1];
                file->read(temp, 1);
                T sample = singleByteToSample ((uint8_t)*temp);
                samples[channel].push(sample);
                delete(temp);
            }
            else if (bitDepth == 16)
            {
                int16_t sampleAsInt = twoBytesToInt (file, sampleIndex);
                T sample = sixteenBitIntToSample (sampleAsInt);
                samples[channel].push (sample);
            }
            else if (bitDepth == 24)
            {
                int32_t sampleAsInt = 0;
                file->seek(sampleIndex);
                char* temp = new char[3];
                file->read(temp, 3);
                sampleAsInt = (temp[2] << 16) | (temp[1] << 8) | temp[0];
                delete(temp);

                if (sampleAsInt & 0x800000) //  if the 24th bit is set, this is a negative number in 24-bit world
                    sampleAsInt = sampleAsInt | ~0xFFFFFF; // so make sure sign is extended to the 32 bit float

                T sample = (T)sampleAsInt / (T)8388608.;
                samples[channel].push_back (sample);
            }
            else
            {

                // assert (false);
            }
        }
    }

    return true;
}

//=============================================================
template <class T>
int ArduinoAudioFile<T>::getIndexOfString (File *file, char* stringToSearchFor)
{
  int index = -1;
  int stringLength = (int)sizeof(stringToSearchFor);

  for (int i = 0; i < file->size() - stringLength;i++)
  {
    file->seek(i);
    char * section = new char[stringLength] ;
    file->read(section, stringLength);
    if (section == stringToSearchFor)
      {
          index = i;
          break;
      }
    delete(section);
  }

  return index;
}

//=============================================================
template <class T>
int16_t ArduinoAudioFile<T>::twoBytesToInt (File *file, int startIndex)
{
  int16_t result;
  file->seek(startIndex);
  unsigned char * temp = new unsigned char[2];
  file->read((char*)temp, sizeof(temp));

  result = (temp[1] << 8) | temp[0];
  delete(temp);
  return result;
}

//=============================================================
template <class T>
int32_t ArduinoAudioFile<T>::fourBytesToInt (File *file, int startIndex)
{
    int32_t result;
    file->seek(startIndex);
    unsigned char * temp = new unsigned char[4];
    file->read((char*)temp, sizeof(temp));
    result = (temp[3] << 24) | (temp[2] << 16) | (temp[1] << 8) | temp[0];
    delete(temp);
    return result;
}

//=============================================================
template <class T>
void ArduinoAudioFile<T>::clearAudioBuffer()
{
    for (int i = 0; i < samples.size();i++)
    {
        samples[i].clear();
    }

    samples.clear();
}

//=============================================================
template <class T>
T ArduinoAudioFile<T>::singleByteToSample (uint8_t sample)
{
    return static_cast<T> (sample - 128) / static_cast<T> (128.);
}

//=============================================================
template <class T>
T ArduinoAudioFile<T>::sixteenBitIntToSample (int16_t sample)
{
    return static_cast<T> (sample) / static_cast<T> (32768.);
}

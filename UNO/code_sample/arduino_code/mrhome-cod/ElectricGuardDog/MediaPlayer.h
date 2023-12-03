/***
 * Excerpted from "Programming Your Home",
 * published by The Pragmatic Bookshelf.
 * Copyrights apply to this code. It may not be used to create training material, 
 * courses, books, articles, and the like. Contact us if you are in doubt.
 * We make no guarantees that this code is fit for any purpose. 
 * Visit http://www.pragmaticprogrammer.com/titles/mrhome for more book information.
***/
#ifndef MEDIAPLAYER_H
#define MEDIAPLAYER_H

#include <AF_Wave.h>
#include "wave.h"

class MediaPlayer
{
public:
  MediaPlayer();
  ~MediaPlayer();
  bool play(char *fileName);
  void resume();
  void stop();
  void pause();
  bool isPlaying();
  bool onPause();
  int exploreSDcard(const bool print=0);
  void fileName(const int fileNumber, char* _fileName);
  
private:
  File closeFile(const File file);
  void openMemoryCard();
  void setupWaveShieldPins();

  AF_Wave card;
  File file; // struct fat16_file_struct*
  Wavefile waveFile;
  uint32_t pausePosition;    
};

#endif
 

void playComplete(char* fileName)
{ putstring("\nPlay complete: ");  
  Serial.print(fileName);
  mediaPlayer.play(fileName);
  while (mediaPlayer.isPlaying())
  { putstring(".");
    delay(100);
  }
}

void playComplete(const int fileNumber)
{ char fileName[13];
  mediaPlayer.fileName(fileNumber, fileName);
  playComplete(fileName);
}

 

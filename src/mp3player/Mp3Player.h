#ifndef MP3_PLAYER_H
#define MP3_PLAYER_H

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

class Mp3Player {
public:
    // Konstruktor
    Mp3Player();

    // Initialisiert den Player mit einem seriellen Port
    bool begin(HardwareSerial& serialPort);

    // Grundlegende Steuerungsfunktionen
    void play(int trackNumber);
    // void pause();
    // void resume(); // Setzt die Wiedergabe fort
    // void stop();
    // void next();
    // void previous();

    // Lautstärkeregelung (0-30)
    void setVolume(int volume);

private:
    // Ein internes Objekt der eigentlichen DFPlayer-Bibliothek
    DFRobotDFPlayerMini myDFPlayer;

    // Ein Zeiger, um sich den seriellen Port zu merken
    HardwareSerial* mp3Serial;
};

#endif // MP3_PLAYER_H
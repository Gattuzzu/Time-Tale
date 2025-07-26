#ifndef MP3_PLAYER_H
#define MP3_PLAYER_H

#include <Arduino.h>
#include <DFRobotDFPlayerMini.h>

class Mp3Player {
public:
    // Statische Methode als einziger Zugriffspunkt
    static Mp3Player& getInstance() {
        static Mp3Player instance; // Wird nur einmal erstellt
        return instance;
    }

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
    // Konstruktor
    Mp3Player();

    // Verhindert das Kopieren der Instanz
    Mp3Player(const Mp3Player&) = delete;
    Mp3Player& operator=(const Mp3Player&) = delete;

    // Ein internes Objekt der eigentlichen DFPlayer-Bibliothek
    DFRobotDFPlayerMini myDFPlayer;

    // Ein Zeiger, um sich den seriellen Port zu merken
    HardwareSerial* mp3Serial;
};

#endif // MP3_PLAYER_H
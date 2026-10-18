#include "Mp3Player.h"
#include "logger/Logger.h"

// Definitionen für die seriellen Pins (anpassen falls nötig)
#define MP3_RX_PIN D3
#define MP3_TX_PIN D2

Mp3Player::Mp3Player() {
    // Der Konstruktor ist leer, die Initialisierung passiert in begin()
}

bool Mp3Player::begin(HardwareSerial& serialPort) {
    mp3Serial = &serialPort;
    // Starte die serielle Kommunikation zum DFPlayer
    mp3Serial->begin(9600, SERIAL_8N1, MP3_RX_PIN, MP3_TX_PIN);

    Logger::log(LogLevel::Info, "Initialisiere DFPlayer Mini ...");

    // Initialisiere die Bibliothek
    if (!myDFPlayer.begin(*mp3Serial)) {
        Logger::log(LogLevel::Error, "Verbindung zum DFPlayer Mini fehlgeschlagen!\n1. Bitte Verkabelung prüfen.\n2. Bitte SD-Karte einlegen und formatieren.");
        return false;
    }
    Logger::log(LogLevel::Info, "DFPlayer Mini bereit.");

    return true;
}

void Mp3Player::play(int trackNumber) {
    // Spielt einen bestimmten Track aus dem /mp3 Ordner
    myDFPlayer.play(trackNumber);
}

// void Mp3Player::pause() {
//     myDFPlayer.pause();
// }

// void Mp3Player::resume() {
//     // Die Bibliothek nennt die "resume"-Funktion "start"
//     myDFPlayer.start();
// }

// void Mp3Player::stop() {
//     myDFPlayer.stop();
// }

// void Mp3Player::next() {
//     myDFPlayer.next();
// }

// void Mp3Player::previous() {
//     myDFPlayer.previous();
// }

void Mp3Player::setVolume(int volume) {
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 30) {
        volume = 30;
    }
    myDFPlayer.volume(volume);
}
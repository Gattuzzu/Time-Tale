#include "PollenClient.h"
#include "PollenData.h"

bool PollenClient::getCurrentPollen(float latitude, float longitude, PollenData& outPollenData) {
    outPollenData.reset(); // Vor dem Abruf zurücksetzen

    // HINWEIS: Der Host für die Pollen API ist "pollen.googleapis.com".
    // Stellen Sie sicher, dass Sie getInstance() mit dem korrekten Host aufrufen.
    // Beispiel: PollenClient::getInstance("pollen.googleapis.com", "YOUR_API_KEY");

    String path = "/v1/forecast:lookup?location.longitude=";
    path += String(longitude, 6);
    path += "&location.latitude=";
    path += String(latitude, 6);
    path += "&plantsDescription=false"; // Um unnötige Daten in der Antwort zu vermeiden

    JsonDocument doc;
    if (!sendGetRequest(path, doc)) { // Aufruf der Basisklassenmethode
      return false;
    }

    // Spezifisches Parsing der Pollendaten
    return parsePollenJson(doc, outPollenData);
}

bool PollenClient::parsePollenJson(JsonDocument& doc, PollenData& outPollenData) {
    // Überprüfe, ob "dailyInfo" vorhanden ist und mindestens ein Element hat
    JsonArray dailyInfoArray = doc["dailyInfo"].as<JsonArray>();

    if (dailyInfoArray.isNull() || dailyInfoArray.size() == 0) {
      Logger::log(LogLevel::Error, "PollenClient: 'dailyInfo' Array fehlt oder ist leer im JSON.");
      return false;
    }

    // Wir interessieren uns nur für den ersten Eintrag im dailyInfo (heutige Daten)
    JsonObject todayInfo = dailyInfoArray[0];
    JsonArray pollenTypeInfoArray = todayInfo["pollenTypeInfo"].as<JsonArray>();

    if (pollenTypeInfoArray.isNull() || pollenTypeInfoArray.size() == 0) {
      Logger::log(LogLevel::Error, "PollenClient: 'pollenTypeInfo' Array fehlt oder ist leer im JSON für den heutigen Tag.");
      return false;
    }

    // Iteriere durch das pollenTypeInfoArray und extrahiere die gewünschten Werte
    for (JsonObject pollenType : pollenTypeInfoArray) {
      String code = pollenType["code"].as<String>();
      if (!pollenType["indexInfo"].isNull() && !pollenType["indexInfo"]["value"].isNull()){
        int value = pollenType["indexInfo"]["value"] | -1; // Standardwert -1 falls nicht gefunden

        if (code == "GRASS") {
          outPollenData.grassPollenLevel = value;
        } else if (code == "TREE") {
          outPollenData.treePollenLevel = value;
        } else if (code == "WEED") {
          outPollenData.weedPollenLevel = value;
        }
      }

      // Wenn du Debugging-Ausgaben möchtest:
      // Logger::log(LogLevel::Debug, "PollenType: " + code + ", Value: " + String(value));
    }

    // Überprüfen, ob alle gewünschten Werte gefunden wurden (optional)
    if (outPollenData.grassPollenLevel == -1 ||
        outPollenData.treePollenLevel  == -1 ||
        outPollenData.weedPollenLevel  == -1) {
        Logger::log(LogLevel::Error, "PollenClient: Nicht alle Pollentypen (Grass, Tree, Weed) wurden im JSON gefunden oder hatten gültige Werte.");
        // Du könntest hier auch false zurückgeben, je nachdem, wie kritisch das Fehlen dieser Daten ist.
        // Für dieses Beispiel lassen wir es bei true, wenn zumindest ein Teil geparst wurde.
    }

    return true;
}
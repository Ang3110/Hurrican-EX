// Datei : Gameplay.hpp

// --------------------------------------------------------------------------------------
//
// Beinhaltet den Haupt Game-Loop
//
// (c) 2002 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

#ifndef _GAMEPLAY_HPP_
#define _GAMEPLAY_HPP_

#include "DX8Sprite.hpp"
#include "Tileengine.hpp"

#include <string>

// --------------------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------------------

enum {
  DETAIL_LOW = 0,      // alles total billig
  DETAIL_MEDIUM = 1,   // animiertes Wasser dazu
  DETAIL_HIGH = 2,     // geiler Blitz, Leuchteffekte und Lightmaps
  DETAIL_MAXIMUM = 3   // Schockwelle
};

// --------------------------------------------------------------------------------------
// Prototypen von Funktionen
// --------------------------------------------------------------------------------------

void InitNewGame();                        // Neues Spiel initialisieren
void OfferTutorialLesson(int player, int lesson, float distanceSquared);
void InitNewGameLevel();                   // Alles für den Levelstart in Stage NR vorbereiten
void GameLoop();                           // Haupt-Spielablauf
void ShakeScreen(float strength, float worldX, float worldY, bool rumble = false);
void StageClear(bool PlaySong = true);         // Spieler aus dem Level abgehen lassen
enum class CoopGateSide { None, Left, Right, Top, Bottom };
bool CoopNeedsPartner();
bool CoopPlayerReady(const PlayerClass &player);
void WaitForCoopPartner(int player, CoopGateSide side = CoopGateSide::None, float limit = 0.0f);
void HoldCoopLiftForBoss();
bool CoopLiftWaitingForBoss();
void RecordPowerBlock(PlayerClass *finder);
void SummaryScreen();                      // Zusammenfassung des Levels
bool LoadConfig();                         // Konfigurationen laden
void SaveConfig();                         // Konfigurationen speichern
void CreateDefaultControlsConfig(int player);  // Load per-player default controls configuration
void CreateDefaultConfig();                // DefaultKonfigurationen erstellen
bool DisplayLoadInfo(const char Text[100]);    // Text beim Loading anzeigen
bool NewDemo(const char Filename[]);           // Neues Demo aufzeichnen
void RecordDemo();                         // Spielereingaben sichern
bool LoadDemo(const char Filename[]);          // Demo laden
void PlayDemo();                           // Demo laufen lassen
void EndDemo();                            // Demo anhalten
void LeaveGameLoop();                      // Game Loop verlassen
void ScrolltoPlayeAfterBoss();             // Der Name ist Programm
void ShowGameOver();
const char *convertText(const char *text);
PlayerClass *ChooseAim();

// --------------------------------------------------------------------------------------
// Externals
// --------------------------------------------------------------------------------------

extern GameStateEnum SpielZustand;
extern bool NochKeinFullScreen;

//----- externe Variablen zum Screen-Wackeln lassen (zB bei SpinnenBombe)

extern int MAX_LEVELS;
extern int NUMPLAYERS;
extern float FahrstuhlPos;   // Nur im Fahrstuhl Level
extern float ScreenWinkel;  // in welchem zWinkel steht der Screen grad (für Säulen, die das Level zum Kippen bringen)
extern float WarningCount;  // Counter, ob ein "Warning" angezeigt wird
extern bool RunningTutorial;
extern int options_Detail;
extern bool StopStageMusicAtStart;
extern bool JoystickFound;
extern bool ShowSummary;
extern bool FlugsackFliesFree;
extern int DisplayHintNr;

extern const float ShockWaveX;
extern const float ShockWaveY;

extern std::string StageReihenfolge[256];

#endif

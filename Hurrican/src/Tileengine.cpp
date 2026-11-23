// Datei : Tileengine.cpp

// --------------------------------------------------------------------------------------
//
// 2D Tile-Engine für Hurrican
// bestehend aus einem Vordergrund-Layer in verschiedenen Helligkeitsstufen
// und einem Overlay-Layer
//
// (c) 2002 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------------

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <string>
namespace fs = std::filesystem;

#include "Console.hpp"
#include "DX8Graphics.hpp"
#include "DX8Input.hpp"
#include "DX8Sound.hpp"
#include "DX8Sprite.hpp"
#include "DX8Texture.hpp"
#include "Gameplay.hpp"
#include "Gegner_Helper.hpp"
#include "Globals.hpp"
#include "HUD.hpp"
#include "Logdatei.hpp"
#include "Main.hpp"
#include "Mathematics.hpp"
#include "Partikelsystem.hpp"
#include "Player.hpp"
#include "Projectiles.hpp"
#include "Tileengine.hpp"
#include "Timer.hpp"

// --------------------------------------------------------------------------------------
// externe Variablen
// --------------------------------------------------------------------------------------

extern Logdatei Protokoll;
extern DirectInputClass DirectInput;
extern TimerClass Timer;
extern GegnerListClass Gegner;
extern HUDClass HUD;
extern PartikelsystemClass PartikelSystem;
extern ProjectileListClass Projectiles;

extern DirectGraphicsSprite *pGegnerGrafix[MAX_GEGNERGFX];  // Grafiken  der Gegner


D3DCOLOR Col1, Col2, Col3;  // Farben für Wasser/Lava etc
bool DrawDragon;            // Für den Drachen im Turm Level
float ShadowAlpha;

namespace {
constexpr int waterfallWidth = 60;
constexpr int waterfallHeight = 240;
}

// --------------------------------------------------------------------------------------
// Klassendefunktionen
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// TileEngine Klasse
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

TileEngineClass::TileEngineClass() {
    bDrawShadow = false;

    XOffset = 0.0f;
    YOffset = 0.0f;

    NewXOffset = -1.0f;
    NewYOffset = -1.0f;

    RenderPosX = 0;
    RenderPosY = 0;
    RenderPosXTo = 0;
    RenderPosYTo = 0;

    LEVELSIZE_X = 128;
    LEVELSIZE_Y = 96;

    // Level scrollt von alleine
    ScrollSpeedX = 0.0f;
    ScrollSpeedY = 0.0f;

    // Speed für "Scrollto" Funktion
    SpeedX = 0.0f;
    SpeedY = 0.0f;
    CloudMovement = 0.0f;
    TileAnimCount = 0.0f;
    TileAnimPhase = 0;
    LoadedTilesets = 0;

    // DKS - Added this:
    memset(Tiles, 0, sizeof(Tiles));

    for (int i = 0; i < MAX_LEVELSIZE_X; i++)
        for (int j = 0; j < MAX_LEVELSIZE_Y; j++) {
            // DKS - Disabled this in favor of memsetting entire array above the loops:
            // memset (&Tiles[i][j], 0, sizeof (Tiles[i][j]));

            // DKS - Was already commented out in original source:
            //		 for (int k = 0; k < 3; k++)
            //			 TileAt(i, j).Color[k] = 0xFFFFFFFF;

            LevelTileStruct& tile = Tiles[i][j];
            tile.Red = 255;
            tile.Green = 255;
            tile.Blue = 255;
            tile.Alpha = 255;

            tile.move_v1 = tile.move_v2 = tile.move_v3 = tile.move_v4 = false;
        }

    for (auto &gfx : TileGfx)
        // DKS - Adapted to new TexturesystemClass
        gfx.itsTexIdx = -1;

    // DKS - Moved these to the new TileEngineClass::LoadSprites() function (see note there)
    //// Wasserfall Textur laden
    // Wasserfall[0].LoadImage("wasserfall.png",  60,  240, 60,  240, 1, 1);
    // Wasserfall[1].LoadImage("wasserfall2.png", 640, 480, 640, 480, 1, 1);
    // LiquidGfx[0].LoadImage("water.png", 128, 128, 128, 128, 1, 1);
    // LiquidGfx[1].LoadImage("water2.png", 128, 128, 128, 128, 1, 1);

    // Texturkoordinaten für das Wasser vorberechnen
    for (int i = 0; i < 9; i++) {
        float const w = 128.0f / 8.0f * static_cast<float>(i) / 128.0f;
        WasserU[i] = w;
        WasserV[i] = w;
    }

    // DKS - Moved these to the new TileEngineClass::LoadSprites() function (see note there)
    //// GameOver Schriftzug laden
    // GameOver.LoadImage("gameover.png", 400, 90, 400, 90, 1, 1);
    //// Shatten für das Alien Level laden
    // Shadow.LoadImage ("shadow.png", 512, 512, 512, 512, 1, 1);

    WasserfallOffset = 0.0f;

    Zustand = TileStateEnum::SCROLLBAR;

    // Tile Ausschnitte vorberechnen
    //
    for (int i = 0; i < MAX_TILERECTS; i++) {
        TileRects[i].top = (i / 12) * TILESIZE_X;
        TileRects[i].left = (i % 12) * TILESIZE_Y;
        TileRects[i].right = TileRects[i].left + TILESIZE_X;
        TileRects[i].bottom = TileRects[i].top + TILESIZE_Y;
    }

    /* DKS - Replaced both SinList2 and WaterList lookup tables with new class
       WaterSinTableClass. See its comments in Tileengine.h for more info.    */
#if 0
    int i = 0;
    float w = 0.0f;
    while (i < 4000)
    {
        /* SinList  [i] = 0.0f; */  //DKS - Disabled this and all eliminated all uses of it,
                                    // as it was only ever filled with zeroes and had no effect.
        //DKS - This is to ensure libm should explicitly be called here and not the lookup table:
        //SinList2 [i] = float (sin(w)) * 5.0f;
        //WaterList[i] = float (sin(w)) * 2.5f;
        SinList2 [i] = sinf(w) * 5.0f;
        WaterList[i] = sinf(w) * 2.5f;
        w += PI / TILESIZE_X;
        i++;
    }

    //SinPos   = 0.0f;      //DKS - unused; disabled
    SinPos2  = 0.0f;
    WaterPos = 0.0f;
#endif  // 0

    // DKS - Lightmap code in original game was never used and all related code has now been disabled:
    //// LightMaps laden
    // lightmaps[LIGHTMAP_BLITZ].Load("lightmap_blitz.bmp");
    // lightmaps[LIGHTMAP_EXPLOSION].Load("lightmap_explosion.bmp");
    // lightmaps[LIGHTMAP_GOLEMSHOT].Load("lightmap_golem.bmp");
    // lightmaps[LIGHTMAP_LILA].Load("lightmap_lila.bmp");

    pDragonHack = nullptr;
}

// --------------------------------------------------------------------------------------
// Destruktor
// --------------------------------------------------------------------------------------

TileEngineClass::~TileEngineClass() {
    if (pDragonHack != nullptr)
        delete pDragonHack;
}

// DKS - Added initialization function that will load the sprites.
//      This was necessary since making TileEngineClass a global
//      static in Main.cpp instead of a dyanmically-allocated pointer.
//      The class constructor therefore should never load sprites by
//      itself, since graphics system should be initialized first.
//      All textures here will be automatically freed by Textures::Exit(),
//      so no need to worry about the sprite destructors being called in
//      any specific order (i.e. after graphics system has been shutdown.
void TileEngineClass::LoadSprites() {
    // Wasserfall Textur laden
    Wasserfall[0].LoadImage("wasserfall.png", waterfallWidth, waterfallHeight, waterfallWidth, waterfallHeight,
                           1, 1, TextureLayout::Repeat);
    Wasserfall[1].LoadImage("wasserfall2.png", RENDERWIDTH, RENDERHEIGHT, RENDERWIDTH, RENDERHEIGHT, 1, 1,
                           TextureLayout::MirrorRepeat);

    LiquidGfx[0].LoadImage("water.png", 128, 128, 128, 128, 1, 1);
    LiquidGfx[1].LoadImage("water2.png", 128, 128, 128, 128, 1, 1);

    // GameOver Schriftzug laden
    GameOver.LoadImage("gameover.png", 400, 90, 400, 90, 1, 1);

    // Shatten für das Alien Level laden
    Shadow.LoadImage("shadow.png", 512, 512, 512, 512, 1, 1);
}

// --------------------------------------------------------------------------------------
// Neues, leeres Level der Grösse xSize/ySize erstellen
// --------------------------------------------------------------------------------------

void TileEngineClass::InitNewLevel(int xSize, int ySize) {
    wideBossArena = false;
    walkoutExits.clear();
    levelSplitScreenType = DirectGraphics.GetDisplaySettings().splitScreenType;
    ResetCoopViews();
    LEVELSIZE_X = xSize;
    LEVELSIZE_Y = ySize;

    ScrollSpeedX = 0.0f;
    ScrollSpeedY = 0.0f;

    memset(&Tiles, 0, sizeof(Tiles));

    /* DKS - Replaced both SinList2 and WaterList lookup tables with new class
       WaterSinTableClass. See its comments in Tileengine.h for more info.    */
    WaterSinTable.ResetPosition();
#if 0
    //SinPos   = 0.0f;
    SinPos2  = 0.0f;
    WaterPos = 0.0f;
#endif  // 0

    DrawDragon = true;

    XOffset = 0.0f;
    YOffset = 0.0f;

    NewXOffset = -1.0f;
    NewYOffset = -1.0f;
}

// --------------------------------------------------------------------------------------
// Level freigeben
// --------------------------------------------------------------------------------------

void TileEngineClass::ClearLevel() {
    // Zuerst alle Gegner-Texturen freigeben , damit
    // nur die geladen werden, die auch benötigt werden
    // Objekte, die immer benötigt werden, wie Extraleben, Diamanten etc.
    // werden nicht released
    for (int i = 4; i < MAX_GEGNERGFX; i++)
        if (i != PUNISHER && pGegnerGrafix[i] != nullptr)  // Ist eine Textur geladen ?
        {
            delete (pGegnerGrafix[i]);  // dann diese löschen
            pGegnerGrafix[i] = nullptr;    // und auf NULL setzen
        }

    for (auto &gfx : TileGfx) {
        // DKS - Adapted to new TexturesystemClass
#if 0
        if (gfx.itsTexture)
        {
            glDeleteTextures( 1, &gfx.itsTexture );
            gfx.itsTexture = 0;
        }
#endif  // 0
        Textures.UnloadTexture(gfx.itsTexIdx);
        gfx.itsTexIdx = -1;
    }

    for (auto *layer : {&Background, &ParallaxLayer[0], &ParallaxLayer[1], &ParallaxLayer[2], &CloudLayer}) {
        Textures.UnloadTexture(layer->itsTexIdx);
        layer->itsTexIdx = -1;
    }

    Projectiles.ClearAll();
    PartikelSystem.ClearAll();

    XOffset = 0.0f;
    YOffset = 0.0f;

    // DKS - Added:
    // SoundManager.StopAllSongs(false);
    SoundManager.StopSongs();
    SoundManager.StopSounds();

    // DKS - Added unloading of Punisher.it music (it is now loaded on-demand in Gegner_Helper.cpp)
    SoundManager.UnloadSong(MUSIC::PUNISHER);
    // DKS - Added unloading of flugsack.it music (it is now loaded on-demand in Gegner_Helper.cpp)
    SoundManager.UnloadSong(MUSIC::FLUGSACK);
    // DKS - Might as well unload stage & boss music too, although LoadSong() will free any old songs for us
    SoundManager.UnloadSong(MUSIC::STAGEMUSIC);
    SoundManager.UnloadSong(MUSIC::BOSS);
}

// --------------------------------------------------------------------------------------
// Level laden
// --------------------------------------------------------------------------------------

bool TileEngineClass::LoadLevel(const std::string &Filename) {
    std::string Temp;
    FileHeader DateiHeader;  // Header der Level-Datei
    LevelObjectStruct LoadObject;

    MustCenterPlayer = false;

    DisplayHintNr = GetRandom(30);

    IsElevatorLevel = false;
    FlugsackFliesFree = true;
    g_Fahrstuhl_yPos = -1.0f;

    // Zuerst checken, ob sich das Level in einem MOD-Ordner befindet
    if (CommandLineParams.RunOwnLevelList) {
        Temp = g_storage_ext + "/data/levels/" + CommandLineParams.OwnLevelList + "/" + Filename;
        if (fs::exists(Temp) && fs::is_regular_file(Temp))
            goto loadfile;
    }

    // Dann checken, ob sich das File im Standard Ordner befindet
    Temp = g_storage_ext + "/data/levels/" + Filename;
    if (fs::exists(Temp) && fs::is_regular_file(Temp))
        goto loadfile;

    Protokoll << "\n-> Error loading level " << Filename << "!" << std::endl;
    GameRunning = false;
    return false;

loadfile:

    Protokoll << "\n-> Loading Level <-\n" << std::endl;

    StopStageMusicAtStart = false;

    ClearLevel();

    // Drache im Hintergrund ggf. löschen
    if (pDragonHack != nullptr) {
        delete pDragonHack;
        pDragonHack = nullptr;
    }

    // File öffnen
    std::ifstream Datei(Temp, std::ifstream::binary);

    if (!Datei) {
        Protokoll << " \n-> Error loading level !" << std::endl;
        return false;
    }

    // DateiHeader auslesen
    Datei.read(reinterpret_cast<char *>(&DateiHeader), sizeof(DateiHeader));

    // und Werte übertragen
    LEVELSIZE_X = FixEndian(DateiHeader.SizeX);
    LEVELSIZE_Y = FixEndian(DateiHeader.SizeY);
    LEVELPIXELSIZE_X = LEVELSIZE_X * TILESIZE_X;
    LEVELPIXELSIZE_Y = LEVELSIZE_Y * TILESIZE_Y;
    LoadedTilesets = DateiHeader.UsedTilesets;
    strcpy_s(Beschreibung, DateiHeader.Beschreibung);
    bScrollBackground = DateiHeader.ScrollBackground;

    // Benutzte Tilesets laden
    for (int i = 0; i < LoadedTilesets; i++)
        TileGfx[i].LoadImage(DateiHeader.SetNames[i], 256, 256, TILESIZE_X, TILESIZE_Y, 12, 12);

    // Benutzte Hintergrundgrafiken laden

    Background.LoadImage(DateiHeader.BackgroundFile, 640, 480, 640, 480, 1, 1, TextureLayout::Parallax);
    ParallaxLayer[0].LoadImage(DateiHeader.ParallaxAFile, 640, 480, 640, 480, 1, 1, TextureLayout::Parallax);
    ParallaxLayer[1].LoadImage(DateiHeader.ParallaxBFile, 640, 480, 640, 480, 1, 1, TextureLayout::Parallax);
    CloudLayer.LoadImage(DateiHeader.CloudFile, 640, 240, 640, 240, 1, 1, TextureLayout::Parallax);
    Timelimit = static_cast<float>(FixEndian(DateiHeader.Timelimit));
    DateiHeader.NumObjects = FixEndian(DateiHeader.NumObjects);

    if (Timelimit <= 0.0f)
        Timelimit = 500.0f;

    Player[0].PunisherActive = false;
    Player[1].PunisherActive = false;

    bool hasSecondStart = false;
    std::optional<LevelObjectStruct> firstStart;
    Player[1].xpos = Player[1].ypos = 0.0f;

    TimelimitSave = Timelimit;
    MaxSecrets = 0;
    MaxDiamonds = 0;
    MaxOneUps = 0;
    MaxBlocks = 0;

    // LevelDaten laden
    InitNewLevel(LEVELSIZE_X, LEVELSIZE_Y);

    LevelTileLoadStruct LoadTile;

    for (int i = 0; i < LEVELSIZE_X; i++)
        for (int j = 0; j < LEVELSIZE_Y; j++) {
            Datei.read(reinterpret_cast<char *>(&LoadTile), sizeof(LoadTile));

            if (LoadTile.TileSetBack > LoadedTilesets)
                LoadTile.TileSetBack = LoadedTilesets;
            if (LoadTile.TileSetFront > LoadedTilesets)
                LoadTile.TileSetFront = LoadedTilesets;

            LevelTileStruct& tile = TileAt(i, j);

            // Geladenes Leveltile übernehmen
            //
            tile.Alpha = LoadTile.Alpha;
            tile.BackArt = LoadTile.BackArt;
            tile.Block = FixEndian(LoadTile.Block);
            tile.Blue = LoadTile.Blue;
            tile.FrontArt = LoadTile.FrontArt;
            tile.Green = LoadTile.Green;
            tile.Red = LoadTile.Red;
            tile.TileSetBack = LoadTile.TileSetBack;
            tile.TileSetFront = LoadTile.TileSetFront;

            tile.Color[0] = D3DCOLOR_RGBA(LoadTile.Red, LoadTile.Green, LoadTile.Blue, LoadTile.Alpha);
            tile.Color[1] = D3DCOLOR_RGBA(LoadTile.Red, LoadTile.Green, LoadTile.Blue, LoadTile.Alpha);
            tile.Color[2] = D3DCOLOR_RGBA(LoadTile.Red, LoadTile.Green, LoadTile.Blue, LoadTile.Alpha);
            tile.Color[3] = D3DCOLOR_RGBA(LoadTile.Red, LoadTile.Green, LoadTile.Blue, LoadTile.Alpha);

            // Eine Flüssigkeit als Block?
            // damit man nicht immer auf alle vier möglichen Flüssigkeiten checken muss,
            // sondern nur auf BLOCKWERT_LIQUID

            if (tile.Block & BLOCKWERT_WASSER || tile.Block & BLOCKWERT_SUMPF)
                tile.Block ^= BLOCKWERT_LIQUID;
        }

    // eventuelle Schrägen ermitteln und Ecken für die Wasseranim festlegen
    // EDIT_ME wieder reinmachen und diesmal richtig machen =)

    for (int i = 1; i < LEVELSIZE_X - 1; i++)
        for (int j = 2; j < LEVELSIZE_Y - 1; j++) {
            // Schräge links hoch
            if (TileAt(i + 0, j + 0).Block & BLOCKWERT_WAND && !(TileAt(i + 1, j + 0).Block & BLOCKWERT_WAND) &&
                TileAt(i + 1, j + 1).Block & BLOCKWERT_WAND && !(TileAt(i + 0, j - 1).Block & BLOCKWERT_WAND)) {
                if (!(TileAt(i + 1, j + 0).Block & BLOCKWERT_SCHRAEGE_L))
                    TileAt(i + 1, j + 0).Block ^= BLOCKWERT_SCHRAEGE_L;
            }

            // Schräge rechts hoch
            if (TileAt(i + 0, j + 0).Block & BLOCKWERT_WAND && !(TileAt(i - 1, j + 0).Block & BLOCKWERT_WAND) &&
                TileAt(i - 1, j + 1).Block & BLOCKWERT_WAND && !(TileAt(i + 0, j - 1).Block & BLOCKWERT_WAND)) {
                if (!(TileAt(i - 1, j + 0).Block & BLOCKWERT_SCHRAEGE_R))
                    TileAt(i - 1, j + 0).Block ^= BLOCKWERT_SCHRAEGE_R;
            }

            // Wasseranim
            //
            uint32_t bl = TileAt(i - 1, j + 0).Block;
            uint32_t br = TileAt(i + 1, j + 0).Block;
            uint32_t bo = TileAt(i + 0, j - 1).Block;
            uint32_t bu = TileAt(i + 0, j + 1).Block;

            LevelTileStruct& tile = TileAt(i, j);

            if (!(TileAt(i - 1, j - 1).Block & BLOCKWERT_WAND) && !(TileAt(i, j - 1).Block & BLOCKWERT_WASSERFALL) &&
                !(TileAt(i - 1, j - 1).Block & BLOCKWERT_WASSERFALL) &&
                (bl & BLOCKWERT_LIQUID && (!(bo & BLOCKWERT_WAND))))
                tile.move_v1 = true;
            else
                tile.move_v1 = false;

            if (!(TileAt(i - 1, j + 1).Block & BLOCKWERT_WAND) && (bl & BLOCKWERT_LIQUID && bu & BLOCKWERT_LIQUID))
                tile.move_v3 = true;
            else
                tile.move_v3 = false;

            if (!(TileAt(i + 1, j - 1).Block & BLOCKWERT_WAND) && !(TileAt(i, j - 1).Block & BLOCKWERT_WASSERFALL) &&
                !(TileAt(i + 1, j - 1).Block & BLOCKWERT_WASSERFALL) &&
                (br & BLOCKWERT_LIQUID && (!(bo & BLOCKWERT_WAND))))
                tile.move_v2 = true;
            else
                tile.move_v2 = false;

            if (!(TileAt(i + 1, j + 1).Block & BLOCKWERT_WAND) && (br & BLOCKWERT_LIQUID && bu & BLOCKWERT_LIQUID))
                tile.move_v4 = true;
            else
                tile.move_v4 = false;
        }

    // Objekt Daten laden und gleich Liste mit Objekten erstellen
    if (DateiHeader.NumObjects > 0) {
        for (int i = 0; i < static_cast<int>(DateiHeader.NumObjects); i++) {
            Datei.read(reinterpret_cast<char *>(&LoadObject), sizeof(LoadObject));  // Objekt laden

            LoadObject.ObjectID = FixEndian(LoadObject.ObjectID);
            LoadObject.XPos = FixEndian(LoadObject.XPos);
            LoadObject.YPos = FixEndian(LoadObject.YPos);
            LoadObject.Value1 = FixEndian(LoadObject.Value1);
            LoadObject.Value2 = FixEndian(LoadObject.Value2);

            // Startposition des Spielers
            if (LoadObject.ObjectID == 0) {
                if (LoadObject.Value2 != 0 && LoadObject.Value2 != 1)
                    continue;
                if (LoadObject.Value2 == 1)
                    hasSecondStart = true;
                else if (!firstStart)
                    firstStart = LoadObject;
                // Anfängliche Blickrichtung auch aus Leveldatei lesen
                if (LoadObject.Value1 == 0)
                    Player[LoadObject.Value2].Blickrichtung = DirectionEnum::RECHTS;
                else
                    Player[LoadObject.Value2].Blickrichtung = DirectionEnum::LINKS;

                Player[LoadObject.Value2].xpos = static_cast<float>(LoadObject.XPos);
                Player[LoadObject.Value2].ypos = static_cast<float>(LoadObject.YPos);
                Player[LoadObject.Value2].xposold = Player[LoadObject.Value2].xpos;
                Player[LoadObject.Value2].yposold = Player[LoadObject.Value2].ypos;
                Player[LoadObject.Value2].JumpxSave = Player[LoadObject.Value2].xpos;
                Player[LoadObject.Value2].JumpySave = Player[LoadObject.Value2].ypos;
                Player[LoadObject.Value2].CenterLevel();
                UpdateLevel();
            } else {
                // Gegner und andere Objekte laden und ins Level setzen
                switch (LoadObject.ObjectID) {
                    // Count Secrets, OneUps etc., for Summary-Box
                    case SECRET:
                        MaxSecrets++;
                        break;
                    case DIAMANT:
                        MaxDiamonds++;
                        break;
                    case ONEUP:
                        MaxOneUps++;
                        break;
                    case POWERBLOCK:
                        MaxBlocks++;
                        break;

                    case SPITTERBOMBE:
                        // Spitter laden, wenn die Spitterbombe geladen wird
                        LoadGegnerGrafik(SPITTER);
                        break;

                    case RIESENPIRANHA:
                        // Kleinen Piranha laden, wenn der Riesen Piranha geladen wird
                        LoadGegnerGrafik(PIRANHA);
                        break;

                    case NEST:
                        // Mücke laden, wenn das Nest geladen wird
                        LoadGegnerGrafik(STAHLMUECKE);
                        break;

                    case KUGELRIESIG:
                    case KUGELGROSS:
                    case KUGELMEDIUM:
                        // Kleine Kugeln laden, wenn eine grosse geladen wird
                        LoadGegnerGrafik(KUGELGROSS);
                        LoadGegnerGrafik(KUGELMEDIUM);
                        LoadGegnerGrafik(KUGELKLEIN);
                        break;

                    case FAHRSTUHLBOSS:
                        // Boulder und Stelzsack laden, wenn der Fahrstuhlendboss geladen wird
                        LoadGegnerGrafik(BOULDER);
                        LoadGegnerGrafik(STELZSACK);
                        break;

                    case LAVABALLSPAWNER:
                        // lava Ball laden, wenn dessen Spawner geladen wird
                        LoadGegnerGrafik(LAVABALL);
                        break;

                    case BRATKLOPS:
                    case PARTIKELSPAWN:
                        // Made laden, wenn der Bratklops oder der Partikelspawner geladen wird
                        LoadGegnerGrafik(MADE);
                        break;

                    case SHRINE:
                        // Steine laden, wenn der Schrein geladen wird
                        LoadGegnerGrafik(FALLINGROCK);
                        break;

                    case SHOOTPLATTFORM:
                        // ShootButton laden, wenn die entsprechende Plattform geladen wird
                        LoadGegnerGrafik(SHOOTBUTTON);
                        break;

                    case SCHWABBEL:
                        // Made laden, wenn der Schwabbelsack geladen wird
                        LoadGegnerGrafik(MADE);
                        break;

                    case METALHEAD:
                        // Boulder laden, wenn der MetalHead Boss geladen wird
                        LoadGegnerGrafik(BOULDER);
                        break;

                    case SCHLEIMMAUL:
                        // Schleimbollen laden, wenn das Schleimmaul geladen wird
                        LoadGegnerGrafik(SCHLEIMALIEN);
                        break;

                    case WUXESPINNEN:
                        // Mittelgroße Spinne laden, wenn der Spinnen Ansturm geladen wird
                        LoadGegnerGrafik(MITTELSPINNE);
                        break;

                    case GOLEM:
                        // Blauen Boulder laden, wenn der Golem geladen wird
                        LoadGegnerGrafik(BOULDER);
                        break;

                    case SPINNENMASCHINE:
                        // Climbspider laden, wenn die Spinnenmaschine geladen wird
                        LoadGegnerGrafik(CLIMBSPIDER);
                        // Drone laden, wenn die Spinnenmaschine geladen wird
                        LoadGegnerGrafik(DRONE);
                        break;

                    case PRESSWURST:
                        // Fette Spinne laden, wenn die Spinnen Presswurst geladen wird
                        LoadGegnerGrafik(FETTESPINNE);
                        break;

                    case LAFASSSPAWNER:
                        // La Fass laden, wenn der La Fass Spawner geladen wird
                        LoadGegnerGrafik(LAFASS);
                        break;

                    case STACHELBEERE:
                        // Minirakete laden, wenn Stachelbeere geladen wird
                        LoadGegnerGrafik(MINIROCKET);
                        break;

                    case RIESENSPINNE:
                    case UFO:
                    case SKELETOR:
                    case DRACHE:
                        // Fette Rakete laden, wenn Riesenspinne oder Drache geladen wird
                        LoadGegnerGrafik(FETTERAKETE);

                        if (LoadObject.ObjectID == RIESENSPINNE) {
                            // Spinnenbombe laden, wenn die Riesenspinne geladen wird
                            LoadGegnerGrafik(SPIDERBOMB);
                        }

                        // Minidragon laden, wenn Drache geladen wird
                        if (LoadObject.ObjectID == DRACHE) {
                            LoadGegnerGrafik(MINIDRAGON);

                            // Drache wird geladen?
                            if (LoadObject.Value2 == 0)
                                pDragonHack = new CDragonHack();
                        }

                        // Skull laden, wenn der Skeletor geladen wird
                        if (LoadObject.ObjectID == SKELETOR) {
                            LoadGegnerGrafik(SKULL);
                        }
                        break;

                    case SCHNEEKOENIG:
                        // Schneekoppe laden, wenn der Schneekönig geladen wird
                        LoadGegnerGrafik(SCHNEEKOPPE);
                        break;

                    case BIGFISH:
                        // Ein Paar Gegner laden, wenn der BigFish geladen wird
                        LoadGegnerGrafik(PIRANHA);
                        LoadGegnerGrafik(SWIMWALKER);
                        LoadGegnerGrafik(KUGELKLEIN);
                        LoadGegnerGrafik(KUGELMEDIUM);
                        LoadGegnerGrafik(KUGELGROSS);
                        LoadGegnerGrafik(KUGELRIESIG);
                        break;

                    case ROLLMOPS:
                        // Kettenglied laden, wenn der Rollmops geladen wird
                        LoadGegnerGrafik(KETTENGLIED);
                        break;

                    case TUBE:
                        // Mutant laden, wenn die Tube geladen wird
                        LoadGegnerGrafik(MUTANT);
                        break;

                    default:
                        break;
                }

                // Gegner laden, wenn er nicht schon geladen wurde
                LoadGegnerGrafik(LoadObject.ObjectID);

                // Gegner bei aktuellem Skill level überhaupt erzeugen ?
                if (LoadObject.Skill <= Skill) {
                    if (LoadObject.ObjectID == ENDLEVEL && (LoadObject.Value2 == AKTION_LINKS ||
                        LoadObject.Value2 == AKTION_RECHTS || LoadObject.Value2 == AKTION_OBEN || LoadObject.Value2 == AKTION_UNTEN))
                        walkoutExits.push_back(LoadObject);
                    Gegner.PushGegner(static_cast<float>(LoadObject.XPos),
                                      static_cast<float>(LoadObject.YPos), LoadObject.ObjectID,
                                      LoadObject.Value1, LoadObject.Value2, LoadObject.ChangeLight);

                    if (LoadObject.ObjectID == REITFLUGSACK && NUMPLAYERS == 2)
                        Gegner.PushGegner(static_cast<float>(LoadObject.XPos + 60),
                                          static_cast<float>(LoadObject.YPos + 40), LoadObject.ObjectID,
                                          LoadObject.Value1, LoadObject.Value2, LoadObject.ChangeLight);
                }
            }
        }
    }

    // Fall back to the level's first start if it has no explicit P2 marker.
    if (!hasSecondStart) {
        // Some original maps (including Temple) mark both authored starts as P1.
        if (NUMPLAYERS == 2 && firstStart) {
            Player[1].Blickrichtung = firstStart->Value1 == 0 ? DirectionEnum::RECHTS : DirectionEnum::LINKS;
            Player[1].xpos = static_cast<float>(firstStart->XPos);
            Player[1].ypos = static_cast<float>(firstStart->YPos);
        } else {
            Player[1].Blickrichtung = Player[0].Blickrichtung;
            Player[1].xpos = Player[0].xpos + 40.0f;
            Player[1].ypos = Player[0].ypos;
        }
    }
    if (NUMPLAYERS == 2)
        Player[1].SeparateCoopSpawn(Player[0]);
    Player[1].xposold = Player[1].JumpxSave = Player[1].xpos;
    Player[1].yposold = Player[1].JumpySave = Player[1].ypos;

    Datei.read(reinterpret_cast<char *>(&DateiAppendix), sizeof(DateiAppendix));

    DateiAppendix.UsedPowerblock = FixEndian(DateiAppendix.UsedPowerblock);

    bDrawShadow = DateiAppendix.Taschenlampe;
    ShadowAlpha = 255.0f;

    // Datei schliessen
    Datei.close();

    // Temp Datei löschen und speicher freigeben
    fs::remove(fs::path(TEMP_FILE_PREFIX "temp.map"));

    // Liquid Farben setzen
    ColR1 = std::stoi(std::string(&DateiAppendix.Col1[0], 2), nullptr, 16);
    ColG1 = std::stoi(std::string(&DateiAppendix.Col1[2], 2), nullptr, 16);
    ColB1 = std::stoi(std::string(&DateiAppendix.Col1[4], 2), nullptr, 16);

    ColR2 = std::stoi(std::string(&DateiAppendix.Col2[0], 2), nullptr, 16);
    ColG2 = std::stoi(std::string(&DateiAppendix.Col2[2], 2), nullptr, 16);
    ColB2 = std::stoi(std::string(&DateiAppendix.Col2[4], 2), nullptr, 16);
    
    int const ColA1 = std::stoi(std::string(&DateiAppendix.Col1[6], 2), nullptr, 16);
    int const ColA2 = std::stoi(std::string(&DateiAppendix.Col2[6], 2), nullptr, 16);

    Col1 = D3DCOLOR_RGBA(ColR1, ColG1, ColB1, ColA1);

    Col2 = D3DCOLOR_RGBA(ColR2, ColG2, ColB2, ColA2);

    ColR3 = ColR1 + ColR2 + 32;
    if (ColR3 > 255)
        ColR3 = 255;
    ColG3 = ColG1 + ColG2 + 32;
    if (ColG3 > 255)
        ColG3 = 255;
    ColB3 = ColB1 + ColB2 + 32;
    if (ColB3 > 255)
        ColB3 = 255;
    int ColA3 = ColA1 + ColA2;
    if (ColA3 > 255)
        ColA3 = 255;

    Col3 = D3DCOLOR_RGBA(ColR3, ColG3, ColB3, ColA3);

    ComputeCoolLight();

    // Level korrekt geladen
    Protokoll << "-> Load Level : " << Filename << " successful ! <-\n" << std::endl;

    Zustand = TileStateEnum::SCROLLBAR;

    return true;
}

// --------------------------------------------------------------------------------------
// Neue Scrollspeed setzen
// --------------------------------------------------------------------------------------

void TileEngineClass::SetScrollSpeed(float xSpeed, float ySpeed) {
    ScrollSpeedX = xSpeed;
    ScrollSpeedY = ySpeed;
}

// --------------------------------------------------------------------------------------
// Ausrechnen, welcher Levelausschnitt gerendert werden soll
// --------------------------------------------------------------------------------------

TileEngineClass::HorizontalBounds TileEngineClass::HorizontalPlayfieldAt(float cameraX, bool wide) const {
    const int width = wide ? DirectGraphics.GetViewWidth() : RENDERWIDTH;
    if (width <= RENDERWIDTH)
        return {cameraX, cameraX + RENDERWIDTH};

    const float worldLeft = std::clamp(cameraX + DirectGraphics.InterfaceLeft(),
                                      0.0f, std::max(0.0f, LEVELPIXELSIZE_X - width));
    const float left = std::clamp(worldLeft, cameraX + RENDERWIDTH - width, cameraX);
    // A wider display can show both ends of a narrow map at once.
    return {left, std::max(left + PLAYER_FRAME_WIDTH, std::min(left + width, LEVELPIXELSIZE_X))};
}

float TileEngineClass::CalculateViewLeft() const {
    return HorizontalPlayfieldAt(XOffset, true).left - XOffset;
}

bool TileEngineClass::HasWideBossArena() const {
    return wideBossArena && !IsElevatorLevel && g_Fahrstuhl_yPos < 0.0f && DirectGraphics.GetViewWidth() > RENDERWIDTH &&
           (Zustand == TileStateEnum::SCROLLTOLOCK || Zustand == TileStateEnum::LOCKED);
}

void TileEngineClass::ScrollBossArena(float x, float y, TileStateEnum state) {
    ScrollLevel(x, y, state);
    wideBossArena = true;
}

bool TileEngineClass::HasFreeCoopCamera() const {
    return NUMPLAYERS == 2 && Zustand == TileStateEnum::SCROLLBAR && FlugsackFliesFree &&
           !IsElevatorLevel && g_Fahrstuhl_yPos < 0.0f && ScrollSpeedX == 0.0f && ScrollSpeedY == 0.0f &&
           NewXOffset == -1.0f && NewYOffset == -1.0f && !ShowSummary &&
           !Player[0].DoFesteAktion && !Player[1].DoFesteAktion &&
           !Player[0].StageClearRunning && !Player[1].StageClearRunning;
}

namespace {
constexpr float coopSplitMargin = 64.0f;
constexpr float coopMergeMargin = 104.0f;
constexpr float coopSwapMargin = 6.0f;
}

void TileEngineClass::ResetCoopViews() {
    coopSeparatedX = coopSeparatedY = false;
    coopSplitAmount = 0.0f;
    coopViewScale = coopTargetViewScale = 1.0f;
    coopLook = {};
}

void TileEngineClass::UpdateCoopViews() {
    if (!CanSplitCoop()) {
        ResetCoopViews();
        return;
    }

    if (Player[0].Handlung == PlayerActionEnum::TOT || Player[1].Handlung == PlayerActionEnum::TOT) {
        if (IsCoopSplit()) {
            const auto &survivor = Player[Player[0].Handlung == PlayerActionEnum::TOT ? 1 : 0];
            XOffset = survivor.xpos + 35.0f - RENDERWIDTH / 2;
            YOffset = survivor.ypos + 40.0f - RENDERHEIGHT / 2;
            CheckBounds();
        }
        ResetCoopViews();
        return;
    }

    const int fullWidth = DirectGraphics.GetViewWidth();
    const bool wasSplit = IsCoopSplit();
    const bool wasSeparatedX = coopSeparatedX;
    const bool wasSeparatedY = coopSeparatedY;
    const float dx = Player[1].xpos - Player[0].xpos;
    const float dy = Player[1].ypos - Player[0].ypos;
    // Ease out as the players spread apart, then split only once they exceed the wider view.
    const float radialDistance = std::hypot(dx / (fullWidth - 2.0f * coopSplitMargin),
                                           dy / (RENDERHEIGHT - 2.0f * coopSplitMargin));
    const float t = glm::smoothstep(0.45f, 1.0f, radialDistance);
    coopTargetViewScale = glm::mix(1.0f, maxCoopViewScale, t);
    const float xMargin = wasSeparatedX ? coopMergeMargin : coopSplitMargin;
    const float yMargin = wasSeparatedY ? coopMergeMargin : coopSplitMargin;
    coopSeparatedX = std::abs(dx) > (fullWidth - 2.0f * xMargin) * maxCoopViewScale;
    coopSeparatedY = std::abs(dy) > (RENDERHEIGHT - 2.0f * yMargin) * maxCoopViewScale;
    if (!IsCoopSplit()) {
        // Dropping to a different aspect ratio while paused invalidates the old panes.
        const int oldWidth = coopViews[0].w + (coopSplitLeftRight ? coopViews[1].w : 0);
        if (oldWidth != fullWidth)
            coopSplitAmount = 0.0f;
        return;
    }

    const bool wasLeftRight = coopSplitLeftRight;
    if (coopSeparatedX != coopSeparatedY) {
        coopSplitLeftRight = coopSeparatedX;
    } else if (!wasSplit) {
        const float xSeparation = std::abs(dx) / (fullWidth - 2.0f * coopSplitMargin);
        const float ySeparation = std::abs(dy) / (RENDERHEIGHT - 2.0f * coopSplitMargin);
        coopSplitLeftRight = xSeparation >= ySeparation;
    } else if (!wasSeparatedX || !wasSeparatedY) {
        // A new separation takes priority, such as a climb while already split left and right.
        coopSplitLeftRight = !wasSeparatedX;
    }

    // Keep pane order steady while players are nearly aligned along the split axis.
    const float separation = coopSplitLeftRight ? dx : dy;
    if (!wasSplit || coopSplitLeftRight != wasLeftRight || std::abs(separation) > coopSwapMargin)
        coopFirstPlayer = separation >= 0.0f ? 0 : 1;

    UpdateCoopViewBounds(fullWidth);
}

void TileEngineClass::UpdateCoopViewBounds(int fullWidth) {
    const int height = coopSplitLeftRight ? RENDERHEIGHT : RENDERHEIGHT / 2;
    for (int p = 0; p < 2; ++p) {
        // The opening pane (P2) receives the extra column at odd logical widths.
        const int width = coopSplitLeftRight ? (fullWidth + p) / 2 : fullWidth;
        const auto look = CoopLookOffset(p, {width, height});
        const float x = std::clamp(Player[p].xpos + 35.0f - width / 2.0f + look.x,
                                   0.0f, std::max(0.0f, LEVELPIXELSIZE_X - width));
        const float y = std::clamp(Player[p].ypos + 40.0f - height / 2.0f + look.y,
                                   0.0f, std::max(0.0f, LEVELPIXELSIZE_Y - height));
        const auto origin = ExitViewOrigin({x,y}, {width,height}, {Player[p].xpos + 35, Player[p].ypos + 40});
        coopViews[p] = {static_cast<int>(origin.x), static_cast<int>(origin.y), width, height};
    }
}

glm::vec2 TileEngineClass::CoopSplitNormal() const {
    if (coopTurnProgress < 1.0f)
        return glm::normalize(glm::vec2(cos(coopSplitAngle), sin(coopSplitAngle)));

    const float side = coopFirstPlayer == 0 ? 1.0f : -1.0f;
    return coopSplitLeftRight ? glm::vec2(side, 0.0f) : glm::vec2(0.0f, side);
}

void TileEngineClass::AdvanceCoopTransition(float frameSeconds) {
    constexpr float duration = 0.38f;
    const float frameTime = std::min(frameSeconds, 0.05f);
    const float step = frameTime / duration;
    coopViewScale = glm::mix(coopViewScale, coopTargetViewScale, 1.0f - std::exp(-12.0f * frameTime));
    if (std::abs(coopViewScale - coopTargetViewScale) < 0.0005f)
        coopViewScale = coopTargetViewScale;
    const bool firstIsPlayerOne = coopFirstPlayer == 0;
    const float targetAngle = coopSplitLeftRight ? (firstIsPlayerOne ? 0.0f : PI)
                                                : (firstIsPlayerOne ? HALF_PI : -HALF_PI);
    if (coopSplitAmount == 0.0f) {
        coopSplitAngle = coopTurnTarget = targetAngle;
        coopTurnProgress = 1.0f;
    } else if (targetAngle != coopTurnTarget) {
        // Start at the displayed angle, even if the players change direction mid-turn.
        coopTurnStart = targetAngle - std::remainder(targetAngle - coopSplitAngle, TWO_PI);
        coopTurnTarget = targetAngle;
        coopTurnProgress = 0.0f;
    }
    if (coopTurnProgress < 1.0f) {
        coopTurnProgress = std::min(1.0f, coopTurnProgress + step);
        const float t = coopTurnProgress;
        const float eased = t * t * (3.0f - 2.0f * t);
        coopSplitAngle = coopTurnStart + (coopTurnTarget - coopTurnStart) * eased;
    }
    const bool split = IsCoopSplit();
    coopSplitAmount = std::clamp(coopSplitAmount + (split ? step : -step), 0.0f, 1.0f);
    if (coopSplitAmount == 0.0f) {
        coopLook = {};
    } else {
        const float lookEase = 1.0f - std::exp(-11.0f * frameTime);
        for (int p = 0; p < 2; ++p) {
            const auto &player = Player[p];
            glm::vec2 target(0.0f);
            if (split && !Console.Showing && player.GameOverTimer == 0.0f) {
                target.x = static_cast<float>(player.Aktion[ACTION_LOOK_RIGHT]) -
                           static_cast<float>(player.Aktion[ACTION_LOOK_LEFT]);
                target.y = static_cast<float>(player.Aktion[AKTION_UNTEN]) -
                         static_cast<float>(player.Aktion[AKTION_OBEN]);
            }
            coopLook[p] = glm::mix(coopLook[p], target, lookEase);
            if (glm::length(coopLook[p] - target) < 0.002f)
                coopLook[p] = target;
        }
        if (split)
            UpdateCoopViewBounds(DirectGraphics.GetViewWidth());
    }
}

glm::vec2 TileEngineClass::ExitViewOrigin(glm::vec2 origin, glm::vec2 size, glm::vec2 focus) const {
    const auto &rect = GegnerRect[ENDLEVEL];
    for (const auto &exit : walkoutExits) {
        const bool sideways = exit.Value2 == AKTION_LINKS || exit.Value2 == AKTION_RECHTS;
        const bool forward = exit.Value2 == AKTION_RECHTS || exit.Value2 == AKTION_UNTEN;
        const int axis = sideways ? 0 : 1;
        const int cross = 1 - axis;
        const glm::vec2 low(exit.XPos + rect.left, exit.YPos + rect.top);
        const glm::vec2 high(exit.XPos + rect.right, exit.YPos + rect.bottom);
        const float entry = forward ? low[axis] : high[axis];
        const float depth = (focus[axis] - entry) * (forward ? 1 : -1);
        // Keep the doorway framed while the automatic walk carries the suits offscreen.
        const bool leaving = Player[0].StageClearRunning && NewStage == exit.Value1 &&
            origin[axis] < high[axis] && origin[axis] + size[axis] > low[axis];
        if (depth < -size[axis] || (depth > high[axis] - low[axis] + 80 && !leaving))
            continue;
        const float distance = std::max(low[cross] - focus[cross], focus[cross] - high[cross]);
        const float influence = 1.0f - glm::smoothstep(40.0f, 140.0f, distance);
        // The first two tiles of the trigger are visible. The rest is the offscreen run-out.
        const float reach = 2.0f * (sideways ? TILESIZE_X : TILESIZE_Y);
        const float edge = forward ? std::min(origin[axis], entry + reach - size[axis]) : std::max(origin[axis], entry - reach);
        origin[axis] = glm::mix(origin[axis], edge, influence);
    }
    return glm::clamp(origin, glm::vec2(0), glm::max(glm::vec2(0), glm::vec2(LEVELPIXELSIZE_X, LEVELPIXELSIZE_Y) - size));
}

glm::vec2 TileEngineClass::SharedViewOrigin(float worldScale) const {
    const glm::vec2 standard(DirectGraphics.GetViewWidth(), RENDERHEIGHT);
    const auto size = standard * worldScale;
    auto origin = glm::vec2(XOffset + CalculateViewLeft(), YOffset) + (standard - size) / 2.0f;
    for (int p = 0; p < NUMPLAYERS; ++p) {
        if (CoopPlayerReady(Player[p]))
            origin = ExitViewOrigin(origin, size, {Player[p].xpos + 35, Player[p].ypos + 40});
    }
    return glm::clamp(origin, glm::vec2(0), glm::max(glm::vec2(0), glm::vec2(LEVELPIXELSIZE_X, LEVELPIXELSIZE_Y) - size));
}

bool TileEngineClass::IsInPlayerViews(float left, float top, float right, float bottom, bool widescreen) const {
    if (IsCoopSplit()) {
        for (const auto &view : coopViews) {
            if (right >= view.x && left <= view.x + view.w && bottom >= view.y && top <= view.y + view.h)
                return true;
        }
        return false;
    }

    const float scale = widescreen ? coopViewScale : 1.0f;
    const auto origin = widescreen ? SharedViewOrigin(scale) : glm::vec2(XOffset, YOffset);
    const int width = widescreen ? DirectGraphics.GetViewWidth() : RENDERWIDTH;
    return right >= origin.x && left <= origin.x + width * scale &&
           bottom >= origin.y && top <= origin.y + RENDERHEIGHT * scale;
}

void TileEngineClass::CalcRenderRange() {
    CalcRenderRange(CalculateViewLeft(), DirectGraphics.GetViewWidth(), RENDERHEIGHT);
}

void TileEngineClass::CalcRenderRange(float left, int width, int height) {
    RenderLeft = left;
    RenderRight = left + width;
    RenderHeight = height;

    xLevel = std::max(0, static_cast<int>(XOffset) / TILESIZE_X);
    yLevel = std::max(0, static_cast<int>(YOffset) / TILESIZE_Y);
    // Tiles and actors must share the fractional camera origin as a pane moves or tilts.
    xTileOffs = XOffset - xLevel * TILESIZE_X;
    yTileOffs = YOffset - yLevel * TILESIZE_Y;

    // Keep one extra tile at each edge for water distortion and screen shake.
    RenderPosX = std::max(-xLevel, static_cast<int>(std::floor((RenderLeft + xTileOffs) / TILESIZE_X)) - 1);
    RenderPosXTo = std::min(LEVELSIZE_X - xLevel,
                           static_cast<int>(std::floor((RenderRight + xTileOffs) / TILESIZE_X)) + 2);
    RenderPosY = std::max(-yLevel, -1);
    RenderPosYTo = std::min(LEVELSIZE_Y - yLevel, height / TILESIZE_Y + 2);

    // Allocate for the largest batch actually visible, including zoom and shake.
    const size_t columns = std::max(0, RenderPosXTo - RenderPosX);
    const size_t rows = std::max(0, RenderPosYTo - RenderPosY);
    const size_t vertices = columns * rows * 6;
    if (TilesToRender.size() < vertices)
        TilesToRender.resize(vertices);
    WaterSinTable.UpdateTableIndexes(xLevel, yLevel);
}

// --------------------------------------------------------------------------------------
// Hintergrund Parallax Layer anzeigen
// --------------------------------------------------------------------------------------

void TileEngineClass::UpdateBackground() {
    CloudMovement += Timer.getSpeedFactor();
    if (CloudMovement >= RENDERWIDTH)
        CloudMovement -= RENDERWIDTH;

    if (pDragonHack != nullptr && !Console.Showing)
        pDragonHack->Run();
}

glm::vec2 TileEngineClass::BackdropCamera(glm::vec2 origin, glm::vec2 viewSize) const {
    // The parallax artwork was composed around a 640 x 480 view. Its anchor
    // follows the visible world rectangle, including when that rectangle stops at a wall.
    return ClampCameraOrigin(origin + (viewSize - glm::vec2(RENDERWIDTH, RENDERHEIGHT)) * 0.5f);
}

void TileEngineClass::DrawBackground() {
    const glm::vec2 origin(XOffset + RenderLeft, YOffset);
    const glm::vec2 size(RenderRight - RenderLeft, RENDERHEIGHT);
    const auto camera = BackdropCamera(origin, size);
    DrawBackground(origin - camera, size, camera);
}

void TileEngineClass::DrawBackground(glm::vec2 crop, glm::vec2 viewSize, glm::vec2 camera) {
    // Begin the world pass with no local sprite transform. The projection carries the camera tilt.
    g_matView = g_matModelView = glm::mat4x4(1.0f);
#if defined(USE_GL1)
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif

    // Sample at one texel per world unit before projection. The terrain and
    // parallax use the same zoom, and each pane can crop its own camera view.
    const auto drawLayer = [](const DirectGraphicsSprite &layer, glm::vec2 position, glm::vec2 size, glm::vec2 source) {
        if (layer.itsTexIdx < 0 || size.x <= 0.0f || size.y <= 0.0f)
            return;
        const auto end = position + size;
        const glm::vec2 texelScale(layer.itsXTexScale, layer.itsYTexScale);
        // Keep UVs local even far into a level. Two periods preserve both the
        // repeated and the mirrored axes without losing sub-texel precision.
        glm::vec2 uv = source * texelScale;
        uv.x = std::fmod(uv.x, 2.0f);
        uv.y = std::fmod(uv.y, 2.0f);
        const auto uvEnd = uv + size * texelScale;
        VERTEX2D quad[] = {
            {position.x, position.y, 0xFFFFFFFF, uv.x,    uv.y},
            {end.x,      position.y, 0xFFFFFFFF, uvEnd.x, uv.y},
            {position.x, end.y,      0xFFFFFFFF, uv.x,    uvEnd.y},
            {end.x,      end.y,      0xFFFFFFFF, uvEnd.x, uvEnd.y},
        };
        DirectGraphics.SetTexture(layer.itsTexIdx);
        DirectGraphics.RendertoBuffer(GL_TRIANGLE_STRIP, 2, quad);
    };
    const auto drawScrollingLayer = [&](const DirectGraphicsSprite &layer, float scroll, float y) {
        const float layerTop = y - crop.y;
        const float top = std::max(0.0f, layerTop);
        drawLayer(layer, {RenderLeft, top}, {viewSize.x, viewSize.y - top}, {crop.x + scroll, top - layerTop});
    };

    // Mirroring the outer rows fills the extra height from zoom without
    // stretching the artwork or exposing the bottom edges of the layers.
    DirectGraphics.SetColorKeyMode();
    const float backgroundScroll = bScrollBackground ? camera.x / 5.0f : 0.0f;
    drawLayer(Background, {RenderLeft, 0.0f}, viewSize, {crop.x + backgroundScroll, crop.y});

    const float scrollHeight = static_cast<float>(std::max(1, (LEVELSIZE_Y - SCREENSIZE_Y) * TILESIZE_Y));
    drawScrollingLayer(ParallaxLayer[0], camera.x / 3.0f, 180.0f - 150.0f / scrollHeight * camera.y);
    drawScrollingLayer(ParallaxLayer[1], camera.x / 2.0f, 200.0f - 200.0f / scrollHeight * camera.y);

    //----- Im Fahrstuhl-Level noch den vertikalen Parallax-Layer anzeigen

    if (IsElevatorLevel)
        drawLayer(ParallaxLayer[2], {390.0f - XOffset, 0.0f},
                  {static_cast<float>(ParallaxLayer[2].itsXFrameSize), viewSize.y}, {0.0f, YOffset / 1.5f});

    //----- Wolken Layer (Wenn Focus des Level GANZ oben, dann wird er GANZ angezeigt)

    DirectGraphics.SetAdditiveMode();

    const float cloudTop = std::clamp(120.0f / scrollHeight * camera.y, 0.0f, 240.0f);
    const float cloudHeight = std::min(viewSize.y, 240.0f - cloudTop - crop.y);
    drawLayer(CloudLayer, {RenderLeft, 0.0f}, {viewSize.x, cloudHeight},
              {crop.x + camera.x / 4.0f + CloudMovement, cloudTop + crop.y});

    DirectGraphics.SetColorKeyMode();

    //----- Dragon Hack

    if (pDragonHack != nullptr && !Console.Showing)
        pDragonHack->Draw();
}

// --------------------------------------------------------------------------------------
// Level Hintergrund anzeigen
//
// Das Level wird Tile für Tile durchgegangen das Vertex-Array solange mit
// Vertices gefüllt, die das selbe Tileset verwenden, bis ein anderes Tileset
// gesetzt werden muss, worauf alle Vetices im Array mit der alten Textur gerendert
// werden und alles mit der neuen Textur von vorne wieder losgeht, bis alle Tiles
// durchgenommen wurden.

// So funzt das auch bei FrontLevel und (vielleicht bald) Overlay
//
// Hier werden alle Tiles im Back-Layer gesetzt, die KEINE Wand sind,
// da die Wände später gesetzt werden, da sie alles verdecken, was in sie reinragt
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawBackLevel() {

    // Am Anfang noch keine Textur gewählt
    int ActualTexture = -1;

    // x und ypos am screen errechnen
    xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);
    yScreen = static_cast<float>(-yTileOffs + RenderPosY * TILESIZE_Y);

    DirectGraphics.SetColorKeyMode();

    // Noch keine Tiles zum rendern
    int NumToRender = 0;

    // DKS - WaterList lookup table has been replaced with WaterSinTableClass,
    //      see comments for it in Tileengine.h
    // int off  = 0;

    for (int j = RenderPosY; j < RenderPosYTo; j++) {
        xScreen = static_cast<float>(-xTileOffs) + RenderPosX * TILESIZE_X;

        for (int i = RenderPosX; i < RenderPosXTo; i++) {
            const LevelTileStruct& tile = TileAt(xLevel + i, yLevel + j);

            if (tile.BackArt > 0 &&  // Überhaupt ein Tile drin ?
                (!(tile.Block & BLOCKWERT_WAND) ||
                 (tile.FrontArt > 0 &&
                  tile.Block & BLOCKWERT_VERDECKEN))) {
                // Neue Textur ?
                if (tile.TileSetBack != ActualTexture) {
                    // Aktuelle Textur sichern
                    ActualTexture = tile.TileSetBack;

                    // Tiles zeichnen
                    if (NumToRender > 0)
                        DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);

                    // Neue aktuelle Textur setzen
                    DirectGraphics.SetTexture(TileGfx[ActualTexture].itsTexIdx);

                    // Und beim rendern wieder von vorne anfangen
                    NumToRender = 0;
                }

                unsigned int Type = tile.BackArt - INCLUDE_ZEROTILE;

                // Animiertes Tile ?
                if (tile.Block & BLOCKWERT_ANIMIERT_BACK)
                    Type += 36 * TileAnimPhase;

                // richtigen Ausschnitt für das aktuelle Tile setzen
                RECT_struct const Rect = TileRects[Type];

                // Screen-Koordinaten der Vertices
                float const l = xScreen;               // Links
                float const o = yScreen;               // Oben
                float const r = xScreen + TILESIZE_X;  // Rechts
                float const u = yScreen + TILESIZE_Y;  // Unten

                // Textur-Koordinaten
                const auto [tl, tr, to, tu] = TileGfx[ActualTexture].FrameUV(Rect);

                // Vertices definieren
                v1.color = tile.Color[0];
                v2.color = tile.Color[1];
                v3.color = tile.Color[2];
                v4.color = tile.Color[3];

                v1.x = l;  // Links oben
                v1.y = o;
                v1.tu = tl;
                v1.tv = to;

                v2.x = r;  // Rechts oben
                v2.y = o;
                v2.tu = tr;
                v2.tv = to;

                v3.x = l;  // Links unten
                v3.y = u;
                v3.tu = tl;
                v3.tv = tu;

                v4.x = r;  // Rechts unten
                v4.y = u;
                v4.tu = tr;
                v4.tv = tu;

                // Hintergrund des Wasser schwabbeln lassen

                // DKS - WaterList lookup table has been replaced with WaterSinTableClass,
                //      see comments for it in Tileengine.h
#if 0
                off = (static_cast<int>(SinPos2) + (yLevel * 2) % 40 + j*2) % 1024;

                //DKS - Fixed out of bounds access to Tiles[][] array on y here:
                //      When yLevel is 1 and j is -1 (indicating that the one-tile overdraw
                //      border is being drawn on the top screen edge), this was ending up
                //      trying to access a row higher than the top screen border which doesn't
                //      exist. Loading the Eis map (level 7) would crash on some machines.
                //if (TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)                    // Original line
                if (yLevel+j > 0 &&    // DKS Added this check to above line
                        TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)
                {
                    if (TileAt(xLevel+i, yLevel+j).move_v1 == true) v1.x += SinList2[off];
                    if (TileAt(xLevel+i, yLevel+j).move_v2 == true) v2.x += SinList2[off];
                }

                if (TileAt(xLevel+i, yLevel+j).move_v3 == true) v3.x += SinList2[off + 2];
                if (TileAt(xLevel+i, yLevel+j).move_v4 == true) v4.x += SinList2[off + 2];
#endif  // 0

                if (tile.move_v1 || tile.move_v2 ||
                    tile.move_v3 || tile.move_v4) {
                    float x_offs[2];
                    WaterSinTable.GetNonWaterSin(j, x_offs);

                    // DKS - Fixed out of bounds access to Tiles[][] array on y here:
                    //      When yLevel is 1 and j is -1 (indicating that the one-tile overdraw
                    //      border is being drawn on the top screen edge), this was ending up
                    //      trying to access a row higher than the top screen border which doesn't
                    //      exist. Loading the Eis map (level 7) would crash on some machines.
                    // if (TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)                    // Original line
                    if (yLevel + j > 0 &&  // DKS Added this check to above line
                        TileAt(xLevel + i, yLevel + j - 1).Block & BLOCKWERT_LIQUID) {
                        if (tile.move_v1 == true)
                            v1.x += x_offs[0];
                        if (tile.move_v2 == true)
                            v2.x += x_offs[0];
                    }

                    if (tile.move_v3 == true)
                        v3.x += x_offs[1];
                    if (tile.move_v4 == true)
                        v4.x += x_offs[1];
                }

                // Zu rendernde Vertices ins Array schreiben
                TilesToRender[NumToRender * 6 + 0] = v1;  // Jeweils 2 Dreicke als
                TilesToRender[NumToRender * 6 + 1] = v2;  // als ein viereckiges
                TilesToRender[NumToRender * 6 + 2] = v3;  // Tile ins Array kopieren
                TilesToRender[NumToRender * 6 + 3] = v3;
                TilesToRender[NumToRender * 6 + 4] = v2;
                TilesToRender[NumToRender * 6 + 5] = v4;

                NumToRender++;  // Weiter im Vertex Array
            }
            xScreen += TILESIZE_X;  // Am Screen weiter
        }
        yScreen += TILESIZE_Y;  // Am Screen weiter
    }

    if (NumToRender > 0)
        DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);
}

// --------------------------------------------------------------------------------------
// Level Vodergrund anzeigen
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawFrontLevel() {

    // Am Anfang noch keine Textur gewählt
    int ActualTexture = -1;

    // x und ypos am screen errechnen
    xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);
    yScreen = static_cast<float>(-yTileOffs + RenderPosY * TILESIZE_Y);

    DirectGraphics.SetColorKeyMode();

    // Noch keine Tiles zum rendern
    int NumToRender = 0;

    // DKS - WaterList lookup table has been replaced with WaterSinTableClass,
    //      see comments for it in Tileengine.h
    // int off = 0;

    for (int j = RenderPosY; j < RenderPosYTo; j++) {
        xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);

        for (int i = RenderPosX; i < RenderPosXTo; i++) {
            const LevelTileStruct& tile = TileAt(xLevel + i, yLevel + j);

            if (tile.FrontArt > 0 &&
                !(tile.Block & BLOCKWERT_VERDECKEN) &&
                !(tile.Block & BLOCKWERT_WAND)) {
                // Neue Textur ?
                if (tile.TileSetFront != ActualTexture) {
                    // Aktuelle Textur sichern
                    ActualTexture = tile.TileSetFront;

                    // Tiles zeichnen
                    if (NumToRender > 0)
                        DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);

                    // Neue aktuelle Textur setzen
                    DirectGraphics.SetTexture(TileGfx[ActualTexture].itsTexIdx);

                    // Und beim rendern wieder von vorne anfangen
                    NumToRender = 0;
                }

                unsigned int Type = tile.FrontArt - INCLUDE_ZEROTILE;

                // Animiertes Tile ?
                if (tile.Block & BLOCKWERT_ANIMIERT_FRONT)
                    Type += 36 * TileAnimPhase;

                // richtigen Ausschnitt für das aktuelle Tile setzen
                RECT_struct const Rect = TileRects[Type];

                // Screen-Koordinaten der Vertices
                float const l = xScreen;               // Links
                float const o = yScreen;               // Oben
                float const r = xScreen + TILESIZE_X;  // Rechts
                float const u = yScreen + TILESIZE_Y;  // Unten

                // Textur-Koordinaten
                const auto [tl, tr, to, tu] = TileGfx[ActualTexture].FrameUV(Rect);

                // Licht setzen (prüfen auf Overlay light, wegen hellen Kanten)
                if (tile.Block & BLOCKWERT_OVERLAY_LIGHT) {
                    v1.color = tile.Color[0];
                    v2.color = tile.Color[1];
                    v3.color = tile.Color[2];
                    v4.color = tile.Color[3];
                } else {
                    v1.color = v2.color = v3.color = v4.color =
                        D3DCOLOR_RGBA(255, 255, 255, tile.Alpha);
                }

                v1.x = l;  // Links oben
                v1.y = o;
                v1.tu = tl;
                v1.tv = to;

                v2.x = r;  // Rechts oben
                v2.y = o;
                v2.tu = tr;
                v2.tv = to;

                v3.x = l;  // Links unten
                v3.y = u;
                v3.tu = tl;
                v3.tv = tu;

                v4.x = r;  // Rechts unten
                v4.y = u;
                v4.tu = tr;
                v4.tv = tu;

                // Hintergrund des Wasser schwabbeln lassen

                // DKS - WaterList lookup table has been replaced with WaterSinTableClass,
                //      see comments for it in Tileengine.h
#if 0
                off = (static_cast<int>(SinPos2) + (yLevel * 2) % 40 + j*2) % 1024;

                //DKS - Fixed out of bounds access to Tiles[][] array on y here:
                //      When yLevel is 1 and j is -1 (indicating that the one-tile overdraw
                //      border is being drawn on the top screen edge), this was ending up
                //      trying to access a row higher than the top screen border which doesn't
                //      exist. Loading the Eis map (level 7) would crash on some machines.
                //if (TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)                    // Original line
                if (yLevel+j > 0 &&    // DKS Added this check to above line
                        TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)
                {
                    if (TileAt(xLevel+i, yLevel+j).move_v1 == true) v1.x += SinList2[off];
                    if (TileAt(xLevel+i, yLevel+j).move_v2 == true) v2.x += SinList2[off];
                }

                if (TileAt(xLevel+i, yLevel+j).move_v3 == true) v3.x += SinList2[off + 2];
                if (TileAt(xLevel+i, yLevel+j).move_v4 == true) v4.x += SinList2[off + 2];
#endif  // 0

                if (tile.move_v1 || tile.move_v2 ||
                    tile.move_v3 || tile.move_v4) {
                    float x_offs[2];
                    WaterSinTable.GetNonWaterSin(j, x_offs);

                    // DKS - Fixed out of bounds access to Tiles[][] array on y here:
                    //      When yLevel is 1 and j is -1 (indicating that the one-tile overdraw
                    //      border is being drawn on the top screen edge), this was ending up
                    //      trying to access a row higher than the top screen border which doesn't
                    //      exist. Loading the Eis map (level 7) would crash on some machines.
                    // if (TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID)                    // Original line
                    if (yLevel + j > 0 &&  // DKS Added this check to above line
                        TileAt(xLevel + i, yLevel + j - 1).Block & BLOCKWERT_LIQUID) {
                        if (tile.move_v1 == true)
                            v1.x += x_offs[0];
                        if (tile.move_v2 == true)
                            v2.x += x_offs[0];
                    }

                    if (tile.move_v3 == true)
                        v3.x += x_offs[1];
                    if (tile.move_v4 == true)
                        v4.x += x_offs[1];
                }

                // Zu rendernde Vertices ins Array schreiben
                TilesToRender[NumToRender * 6 + 0] = v1;  // Jeweils 2 Dreicke als
                TilesToRender[NumToRender * 6 + 1] = v2;  // als ein viereckiges
                TilesToRender[NumToRender * 6 + 2] = v3;  // Tile ins Array kopieren
                TilesToRender[NumToRender * 6 + 3] = v3;
                TilesToRender[NumToRender * 6 + 4] = v2;
                TilesToRender[NumToRender * 6 + 5] = v4;

                NumToRender++;  // Weiter im Vertex Array
            }
            xScreen += TILESIZE_X;  // Am Screen weiter
        }
        yScreen += TILESIZE_Y;  // Am Screen weiter
    }

    if (NumToRender > 0)

        DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);
}

// --------------------------------------------------------------------------------------
// Level Ausschnitt scrollen
// --------------------------------------------------------------------------------------

void TileEngineClass::ScrollLevel(float x, float y, TileStateEnum neu, float sx, float sy) {
    wideBossArena = false;
    if (NUMPLAYERS == 2 && neu == TileStateEnum::SCROLLTOLOCK) {
        const auto origin = ClampCameraOrigin({x, y});
        x = origin.x;
        y = origin.y;
    }
    ScrolltoX = x;
    ScrolltoY = y;
    SpeedX = sx;
    SpeedY = sy;
    Zustand = neu;
    Player[0].BronsonCounter = 0.0f;
    Player[1].BronsonCounter = 0.0f;
}

// --------------------------------------------------------------------------------------
// Wandstücke, die den Spieler vedecken, erneut zeichnen
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawBackLevelOverlay() {

    // Am Anfang noch keine Textur gewählt
    int ActualTexture = -1;

    // x und ypos am screen errechnen
    xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);
    yScreen = static_cast<float>(-yTileOffs + RenderPosY * TILESIZE_Y);

    DirectGraphics.SetColorKeyMode();

    // Noch keine Tiles zum rendern
    int NumToRender = 0;
    // int al;
    // DKS - Variable was unused in original source, disabled:
    // int off = 0;

    for (int j = RenderPosY; j < RenderPosYTo; j++) {
        xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);

        for (int i = RenderPosX; i < RenderPosXTo; i++) {
            const LevelTileStruct& tile = TileAt(xLevel + i, yLevel + j);

            // Hintergrundtile nochmal neu setzen?
            //
            if (tile.BackArt > 0 && tile.Block & BLOCKWERT_WAND &&
                (!(tile.FrontArt > 0 &&
                   tile.Block & BLOCKWERT_VERDECKEN))) {
                // Neue Textur ?
                if (tile.TileSetBack != ActualTexture) {
                    // Aktuelle Textur sichern
                    ActualTexture = tile.TileSetBack;

                    // Tiles zeichnen
                    if (NumToRender > 0)
                        DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);

                    // Neue aktuelle Textur setzen
                    DirectGraphics.SetTexture(TileGfx[ActualTexture].itsTexIdx);

                    // Und beim rendern wieder von vorne anfangen
                    NumToRender = 0;
                }

                unsigned int Type = tile.BackArt - INCLUDE_ZEROTILE;

                // Animiertes Tile ?
                if (tile.Block & BLOCKWERT_ANIMIERT_BACK)
                    Type += 36 * TileAnimPhase;

                // richtigen Ausschnitt für das aktuelle Tile setzen
                RECT_struct const Rect = TileRects[Type];

                // Screen-Koordinaten der Vertices
                float const l = xScreen;               // Links
                float const o = yScreen;               // Oben
                float const r = xScreen + TILESIZE_X;  // Rechts
                float const u = yScreen + TILESIZE_Y;  // Unten

                // Textur-Koordinaten
                const auto [tl, tr, to, tu] = TileGfx[ActualTexture].FrameUV(Rect);

                // al = tile.Alpha;

                // DKS - Variable was unused in original source, disabled:
                // off = (static_cast<int>(SinPos2) + (yLevel * 2) % 40 + j*2) % 1024;

                v1.color = tile.Color[0];
                v2.color = tile.Color[1];
                v3.color = tile.Color[2];
                v4.color = tile.Color[3];

                v1.x = l;  // Links oben
                v1.y = o;
                v1.tu = tl;
                v1.tv = to;

                v2.x = r;  // Rechts oben
                v2.y = o;
                v2.tu = tr;
                v2.tv = to;

                v3.x = l;  // Links unten
                v3.y = u;
                v3.tu = tl;
                v3.tv = tu;

                v4.x = r;  // Rechts unten
                v4.y = u;
                v4.tu = tr;
                v4.tv = tu;

                // Zu rendernde Vertices ins Array schreiben
                TilesToRender[NumToRender * 6 + 0] = v1;  // Jeweils 2 Dreicke als
                TilesToRender[NumToRender * 6 + 1] = v2;  // als ein viereckiges
                TilesToRender[NumToRender * 6 + 2] = v3;  // Tile ins Array kopieren
                TilesToRender[NumToRender * 6 + 3] = v3;
                TilesToRender[NumToRender * 6 + 4] = v2;
                TilesToRender[NumToRender * 6 + 5] = v4;

                NumToRender++;  // Weiter im Vertex Array
            }
            xScreen += TILESIZE_X;  // Am Screen weiter
        }
        yScreen += TILESIZE_Y;  // Am Screen weiter
    }

    if (NumToRender > 0)
        DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);
}

// --------------------------------------------------------------------------------------
// Die Levelstücke zeigen, die den Spieler verdecken
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawOverlayLevel() {

    // Am Anfang noch keine Textur gewählt
    int ActualTexture = -1;
    const float movement = 60.0f / 256.0f * std::fmod(WasserfallOffset, 120.0f) / 120.0f;

    // x und ypos am screen errechnen
    xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);
    yScreen = static_cast<float>(-yTileOffs + RenderPosY * TILESIZE_Y);

    DirectGraphics.SetColorKeyMode();

    // Noch keine Tiles zum rendern
    int NumToRender = 0;
    // int al;
    // DKS - Variable was unused in original source, disabled:
    // int off = 0;

    for (int j = RenderPosY; j < RenderPosYTo; j++) {
        xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);

        for (int i = RenderPosX; i < RenderPosXTo; i++) {
            const LevelTileStruct& tile = TileAt(xLevel + i, yLevel + j);

            // Vordergrund Tiles setzen, um Spieler zu verdecken
            if ((tile.FrontArt > 0 &&
                 (tile.Block & BLOCKWERT_VERDECKEN ||
                  tile.Block & BLOCKWERT_WAND)) ||
                tile.Block & BLOCKWERT_WASSERFALL ||
                tile.Block & BLOCKWERT_MOVELINKS ||
                tile.Block & BLOCKWERT_MOVERECHTS ||
                tile.Block & BLOCKWERT_MOVEVERTICAL) {
                // Screen-Koordinaten der Vertices
                float const l = xScreen;               // Links
                float const o = yScreen;               // Oben
                float const r = xScreen + TILESIZE_X;  // Rechts
                float const u = yScreen + TILESIZE_Y;  // Unten
                /*
                                // Wasserfall
                                //
                                if (TileAt(xLevel+i, yLevel+j).Block & BLOCKWERT_WASSERFALL)
                                {
                                    // Tiles zeichnen
                                    if (NumToRender > 0)
                                    {
                                        DirectGraphics.RendertoBuffer (GL_TRIANGLES, NumToRender*2,
                   &TilesToRender[0]); NumToRender   = 0;
                                    }

                                    ActualTexture = -1;

                                    // Drei Schichten Wasserfall rendern =)
                                    //

                                    // Schicht 1
                                    //
                                    int xoff = (i+xLevel) % 3 * 20;
                                    int yoff = (j+yLevel) % 3 * 20 + 120 - int (WasserfallOffset);

                                    Wasserfall[0].SetRect (xoff, yoff, xoff+20, yoff+20);
                                    Wasserfall[0].RenderSprite(static_cast<float>(i*20-xTileOffs),
                   static_cast<float>(j*20-yTileOffs), Col1);

                                    // Schicht 2
                                    //
                                    xoff = (i+xLevel+1) % 3 * 20;
                                    yoff = (j+yLevel)   % 3 * 20 + 120 - int (WasserfallOffset / 2.0f);

                                    Wasserfall[0].SetRect (xoff, yoff, xoff+20, yoff+20);
                                    Wasserfall[0].RenderSprite(static_cast<float>(i*20-xTileOffs),
                   static_cast<float>(j*20-yTileOffs), Col2);

                                    // Glanzschicht (Schicht 3) drüber
                                    //
                                    DirectGraphics.SetAdditiveMode();
                                    Wasserfall[1].SetRect ((i*20-xTileOffs)%640, (j*20-yTileOffs)%480,
                   (i*20-xTileOffs)%640+20, (j*20-yTileOffs)%480+20);
                                    Wasserfall[1].RenderSprite(static_cast<float>(i*20-xTileOffs),
                   static_cast<float>(j*20-yTileOffs), D3DCOLOR_RGBA (180, 240, 255, 60));
                                    DirectGraphics.SetColorKeyMode();
                                }


                                // normales Overlay Tile
                                else
                */
                if ((tile.FrontArt > 0 &&
                     (tile.Block & BLOCKWERT_VERDECKEN ||
                      tile.Block & BLOCKWERT_MOVEVERTICAL ||
                      tile.Block & BLOCKWERT_MOVELINKS ||
                      tile.Block & BLOCKWERT_MOVERECHTS ||
                      tile.Block & BLOCKWERT_WAND))) {
                    // Neue Textur ?
                    if (tile.TileSetFront != ActualTexture) {
                        // Aktuelle Textur sichern
                        ActualTexture = tile.TileSetFront;

                        // Tiles zeichnen
                        if (NumToRender > 0)
                            DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);

                        // Neue aktuelle Textur setzen
                        DirectGraphics.SetTexture(TileGfx[ActualTexture].itsTexIdx);

                        // Und beim rendern wieder von vorne anfangen
                        NumToRender = 0;
                    }

                    // "normales" Overlay Tile setzen
                    unsigned int Type = tile.FrontArt - INCLUDE_ZEROTILE;

                    // Animiertes Tile ?
                    if (tile.Block & BLOCKWERT_ANIMIERT_FRONT)
                        Type += 36 * TileAnimPhase;

                    // richtigen Ausschnitt für das aktuelle Tile setzen
                    RECT_struct const Rect = TileRects[Type];

                    // Textur-Koordinaten
                    auto [tl, tr, to, tu] = TileGfx[ActualTexture].FrameUV(Rect);

                    // bewegtes Tile vertikal
                    if (tile.Block & BLOCKWERT_MOVEVERTICAL) {
                        to -= movement;
                        tu -= movement;
                    }

                    // bewegtes Tile links
                    if (tile.Block & BLOCKWERT_MOVELINKS) {
                        tl += movement;
                        tr += movement;
                    }

                    // bewegtes Tile rechts
                    if (tile.Block & BLOCKWERT_MOVERECHTS) {
                        tl -= movement;
                        tr -= movement;
                    }

                    // al = tile.Alpha;

                    if (tile.Block & BLOCKWERT_OVERLAY_LIGHT) {
                        v1.color = tile.Color[0];
                        v2.color = tile.Color[1];
                        v3.color = tile.Color[2];
                        v4.color = tile.Color[3];
                    } else {
                        v1.color = v2.color = v3.color = v4.color =
                            D3DCOLOR_RGBA(255, 255, 255, tile.Alpha);
                    }

                    // DKS - Variable was unused in original source, disabled:
                    // off = (static_cast<int>(SinPos2) + (yLevel * 2) % 40 + j*2) % 1024;

                    v1.x = l;  // Links oben
                    v1.y = o;
                    v1.tu = tl;
                    v1.tv = to;

                    v2.x = r;  // Rechts oben
                    v2.y = o;
                    v2.tu = tr;
                    v2.tv = to;

                    v3.x = l;  // Links unten
                    v3.y = u;
                    v3.tu = tl;
                    v3.tv = tu;

                    v4.x = r;  // Rechts unten
                    v4.y = u;
                    v4.tu = tr;
                    v4.tv = tu;

                    // Zu rendernde Vertices ins Array schreiben
                    TilesToRender[NumToRender * 6 + 0] = v1;  // Jeweils 2 Dreicke als
                    TilesToRender[NumToRender * 6 + 1] = v2;  // als ein viereckiges
                    TilesToRender[NumToRender * 6 + 2] = v3;  // Tile ins Array kopieren
                    TilesToRender[NumToRender * 6 + 3] = v3;
                    TilesToRender[NumToRender * 6 + 4] = v2;
                    TilesToRender[NumToRender * 6 + 5] = v4;

                    NumToRender++;  // Weiter im Vertex Array
                }
            }

            xScreen += TILESIZE_X;  // Am Screen weiter
        }

        yScreen += TILESIZE_Y;  // Am Screen weiter
    }

    if (NumToRender > 0)
        DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);
}

// --------------------------------------------------------------------------------------
// Die Levelstücke zeigen, die den Spieler verdecken
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawWater() {

    // x und ypos am screen errechnen
    xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);
    yScreen = static_cast<float>(-yTileOffs + RenderPosY * TILESIZE_Y);

    // Noch keine Tiles zum rendern
    int NumToRender = 0;

    // DKS - WaterList lookup table has been replaced with WaterSinTableClass,
    //      see comments for it in Tileengine.h
    // int off = static_cast<int>((WaterPos) + xLevel % 32 + (yLevel * 10) % 40) % 1024;

    DirectGraphics.SetFilterMode(true);
    DirectGraphics.SetColorKeyMode();

    // billiges Wasser?
    //
    if (options_Detail < DETAIL_MEDIUM) {
        DirectGraphics.SetTexture(-1);

        for (int j = RenderPosY; j < RenderPosYTo; j++) {
            xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);

            for (int i = RenderPosX; i < RenderPosXTo; i++) {
                // Vordergrund Tiles setzen um Spieler zu verdecken
                if (TileAt(i + xLevel, j + yLevel).Block & BLOCKWERT_LIQUID) {
                    // Screen-Koordinaten der Vertices
                    float const l = xScreen;               // Links
                    float const o = yScreen;               // Oben
                    float const r = xScreen + TILESIZE_X;  // Rechts
                    float const u = yScreen + TILESIZE_Y;  // Unten

                    // DKS - Original code was setting texture coordinates when no texture
                    //      is bound.. disabled the code:
                    /* DISABLED BLOCK
                    // Textur Koordinaten setzen
                    //
                    xo = (i + xLevel) % 8;
                    yo = (j + yLevel) % 8;

                    v1.tu = WasserU [xo];
                    v1.tv = WasserV [yo];
                    v2.tu = WasserU [xo+1];
                    v2.tv = WasserV [yo];
                    v3.tu = WasserU [xo];
                    v3.tv = WasserV [yo+1];
                    v4.tu = WasserU [xo+1];
                    v4.tv = WasserV [yo+1];
                    END DISABLED BLOCK */

                    v1.x = l;  // Links oben
                    v1.y = o;
                    v2.x = r;  // Rechts oben
                    v2.y = o;
                    v3.x = l;  // Links unten
                    v3.y = u;
                    v4.x = r;  // Rechts unten
                    v4.y = u;

                    // Farbe festlegen
                    //
                    v1.color = v2.color = v3.color = v4.color = Col1;

                    // Zu rendernde Vertices ins Array schreiben
                    TilesToRender[NumToRender * 6 + 0] = v1;  // Jeweils 2 Dreicke als
                    TilesToRender[NumToRender * 6 + 1] = v2;  // als ein viereckiges
                    TilesToRender[NumToRender * 6 + 2] = v3;  // Tile ins Array kopieren
                    TilesToRender[NumToRender * 6 + 3] = v3;
                    TilesToRender[NumToRender * 6 + 4] = v2;
                    TilesToRender[NumToRender * 6 + 5] = v4;

                    NumToRender++;  // Weiter im Vertex Array
                }

                xScreen += TILESIZE_X;  // Am Screen weiter
            }

            yScreen += TILESIZE_Y;  // Am Screen weiter
        }

        if (NumToRender > 0)
            DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);
    }

    // oder geiles, animiertes Wasser?
    else {
        // zwei Schichten Wasser rendern
        for (int schicht = 0; schicht < 2; schicht++) {
            // ypos am screen errechnen
            yScreen = static_cast<float>(-yTileOffs + RenderPosY * TILESIZE_Y);

            for (int j = RenderPosY; j < RenderPosYTo; j++) {
                xScreen = static_cast<float>(-xTileOffs + RenderPosX * TILESIZE_X);

                for (int i = RenderPosX; i < RenderPosXTo; i++) {
                    const LevelTileStruct& tile = TileAt(xLevel + i, yLevel + j);

                    // Vordergrund Tiles setzen um Spieler zu verdecken
                    if (tile.Block & BLOCKWERT_LIQUID) {
                        // Screen-Koordinaten der Vertices
                        float const l = xScreen;               // Links
                        float const o = yScreen;               // Oben
                        float const r = xScreen + TILESIZE_X;  // Rechts
                        float const u = yScreen + TILESIZE_Y;  // Unten

                        // Vertices definieren
                        v1.x = l;  // Links oben
                        v1.y = o;
                        v2.x = r;  // Rechts oben
                        v2.y = o;
                        v3.x = l;  // Links unten
                        v3.y = u;
                        v4.x = r;  // Rechts unten
                        v4.y = u;

                        if (schicht == 0) {
                            v1.color = v2.color = v3.color = v4.color = Col1;
                        } else {
                            v1.color = v2.color = v3.color = v4.color = Col2;
                        }

                        // Oberfläche des Wassers aufhellen
                        // DKS - Fixed potential out of bounds access to Tiles[][] array here
                        //      by adding check for yLevel+j-1 >= 0:
                        // if (!(TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_LIQUID) &&           (ORIGINAL TWO
                        // LINES)
                        //        !(TileAt(xLevel+i, yLevel+j-1).Block & BLOCKWERT_WASSERFALL))
                        if (yLevel + j - 1 >= 0 && !(TileAt(xLevel + i, yLevel + j - 1).Block & BLOCKWERT_LIQUID) &&
                            !(TileAt(xLevel + i, yLevel + j - 1).Block & BLOCKWERT_WASSERFALL)) {
                            // DKS - Fixed potential out of bounds access to Tiles[][] array here
                            //      by adding check for xLevel+i-1 >= 0:
                            // if (!(TileAt(xLevel+i-1, yLevel+j-1).Block & BLOCKWERT_LIQUID) &&
                            //        !(TileAt(xLevel+i-1, yLevel+j-1).Block & BLOCKWERT_WASSERFALL))
                            if (xLevel + i - 1 >= 0 &&
                                !(TileAt(xLevel + i - 1, yLevel + j - 1).Block & BLOCKWERT_LIQUID) &&
                                !(TileAt(xLevel + i - 1, yLevel + j - 1).Block & BLOCKWERT_WASSERFALL))
                                v1.color = Col3;

                            // DKS - Fixed potential out of bounds access to Tiles[][] array here
                            //      by adding check for xLevel+i+1 < LEVELSIZE_X
                            // if (!(TileAt(xLevel+i+1, yLevel+j-1).Block & BLOCKWERT_LIQUID) &&
                            //        !(TileAt(xLevel+i+1, yLevel+j-1).Block & BLOCKWERT_WASSERFALL))
                            if (xLevel + i + 1 < LEVELSIZE_X &&
                                !(TileAt(xLevel + i + 1, yLevel + j - 1).Block & BLOCKWERT_LIQUID) &&
                                !(TileAt(xLevel + i + 1, yLevel + j - 1).Block & BLOCKWERT_WASSERFALL))
                                v2.color = Col3;
                        }

                        int const xo = (i + xLevel) % 8;
                        int const yo = (j + yLevel) % 8;

                        // Schicht 0 == langsam, gespiegelt
                        if (schicht == 0) {
                            v1.tu = WasserU[8 - xo];
                            v1.tv = WasserV[8 - yo];
                            v2.tu = WasserU[8 - xo - 1];
                            v2.tv = WasserV[8 - yo];
                            v3.tu = WasserU[8 - xo];
                            v3.tv = WasserV[8 - yo - 1];
                            v4.tu = WasserU[8 - xo - 1];
                            v4.tv = WasserV[8 - yo - 1];
                        }

                        // Schicht 0 == schnell
                        else {
                            v1.tu = WasserU[xo];
                            v1.tv = WasserV[yo];
                            v2.tu = WasserU[xo + 1];
                            v2.tv = WasserV[yo];
                            v3.tu = WasserU[xo];
                            v3.tv = WasserV[yo + 1];
                            v4.tu = WasserU[xo + 1];
                            v4.tv = WasserV[yo + 1];
                        }

                        // DKS - WaterList lookup table has been replaced with WaterSinTableClass,
                        //      see comments for it in Tileengine.h
                        // if (TileAt(xLevel+i, yLevel+j).move_v1 == true)
                        //    v1.y += WaterList[off + j*10 + i * 2];
                        // if (TileAt(xLevel+i, yLevel+j).move_v2 == true)
                        //    v2.y += WaterList[off + j*10 + i * 2];
                        // if (TileAt(xLevel+i, yLevel+j).move_v3 == true)
                        //    v3.y += WaterList[off + j*10 + i * 2 + 10];
                        // if (TileAt(xLevel+i, yLevel+j).move_v4 == true)
                        //    v4.y += WaterList[off + j*10 + i * 2 + 10];
                        if (tile.move_v1 || tile.move_v2 ||
                            tile.move_v3 || tile.move_v4) {
                            // Adjacent tiles must displace their shared
                            // vertices by the same amount.
                            float left[2], right[2];
                            WaterSinTable.GetWaterSin(i, j, left);
                            WaterSinTable.GetWaterSin(i + 1, j, right);
                            if (tile.move_v1 == true)
                                v1.y += left[0];
                            if (tile.move_v2 == true)
                                v2.y += right[0];
                            if (tile.move_v3 == true)
                                v3.y += left[1];
                            if (tile.move_v4 == true)
                                v4.y += right[1];
                        }

                        // Zu rendernde Vertices ins Array schreiben
                        TilesToRender[NumToRender * 6 + 0] = v1;  // Jeweils 2 Dreicke als
                        TilesToRender[NumToRender * 6 + 1] = v2;  // als ein viereckiges
                        TilesToRender[NumToRender * 6 + 2] = v3;  // Tile ins Array kopieren
                        TilesToRender[NumToRender * 6 + 3] = v3;
                        TilesToRender[NumToRender * 6 + 4] = v2;
                        TilesToRender[NumToRender * 6 + 5] = v4;

                        NumToRender++;  // Weiter im Vertex Array
                    }

                    xScreen += TILESIZE_X;  // Am Screen weiter
                }

                yScreen += TILESIZE_Y;  // Am Screen weiter
            }

            if (NumToRender > 0) {
                if (schicht > 0) {
                    DirectGraphics.SetAdditiveMode();
                    DirectGraphics.SetTexture(LiquidGfx[1].itsTexIdx);
                } else {
                    DirectGraphics.SetTexture(LiquidGfx[0].itsTexIdx);
                }

                DirectGraphics.RendertoBuffer(GL_TRIANGLES, NumToRender * 2, &TilesToRender[0]);
            }

            NumToRender = 0;
        }
    }

    // The two flows scroll through whole texture periods at their original
    // speeds. Keep the highlight at its authored scale on every viewport.
    const D3DCOLOR waterfallColors[] = {Col1, Col2, D3DCOLOR_RGBA(180, 240, 255, 60)};
    DirectGraphics.SetColorKeyMode();
    for (int layer = 0; layer < 3; ++layer) {
        const bool highlight = layer == 2;
        const auto &texture = Wasserfall[highlight ? 1 : 0];
        const auto color = waterfallColors[layer];
        const float scrollY = WasserfallOffset / (layer + 1);
        size_t vertexCount = 0;
        for (int j = RenderPosY; j < RenderPosYTo; ++j) {
            const int tileY = j + yLevel;
            const float top = static_cast<float>(j * TILESIZE_Y - yTileOffs);
            for (int i = RenderPosX; i < RenderPosXTo; ++i) {
                const int tileX = i + xLevel;
                if (!(TileAt(tileX, tileY).Block & BLOCKWERT_WASSERFALL))
                    continue;

                const float left = static_cast<float>(i * TILESIZE_X - xTileOffs);
                const float right = left + TILESIZE_X;
                const float bottom = top + TILESIZE_Y;
                constexpr int tilesPerStrip = waterfallWidth / TILESIZE_X;
                const float textureX = highlight ? left : (tileX + layer) % tilesPerStrip * TILESIZE_X;
                const float textureY = highlight ? top : tileY % tilesPerStrip * TILESIZE_Y + waterfallHeight / 2 - scrollY;
                const float u0 = textureX * texture.itsXTexScale;
                const float u1 = (textureX + TILESIZE_X) * texture.itsXTexScale;
                const float v0 = textureY * texture.itsYTexScale;
                const float v1 = (textureY + TILESIZE_Y) * texture.itsYTexScale;
                const VERTEX2D upperLeft{left, top, color, u0, v0};
                const VERTEX2D upperRight{right, top, color, u1, v0};
                const VERTEX2D lowerLeft{left, bottom, color, u0, v1};
                const VERTEX2D lowerRight{right, bottom, color, u1, v1};
                TilesToRender[vertexCount++] = upperLeft;
                TilesToRender[vertexCount++] = upperRight;
                TilesToRender[vertexCount++] = lowerLeft;
                TilesToRender[vertexCount++] = lowerLeft;
                TilesToRender[vertexCount++] = upperRight;
                TilesToRender[vertexCount++] = lowerRight;
            }
        }
        if (vertexCount == 0)
            break;
        DirectGraphics.SetTexture(texture.itsTexIdx);
        DirectGraphics.RendertoBuffer(GL_TRIANGLES, vertexCount / 3, TilesToRender.data());
    }

    DirectGraphics.SetFilterMode(false);
}

// --------------------------------------------------------------------------------------
// Grenzen checken
// --------------------------------------------------------------------------------------

glm::vec2 TileEngineClass::ClampCameraOrigin(glm::vec2 origin) const {
    constexpr float xtilesize = static_cast<float>(TILESIZE_X);
    constexpr float ytilesize = static_cast<float>(TILESIZE_Y);

    return {
        std::clamp(origin.x, xtilesize, std::max(xtilesize, LEVELPIXELSIZE_X - RENDERWIDTH - xtilesize)),
        std::clamp(origin.y, ytilesize, std::max(ytilesize, LEVELPIXELSIZE_Y - RENDERHEIGHT - ytilesize))
    };
}

void TileEngineClass::CheckBounds() {
    const auto origin = ClampCameraOrigin({XOffset, YOffset});
    XOffset = origin.x;
    YOffset = origin.y;
}

// --------------------------------------------------------------------------------------

void TileEngineClass::WertAngleichen(float &nachx, float &nachy, float vonx, float vony) {
    //	nachx = static_cast<float>(static_cast<int>(vonx));
    //	nachy = static_cast<float>(static_cast<int>(vony));

    //	return;

    float const rangex = std::clamp(vonx - nachx, -50.0f, 50.0f);
    float const rangey = std::clamp(vony - nachy, -60.0f, 60.0f);

    nachx += Timer.sync(rangex * 0.8f);

    /*
    if (NUMPLAYERS == 1 &&
        (FlugsackFliesFree == false ||
         Player[0].Riding() == false)
        &&
        (Player[0].Aktion[AKTION_OBEN] ||
         Player[0].Aktion[AKTION_UNTEN]))
    {
        int a;
        a = 0;
    }
    else */
    if (FlugsackFliesFree == true)
        nachy += Timer.sync(rangey * 0.75f);
}

// --------------------------------------------------------------------------------------
// Level scrollen und Tiles animieren usw
// --------------------------------------------------------------------------------------

void TileEngineClass::UpdateLevel() {
    if (Zustand == TileStateEnum::SCROLLBAR)
        wideBossArena = false;
    if (Console.Showing)
        return;

    // Zeit ablaufen lassen
    if (RunningTutorial == false && Timelimit > 0.0f && !HUD.isBossHUDActive()) {
        if (Skill == SKILL_EASY)
            Timelimit -= Timer.sync(0.05f);
        else if (Skill == SKILL_MEDIUM)
            Timelimit -= Timer.sync(0.075f);
        else
            Timelimit -= Timer.sync(0.1f);
    }

    // Zeit abgelaufen? Dann Punisher erscheinen lassen
    //
    if (Timelimit < 0.0f) {
        if (Player[0].PunisherActive == false && !HUD.isBossHUDActive()) {
            Gegner.PushGegner(0, 0, PUNISHER, 0, 0, false);
            Player[0].PunisherActive = true;
            PartikelSystem.SetThunderColor(0, 0, 0, 255);
        }

        Timelimit = 0.0f;
    }

    XOffset += ScrollSpeedX * Timer.getSpeedFactor();
    YOffset += ScrollSpeedY * Timer.getSpeedFactor();

    // Tiles animieren
    TileAnimCount += Timer.getSpeedFactor();        // Counter erhöhen
    if (TileAnimCount > TILEANIM_SPEED)  // auf Maximum prüfen
                                         // if (TileAnimCount > 0.5f)			// auf Maximum prüfen
    {
        TileAnimCount = 0.0f;    // Counter wieder auf 0 setzen
        TileAnimPhase++;         // und nächste Animphase setzen
        if (TileAnimPhase >= 4)  // Animation wieer von vorne ?
            TileAnimPhase = 0;
    }

    // Sichtbaren Level-Ausschnitt scrollen
    if (Zustand == TileStateEnum::SCROLLTO || Zustand == TileStateEnum::SCROLLTOLOCK) {
        if (XOffset < ScrolltoX) {
            XOffset += Timer.sync(SpeedX);

            if (XOffset > ScrolltoX)
                XOffset = ScrolltoX;
        }

        if (XOffset > ScrolltoX) {
            XOffset -= Timer.sync(SpeedX);

            if (XOffset < ScrolltoX)
                XOffset = ScrolltoX;
        }

        if (YOffset < ScrolltoY) {
            YOffset += Timer.sync(SpeedY);

            if (YOffset > ScrolltoY)
                YOffset = ScrolltoY;
        }

        if (YOffset > ScrolltoY) {
            YOffset -= Timer.sync(SpeedY);

            if (YOffset < ScrolltoY)
                YOffset = ScrolltoY;
        }

        if (XOffset == ScrolltoX && YOffset == ScrolltoY) {
            if (Zustand == TileStateEnum::SCROLLTOLOCK)
                Zustand = TileStateEnum::LOCKED;
            else
                Zustand = TileStateEnum::SCROLLBAR;
        }
    }

    // Level-Ausschnitt zum Spieler Scrollen
    if (Zustand == TileStateEnum::SCROLLTOPLAYER) {
        if (CoopNeedsPartner()) {
            ScrolltoX = (Player[0].xpos + Player[1].xpos) * 0.5f;
            ScrolltoY = (Player[0].ypos + Player[1].ypos) * 0.5f;
        } else {
            const auto &focus = Player[NUMPLAYERS == 2 && Player[0].Handlung == PlayerActionEnum::TOT ? 1 : 0];
            ScrolltoX = focus.xpos;
            ScrolltoY = focus.ypos;
        }

        bool isScrolling = false;
        /*
                if (ScrolltoX - XOffset <  SCROLL_BORDER_LEFT)
                {
                    XOffset -= Timer.sync(SpeedX);
                    isScrolling = true;
                }

                if (ScrolltoX - XOffset >  SCROLL_BORDER_RIGHT)
                {
                    XOffset += Timer.sync(SpeedX);
                    isScrolling = true;
                }
        */
        if (ScrolltoY - YOffset < SCROLL_BORDER_EXTREME_TOP) {
            YOffset -= Timer.sync(SpeedY);
            isScrolling = true;
        }

        if (ScrolltoY - YOffset > SCROLL_BORDER_EXTREME_BOTTOM) {
            YOffset += Timer.sync(SpeedY);
            isScrolling = true;
        }

        if (isScrolling == false)
            Zustand = TileStateEnum::SCROLLBAR;
    }

    // Scrollbar und 2-Spieler Modus? Dann Kamera entsprechend setzen
    if (Zustand == TileStateEnum::SCROLLBAR) {
        float newx, newy;

        if (NUMPLAYERS == 2) {
            if (Player[0].Handlung == PlayerActionEnum::TOT) {
                newx = Player[1].xpos + 35;
                newy = Player[1].ypos;
            } else if (Player[1].Handlung == PlayerActionEnum::TOT) {
                newx = Player[0].xpos + 35;
                newy = Player[0].ypos;
            } else {
                float const xdist = static_cast<float>(Player[1].xpos - Player[0].xpos);
                newx = (Player[0].xpos + 35) + xdist / 2.0f;

                float const ydist = static_cast<float>(Player[1].ypos - Player[0].ypos);
                newy = (Player[0].ypos) + ydist / 2.0f;
            }
        } else {
            newx = Player[0].xpos + 35;
            newy = Player[0].ypos;
        }

        if (newx < 0)
            newx = 0;
        if (newy < 0)
            newy = 0;

        float angleichx = XOffset;
        float angleichy = YOffset;

        // Nach Boss wieder auf Spieler zentrieren?
        //		if (MustCenterPlayer == true)
        {
            bool CenterDone = true;

            if (newx < XOffset + 320.0f - SCROLL_BORDER_HORIZ) {
                CenterDone = false;
                angleichx = newx - 320.0f + SCROLL_BORDER_HORIZ;
            }
            if (newx > XOffset + 320.0f + SCROLL_BORDER_HORIZ) {
                CenterDone = false;
                angleichx = newx - 320.0f + SCROLL_BORDER_HORIZ;
            }
            if (newy < YOffset + 240.0f - SCROLL_BORDER_TOP) {
                CenterDone = false;
                angleichy = newy - 240.0f + SCROLL_BORDER_TOP;
            }
            if (newy > YOffset + 240.0f + SCROLL_BORDER_BOTTOM) {
                CenterDone = false;
                angleichy = newy - 240.0f - SCROLL_BORDER_BOTTOM;
            }

            WertAngleichen(XOffset, YOffset, angleichx, angleichy);

            if (CenterDone == true)
                MustCenterPlayer = false;
        }
        /*
                // oder einfach Kanten checken?
                else
                {
                    if (newx < XOffset + 320.0f - SCROLL_BORDER_HORIZ)	 XOffset = newx - 320.0f + SCROLL_BORDER_HORIZ;
                    if (newx > XOffset + 320.0f + SCROLL_BORDER_HORIZ)	 XOffset = newx - 320.0f - SCROLL_BORDER_HORIZ;
                    if (newy < YOffset + 240.0f - SCROLL_BORDER_TOP)	 YOffset = newy - 240.0f + SCROLL_BORDER_TOP;
                    if (newy > YOffset + 240.0f + SCROLL_BORDER_BOTTOM) YOffset = newy - 240.0f - SCROLL_BORDER_TOP;
                }
                */
    }

    // Wasserfall animieren
    WasserfallOffset += Timer.sync(16.0f);

    // The slower flow runs at half speed, so wrap once both layers repeat.
    while (WasserfallOffset >= waterfallHeight * 2)
        WasserfallOffset -= waterfallHeight * 2;

    /* DKS - Replaced both SinList2 and WaterList lookup tables with new
             class WaterSinTableClass.  See comments in Tileengine.h */
    WaterSinTable.AdvancePosition(Timer.getSpeedFactor());
#if 0
    // Schwabbeln des Levels animieren
    //SinPos   += Timer.sync(3.0f);    //DKS - unused; disabled
    SinPos2  += Timer.sync(2.0f);
    WaterPos += Timer.sync(2.0f);

    //while (SinPos   >= 40.0f) SinPos   -= 40.0f;  //DKS - unused; disabled
    while (SinPos2  >= 40.0f) SinPos2  -= 40.0f;
    while (WaterPos >= 40.0f) WaterPos -= 40.0f;
#endif  // 0

    // Level an vorgegebene Position anpassen
    // weil z.B. der Fahrstuhl selber scrollt?
    if (NewXOffset != -1.0f)
        XOffset = NewXOffset;

    if (NewYOffset != -1.0f)
        YOffset = NewYOffset;

    if (HasFreeCoopCamera() && levelSplitScreenType == SplitScreenType::OneScreen &&
        CoopPlayerReady(Player[0]) && CoopPlayerReady(Player[1])) {
        // Keep a stationary partner on screen as the camera's dead zone follows the leader.
        const float width = DirectGraphics.GetViewWidth();
        const float left = XOffset + CalculateViewLeft();
        const float minX = std::max(Player[0].xpos, Player[1].xpos) + PLAYER_FRAME_WIDTH - width;
        const float maxX = std::min(Player[0].xpos, Player[1].xpos);
        if (minX <= maxX && (left < minX || left > maxX))
            XOffset = std::clamp(left, minX, maxX) - DirectGraphics.InterfaceLeft();
        const float minY = std::max(Player[0].ypos, Player[1].ypos) + PLAYER_FRAME_HEIGHT - RENDERHEIGHT;
        const float maxY = std::min(Player[0].ypos, Player[1].ypos);
        if (minY <= maxY)
            YOffset = std::clamp(YOffset, minY, maxY);
    } else if (CanSplitCoop() && !IsCoopSplit() &&
               Player[0].Handlung != PlayerActionEnum::TOT && Player[1].Handlung != PlayerActionEnum::TOT) {
        // Keep both sprites inside the shared view while the regular camera catches up.
        const float halfWidth = DirectGraphics.GetViewWidth() / 2.0f;
        const float centerX = (Player[0].xpos + Player[1].xpos) / 2.0f + 35.0f - RENDERWIDTH / 2;
        const float centerY = (Player[0].ypos + Player[1].ypos) / 2.0f + 40.0f - RENDERHEIGHT / 2;
        const float slackX = std::max(0.0f, halfWidth - 48.0f - std::abs(Player[0].xpos - Player[1].xpos) / 2.0f);
        const float slackY = std::max(0.0f, RENDERHEIGHT / 2 - 48.0f - std::abs(Player[0].ypos - Player[1].ypos) / 2.0f);
        XOffset = std::clamp(XOffset, centerX - slackX, centerX + slackX);
        YOffset = std::clamp(YOffset, centerY - slackY, centerY + slackY);
    }
}

// --------------------------------------------------------------------------------------
// Die Ränder bei den Overlay Tiles der zerstörbaren Wände
// ausfransen, indem die Alphawerte der angrenzenden Tiles auf 0 gesetzt werden
// damit die Tiles nicht so abgeschnitten aussehen
// --------------------------------------------------------------------------------------
#if 0 // unused
void TileEngineClass::MakeBordersLookCool(int x, int y) {
    /*	if (TileAt(x+1, y+0).Block & BLOCKWERT_DESTRUCTIBLE)
        {
            TileAt(x+1, y+0).ColorOverlay[0] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
                                                            TileAt(x+1, y+0).Green,
                                                            TileAt(x+1, y+0).Blue,
                                                            0);
            TileAt(x+1, y+0).ColorOverlay[2] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
                                                            TileAt(x+1, y+0).Green,
                                                            TileAt(x+1, y+0).Blue,
                                                            0);
        }

        if (TileAt(x+0, y+1).Block & BLOCKWERT_DESTRUCTIBLE)
        {
            TileAt(x+0, y+1).ColorOverlay[0] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
                                                            TileAt(x+1, y+0).Green,
                                                            TileAt(x+1, y+0).Blue,
                                                            0);
            TileAt(x+0, y+1).ColorOverlay[1] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
                                                            TileAt(x+1, y+0).Green,
                                                            TileAt(x+1, y+0).Blue,
                                                            0);
        }

        if (TileAt(x+0, y-1).Block & BLOCKWERT_DESTRUCTIBLE)
        {
            TileAt(x+0, y-1).ColorOverlay[2] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
                                                            TileAt(x+1, y+0).Green,
                                                            TileAt(x+1, y+0).Blue,
                                                            0);
            TileAt(x+0, y-1).ColorOverlay[3] = D3DCOLOR_RGBA(TileAt(x+1, y+0).Red,
                                                            TileAt(x+1, y+0).Green,
                                                            TileAt(x+1, y+0).Blue,
                                                            0);
        }*/
}
#endif
// DKS - After discovering some particles (LONGFUNKE particles emitted when spreadshot projectiles
//      hit walls) get lodged inside wall tiles and stay there (only visible when drawing of
//      overlay tile layers is disabled), I had a look at the Block* functions below.
//      It was clear that they were sometimes accessing the Tiles[][] array out-of-bounds, and
//      I confirmed this through use of a new TileAt() function that does range-checking in
//      debug-mode. Furthermore, the code was messy, returned int type and not uint32_t,
//      and looped for no discernable reason. Also, many of parameters were marked as
//      references, when only a few in any given function were ever modified.
//      I have rewritten them, addressing all these issues. I have left the original functions
//      here as a reference, inside the '#if 0' block. New ones are after.
//      The ResolveLinks() function that was in the middle of these was never used anywhere,
//      and I've commented it out and made no replacement.
#if 0   // BEGIN COPY OF ORIGINAL VERSIONS THAT HAVE BEEN REWRITTEN
// --------------------------------------------------------------------------------------
// Zurückliefern, welche BlockArt sich Rechts vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockRechts(float &x, float &y, float  &xo, float &yo, RECT_struct rect, bool resolve)
{
    // Nach rechts muss nicht gecheckt werden ?
    if (xo > x)
        return 0;


    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int Art	= 0;
    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(static_cast<int>(Timer.sync(xs)))+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l++)
    {
        for(int i=rect.top; i<rect.bottom; i+= TILESIZE_Y)
        {
            xlevel = (unsigned int)((x+rect.right+1) * 0.05f);
            ylevel = (unsigned int)((y+i) * 0.05f);

            if (TileAt(xlevel, ylevel).Block > 0 && !(Art & BLOCKWERT_WAND))
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND)
            {
                if (resolve)
                {
                    x = static_cast<float>(xlevel*TILESIZE_X)-rect.right-1;
                    xo = x;
                }

                return Art;
            }
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, ob sich rechts eine zerstörbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyRechts(float &x, float &y, float  &xo, float &yo, RECT_struct rect)
{
    // Nach rechts muss nicht gecheckt werden ?
    if (xo > x)
        return 0;

    int xlevel;
    int ylevel;
    int laenge;

    if (x < 0 || y < 0 ||
            x > LEVELPIXELSIZE_X ||
            y > LEVELPIXELSIZE_Y)
        return false;

    //laenge = abs(static_cast<int>(Timer.sync(xs)))+1;
    laenge = 5;

    for (int l=0; l<laenge; l++)
    {
        for(int i=rect.top; i<rect.bottom; i+= TILESIZE_Y)
        {
            xlevel = (unsigned int)((x+rect.right+1) * 0.05f);
            ylevel = (unsigned int)((y+i) * 0.05f);

            if (TileAt(xlevel, ylevel).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(xlevel, ylevel);
                return true;
            }
        }
    }

    return false;
}

//DKS - Never used anywhere, so I've not made a replacement version:
void TileEngineClass::ResolveLinks (float &x, float &y, float &xo, float &yo, RECT_struct rect)
{
    float laenge;
//	float xr, yr;
    Vector2D ppos;
    Vector2D pdir;
//	Vector2D lpos;
//	Vector2D ldir;

    for (laenge = y + rect.top; laenge < y + rect.bottom; laenge += TILESIZE_Y)
    {
        ppos.x = x;
        ppos.y = laenge;
        pdir.x = x - xo;
        pdir.y = y - yo;


        D3DXVECTOR2 p1, p2;
        p1.x = ppos.x;
        p1.y = ppos.y;
        p2.x = p1.x + pdir.x;
        p2.y = p1.x + pdir.x;
        RenderLine(p1, p2, 0xFFFFFFFF);
    }
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche BlockArt sich Links vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockLinks(float &x, float &y, float &xo, float &yo, RECT_struct rect, bool resolve)
{
    // Nach links muss nicht gecheckt werden ?
    if (xo < x ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int Art	= 0;
    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(static_cast<int>(Timer.sync(xs)))+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l++)
    {
        for(int i=rect.top; i<rect.bottom; i+=TILESIZE_Y)
        {
            xlevel = (unsigned int)((x+rect.left-1) * 0.05f);
            ylevel = (unsigned int)((y+i) * 0.05f);

            if (TileAt(xlevel, ylevel).Block > 0 && !(Art & BLOCKWERT_WAND))
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND)
            {
                if (resolve)
                {
                    x = static_cast<float>(xlevel*TILESIZE_X) + TILESIZE_X - rect.left;
                    xo = x;
                }
                return Art;
            }
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, ob sich links eine zerstörbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyLinks(float &x, float &y, float &xo, float &yo, RECT_struct rect)
{
    // Nach links muss nicht gecheckt werden ?
    if (xo < x ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(static_cast<int>(Timer.sync(xs)))+1;
    laenge = 5;

    for (int l=0; l<laenge; l++)
    {
        for(int i=rect.top; i<rect.bottom; i+=TILESIZE_Y)
        {
            xlevel = (unsigned int)((x+rect.left-1) * 0.05f);
            ylevel = (unsigned int)((y+i) * 0.05f);

            if (TileAt(xlevel, ylevel).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(xlevel, ylevel);
                return true;
            }
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche BlockArt sich oberhalb vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockOben(float &x, float &y, float &xo, float &yo, RECT_struct rect, bool resolve)
{
    // Nach oben muss nicht gecheckt werden ?
    if (yo < y ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int Art	= 0;
    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(static_cast<int>(yo - y))+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l++)
    {
        for(int i=rect.left; i<rect.right; i+=TILESIZE_X)
        {
            xlevel = (unsigned int)((x+i)   * 0.05f);
            ylevel = (unsigned int)((y+rect.top-l-1) * 0.05f);

            if (TileAt(xlevel, ylevel).Block > 0 && !(Art & BLOCKWERT_WAND))
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND)
            {
                if (resolve)
                {
                    y  = static_cast<float>(ylevel*TILESIZE_Y + 21 - rect.top);
                    yo = y;
                }

                return Art;
            }
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, ob sich oben eine zerstörbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyOben(float &x, float &y, float &xo, float &yo, RECT_struct rect)
{
    // Nach oben muss nicht gecheckt werden ?
    if (yo < y ||
            x  < 0 ||
            y  < 0)
        return 0;

    if (x < 0 || y < 0 ||
            x > LEVELPIXELSIZE_X ||
            y > LEVELPIXELSIZE_Y)
        return false;

    int xlevel;
    int ylevel;
    int laenge;

    //laenge = abs(static_cast<int>(yo - y))+1;
    laenge = 5;

    for (int l=0; l<laenge; l++)
    {
        for(int i=rect.left; i<rect.right; i+=TILESIZE_X)
        {
            xlevel = (unsigned int)((x+i)   * 0.05f);
            ylevel = (unsigned int)((y+rect.top-l-1) * 0.05f);

            if (TileAt(xlevel, ylevel).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(xlevel, ylevel);
                return true;
            }
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche BlockArt sich unterhalb vom übergebenen Rect befindet
// und dabei nicht "begradigen" sprich die y-Position an das Tile angleichen
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockUntenNormal(float &x, float &y, float &xo, float &yo, RECT_struct rect)
{
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;

    int Art	= 0;
    int xlevel;
    int ylevel;
    int laenge;

    laenge = static_cast<int>(y - yo)+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l+=20)
    {
        for(int i=rect.left+1; i<rect.right-1; i++)
        {
            xlevel = (unsigned int)((x+i) * 0.05f);
            ylevel = (unsigned int)((y+rect.bottom+l+1) * 0.05f);

            if (TileAt(xlevel, ylevel).Block > 0 && !(Art & BLOCKWERT_WAND))
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND ||
                    Art & BLOCKWERT_PLATTFORM)
                return Art;
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche BlockArt sich unterhalb vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

int	TileEngineClass::BlockUnten(float &x, float &y, float &xo, float &yo, RECT_struct rect, bool resolve)
{
    // Nach unten muss nicht gecheckt werden ?
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y ||
            x  < 0 ||
            y  < 0)
        return 0;

    // Ausserhalb des Arrays ?
    if (x < 0 ||
            x > LEVELPIXELSIZE_X ||
            y < 0 ||
            y > LEVELPIXELSIZE_Y)
        return 0;


    int  Art	= 0;
    int  xlevel;
    int  ylevel;
    int  laenge;
    //bool checkagain = true;

    //laenge = static_cast<int>(y - yo)+1;
    laenge = 5;

    for (int l=0; l<laenge && Art == 0; l+=20)
    {
        for(int i=rect.left+1; i<rect.right-1; i++)
        {
            xlevel = (unsigned int)((x+i) * 0.05f);
            ylevel = (unsigned int)((y+rect.bottom+l+1) * 0.05f);

            if (!(Art & BLOCKWERT_WAND) &&
                    !(Art & BLOCKWERT_PLATTFORM) &&
                    TileAt(xlevel, ylevel).Block > 0)
                Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_WAND ||
                    Art & BLOCKWERT_PLATTFORM)
            {
//				while ((TileAt(xlevel, ylevel - 1).Block & BLOCKWERT_WAND) &&
//					    yLevel > 0)
//					ylevel--;

                if (resolve == true)
                {
                    // resolven
                    y = static_cast<float>(ylevel*TILESIZE_Y-rect.bottom);
                    yo = y;
                }

                return Art;
            }
        }
    }

    return Art;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche BlockArt sich unterhalb vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyUnten(float &x, float &y, float &xo, float &yo, RECT_struct rect)
{
    // Nach unten muss nicht gecheckt werden ?
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y ||
            x  < 0 ||
            y  < 0)
        return 0;

    if (x < 0 || y < 0 ||
            x > LEVELPIXELSIZE_X ||
            y > LEVELPIXELSIZE_Y)
        return false;

    int  xlevel;
    int  ylevel;
    int  laenge;
    //bool checkagain = true;

    //laenge = static_cast<int>(y - yo)+1;
    laenge = 5;

    // so lange ausführen, bis kein Block mehr unter den Füßen ist
    //do
    for (int l=0; l<laenge; l++)
    {
        for(int i=rect.left+1; i<rect.right-1; i+=TILESIZE_X)
        {
            xlevel = (unsigned int)((x+i) * 0.05f);
            ylevel = (unsigned int)((y+rect.bottom+l+1) * 0.05f);

            if (TileAt(xlevel, ylevel).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(xlevel, ylevel);
                return true;
            }
        }
    }

    return false;
}
#endif  // 0
// DKS - BEGIN REWRITTEN VERSIONS OF ABOVE FUNCTIONS:
namespace {
struct TileSpan {
    int begin;
    int end;
};

TileSpan TilesOverlapping(float begin, float end, int tileSize, int levelSize) {
    if (begin >= end)
        return {0, 0};
    return {std::clamp(static_cast<int>(std::floor(begin / tileSize)), 0, levelSize),
            std::clamp(static_cast<int>(std::ceil(end / tileSize)), 0, levelSize)};
}
}

bool TileEngineClass::OverlapsSolidWall(float x, float y, const RECT_struct &rect) const {
    const auto columns = TilesOverlapping(x + rect.left, x + rect.right, TILESIZE_X, LEVELSIZE_X);
    const auto rows = TilesOverlapping(y + rect.top, y + rect.bottom, TILESIZE_Y, LEVELSIZE_Y);
    for (int column = columns.begin; column < columns.end; ++column)
        for (int row = rows.begin; row < rows.end; ++row)
            if (Tiles[column][row].Block & BLOCKWERT_WAND)
                return true;
    return false;
}

float TileEngineClass::ClipWallMotion(float from, float to, float perpendicular,
                                      const RECT_struct &rect, bool horizontal, uint32_t blocking) const {
    if (from == to)
        return to;

    const bool forward = to > from;
    const int edge = horizontal ? (forward ? rect.right : rect.left) : (forward ? rect.bottom : rect.top);
    const float oldEdge = from + edge;
    const float newEdge = to + edge;
    const int stepSize = horizontal ? TILESIZE_X : TILESIZE_Y;
    const auto path = TilesOverlapping(std::min(oldEdge, newEdge), std::max(oldEdge, newEdge),
                                        stepSize, horizontal ? LEVELSIZE_X : LEVELSIZE_Y);
    const auto side = horizontal
        ? TilesOverlapping(perpendicular + rect.top, perpendicular + rect.bottom, TILESIZE_Y, LEVELSIZE_Y)
        : TilesOverlapping(perpendicular + rect.left, perpendicular + rect.right, TILESIZE_X, LEVELSIZE_X);
    const int direction = forward ? 1 : -1;
    const int stop = forward ? path.end : path.begin - 1;
    for (int tile = forward ? path.begin : path.end - 1; tile != stop; tile += direction) {
        const int wallEdge = (forward ? tile : tile + 1) * stepSize;
        // Only block boundaries crossed by the leading edge during this step.
        if ((wallEdge - oldEdge) * direction < 0.0f)
            continue;
        for (int other = side.begin; other < side.end; ++other)
            if (Tiles[horizontal ? tile : other][horizontal ? other : tile].Block & blocking)
                return static_cast<float>(wallEdge - edge);
    }
    return to;
}

float TileEngineClass::ClipVerticalMotion(float from, float to, float x, const RECT_struct &rect) const {
    if (to <= from)
        return ClipWallMotion(from, to, x, rect, false);

    float result = ClipWallMotion(from, to, x, rect, false, BLOCKWERT_WAND | BLOCKWERT_PLATTFORM);
    const float left = x + rect.left;
    const float right = x + rect.right;
    const auto columns = TilesOverlapping(left, right, TILESIZE_X, LEVELSIZE_X);
    const auto rows = TilesOverlapping(from + rect.bottom + 1.0f, result + rect.bottom + 1.0f,
                                      TILESIZE_Y, LEVELSIZE_Y);
    for (int row = rows.begin; row < rows.end; ++row) {
        for (int col = columns.begin; col < columns.end; ++col) {
            const auto block = Tiles[col][row].Block;
            const float tileLeft = static_cast<float>(col * TILESIZE_X);
            const float tileRight = tileLeft + TILESIZE_X;
            float rise;
            if (block & BLOCKWERT_SCHRAEGE_L)
                rise = std::max(left, tileLeft) - tileLeft;
            else if (block & BLOCKWERT_SCHRAEGE_R)
                rise = tileRight - std::min(right, tileRight);
            else
                continue;
            const float surface = row * TILESIZE_Y + rise - rect.bottom - 1.0f;
            if (surface >= from && surface < result)
                result = surface;
        }
    }
    return result;
}

float TileEngineClass::ResolveFloorOverlap(float x, float y, const RECT_struct &rect) const {
    // An authored enemy can start with its feet inside the top of a solid tile.
    const float floorY = std::floor((y + rect.bottom) / TILESIZE_Y) * TILESIZE_Y - rect.bottom;
    if (ClipWallMotion(floorY, y, x, rect, false) < y && !OverlapsSolidWall(x, floorY, rect))
        return floorY;
    return y;
}

uint32_t TileEngineClass::GroundContact(float x, float y, const RECT_struct &rect) const {
    const float feet = y + rect.bottom;
    const int row = static_cast<int>(std::floor(feet / TILESIZE_Y));
    if (row < 0 || row >= LEVELSIZE_Y || feet != row * TILESIZE_Y)
        return 0;

    const auto columns = TilesOverlapping(x + rect.left, x + rect.right, TILESIZE_X, LEVELSIZE_X);
    uint32_t blocks = 0;
    for (int column = columns.begin; column < columns.end; ++column)
        blocks |= Tiles[column][row].Block & (BLOCKWERT_WAND | BLOCKWERT_PLATTFORM | BLOCKWERT_GEGNERWAND);
    return blocks;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche BlockArt sich Rechts vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockRechts(float &x, float y, float &xo, float yo, RECT_struct rect, bool resolve) {
    // Nach rechts muss nicht gecheckt werden ?
    if (xo > x)
        return 0;

    int xlev = static_cast<int>((x + rect.right + 1) * (1.0f / TILESIZE_X));
    if (xlev < 0 || xlev >= LEVELSIZE_X)
        return 0;

    uint32_t block = 0;

    const auto rows = TilesOverlapping(y + rect.top, y + rect.bottom, TILESIZE_Y, LEVELSIZE_Y);
    for (int ylev = rows.begin; ylev < rows.end; ++ylev) {
        bool blockWand = block & BLOCKWERT_WAND;

        if (!blockWand) {
            uint32_t const newBlock = TileAt(xlev, ylev).Block;
            if (newBlock != 0) {
                block = newBlock;
                blockWand = block & BLOCKWERT_WAND;
            }
        }

        if (blockWand) {
            if (resolve) {
                x = static_cast<float>((xlev * TILESIZE_X) - rect.right - 1);
                xo = x;
            }

            return block;
        }
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, ob sich rechts eine zerstörbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyRechts(float x, float y, float xo, float yo, RECT_struct rect) {
    // Nach rechts muss nicht gecheckt werden ?
    if (xo > x)
        return false;

    int xlev = static_cast<int>((x + rect.right + 1) * (1.0f / TILESIZE_X));
    if (xlev < 0 || xlev >= LEVELSIZE_X)
        return false;

    for (int j = rect.top; j < rect.bottom; j += TILESIZE_Y) {
        int ylev = static_cast<int>((y + j) * (1.0f / TILESIZE_Y));

        if (ylev < 0)
            continue;
        else if (ylev >= LEVELSIZE_Y)
            break;

        if (TileAt(xlev, ylev).Block & BLOCKWERT_DESTRUCTIBLE) {
            ExplodeWall(xlev, ylev);
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche BlockArt sich Links vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockLinks(float &x, float y, float &xo, float yo, RECT_struct rect, bool resolve) {
    // Nach links muss nicht gecheckt werden ?
    if (xo < x)
        return 0;

    int xlev = static_cast<int>((x + rect.left - 1) * (1.0f / TILESIZE_X));
    if (xlev < 0 || xlev >= LEVELSIZE_X)
        return 0;

    uint32_t block = 0;

    const auto rows = TilesOverlapping(y + rect.top, y + rect.bottom, TILESIZE_Y, LEVELSIZE_Y);
    for (int ylev = rows.begin; ylev < rows.end; ++ylev) {
        bool blockWand = block & BLOCKWERT_WAND;

        if (!blockWand) {
            uint32_t const newBlock = TileAt(xlev, ylev).Block;
            if (newBlock != 0) {
                block = newBlock;
                blockWand = block & BLOCKWERT_WAND;
            }
        }

        if (blockWand) {
            if (resolve) {
                x = static_cast<float>((xlev * TILESIZE_X) + TILESIZE_X - rect.left);
                xo = x;
            }
            return block;
        }
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, ob sich links eine zerstörbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyLinks(float x, float y, float xo, float yo, RECT_struct rect) {
    // Nach links muss nicht gecheckt werden ?
    if (xo < x)
        return false;

    int xlev = static_cast<int>((x + rect.left - 1) * (1.0f / TILESIZE_X));
    if (xlev < 0 || xlev >= LEVELSIZE_X)
        return false;

    for (int j = rect.top; j < rect.bottom; j += TILESIZE_Y) {
        int ylev = static_cast<int>((y + j) * (1.0f / TILESIZE_Y));

        if (ylev < 0)
            continue;
        else if (ylev >= LEVELSIZE_Y)
            break;

        if (TileAt(xlev, ylev).Block & BLOCKWERT_DESTRUCTIBLE) {
            ExplodeWall(xlev, ylev);
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche Blockblock sich oberhalb vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockOben(float x, float &y, float xo, float &yo, RECT_struct rect, bool resolve) {
    // Nach oben muss nicht gecheckt werden ?
    if (yo < y)
        return 0;

    int ylev = static_cast<int>((y + rect.top - 1) * (1.0f / TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return 0;

    uint32_t block = 0;

    const auto columns = TilesOverlapping(x + rect.left, x + rect.right, TILESIZE_X, LEVELSIZE_X);
    for (int xlev = columns.begin; xlev < columns.end; ++xlev) {
        bool blockWand = block & BLOCKWERT_WAND;

        if (!blockWand) {
            uint32_t const newBlock = TileAt(xlev, ylev).Block;
            if (newBlock > 0) {
                block = newBlock;
                blockWand = block & BLOCKWERT_WAND;
            }
        }

        if (blockWand) {
            if (resolve) {
                y = static_cast<float>((ylev * TILESIZE_Y) + TILESIZE_Y - rect.top);
                yo = y;
            }

            return block;
        }
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, ob sich oben eine zerstörbare Wand befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyOben(float x, float y, float xo, float yo, RECT_struct rect) {
    // Nach oben muss nicht gecheckt werden ?
    if (yo < y)
        return false;

    int ylev = static_cast<int>((y + rect.top - 1) * (1.0f / TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return false;

    for (int i = rect.left; i < rect.right; i += TILESIZE_X) {
        int xlev = static_cast<int>((x + i) * (1.0f / TILESIZE_X));

        if (xlev < 0)
            continue;
        else if (xlev >= LEVELSIZE_X)
            break;

        if (TileAt(xlev, ylev).Block & BLOCKWERT_DESTRUCTIBLE) {
            ExplodeWall(xlev, ylev);
            return true;
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche Blockblock sich unterhalb vom übergebenen Rect befindet
// und dabei nicht "begradigen" sprich die y-Position an das Tile angleichen
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockUntenNormal(float x, float y, float xo, float yo, RECT_struct rect) {
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y)
        return false;

    int ylev = static_cast<int>((y + rect.bottom + 1) * (1.0f / TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return 0;

    uint32_t block = 0;

    // BIG TODO: see if you can make this increment by TILESIZE_X
    for (int i = rect.left; i < rect.right; i++) {
        int xlev = static_cast<int>((x + i) * (1.0f / TILESIZE_X));

        if (xlev < 0)
            continue;
        else if (xlev >= LEVELSIZE_X)
            break;

        bool blockWand = block & BLOCKWERT_WAND;

        if (!blockWand) {
            uint32_t const newBlock = TileAt(xlev, ylev).Block;
            if (newBlock > 0) {
                block = newBlock;
                blockWand = block & BLOCKWERT_WAND;
            }
        }

        if (blockWand || block & BLOCKWERT_PLATTFORM)
            return block;
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche Blockblock sich unterhalb vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

uint32_t TileEngineClass::BlockUnten(float x, float &y, float xo, float &yo, RECT_struct rect, bool resolve) {
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y)
        return 0;

    int ylev = static_cast<int>((y + rect.bottom + 1) * (1.0f / TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return 0;

    uint32_t block = 0;

    // BIG TODO: see if you can make this increment by TILESIZE_X
    for (int i = rect.left; i < rect.right; i++) {
        int xlev = static_cast<int>((x + i) * (1.0f / TILESIZE_X));

        if (xlev < 0)
            continue;
        else if (xlev >= LEVELSIZE_X)
            break;

        bool blockWand = block & BLOCKWERT_WAND;
        bool blockPlatform = block & BLOCKWERT_PLATTFORM;

        if (!blockWand && !blockPlatform) {
            uint32_t const newBlock = TileAt(xlev, ylev).Block;
            if (newBlock > 0) {
                block = newBlock;
                blockWand = block & BLOCKWERT_WAND;
                blockPlatform = block & BLOCKWERT_PLATTFORM;
            }
        }

        if (blockWand || blockPlatform) {
            const float contactY = static_cast<float>(ylev * TILESIZE_Y - rect.bottom);
            if (resolve && !OverlapsSolidWall(x, contactY, rect)) {
                y = contactY;
                yo = y;
            }

            return block;
        }
    }

    return block;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welche Blockblock sich unterhalb vom übergebenen Rect befindet
// --------------------------------------------------------------------------------------

bool TileEngineClass::BlockDestroyUnten(float x, float y, float xo, float yo, RECT_struct rect) {
    // Nach unten muss nicht gecheckt werden ?
    if (yo > y)
        return 0;

    int ylev = static_cast<int>((y + rect.bottom + 1) * (1.0f / TILESIZE_Y));
    if (ylev < 0 || ylev >= LEVELSIZE_Y)
        return false;

    for (int i = rect.left; i < rect.right; i += TILESIZE_X) {
        int xlev = static_cast<int>((x + i) * (1.0f / TILESIZE_X));

        if (xlev < 0)
            continue;
        else if (xlev >= LEVELSIZE_X)
            break;

        if (TileAt(xlev, ylev).Block & BLOCKWERT_DESTRUCTIBLE) {
            ExplodeWall(xlev, ylev);
            return true;
        }
    }

    return false;
}
// DKS - END BLOCK OF NEW FUNCTIONS

// --------------------------------------------------------------------------------------
// Auf Schrägen prüfen
// --------------------------------------------------------------------------------------
// DKS - Rewrote this function to not access Tiles[][] array out of bounds, and to be
//      a bit more efficient and clean. It also now only take parameters that it actually
//      needs (xo/yo params were unused) and x parameter didn't need to be a reference.
//      It returns the correct type for Block data as well (uint32_t vs int).
// ORIGINAL CODE:
#if 0
int	TileEngineClass::BlockSlopes(float &x, float &y, float &xo, float &yo, RECT_struct rect, float ySpeed, bool resolve)
{
    int Art	= 0;
    int xlevel;
    int ylevel;
    //int laenge;

    //laenge = static_cast<int>(y - yo)+1;
    //laenge = 5;

    for(int j = rect.bottom; j<rect.bottom+TILESIZE_Y; j++)
    {
        // Schräge links
        // von links anfangen mit der Block-Prüdung
        //
        for(int i = rect.left; i<rect.right; i+=1)
        {
            xlevel = int ((x + i) * 0.05f);
            ylevel = int ((y + j - 1) * 0.05f);

            Art = TileAt(xlevel, ylevel).Block;

            if (Art & BLOCKWERT_SCHRAEGE_L)
            {
                float newy = (ylevel+1) * TILESIZE_Y - rect.bottom - (TILESIZE_Y - float (int (x + i) % TILESIZE_X)) - 1;

                {
                    if (ySpeed == 0.0f ||
                            y > newy)
                    {
                        y = newy;
                        return Art;
                    }
                }
            }
        }

        // Schräge rechts
        // von rechts anfangen mit der Block-Prüdung
        //
        for(int i = rect.right; i>rect.left; i-=1)
        {
            xlevel = int ((x + i) * 0.05f);
            ylevel = int ((y + j - 1) * 0.05f);

            Art = TileAt(xlevel, ylevel).Block;


            if (Art & BLOCKWERT_SCHRAEGE_R)
            {
                float newy = (ylevel+1) * TILESIZE_Y - rect.bottom - (float (int (x + i) % TILESIZE_X)) - 1;

                {
                    if (ySpeed == 0.0f ||
                            y > newy)
                    {
                        y = newy;
                        return Art;
                    }
                }
            }
        }
    }

    return 0;
}
#endif  // 0
// DKS - Rewritten version of above function:
uint32_t TileEngineClass::BlockSlopes(const float x, float &y, const RECT_struct rect, const float ySpeed) {

    for (int j = rect.bottom; j < rect.bottom + TILESIZE_Y; j++) {
        int ylev = static_cast<int>((y + (j - 1)) * (1.0f / TILESIZE_Y));

        if (ylev < 0)
            continue;
        else if (ylev >= LEVELSIZE_Y)
            break;

        // Schräge links
        // von links anfangen mit der Block-Prüdung

        // DKS - TODO see if you can get this to increment faster, by tile:
        for (int i = rect.left; i < rect.right; i++) {
            int xlev = static_cast<int>((x + i) * (1.0f / TILESIZE_X));

            if (xlev < 0)
                continue;
            else if (xlev >= LEVELSIZE_X)
                break;

            uint32_t const block = TileAt(xlev, ylev).Block;

            if (block & BLOCKWERT_SCHRAEGE_L) {
                float newy = static_cast<float>((ylev + 1) * TILESIZE_Y - rect.bottom -
                    (TILESIZE_Y - (static_cast<int>(x + i) % TILESIZE_X)) - 1);
                if (ySpeed == 0.0f || y + 1.0f >= newy) {
                    y = newy;
                    return block;
                }
            }
        }

        // Schräge rechts
        // von rechts anfangen mit der Block-Prüdung

        // DKS TODO: try to get this to decrement faster, by tile:
        for (int i = rect.right; i > rect.left; i--) {
            int xlev = static_cast<int>((x + i) * (1.0f / TILESIZE_X));

            if (xlev >= LEVELSIZE_X)
                continue;
            else if (xlev < 0)
                break;

            uint32_t const block = TileAt(xlev, ylev).Block;

            if (block & BLOCKWERT_SCHRAEGE_R) {
                float newy = static_cast<float>((ylev + 1) * TILESIZE_Y - rect.bottom -
                    (static_cast<int>(x + i) % TILESIZE_X) - 1);
                if (ySpeed == 0.0f || y + 1.0f >= newy) {
                    y = newy;
                    return block;
                }
            }
        }
    }

    return 0;
}

// --------------------------------------------------------------------------------------
// Checken ob ein Schuss eine zerstörbare Wand getroffen hat und wenn ja, diese
// zerstören und true zurückliefern, damit der Schuss ebenfalls gelöscht wird
// --------------------------------------------------------------------------------------
bool TileEngineClass::CheckDestroyableWalls(float x, float y, float xs, float ys, RECT_struct rect)
{
    // Ausserhalb vom Level?
    //
    if (x < 0 || x > LEVELPIXELSIZE_X ||
            y < 0 || y > LEVELPIXELSIZE_Y)
        return false;

    int xstart = static_cast<int>((x + Timer.sync(xs)) * (1.0f / TILESIZE_X));
    int ystart = static_cast<int>((y + Timer.sync(ys)) * (1.0f / TILESIZE_Y));

    int const xl = rect.right / TILESIZE_X + 2;
    int const yl = rect.bottom / TILESIZE_Y + 2;

    // avoid out-of-bounds access to Tiles[][] array.
    xstart = std::clamp(xstart, 1, LEVELSIZE_X - xl);
    ystart = std::clamp(ystart, 0, LEVELSIZE_Y - yl);

    // Check whole rect of the shot
    for (int i=xstart-1; i<xstart + xl; i++) {
        for (int j=ystart; j<ystart + yl; j++) {
            if (TileAt(i, j).Block & BLOCKWERT_DESTRUCTIBLE)
            {
                ExplodeWall(i, j);
                return true;
            }
        }
    }

    return false;
}

// --------------------------------------------------------------------------------------
// Zurückliefern, welcher Farbwert sich in der Mitte des RECTs an x/y im Level befindet
// damit die Gegner entsprechend dem Licht im Editor "beleuchtet" werden
// --------------------------------------------------------------------------------------

D3DCOLOR TileEngineClass::LightValue(float x, float y, RECT_struct rect, bool forced) {

    int const x_level = static_cast<int>((x + (rect.right - rect.left) / 2) / TILESIZE_X);  // xPosition im Level
    int const y_level = static_cast<int>((y + (rect.bottom - rect.top) / 2) / TILESIZE_Y);  // yPosition im Level

    // DKS - Added check for x_level,y_level being in bounds of levels' dimensions, also,
    //      check forced==false before blindly looking up block value:
    // if (!(TileAt(x_level, y_level).Block & BLOCKWERT_LIGHT) &&		// Soll das Leveltile garnicht
    //        forced == false)
    //    return 0xFFFFFFFF;										// das Licht des Objektes ändern
    if ((x_level >= LEVELSIZE_X || y_level >= LEVELSIZE_Y) ||
        (!forced && !(TileAt(x_level, y_level).Block & BLOCKWERT_LIGHT)))  // Soll das Leveltile garnicht
        return 0xFFFFFFFF;                                               // das Licht des Objektes ändern

    const LevelTileStruct& tile = TileAt(x_level, y_level);
    unsigned int r = tile.Red;
    unsigned int g = tile.Green;
    unsigned int b = tile.Blue;

    r += 48;  // Farbewerte ein wenig erhöhen, damit man selbst bei 0,0,0
    g += 48;  // noch ein wenig was sehen kann und das Sprite nicht
    b += 48;  // KOMPLETT schwarz wird ... minimum ist 48, 48, 48

    if (r > 255)
        r = 255;  // Grenzen überschritten ? Dann angleichen
    if (g > 255)
        g = 255;
    if (b > 255)
        b = 255;

    return D3DCOLOR_RGBA(r, g, b, 255);
}

// --------------------------------------------------------------------------------------
// Neue Lichtberechnung
// Jedes Tile hat an allen vier Ecken eine interpolierte Farbe
// entsprechend der umliegenden Tiles -> smoothe Übergänge -> Geilomat!
// --------------------------------------------------------------------------------------

inline void interpolateColor(const LevelTileStruct& centralTile, const LevelTileStruct& otherTile, int& r, int& g, int& b) {
    if (!((otherTile.Block ^ centralTile.Block) & BLOCKWERT_WAND)) {
        r = otherTile.Red;
        g = otherTile.Green;
        b = otherTile.Blue;
    } else {
        r = centralTile.Red;
        g = centralTile.Green;
        b = centralTile.Blue;
    }
}

void TileEngineClass::ComputeCoolLight() {
    // Lichter im Level interpolieren
    // Dabei werden die Leveltiles in 2er Schritten durchgegangen
    // Dann werden die 4 Ecken des aktuellen Tiles auf die Farben der Nachbarfelder gesetzt
    // Farben der Nachbarfelder werden allerdings nur mit verrechnet, wenn es sich nicht um eine massive Wand handelt.
    // In diesem Falle wird die Standard-Tilefarbe verwendet
    //
    for (int i = 1; i < LEVELSIZE_X - 1; i += 1)
        for (int j = 1; j < LEVELSIZE_Y - 1; j += 1) {
            LevelTileStruct& tile = TileAt(i, j);

            int const al = tile.Alpha;

            int const r4 = tile.Red;
            int const g4 = tile.Green;
            int const b4 = tile.Blue;

            int rn, gn, bn, r1, r2, r3, g1, g2, g3, b1, b2, b3;

            // Ecke links oben
            //
            interpolateColor(tile, TileAt(i - 1, j - 1), r1, g1, b1);
            interpolateColor(tile, TileAt(i + 0, j - 1), r2, g2, b2);
            interpolateColor(tile, TileAt(i - 1, j + 0), r3, g3, b3);

            rn = (r1 + r2 + r3 + r4) / 4;
            gn = (g1 + g2 + g3 + g4) / 4;
            bn = (b1 + b2 + b3 + b4) / 4;

            tile.Color[0] = D3DCOLOR_RGBA(rn, gn, bn, al);

            // Ecke rechts oben
            //
            interpolateColor(tile, TileAt(i - 0, j - 1), r1, g1, b1);
            interpolateColor(tile, TileAt(i + 1, j - 1), r2, g2, b2);
            interpolateColor(tile, TileAt(i + 1, j + 0), r3, g3, b3);

            rn = (r1 + r2 + r3 + r4) / 4;
            gn = (g1 + g2 + g3 + g4) / 4;
            bn = (b1 + b2 + b3 + b4) / 4;

            tile.Color[1] = D3DCOLOR_RGBA(rn, gn, bn, al);

            // Ecke links unten
            //
            interpolateColor(tile, TileAt(i - 1, j - 0), r1, g1, b1);
            interpolateColor(tile, TileAt(i - 1, j + 1), r2, g2, b2);
            interpolateColor(tile, TileAt(i - 0, j + 1), r3, g3, b3);

            rn = (r1 + r2 + r3 + r4) / 4;
            gn = (g1 + g2 + g3 + g4) / 4;
            bn = (b1 + b2 + b3 + b4) / 4;

            tile.Color[2] = D3DCOLOR_RGBA(rn, gn, bn, al);

            // Ecke rechts unten
            //
            interpolateColor(tile, TileAt(i + 1, j - 0), r1, g1, b1);
            interpolateColor(tile, TileAt(i - 0, j + 0), r2, g2, b2);
            interpolateColor(tile, TileAt(i + 1, j + 1), r3, g3, b3);

            rn = (r1 + r2 + r3 + r4) / 4;
            gn = (g1 + g2 + g3 + g4) / 4;
            bn = (b1 + b2 + b3 + b4) / 4;

            tile.Color[3] = D3DCOLOR_RGBA(rn, gn, bn, al);
        }

}  // ComputeCoolLight

// --------------------------------------------------------------------------------------
// "Taschenlampen" Ausschnitt im Alien Level rendern
// --------------------------------------------------------------------------------------

void TileEngineClass::DrawShadow(int viewPlayer) {
    if (!bDrawShadow)
        return;

    float x, y;

    if (NUMPLAYERS == 1 || viewPlayer >= 0) {
        const auto &player = Player[NUMPLAYERS == 1 ? 0 : viewPlayer];
        x = player.xpos + 35.0f - 512.0f - XOffset;
        y = player.ypos + 40.0f - 512.0f - YOffset;
    } else {
        x = XOffset + 320.0f;
        y = YOffset + 240.0f;
    }

    D3DCOLOR col = D3DCOLOR_RGBA(255, 255, 255, static_cast<int>(ShadowAlpha));

    Shadow.RenderSprite(x, y, col);
    Shadow.RenderSprite(x + 512, y, 0, col, true);

    Shadow.RenderMirroredSprite(x, y + 512, col, false, true);
    Shadow.RenderMirroredSprite(x + 512, y + 512, col, true, true);

    col = D3DCOLOR_RGBA(0, 0, 0, static_cast<int>(ShadowAlpha));

    // Seitenränder
    const float width = RenderRight - RenderLeft;
    const float top = std::clamp(y, 0.0f, static_cast<float>(RenderHeight));
    const float bottom = std::clamp(y + 1024, 0.0f, static_cast<float>(RenderHeight));
    const float left = std::clamp(x, RenderLeft, RenderRight);
    const float right = std::clamp(x + 1024, RenderLeft, RenderRight);

    RenderRect(RenderLeft, 0, width, top, col);
    RenderRect(RenderLeft, bottom, width, RenderHeight - bottom, col);
    RenderRect(RenderLeft, top, left - RenderLeft, bottom - top, col);
    RenderRect(right, top, RenderRight - right, bottom - top, col);
}

// --------------------------------------------------------------------------------------
// Wand an x/y explodieren lassen
// --------------------------------------------------------------------------------------

void TileEngineClass::ExplodeWall(int x, int y) {
    // ausserhalb des Levels nicht testen ;)
    if (x < 1 || y < 1 || x > (LEVELSIZE_X - 1) || y > (LEVELSIZE_Y - 1))
        return;

    LevelTileStruct& tile = TileAt(x, y);

    // keine zerstörbare Wand?
    //
    if (!(tile.Block & BLOCKWERT_DESTRUCTIBLE))
        return;

    tile.Block = 0;
    tile.FrontArt = 0;

    for (int i = 0; i < 2; i++)
        PartikelSystem.PushPartikel(static_cast<float>(x * TILESIZE_X + GetRandom(10)),
                                    static_cast<float>(y * TILESIZE_Y + GetRandom(10)), ROCKSPLITTERSMALL);

    for (int k = 0; k < 4; k++)
        PartikelSystem.PushPartikel(static_cast<float>(x * TILESIZE_X + 8), static_cast<float>(y * TILESIZE_Y + 8), FUNKE);

    PartikelSystem.PushPartikel(static_cast<float>(x * TILESIZE_X - TILESIZE_X),
                                static_cast<float>(y * TILESIZE_Y - TILESIZE_Y), SMOKEBIG);

    SoundManager.PlayWave(100, 128, 11025 + GetRandom(2000), SOUND::EXPLOSION1);
}

// --------------------------------------------------------------------------------------
// Wand an x/y und angrenzende Wände explodieren lassen
// --------------------------------------------------------------------------------------

void TileEngineClass::ExplodeWalls(int x, int y) {
    // Rundes Loch erzeugen
    //  xxx
    // xxxxx
    // xxxxx
    // xxxxx
    //  xxx

    ExplodeWall(x - 1, y - 2);
    ExplodeWall(x + 0, y - 2);
    ExplodeWall(x + 1, y - 2);

    ExplodeWall(x - 2, y - 1);
    ExplodeWall(x - 1, y - 1);
    ExplodeWall(x + 0, y - 1);
    ExplodeWall(x + 1, y - 1);
    ExplodeWall(x + 2, y - 1);

    ExplodeWall(x - 2, y + 0);
    ExplodeWall(x - 1, y + 0);
    ExplodeWall(x + 0, y + 0);
    ExplodeWall(x + 1, y + 0);
    ExplodeWall(x + 2, y + 0);

    ExplodeWall(x - 2, y + 1);
    ExplodeWall(x - 1, y + 1);
    ExplodeWall(x + 0, y + 1);
    ExplodeWall(x + 1, y + 1);
    ExplodeWall(x + 2, y + 1);

    ExplodeWall(x - 1, y + 2);
    ExplodeWall(x + 0, y + 2);
    ExplodeWall(x + 1, y + 2);
}

void TileEngineClass::ToggleLamp() {
    if (bDrawShadow == false) {
        bDrawShadow = true;
        ShadowAlpha = 255.0f;
    } else
        bDrawShadow = false;
}


// Datei : Gameplay.cpp

// --------------------------------------------------------------------------------------
//
// Beinhaltet den Haupt Game-Loop
//
// (c) 2002 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------------

#include "Gameplay.hpp"
#include <array>
#include <chrono>
#include <filesystem>
#include <type_traits>
#include <limits>
#include <string_view>
#include <vector>
#include <stdio.h>
#include "Console.hpp"
#include "DX8Font.hpp"
#include "DX8Graphics.hpp"
#include "DX8Input.hpp"
#include "DX8Sound.hpp"
#include "DX8Sprite.hpp"
#include "GUISystem.hpp"
#include "Gegner_Helper.hpp"
#include "HUD.hpp"
#include "Logdatei.hpp"
#include "Main.hpp"
#include "Menu.hpp"
#include "Partikelsystem.hpp"
#include "Player.hpp"
#include "Projectiles.hpp"
#include "Tileengine.hpp"
#include "Timer.hpp"

namespace {
struct CoopWait {
    bool waiting = false;
    glm::vec2 minimum{-std::numeric_limits<float>::infinity()};
    glm::vec2 maximum{std::numeric_limits<float>::infinity()};
};

struct LessonCandidate {
    int id = -1;
    float distance = 120.0f * 120.0f;
};
struct TutorialCard {
    int lesson = -1;
    float opacity = 0.0f;
    uint32_t started = 0;

    bool Update(int next, float step) {
        const bool entered = next >= 0 && (lesson != next || opacity == 0.0f);
        if (entered) {
            lesson = next;
            started = SDL_GetTicks();
        }
        opacity = std::clamp(opacity + (next >= 0 ? step : -step), 0.0f, 1.0f);
        if (opacity == 0.0f)
            lesson = -1;
        return entered;
    }
};
std::array<LessonCandidate, 2> nearbyLessons{};
std::array<TutorialCard, 2> playerLessons{};
TutorialCard sharedLesson{};

// World-space effects are sampled independently by each player's camera.
struct CameraShake {
    float amplitude = 0.0f;
    float age = 0.0f;
    float duration = 0.0f;
    float angle = 0.0f;

    float Remaining() const {
        return duration > 0.0f ? amplitude * std::max(0.0f, 1.0f - age / duration) : 0.0f;
    }

    void Strike(float strength, float durationSeconds) {
        if (strength > 0.0f && strength >= Remaining()) {
            amplitude = strength;
            duration = durationSeconds;
            age = 0.0f;
        }
    }

    void Advance(float seconds) {
        age = std::min(duration, age + seconds);
        angle = Remaining() * sin_rad(age * 48.0f);
    }
};
std::array<CameraShake, 2> cameraShakes{};
std::array<float, 2> summaryOpacity{};
int unclaimedBlocks = 0;
std::array<CoopWait, 2> coopWaits{};
bool coopLiftWaiting = false;
bool coopLiftWasWaiting = false;
}

void OfferTutorialLesson(int player, int lesson, float distanceSquared) {
    auto &candidate = nearbyLessons[player];
    if (lesson >= 0 && lesson <= TEXT::TUTORIAL39 - TEXT::TUTORIAL1 && distanceSquared < candidate.distance)
        candidate = {lesson, distanceSquared};
}

bool CoopNeedsPartner() {
    return NUMPLAYERS == 2 && Player[0].Handlung != PlayerActionEnum::TOT &&
           Player[1].Handlung != PlayerActionEnum::TOT;
}

bool CoopPlayerReady(const PlayerClass &player) {
    return player.Handlung != PlayerActionEnum::TOT && player.Handlung != PlayerActionEnum::EXPLODIEREN;
}

void WaitForCoopPartner(int player, CoopGateSide side, float limit) {
    auto &wait = coopWaits[player];
    const auto &leader = Player[player];
    wait.waiting = true;
    switch (side) {
        case CoopGateSide::Left:   wait.minimum.x = std::max(wait.minimum.x, std::min(limit, leader.xpos)); break;
        case CoopGateSide::Right:  wait.maximum.x = std::min(wait.maximum.x, std::max(limit, leader.xpos)); break;
        case CoopGateSide::Top:    wait.minimum.y = std::max(wait.minimum.y, std::min(limit, leader.ypos)); break;
        case CoopGateSide::Bottom: wait.maximum.y = std::min(wait.maximum.y, std::max(limit, leader.ypos)); break;
        case CoopGateSide::None: break;
    }
}

void HoldCoopLiftForBoss() {
    coopLiftWaiting = true;
}

bool CoopLiftWaitingForBoss() {
    return CoopNeedsPartner() && (coopLiftWaiting || coopLiftWasWaiting);
}

// --------------------------------------------------------------------------------------
// Gameplay Variablen
// --------------------------------------------------------------------------------------

int MAX_LEVELS = 0;
int NUMPLAYERS = 1;
float FahrstuhlPos = -1.0f;
float ScreenWinkel = 0.0f;  // in welchem zWinkel steht der Screen grad (für säulen, die das Level zum Kippen bringen)
float WarningCount = 0.0f;  // Counter, ob ein "Warning" angezeigt wird
bool JoystickFound;
bool ShowSummary = false;

long DEMOPress = 0;          // Counter bis zum nächsten Tastendruck
bool DEMORecording = false;  // demo wird grad aufgenommen
bool DEMOPlaying = false;    // demo spielt gradf ab
std::fstream DEMOFile;       // Datei in der das Demo gespeichert wird

bool FlameThrower = false;
bool HasCheated = false;

bool RunningTutorial = false;

constexpr const char* DEMO_ID = "Hurrican Demo File";  // Kennung
// Preserve the original binding blocks and append new bindings after the existing options.
constexpr size_t legacyBindingBytes = AKTION_WAFFEN_PREV * sizeof(int);
constexpr const char* CONFIGFILE = "Hurrican.cfg";  // Name der Konfigurationsdatei

constexpr const char* DEFAULT_LANG_FILE = "english.lng";

std::string StageReihenfolge[256];

int options_Detail;
bool StopStageMusicAtStart;
bool FlugsackFliesFree;

int DisplayHintNr = -1;

const int TextureCount[10] = {
  117, // Tutorial
  106, // Stages
  109,
   87,
   82,
  130,
  118,
  135,
   88,
  127
};

// --------------------------------------------------------------------------------------
// Ein neues Spiel initialisieren
// --------------------------------------------------------------------------------------

void InitNewGame() {
    FlameThrower = false;
    HasCheated = false;

    int NumTextures = 0;
    for (int p = 0; p < NUMPLAYERS; p++) {
        if (!Player[p].SpritesLoaded)
            NumTextures += 28;
    }

    pMenu->StartProgressBar(NumTextures);

    for (int p = 0; p < NUMPLAYERS; p++) {
        Player[p].Handlung = PlayerActionEnum::STEHEN;
        Player[p].InitPlayer(p);  // DKS: InitPlayer now takes argument specifying which
                                  //      player number each player is. InitPlayer will
                                  //      also load each player's set of sprites if they
                                  //      haven't been already.
        Player[p].InitNewLevel();
        Player[p].SecretFullGame = 0;
        Player[p].DiamondsFullGame = 0;
        Player[p].LivesFullGame = 0;
        Player[p].BlocksFullGame = 0;

        // nur im Tutorial Level
        if (RunningTutorial)
            Player[p].GodMode = true;
    }

    HUD.deactivateBossHUD();
}

// --------------------------------------------------------------------------------------
// Ein neues Level initialisieren und Level "Stage'Nr'.map" laden
// --------------------------------------------------------------------------------------

void InitNewGameLevel() {
    nearbyLessons = {};
    playerLessons = {};
    sharedLesson = {};
    InvalidateTutorialText();
    cameraShakes = {};
    summaryOpacity = {};
    unclaimedBlocks = 0;
    coopWaits = {};
    coopLiftWaiting = coopLiftWasWaiting = false;
    SoundManager.StopSong(MUSIC::STAGEMUSIC, false);
    // DKS - Might as well stop any boss music too:
    SoundManager.StopSong(MUSIC::BOSS, false);

    WarningCount = 0.0f;
    ScreenWinkel = 0.0f;

    for (int p = 0; p < NUMPLAYERS; p++) {
        Player[p].ExplodingTimer = 0.0f;
        Player[p].InitNewLevel();
    }

    // Externes Level aus Command Line laden ?
    //
    int NumTextures = 75; // unknown, use a default
    std::string Name;

    if (RunningTutorial) {
        Name = "tutorial.map";
        NumTextures = TextureCount[0];
    } else if (CommandLineParams.RunUserLevel) {
        Name = CommandLineParams.UserLevelName;
    } else {
        Name = StageReihenfolge[Stage - 1];
        NumTextures = TextureCount[Stage];
    }

    pMenu->StartProgressBar(NumTextures);

    // und Level endlich laden
    if (!TileEngine.LoadLevel(Name)) {
        SpielZustand = GameStateEnum::MAINMENU;
        pMenu->AktuellerZustand = MENUPUNKT_STARTGAME;
        Stage = -1;
        NewStage = -1;
        return;
    }

    // Songs mit Namen aus dem Levelfile Laden
    SoundManager.LoadSong(TileEngine.GetSong(0), MUSIC::STAGEMUSIC);
    SoundManager.LoadSong(TileEngine.GetSong(1), MUSIC::BOSS);

    // DKS - Renamed
    // SoundManager.ResetAllSongVolumes();
    SoundManager.ResetAllSoundVolumes();

    pMenu->ResetProgressBar();
    DisplayLoadInfo("");
    Timer.wait(1000);
    Timer.update();

    // Menu Musik ausfaden
    // DKS - Was already commented out:
    // SoundManager.FadeSong (MUSIC::LOADING, -10.0f, 0, false);

    // DKS - Added function SongIsPlaying() to SoundManagerClass:
    if (SoundManager.SongIsPlaying(MUSIC::MENU)) {
        SoundManager.FadeSong(MUSIC::MENU, -10.0f, 0, false);

        // DKS - FadeSong above will stop the song once fade is complete, with its false parameter (last one)
        // while (SoundManager.its_Songs[MUSIC::MENU]->FadingVolume != 0.0f)
        while (SoundManager.SongIsPlaying(MUSIC::MENU)) {
            // Timer updaten
            Timer.update();
            Timer.wait();

            SoundManager.Update();
        }
    }

    // Level-Musik abspielen
    if (!StopStageMusicAtStart)
        SoundManager.PlaySong(MUSIC::STAGEMUSIC, false);

    FahrstuhlPos = -1.0f;

    SpielZustand = GameStateEnum::GAMELOOP;
}

// --------------------------------------------------------------------------------------
// Game Over Schriftzug anzeigen
// --------------------------------------------------------------------------------------

void ShowGameOver() {
    // DKS - This is not needed, and SDLPort/SDL_fmod.cpp now supports specifying if a song
    //      is to be played looped in a new parameter to PlaySong(). This was causing
    //      game-over music to never be heard in the SDL port.
    //// Game Over Musik anhalten wenn sie beendet wurde
    // if (MUSIC::IsFinished(SoundManager.its_Songs[MUSIC::GAMEOVER]->SongData))
    //    SoundManager.StopSong(MUSIC::GAMEOVER, false);

    // Transparent Wert des Game Over Schriftzuges bestimmen
    int col = static_cast<int>((50.0f - Player[0].GameOverTimer) * 10.0f);
    if (col > 255)  // Obergrenze checken
        col = 255;

    TileEngine.GameOver.RenderSprite((RENDERWIDTH - 400) * 0.5f, (RENDERHEIGHT - 90) * 0.5f, D3DCOLOR_RGBA(255, 255, 255, col));

    Player[0].GameOverTimer -= Timer.sync(0.75f);

    // GameOver vorbei ?
    // Dann schön alle löschen und auf ein neues Spiel vorbereiten
    if (Player[0].GameOverTimer <= 0.0f) {
        Player[0].GameOverTimer = 0.0f;
        pMenu->CheckForNewHighscore();

        Console.Hide();
    }
}

// --------------------------------------------------------------------------------------
// Haupt-Spielablauf
// --------------------------------------------------------------------------------------

namespace {

// Apply the camera after all local sprite transforms, including the parallax and tile meshes.
glm::mat4 ApplyWorldTilt(float left, glm::vec2 size, float degrees) {
    if (degrees == 0.0f)
        return glm::mat4(1.0f);

    const float aspect = std::max(size.x / size.y, size.y / size.x);
    // A newly opening pane can be only one pixel wide. Keep its edge travel bounded.
    const float angle = glm::radians(degrees * std::min(1.0f, 2.0f / aspect));
    const glm::vec3 center(left + size.x * 0.5f, size.y * 0.5f, 0.0f);
    auto tilt = glm::translate(glm::mat4(1.0f), center);
    tilt = glm::rotate(tilt, angle, glm::vec3(0, 0, 1));
    // |cos| + aspect * |sin| fits all four inverse-rotated corners inside the rendered view.
    const float cover = std::abs(tilt[0][0]) + aspect * std::abs(tilt[0][1]);
    tilt = glm::scale(tilt, glm::vec3(cover, cover, 1.0f));
    tilt = glm::translate(tilt, -center);

    matProj *= tilt;
#if defined(USE_GL1)
    load_matrix(GL_PROJECTION, glm::value_ptr(matProj));
#endif
    return tilt;
}

void DrawLevelActors(bool secondView = false) {
    TileEngine.DrawBackLevel();
    TileEngine.DrawFrontLevel();
    Gegner.RenderAll(secondView);

    for (int p = 0; p < NUMPLAYERS; ++p) {
        auto &player = Player[p];
        player.AlreadyDrawn = false;
        player.DrawPlayer(false, false, !secondView);

        if (player.BlinkCounter > 0.0f) {
            if (player.BlinkCounter < 4.0f) {
                const int a = static_cast<int>(player.BlinkCounter * 63.0f);
                if (player.BlinkColor == 1)
                    player.CurrentColor = D3DCOLOR_RGBA(255, 64, 64, a);
                else if (player.BlinkColor == 2)
                    player.CurrentColor = D3DCOLOR_RGBA(64, 64, 255, a);
                else if (player.BlinkColor == 3)
                    player.CurrentColor = D3DCOLOR_RGBA(64, 255, 64, a);
                else if (player.BlinkColor == 4)
                    player.CurrentColor = D3DCOLOR_RGBA(128, 192, 255, a);

                player.DrawPlayer(true, true, !secondView);
                player.DrawPlayer(true, true, !secondView);
                player.DrawPlayer(false, true, !secondView);
            }
        } else if (player.WasDamaged) {
            player.DrawPlayer(true, false, !secondView);
        }
    }
}

void DrawLevelForeground(int viewPlayer = -1) {
    DirectGraphics.SetColorKeyMode();
    TileEngine.DrawWater();
    TileEngine.DrawBackLevelOverlay();
    TileEngine.DrawOverlayLevel();
    TileEngine.DrawShadow(viewPlayer);
}

struct CoopDivider {
    glm::vec2 normal;
    float offset;

    float Distance(const glm::vec2 &point) const {
        return glm::dot(normal, point) - offset;
    }
};

struct CoopPane {
    SDL_Rect viewport{};
    glm::vec2 center{0.0f};
    // A straight cut across a rectangle can produce up to five vertices.
    std::array<glm::vec2, 5> outline{};
    int vertices = 0;
};

CoopPane MakeCoopPane(int width, const CoopDivider &divider, float top = 0.0f) {
    CoopPane pane;
    const float right = static_cast<float>(width);
    constexpr float bottom = RENDERHEIGHT;
    const glm::vec2 corners[] = {{0.0f, top}, {right, top}, {right, bottom}, {0.0f, bottom}};
    for (int i = 0; i < 4; ++i) {
        const auto &a = corners[i];
        const auto &b = corners[(i + 1) % 4];
        const float distanceA = divider.Distance(a);
        const float distanceB = divider.Distance(b);
        if (distanceA <= 0.0f)
            pane.outline[pane.vertices++] = a;
        if ((distanceA < 0.0f && distanceB > 0.0f) || (distanceA > 0.0f && distanceB < 0.0f))
            pane.outline[pane.vertices++] = glm::mix(a, b, distanceA / (distanceA - distanceB));
    }
    if (pane.vertices < 3)
        return pane;

    glm::vec2 minPos(right, bottom), maxPos(0.0f);
    for (int i = 0; i < pane.vertices; ++i) {
        minPos = glm::min(minPos, pane.outline[i]);
        maxPos = glm::max(maxPos, pane.outline[i]);
    }
    const int x = static_cast<int>(std::floor(minPos.x));
    const int y = static_cast<int>(std::floor(minPos.y));
    pane.viewport = {x, y, static_cast<int>(std::ceil(maxPos.x)) - x,
                           static_cast<int>(std::ceil(maxPos.y)) - y};

    const glm::vec2 origin = pane.outline[0];
    float totalArea = 0.0f;
    for (int i = 1; i + 1 < pane.vertices; ++i) {
        const auto a = pane.outline[i] - origin;
        const auto b = pane.outline[i + 1] - origin;
        const float area = a.x * b.y - a.y * b.x;
        pane.center += area * (a + b) / 3.0f;
        totalArea += area;
    }
    pane.center = totalArea > 0.0f ? origin + pane.center / totalArea : (minPos + maxPos) / 2.0f;
    return pane;
}

constexpr int captionSpacing = 0;

// Wrap the localized line at word boundaries, splitting a long word only if necessary.
std::vector<std::string> WrapIdleMessage(std::string_view text, int limit) {
    auto &font = *pDefaultFont;
    std::vector<std::string> lines;
    size_t begin = text.find_first_not_of(" \r\n\t");
    while (begin != std::string_view::npos) {
        size_t end = begin, space = std::string_view::npos;
        int width = 0;
        for (; end < text.size() && text[end] != '\n' && text[end] != '\r'; ++end) {
            const char glyph[] = {text[end], '\0'};
            width += font.StringLength(glyph, captionSpacing);
            if (width > limit && end > begin)
                break;
            if (text[end] == ' ')
                space = end;
        }
        if (width > limit && end < text.size() && text[end] != ' ' && space != std::string_view::npos)
            end = space;
        const auto line = text.substr(begin, end - begin);
        lines.emplace_back(line.substr(0, line.find_last_not_of(" \t") + 1));
        begin = text.find_first_not_of(" \r\n\t", end);
    }
    return lines;
}

void DrawIdleCaptions(int widthOnScreen, const std::array<glm::vec2, 2> &heads, float worldScale,
                      const CoopPane *panes = nullptr, const CoopDivider *sides = nullptr, float blend = 0.0f) {
    auto &font = *pDefaultFont;
    constexpr int paddingX = 9, paddingY = 4;
    const int textWidth = std::min(192, widthOnScreen / 3) - 2 * paddingX;
    glm::vec2 previousCenter{}, previousHalf{};
    for (int p = 0; p < NUMPLAYERS; ++p) {
        const auto &player = Player[p];
        if (player.Handlung != PlayerActionEnum::PISSEN ||
            player.BronsonCounter <= 220.0f || textWidth < 60)
            continue;

        constexpr int count = TEXT::PISS_18 - TEXT::PISS_1 + 1;
        const int index = static_cast<int>((player.BronsonCounter - 220.0f) / 50.0f) % count;
        const auto lines = WrapIdleMessage(TextArray[TEXT::PISS_1 + index], textWidth);
        if (lines.empty())
            continue;
        int width = 0;
        for (const auto &line : lines)
            width = std::max(width, font.StringLength(line.c_str(), captionSpacing));
        const int lineHeight = font.GetYCharSize() + 1;
        width += 2 * paddingX;
        const int height = static_cast<int>(lines.size()) * lineHeight - 1 + 2 * paddingY;
        const glm::vec2 half(width * 0.5f, height * 0.5f);
        const glm::vec2 low = glm::vec2(4, 73) + half;
        const glm::vec2 high = glm::vec2(widthOnScreen - 4, RENDERHEIGHT - 4) - half;
        if (low.x > high.x || low.y > high.y)
            continue;
        const float attachment = glm::smoothstep(0.42f, 0.94f, blend);
        const auto fit = [&](glm::vec2 center) {
            const auto shared = glm::clamp(center, low, high);
            if (!panes || attachment == 0.0f)
                return shared;
            const auto &pane = panes[p].viewport;
            const auto &side = sides[p];
            const auto innerLow = glm::max(low, glm::vec2(pane.x + 4, pane.y + 4) + half);
            const auto innerHigh = glm::min(high, glm::vec2(pane.x + pane.w - 4, pane.y + pane.h - 4) - half);
            if (innerLow.x > innerHigh.x || innerLow.y > innerHigh.y)
                return shared;
            auto own = glm::clamp(center, innerLow, innerHigh);
            const glm::vec2 inside(side.normal.x > 0 ? innerLow.x : innerHigh.x,
                                   side.normal.y > 0 ? innerLow.y : innerHigh.y);
            const float support = glm::dot(glm::abs(side.normal), half) + 3;
            const float excess = side.Distance(own) + support;
            if (excess > 0.0f && side.Distance(inside) + support < 0.0f)
                own = glm::mix(own, inside, excess / glm::dot(side.normal, own - inside));
            return glm::mix(shared, own, attachment);
        };
        const float clearance = CoopNeedsPartner() ? glm::mix(33.0f, 7.0f, blend) : 7.0f;
        const glm::vec2 above = heads[p] - glm::vec2(0, half.y + clearance);
        const glm::vec2 below = heads[p] + glm::vec2(0, half.y + 7 + (PLAYER_FRAME_HEIGHT - 11) / worldScale);
        const float upperEdge = panes ? glm::mix(low.y, std::max(low.y, panes[p].viewport.y + half.y + 4), attachment) : low.y;
        glm::vec2 center = fit(glm::mix(below, above, glm::smoothstep(upperEdge - 10, upperEdge + 10, above.y)));
        if (previousHalf.x > 0) {
            const auto overlaps = [&](glm::vec2 pos) {
                const glm::vec2 gap = glm::abs(pos - previousCenter) - half - previousHalf;
                return gap.x < 2 && gap.y < 2;
            };
            for (const auto alternative : {below, glm::vec2(center.x, previousCenter.y - previousHalf.y - half.y - 3),
                                           glm::vec2(center.x, previousCenter.y + previousHalf.y + half.y + 3)}) {
                if (!overlaps(center))
                    break;
                center = fit(alternative);
            }
        }
        previousCenter = center;
        previousHalf = half;
        const int x = static_cast<int>(std::round(center.x - half.x));
        const int y = static_cast<int>(std::round(center.y - half.y));
        const int alpha = static_cast<int>(std::min(1.0f, (player.BronsonCounter - 220.0f) * 0.2f) * 255);
        GUI.DrawCompactFrame({x, y, width, height}, D3DCOLOR_RGBA(0, 255, 0, alpha * CGUISystem::panelOpacity / 255));
        const D3DCOLOR ink = D3DCOLOR_RGBA(0, 255, 0, alpha);
        for (size_t row = 0; row < lines.size(); ++row) {
            const char *line = lines[row].c_str();
            const int lineX = x + (width - font.StringLength(line, captionSpacing)) / 2;
            font.DrawText(lineX, y + paddingY + row * lineHeight, line, ink, captionSpacing);
        }
    }
}

void DrawPlayerOverlays(const SDL_Rect &pane, float projectionLeft, const glm::mat4 &worldTilt,
                        std::array<glm::vec2, 2> &screenHeads, const CoopDivider *divider = nullptr,
                        glm::vec2 focus = glm::vec2(0.0f), float worldScale = 1.0f) {
    glm::vec2 targets[2]{};
    for (int p = 0; p < NUMPLAYERS; ++p) {
        const auto &player = Player[p];
        targets[p] = glm::vec2(worldTilt * glm::vec4(player.xpos + 35.0f - TileEngine.XOffset,
                                                player.ypos + 11.0f - TileEngine.YOffset, 0.0f, 1.0f));
        const glm::vec2 origin(projectionLeft, 0.0f);
        targets[p] = origin + (targets[p] - origin) / worldScale;
        screenHeads[p] = targets[p] - origin + glm::vec2(pane.x, pane.y);
    }
    if (pane.w < 72 || pane.h < 64)
        return;
    const glm::vec2 center(projectionLeft + pane.w / 2.0f, pane.h / 2.0f);
    const glm::vec2 labelInset(26.0f, 25.0f);
    const glm::vec2 labelRange = glm::vec2(pane.w, pane.h) / 2.0f - labelInset;
    const glm::vec2 labelMin = center - labelRange;
    const glm::vec2 labelMax = center + labelRange;
    const glm::vec2 arrowOrigin = divider ? glm::clamp(focus, labelMin, labelMax) : center;
    const auto stayInsideDivider = [&](glm::vec2 point, glm::vec2 anchor, float inset) {
        if (divider) {
            const float anchorDistance = divider->Distance(anchor);
            const float edge = std::max(anchorDistance, -inset);
            const float distance = divider->Distance(point);
            if (distance > edge)
                return glm::mix(anchor, point, (edge - anchorDistance) / (distance - anchorDistance));
        }
        return point;
    };
    const glm::vec2 overlap = glm::abs(targets[0] - targets[1]);
    const bool close = overlap.x < 26.0f && overlap.y < 18.0f;
    const D3DCOLOR colors[] = {D3DCOLOR_RGBA(255, 65, 71, 255), D3DCOLOR_RGBA(69, 158, 255, 255)};

    g_matModelView = glm::mat4x4(1.0f);
#if defined(USE_GL1)
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
    DirectGraphics.SetColorKeyMode();

    if (!CoopNeedsPartner())
        return;
    for (int p = 0; p < 2; ++p) {
        if (Player[p].Handlung == PlayerActionEnum::TOT)
            continue;
        const glm::vec2 delta = targets[p] - center;
        const bool onScreen = std::abs(delta.x) <= pane.w / 2.0f - 6.0f &&
                              std::abs(delta.y) <= pane.h / 2.0f - 6.0f &&
                              (!divider || divider->Distance(targets[p]) <= -6.0f);
        // Head labels return after the shared camera has fully merged.
        if (onScreen && (TileEngine.IsCoopSplit() || TileEngine.CoopSplitAmount() > 0.0f))
            continue;
        glm::vec2 tip, label;
        if (onScreen) {
            tip = targets[p];
            const float sideways = close ? (p == 0 ? -17.0f : 17.0f) : 0.0f;
            label = tip + glm::vec2(sideways, tip.y < 26.0f ? 20.0f : -20.0f);
            const glm::vec2 inset(16.0f, 9.0f);
            const glm::vec2 size(pane.w, pane.h);
            label = glm::clamp(label, center - size / 2.0f + inset, center + size / 2.0f - inset);
            label = stayInsideDivider(label, focus, 18.0f);
        } else {
            // Keep the marker and arrow on the ray toward the player, including during a turn.
            const auto toward = glm::normalize(targets[p] - arrowOrigin);
            float distance = std::numeric_limits<float>::infinity();
            for (int axis = 0; axis < 2; ++axis) {
                if (toward[axis] != 0.0f) {
                    const float edge = toward[axis] > 0.0f ? labelMax[axis] : labelMin[axis];
                    distance = std::min(distance, (edge - arrowOrigin[axis]) / toward[axis]);
                }
            }
            if (divider) {
                const float outward = glm::dot(divider->normal, toward);
                if (outward > 0.0f)
                    distance = std::min(distance, std::max(0.0f, -24.0f - divider->Distance(arrowOrigin)) / outward);
            }
            label = arrowOrigin + toward * distance;
            tip = stayInsideDivider(label + toward * 21.0f, label, 3.0f);
        }

        const glm::vec2 arrowDirection = tip - label;
        if (glm::dot(arrowDirection, arrowDirection) < 1.0f)
            continue;
        const glm::vec2 direction = glm::normalize(arrowDirection);
        const glm::vec2 sideways(-direction.y, direction.x);
        const glm::vec2 base = tip - direction * 9.0f;
        const glm::vec2 a = base + sideways * 5.0f;
        const glm::vec2 b = base - sideways * 5.0f;
        VERTEX2D arrow[3] = {};
        const glm::vec2 vertices[] = {tip, a, b};
        for (int i = 0; i < 3; ++i) {
            arrow[i].x = vertices[i].x;
            arrow[i].y = vertices[i].y;
            arrow[i].color = colors[p];
        }
        DirectGraphics.SetTexture(-1);
        DirectGraphics.RendertoBuffer(GL_TRIANGLES, 1, arrow);
        RenderRect(label.x - 12.0f, label.y - 7.0f, 24.0f, 14.0f, D3DCOLOR_RGBA(0, 0, 0, 208));
        pDefaultFont->DrawTextCenterAlign(label.x, label.y - 5.0f, p == 0 ? "P1" : "P2", colors[p], 0);
    }
}

void MaskSecondCoopPane(const CoopPane &pane) {
    // Draw the second pane's outline into one stencil bit with color writes disabled.
    glEnable(GL_STENCIL_TEST);
    glStencilMask(1);
    glClearStencil(0);
    glClear(GL_STENCIL_BUFFER_BIT);
    glStencilFunc(GL_ALWAYS, 1, 1);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    VERTEX2D triangles[9]{};
    for (int i = 1; i + 1 < pane.vertices; ++i) {
        const glm::vec2 points[] = {pane.outline[0], pane.outline[i], pane.outline[i + 1]};
        for (int j = 0; j < 3; ++j) {
            auto &vertex = triangles[(i - 1) * 3 + j];
            vertex.x = points[j].x;
            vertex.y = points[j].y;
            vertex.color = 0xFFFFFFFF;
        }
    }
    DirectGraphics.SetTexture(-1);
    DirectGraphics.RendertoBuffer(GL_TRIANGLES, pane.vertices - 2, triangles);

    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilMask(0);
}

void DrawCoopDivider(const CoopDivider &divider, float left, float blend, int fullWidth) {
    const glm::vec2 center = divider.normal * divider.offset + glm::vec2(left, 0.0f);
    const glm::vec2 along = glm::vec2(-divider.normal.y, divider.normal.x) * static_cast<float>(fullWidth + RENDERHEIGHT);
    const glm::vec2 points[] = {center - along - divider.normal, center - along + divider.normal,
                                center + along - divider.normal, center + along + divider.normal};
    VERTEX2D line[4]{};
    for (int i = 0; i < 4; ++i) {
        line[i].x = points[i].x;
        line[i].y = points[i].y;
        line[i].color = D3DCOLOR_RGBA(0, 0, 0, static_cast<int>(255 * blend));
    }
    DirectGraphics.SetColorKeyMode();
    DirectGraphics.SetTexture(-1);
    DirectGraphics.RendertoBuffer(GL_TRIANGLE_STRIP, 2, line);
}

// Summary counts are local to this level. A negative owner selects the team.
std::array<int, 4> SummaryCounts(int owner) {
    std::array<int, 4> counts{owner < 0 ? unclaimedBlocks : 0, 0, 0, 0};
    for (int p = 0; p < NUMPLAYERS; ++p) {
        if (owner >= 0 && owner != p)
            continue;
        counts[0] += Player[p].BlocksThisLevel;
        counts[1] += Player[p].DiamondsThisLevel;
        counts[2] += Player[p].LivesThisLevel;
        counts[3] += Player[p].SecretThisLevel;
    }
    return counts;
}

struct SummaryLayout {
    float scale;
    int width, height, title, icons, counts, bonus, prompt;

    SummaryLayout(float scale, bool atExit = false, bool hasBonus = false) : scale(scale) {
        const int padding = static_cast<int>(std::round(8 * scale));
        const int gap = static_cast<int>(std::round(6 * scale));
        const int iconHeight = static_cast<int>(std::round(32 * scale));
        const int textHeight = pDefaultFont->GetYCharSize();
        width = static_cast<int>(std::round(400 * scale));
        title = padding;
        const int iconTop = title + textHeight + gap;
        icons = iconTop + iconHeight / 2;
        counts = iconTop + iconHeight + gap;
        bonus = counts + textHeight + gap;
        prompt = hasBonus ? bonus + textHeight + gap : bonus;
        height = (atExit ? prompt : counts) + textHeight + padding;
    }

    SDL_Rect CenteredAt(glm::vec2 center) const {
        return {static_cast<int>(std::round(center.x - width * 0.5f)),
                static_cast<int>(std::round(center.y - height * 0.5f)), width, height};
    }
};

void DrawSummaryContents(int owner, const SDL_Rect &frame, const SummaryLayout &layout, D3DCOLOR color,
                         bool exitPrompt, bool revealCheat) {
    const int centerX = frame.x + frame.w / 2;
    const int row = frame.y + layout.icons;
    float columns[4];
    for (int i = 0; i < 4; ++i)
        columns[i] = centerX + std::round((i * 96 - 144) * layout.scale);
    pDefaultFont->DrawTextCenterAlign(centerX, frame.y + layout.title, TextArray[TEXT::SUMMARY_TITLE], color);
    const auto icon = [&](int column, int sprite, int spriteFrame, int nativeSize) {
        const int size = static_cast<int>(std::round(nativeSize * layout.scale));
        pGegnerGrafix[sprite]->RenderSpriteScaled(columns[column] - size / 2, row - size / 2,
                                                 size, size, spriteFrame, color);
    };
    icon(0, POWERBLOCK, 1, 32);
    icon(1, DIAMANT, 0, 29);
    icon(2, ONEUP, 0, 32);
    pDefaultFont->DrawTextCenterAlign(columns[3], row - pDefaultFont->GetYCharSize() / 2,
                                      TextArray[TEXT::SUMMARY_SECRETS], color);
    const auto counts = SummaryCounts(owner);
    const int totals[] = {TileEngine.MaxBlocks, TileEngine.MaxDiamonds, TileEngine.MaxOneUps, TileEngine.MaxSecrets};
    for (size_t i = 0; i < counts.size(); ++i) {
        const std::string stat = std::to_string(counts[i]) + "/" + std::to_string(totals[i]);
        pDefaultFont->DrawTextCenterAlign(columns[i], frame.y + layout.counts, stat.c_str(), color);
    }
    if (revealCheat) {
        std::string code(Cheats[Stage - 1]);
        for (char &c : code)
            c ^= 64;
        const std::string text = std::string(TextArray[TEXT::SUMMARY_CHEATUNLOCK]) + ": " + code;
        pDefaultFont->DrawTextCenterAlign(centerX, frame.y + layout.bonus, text.c_str(), color, 0);
    }
    if (exitPrompt) {
        std::string prompt(TextArray[TEXT::SUMMARY_PRESSFIRE]);
        if (Player[0].UsingJoystick())
            ReplaceAll(prompt, "key", "button");
        pDefaultFont->DrawTextCenterAlign(centerX, frame.y + layout.prompt, prompt.c_str(), color);
    }
}

void UpdateSummaryInput() {
    const float step = std::clamp(static_cast<float>(Timer.getElapsed()), 0.0f, 0.05f) / 0.18f;
    for (int p = 0; p < 2; ++p) {
        const auto &player = Player[p];
        const bool available = p < NUMPLAYERS && !DEMOPlaying && CoopPlayerReady(player) && !player.StageClearRunning && player.GameOverTimer <= 0;
        const bool held = available && !Console.Showing && player.ActionPressed(ACTION_SUMMARY, player.GetJoystick());
        summaryOpacity[p] = available ? std::clamp(summaryOpacity[p] + (held ? step : -step), 0.0f, 1.0f) : 0;
    }
}

void DrawLiveSummary(int owner, const SDL_Rect &pane, glm::vec2 center, const CoopDivider *divider, float blend) {
    const float level = owner < 0 ? std::max(summaryOpacity[0], summaryOpacity[1]) : summaryOpacity[owner];
    const float opacity = glm::smoothstep(0.0f, 1.0f, level);
    if (opacity == 0.0f) return;
    float top = static_cast<float>(pane.y);
    if (divider) {
        top = std::max(top, HUD.Bottom());
        const auto safe = MakeCoopPane(DirectGraphics.GetViewWidth(), *divider, top);
        if (safe.vertices < 3)
            return;
        center = glm::mix(center, safe.center, blend);
    }
    const glm::vec2 local = center - glm::vec2(pane.x, pane.y);
    const glm::vec2 room = glm::min(center - glm::vec2(pane.x, top), glm::vec2(pane.w, pane.h) - local) - 6.0f;
    const auto fits = [&](const SummaryLayout &layout) {
        const glm::vec2 halfSize(layout.width * 0.5f, layout.height * 0.5f);
        return halfSize.x <= room.x && halfSize.y <= room.y &&
            (!divider || divider->Distance(center) + glm::dot(glm::abs(divider->normal), halfSize) <= -6.0f);
    };
    float scale = 1.0f - 0.28f * blend;
    if (!fits(SummaryLayout(scale))) {
        // Text stays at its native size. Fit the rounded panel around it during a diagonal split.
        float low = 0.55f, high = scale;
        if (!fits(SummaryLayout(low)))
            return;
        for (int i = 0; i < 6; ++i) {
            const float mid = (low + high) * 0.5f;
            if (fits(SummaryLayout(mid)))
                low = mid;
            else
                high = mid;
        }
        scale = low;
    }
    const SummaryLayout layout(scale);
    const auto frame = layout.CenteredAt(local);
    DirectGraphics.SetGameViewport(pane, 0.0f);
    g_matModelView = glm::mat4(1.0f);
#if defined(USE_GL1)
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
    DirectGraphics.SetColorKeyMode();
    GUI.DrawCompactFrame(frame, D3DCOLOR_RGBA(0, 255, 0, static_cast<int>(CGUISystem::panelOpacity * opacity)));
    DrawSummaryContents(owner, frame, layout, D3DCOLOR_RGBA(0, 255, 0, static_cast<int>(255 * opacity)), false, false);
}

void UpdateTutorialCards(float seconds) {
    const float fade = seconds * 5.0f;
    int shared = -1;
    float best = std::numeric_limits<float>::infinity();
    bool entered = false;
    for (int p = 0; p < NUMPLAYERS; ++p) {
        const auto &candidate = nearbyLessons[p];
        const int id = CoopPlayerReady(Player[p]) && !Player[p].StageClearRunning ? candidate.id : -1;
        entered |= playerLessons[p].Update(id, fade);
        // Let the players finish reading the shared lesson before choosing another nearby lesson.
        const float priority = id == sharedLesson.lesson ? -1.0f : candidate.distance;
        if (id >= 0 && priority < best) {
            best = priority;
            shared = id;
        }
    }
    const bool sharedEntered = sharedLesson.Update(shared, fade);
    if (TileEngine.CoopSplitAmount() > 0.5f ? entered : sharedEntered)
        SoundManager.PlayWave(100, 128, 11025, SOUND::MESSAGE);
}

void DrawTutorialPanels(int width, const CoopPane *panes, const CoopDivider *sides, float blend) {
    const float individual = panes ? glm::smoothstep(0.45f, 0.88f, blend) : 0.0f;
    if (sharedLesson.opacity > 0.0f && individual < 1.0f) {
        const int profile = CoopNeedsPartner() ? ((SDL_GetTicks() - sharedLesson.started) / 1800u) % 2u :
            (NUMPLAYERS == 2 && !CoopPlayerReady(Player[0]) ? 1 : 0);
        const int owner = CoopNeedsPartner() ? -1 : profile;
        GUI.DrawTutorialCard(sharedLesson.lesson, owner, profile, sharedLesson.opacity * (1.0f - individual) * (1 - std::max(summaryOpacity[0], summaryOpacity[1])),
                             {0, 74, width, RENDERHEIGHT - 82}, true);
    }
    for (int p = 0; individual > 0.0f && p < NUMPLAYERS; ++p) {
        if (playerLessons[p].opacity <= 0.0f)
            continue;
        const auto &side = sides[p];
        const float top = std::max(HUD.Bottom() + 4.0f, static_cast<float>(panes[p].viewport.y));
        const auto safe = MakeCoopPane(width, side, top);
        if (safe.vertices < 3)
            continue;
        const auto &v = safe.viewport;
        const auto center = safe.center;
        glm::vec2 half = glm::min(center - glm::vec2(v.x, v.y), glm::vec2(v.x + v.w, v.y + v.h) - center);
        // Keep all four corners on this side of the divider, including during a turn.
        const float extent = glm::dot(glm::abs(side.normal), half);
        if (extent > 0.0f)
            half *= std::clamp(-side.Distance(center) / extent, 0.0f, 1.0f);
        const SDL_Rect space{static_cast<int>(std::ceil(center.x - half.x)) + 5,
                             static_cast<int>(std::ceil(center.y - half.y)) + 5,
                             static_cast<int>(2 * half.x) - 11, static_cast<int>(2 * half.y) - 11};
        GUI.DrawTutorialCard(playerLessons[p].lesson, p, p, playerLessons[p].opacity * individual * (1 - summaryOpacity[p]), space, false);
    }
}

void DrawCoopViews(float sharedShake) {
    const int fullWidth = DirectGraphics.GetViewWidth();
    const SDL_Rect screen{0, 0, fullWidth, RENDERHEIGHT};
    const float amount = TileEngine.CoopSplitAmount();
    float blend = amount * amount * (3.0f - 2.0f * amount);
    auto normal = TileEngine.CoopSplitNormal();
    const bool turning = std::min(std::abs(normal.x), std::abs(normal.y)) > 0.0001f;
    const bool useStencil = turning && DirectGraphics.EnsureGameStencil();
    if (!useStencil) {
        const float horizontal = normal.x * normal.x - normal.y * normal.y;
        // On render targets without stencil support, close and reopen at the midpoint of a turn.
        if (turning)
            blend *= horizontal * horizontal;
        normal = horizontal >= 0.0f ? glm::vec2(std::copysign(1.0f, normal.x), 0.0f)
                                    : glm::vec2(0.0f, std::copysign(1.0f, normal.y));
    }

    const glm::vec2 halfSize(fullWidth / 2.0f, RENDERHEIGHT / 2.0f);
    const float halfSpan = glm::dot(glm::abs(normal), halfSize);
    const float exposed = useStencil ? halfSpan * blend : std::round(halfSpan * blend);
    // Move the divider in from the edge as the second player's view opens.
    const CoopDivider divider{normal, glm::dot(normal, halfSize) + halfSpan - exposed};
    const CoopDivider sides[] = {divider, {-divider.normal, -divider.offset}};
    const CoopPane panes[] = {MakeCoopPane(fullWidth, sides[0]), MakeCoopPane(fullWidth, sides[1])};
    const bool twoPanes = panes[1].viewport.w > 0 && panes[1].viewport.h > 0;
    const float sharedX = TileEngine.XOffset;
    const float sharedY = TileEngine.YOffset;
    const float sharedLeft = TileEngine.CalculateViewLeft();
    const float sharedScale = TileEngine.CoopViewScale();
    const auto sharedOrigin = TileEngine.SharedViewOrigin(sharedScale);
    const float viewScale = glm::mix(sharedScale, 1.0f, blend);
    const auto sharedBackdrop = TileEngine.BackdropCamera(sharedOrigin, glm::vec2(fullWidth, RENDERHEIGHT) * sharedScale);

    DirectGraphics.ClearBackBuffer();
    if (useStencil && twoPanes) {
        DirectGraphics.SetGameViewport(screen, 0.0f);
        g_matModelView = glm::mat4x4(1.0f);
#if defined(USE_GL1)
        load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
        MaskSecondCoopPane(panes[1]);
    }
    // Run effects once while both views follow the same level simulation.
    Projectiles.DoProjectiles(false);
    PartikelSystem.DoPartikel(false);

    std::array<glm::vec2, 2> projectedHeads[2]{};
    const int views = twoPanes ? 2 : 1;
    for (int p = 0; p < views; ++p) {
        const auto &pane = panes[p].viewport;
        const auto focus = panes[p].center - glm::vec2(pane.x, pane.y);
        const float sharedPaneLeft = sharedLeft + pane.x;
        const float projectionLeft = sharedPaneLeft * (1.0f - blend);
        const glm::vec2 sharedPaneOrigin = sharedOrigin + glm::vec2(pane.x, pane.y) * sharedScale;
        const glm::vec2 playerCenter(Player[p].xpos + 35.0f, Player[p].ypos + 40.0f);
        const glm::vec2 paneSize(pane.w, pane.h);
        const auto look = TileEngine.CoopLookOffset(p, paneSize);
        constexpr float playerInset = 44.0f;
        auto shift = glm::clamp(look, -glm::max(glm::vec2(0), paneSize - focus - playerInset),
                                     glm::max(glm::vec2(0), focus - playerInset));
        if (useStencil && twoPanes) {
            const float outward = glm::dot(-shift, sides[p].normal);
            const float room = std::max(0.0f, -sides[p].Distance(panes[p].center) - playerInset);
            if (outward > room)
                shift *= room / outward;
        }
        const auto cameraFocus = playerCenter + shift;
        const auto worldOrigin = glm::mix(sharedPaneOrigin, cameraFocus - focus, blend);
        const glm::vec2 viewSize = glm::vec2(pane.w, pane.h) * viewScale;
        const auto cropOrigin = TileEngine.ExitViewOrigin(worldOrigin, viewSize, playerCenter);
        TileEngine.XOffset = cropOrigin.x - projectionLeft;
        TileEngine.YOffset = cropOrigin.y;

        if (useStencil && twoPanes)
            glStencilFunc(GL_EQUAL, p, 1);
        DirectGraphics.SetGameViewport(pane, projectionLeft, viewScale);
        TileEngine.CalcRenderRange(projectionLeft, static_cast<int>(std::ceil(viewSize.x)),
                                  static_cast<int>(std::ceil(viewSize.y)));
        const auto backdropCamera = glm::mix(sharedBackdrop, TileEngine.BackdropCamera(cropOrigin, viewSize), blend);
        const float shake = twoPanes ? cameraShakes[p].angle : sharedShake;
        const auto tilt = ApplyWorldTilt(projectionLeft, viewSize, ScreenWinkel + shake);
        TileEngine.DrawBackground(cropOrigin - backdropCamera, viewSize, backdropCamera);
        DrawLevelActors(p != 0);
        Projectiles.DrawProjectiles();
        PartikelSystem.DrawOnly();
        DrawLevelForeground(twoPanes ? p : -1);
        DirectGraphics.SetGameViewport(pane, projectionLeft);
        const CoopDivider paneDivider{sides[p].normal, sides[p].offset -
            glm::dot(sides[p].normal, glm::vec2(pane.x - projectionLeft, pane.y))};
        DrawPlayerOverlays(pane, projectionLeft, tilt, projectedHeads[p], useStencil ? &paneDivider : nullptr,
                           focus + glm::vec2(projectionLeft, 0.0f), viewScale);
    }

    if (useStencil && twoPanes) {
        glDisable(GL_STENCIL_TEST);
        glStencilMask(~0u);
    }
    TileEngine.XOffset = sharedX;
    TileEngine.YOffset = sharedY;
    TileEngine.CalcRenderRange();

    DirectGraphics.SetGameViewport(screen, 0.0f);
    g_matView = g_matModelView = glm::mat4x4(1.0f);
#if defined(USE_GL1)
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
    auto heads = projectedHeads[0];
    if (twoPanes) {
        // P2 is still drawn by the shared camera while the new pane is a sliver.
        const float crossed = glm::smoothstep(-32.0f, 32.0f, divider.Distance(heads[1]));
        const float own = std::max(glm::smoothstep(0.45f, 0.90f, blend), crossed);
        heads[1] = glm::mix(heads[1], projectedHeads[1][1], own);
    }
    DrawIdleCaptions(fullWidth, heads, viewScale, twoPanes ? panes : nullptr, sides, blend);
    DrawTutorialPanels(fullWidth, twoPanes ? panes : nullptr, sides, blend);
    for (int p = 0; p < views; ++p)
        DrawLiveSummary(twoPanes ? p : -1, panes[p].viewport, panes[p].center, twoPanes ? &sides[p] : nullptr, blend);

    const float left = DirectGraphics.InterfaceLeft();
    DirectGraphics.SetGameViewport(screen, left);
    g_matView = g_matModelView = glm::mat4x4(1.0f);
#if defined(USE_GL1)
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
    if (twoPanes)
        DrawCoopDivider(divider, left, blend, fullWidth);
}

// A held jump starts with zero gravity until it reaches its normal apex.
// An overhead camera or rendezvous limit acts like hitting a physical ceiling.
void StopAtVerticalLimit(PlayerClass &player, float allowedY) {
    if (allowedY > player.ypos && player.yspeed < 0.0f) {
        player.yspeed = 0.0f;
        player.JumpAdd = std::max(player.JumpAdd, PLAYER_JUMPADDSPEED);
    } else if (allowedY < player.ypos && player.yspeed > 0.0f) {
        player.yspeed = 0.0f;
    }
}

void ApplyCoopWaits() {
    if (CoopNeedsPartner()) {
        for (int p = 0; p < NUMPLAYERS; ++p) {
            const auto &wait = coopWaits[p];
            auto &player = Player[p];
            if (!wait.waiting || !CoopPlayerReady(player) || player.DoFesteAktion || player.StageClearRunning)
                continue;

            // Stop movement across the far edge of a waiting area and allow retreat.
            const auto clip = [](float current, float previous, float low, float high) {
                if (current < low && previous >= low)
                    return low;
                if (current > high && previous <= high)
                    return high;
                return current;
            };
            const float x = clip(player.xpos, player.xposold, wait.minimum.x, wait.maximum.x);
            const float y = clip(player.ypos, player.yposold, wait.minimum.y, wait.maximum.y);
            if ((x != player.xpos || y != player.ypos) && !TileEngine.OverlapsSolidWall(x, y, player.CollideRect)) {
                if (x != player.xpos)
                    player.xspeed = 0.0f;
                StopAtVerticalLimit(player, y);
                player.xpos = x;
                player.ypos = y;
            }
        }
    }
    coopWaits = {};
    // The lift and boss can appear in either order in the level's object list.
    coopLiftWasWaiting = coopLiftWaiting;
    coopLiftWaiting = false;
}

void KeepPlayersOnOneScreen(const std::array<glm::vec2, 2> &previous) {
    std::array<glm::vec2, 2> positions = {{{Player[0].xpos, Player[0].ypos}, {Player[1].xpos, Player[1].ypos}}};
    const glm::vec2 maxSeparation(DirectGraphics.GetViewWidth() - PLAYER_FRAME_WIDTH,
                                   RENDERHEIGHT - PLAYER_FRAME_HEIGHT);
    for (int axis = 0; axis < 2; ++axis) {
        const int low = positions[0][axis] < positions[1][axis] ? 0 : 1;
        const int high = 1 - low;
        const float excess = positions[high][axis] - positions[low][axis] - maxSeparation[axis];
        if (excess <= 0.0f)
            continue;
        // Limit just the outward part of this step. A stationary partner keeps their footing.
        const float lowStep = std::max(0.0f, previous[low][axis] - positions[low][axis]);
        const float highStep = std::max(0.0f, positions[high][axis] - previous[high][axis]);
        if (lowStep + highStep > 0.0f) {
            const float fraction = std::min(1.0f, excess / (lowStep + highStep));
            positions[low][axis] += lowStep * fraction;
            positions[high][axis] -= highStep * fraction;
        }
    }
    for (int p = 0; p < 2; ++p) {
        auto &player = Player[p];
        auto pos = positions[p];
        if (pos.x == player.xpos && pos.y == player.ypos)
            continue;
        // Slide vertically along a corrected horizontal boundary even on slopes.
        if (TileEngine.OverlapsSolidWall(pos.x, pos.y, player.CollideRect)) {
            if (TileEngine.OverlapsSolidWall(previous[p].x, previous[p].y, player.CollideRect))
                continue;
            pos.x = previous[p].x;
            pos.y = TileEngine.ClipVerticalMotion(previous[p].y, pos.y, pos.x, player.CollideRect);
        }
        if (pos.x != player.xpos)
            player.xspeed = 0.0f;
        StopAtVerticalLimit(player, pos.y);
        player.xpos = pos.x;
        player.ypos = pos.y;
    }
}

void DrawCoopWait() {
    const bool p1Waiting = coopWaits[0].waiting;
    const bool p2Waiting = coopWaits[1].waiting;
    if ((!p1Waiting && !p2Waiting) || Player[0].StageClearRunning || Player[1].StageClearRunning)
        return;

    const auto previous = g_matModelView;
    g_matModelView = glm::mat4x4(1.0f);
#if defined(USE_GL1)
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
    DirectGraphics.SetColorKeyMode();
    constexpr float center = RENDERWIDTH / 2.0f;
    RenderRect(center - 105.0f, RENDERHEIGHT - 36.0f, 210.0f, 25.0f, D3DCOLOR_RGBA(0, 0, 0, 208));
    pDefaultFont->DrawTextCenterAlign(center, RENDERHEIGHT - 29.0f,
        MenuText(p1Waiting && p2Waiting ? "REGROUP TO CONTINUE" : p1Waiting ? "WAITING FOR P2" : "WAITING FOR P1"), 0xFFFFFFFF, 0);
    g_matModelView = previous;
#if defined(USE_GL1)
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
}

}  // namespace

void GameLoop() {
    HUD.bShowArrow = false;

    TileEngine.NewXOffset = -1;
    TileEngine.NewYOffset = -1;

    constexpr float SPD_INC = 0.3f;
    float const SpeedFaktorMax = Timer.getSpeedFactor();

    int chunks = 1;

    // If the hardware can not render fast enough the logic catch up becomes too large
    // In this case the logic needs to be broken up into chunks
    if (SpeedFaktorMax > SPD_INC) {
        chunks = ceilf(SpeedFaktorMax / SPD_INC);
        Timer.setSpeedFactor(SpeedFaktorMax / chunks);
    }

    // Run the Logic
    for (int i = 0; i < chunks; i++) {
        const bool oneScreen = TileEngine.LevelSplitScreenType() == SplitScreenType::OneScreen &&
            TileEngine.HasFreeCoopCamera() && CoopPlayerReady(Player[0]) && CoopPlayerReady(Player[1]);
        const std::array<glm::vec2, 2> previous = {{{Player[0].xpos, Player[0].ypos}, {Player[1].xpos, Player[1].ypos}}};
        for (int p = 0; p < NUMPLAYERS; p++) {
            Player[p].WasDamaged = false;

            if (!Console.Showing && Player[p].Handlung != PlayerActionEnum::TOT) {
                if (Player[p].GameOverTimer == 0.0f) {
                    // Spieler-Eingabe abfragen
                    Player[p].GetPlayerInput();
                    Player[p].AnimatePlayer();
                    Player[p].MovePlayer();
                }
            }

            Player[p].GegnerDran = false;

            // Spieler bewegen
            if (p == 0 && !Console.Showing)
                Player[p].ScrollFlugsack();
        }

        // Spieler auf einer Plattform ?
        for (int p = 0; p < NUMPLAYERS; p++)
            if (Player[p].AufPlattform != nullptr)
                Player[p].DoPlattformStuff();

        ApplyCoopWaits();
        if (oneScreen)
            KeepPlayersOnOneScreen(previous);
        TileEngine.UpdateCoopViews();

        // Gegner bewegen
        if (!Console.Showing) {
            nearbyLessons = {};
            Gegner.RunAll();
        }

        // Level checken
        if (!Console.Showing)
            TileEngine.UpdateLevel();

        TileEngine.CheckBounds();
    }
    Timer.setSpeedFactor(SpeedFaktorMax);  // Restore the factor so other logic can stay in sync
    TileEngine.UpdateCoopViews();

    if (SpielZustand != GameStateEnum::GAMELOOP)
        return;

    TileEngine.AdvanceCoopTransition(static_cast<float>(Timer.getElapsed()));
    UpdateSummaryInput();
    if (!Console.Showing)
        UpdateTutorialCards(std::min(0.05f, static_cast<float>(Timer.getElapsed())));
    TileEngine.UpdateBackground();
    if (!Console.Active) {
        const float elapsed = static_cast<float>(Timer.getElapsed());
        for (auto &shake : cameraShakes)
            shake.Advance(elapsed);
    }
    const int strongest = NUMPLAYERS == 2 && cameraShakes[1].Remaining() > cameraShakes[0].Remaining() ? 1 : 0;
    const float sharedShake = cameraShakes[strongest].angle;
    if (TileEngine.CoopSplitAmount() > 0.0f || TileEngine.CoopViewScale() > 1.0f) {
        DrawCoopViews(sharedShake);
    } else {
        DirectGraphics.SetColorKeyMode();
        const glm::vec2 saved(TileEngine.XOffset, TileEngine.YOffset);
        const float left = TileEngine.CalculateViewLeft();
        const auto origin = TileEngine.SharedViewOrigin(1.0f);
        TileEngine.XOffset = origin.x - left;
        TileEngine.YOffset = origin.y;
        const SDL_Rect view{0, 0, DirectGraphics.GetViewWidth(), RENDERHEIGHT};
        TileEngine.CalcRenderRange(left, view.w, view.h);
        if (view.w > RENDERWIDTH) DirectGraphics.ClearBackBuffer();
        DirectGraphics.SetHorizontalView(left);
        const auto tilt = ApplyWorldTilt(left, {view.w, view.h}, ScreenWinkel + sharedShake);
        TileEngine.DrawBackground();
        DrawLevelActors();
        Projectiles.DoProjectiles();
        PartikelSystem.DoPartikel();
        DrawLevelForeground();
        if (NUMPLAYERS == 2 || Player[0].BronsonCounter > 220.0f) {
            DirectGraphics.SetHorizontalView(left);
            std::array<glm::vec2, 2> heads{};
            DrawPlayerOverlays(view, left, tilt, heads);
            DirectGraphics.SetGameViewport(view, 0.0f);
            DrawIdleCaptions(view.w, heads, 1.0f);
        }
        DirectGraphics.SetGameViewport(view, 0.0f);
        DrawTutorialPanels(view.w, nullptr, nullptr, 0.0f);
        DrawLiveSummary(-1, view, {view.w * 0.5f, view.h * 0.5f}, nullptr, 0.0f);
        TileEngine.XOffset = saved.x;
        TileEngine.YOffset = saved.y;
        TileEngine.CalcRenderRange();
    }

    for (int p = 0; p < NUMPLAYERS; ++p)
        Player[p].FinishDrawingFrame();

    DirectGraphics.SetInterfaceView();
    // HUD anhandeln
    HUD.DoHUD();

    // ggf. BossHUD anzeigen
    HUD.RenderBossHUD();
    DrawCoopWait();

    // evtl Warning Schild rendern
    if (WarningCount > 0.0f) {

        int a = static_cast<int>(WarningCount * 2.55f);
        D3DCOLOR Col = D3DCOLOR_RGBA(255, 255, 255, a);

        DirectGraphics.SetFilterMode(true);

        float off = 100.0f - WarningCount;

        pGegnerGrafix[WARNING]->SetRect(0, 0, 180, 40);
        pGegnerGrafix[WARNING]->RenderSpriteScaled(230.0f - off * 4.5f / 4.0f, 390.0f - off / 4.0f,
                                                   180 + static_cast<int>(off * 4.5f / 2.0f),
                                                   40 + static_cast<int>(off * 1.0f / 2.0f), Col);

        DirectGraphics.SetFilterMode(false);

        DirectGraphics.SetColorKeyMode();
    }

    // Blitz und andere Partikel rendern, die alles überlagern
    PartikelSystem.DoThunder();

    if (!Console.Showing) {
        for (int p = 0; p < NUMPLAYERS; p++)
            if (Player[p].Handlung != PlayerActionEnum::TOT)
                Player[p].CheckForExplode();
    }

    // Game-Over Schrift anzeigen ?
    if (Player[0].GameOverTimer > 0.0f)
        ShowGameOver();

    // Gameloop verlassen ?
    if ((KeyDown(DIK_ESCAPE) || DirectInput.AnyPauseButtonDown())
            && Player[0].GameOverTimer == 0.0f)
        LeaveGameLoop();

#if defined(GCW)
    // On GCW Zero, same check as above, but using the internal controls
    if (DirectInput.InternalJoystickMainMenuButtonDown() && Player[0].GameOverTimer == 0.0f)
        LeaveGameLoop();
#endif  // GCW

    /*
        if (KeyDown(DIK_F1)) Player[0].CurrentWeaponLevel[Player[0].SelectedWeapon] = 1;
        if (KeyDown(DIK_F2)) Player[0].CurrentWeaponLevel[Player[0].SelectedWeapon] = 2;
        if (KeyDown(DIK_F3)) Player[0].CurrentWeaponLevel[Player[0].SelectedWeapon] = 3;
        if (KeyDown(DIK_F4)) Player[0].CurrentWeaponLevel[Player[0].SelectedWeapon] = 4;
        if (KeyDown(DIK_F5)) Player[0].CurrentWeaponLevel[Player[0].SelectedWeapon] = 5;
        if (KeyDown(DIK_F6)) Player[0].CurrentWeaponLevel[Player[0].SelectedWeapon] = 6;
        if (KeyDown(DIK_F7)) Player[0].CurrentWeaponLevel[Player[0].SelectedWeapon] = 7;
        if (KeyDown(DIK_F8)) Player[0].CurrentWeaponLevel[Player[0].SelectedWeapon] = 8;
    */

    if (DEMORecording)
        pDefaultFont->DrawText(10, 455, "Recording demo", D3DCOLOR_RGBA(255, 255, 255, 255));

}  // GameLoop

// --------------------------------------------------------------------------------------
// Game Loop verlassen
// --------------------------------------------------------------------------------------

void LeaveGameLoop() {
    // Movespeed wieder richtig setzen
    Timer.SetMoveSpeed(10.0f);

    // Message Box verstecken
    GUI.HideBoxFast();

    // Console verstecken
    Console.Hide();

    // Schrift im Menu neu einfaden
    pMenu->Rotation = 0.0f;
    pMenu->RotationDir = 1;

    // Ins Hauptmenu wechseln
    SpielZustand = GameStateEnum::MAINMENU;
    pMenu->AktuellerZustand = MENUZUSTAND_MAINMENU;
    if (Player[0].Lives == -1 && Player[1].Lives == -1)
        // DKS - If game is over, make main menu selection be "Start New Game"
        pMenu->AktuellerPunkt = MENUPUNKT_STARTGAME;
    else
        // DKS - If the game was not over yet, make the main menu selection be "Back To Game"
        pMenu->AktuellerPunkt = MENUPUNKT_CONTINUEGAME;

    // Musik pausieren
    // DKS - Now we use use the sound manager's already-present pause functions:
    // SoundManager.StopSong(MUSIC::STAGEMUSIC, true);
    // SoundManager.StopSong(MUSIC::FLUGSACK, true);
    // SoundManager.StopAllSongs(true);
    SoundManager.PauseSongs();
    SoundManager.PauseSounds();

    // DKS
    // Loop Sounds stoppen
    // SoundManager.StopAllLoopedSounds();

    // Menu Musik spielen
    SoundManager.PlaySong(MUSIC::MENU, false);

    // TODO
    // Alle Joysticks anhalten
    // DirectInput.StopForceFeedbackEffect(FFE_BLITZ);
    // GameRunning = false;
}

// --------------------------------------------------------------------------------------
// ScreenWackel werte bearbeiten
// --------------------------------------------------------------------------------------

void ShakeScreen(float strength, float worldX, float worldY, bool rumble) {
    constexpr float fullStrengthRadius = 48.0f;
    const float radius = std::min(560.0f, 160.0f + strength * 40.0f);
    std::array<float, 2> weights{};
    for (int p = 0; p < NUMPLAYERS; ++p) {
        const auto &player = Player[p];
        if (player.Handlung == PlayerActionEnum::TOT)
            continue;
        const float distance = std::hypot(player.xpos + 35.0f - worldX, player.ypos + 40.0f - worldY);
        const float t = std::clamp((radius - distance) / (radius - fullStrengthRadius), 0.0f, 1.0f);
        weights[p] = t * t * (3.0f - 2.0f * t);
    }
    // A single impact belongs to the nearer pane if its radius reaches across the divider.
    // Separate simultaneous impacts can still reach their respective players.
    if (TileEngine.CoopSplitAmount() > 0.0f)
        weights[weights[1] > weights[0] ? 0 : 1] = 0.0f;
    for (int p = 0; p < NUMPLAYERS; ++p) {
        if (weights[p] > 0.0f) {
            cameraShakes[p].Strike(strength * weights[p], 0.20f + 0.055f * strength);
            if (rumble)
                Player[p].PlayFeedback(FFE_MEDIUMRUMBLE, weights[p]);
        }
    }
}

// --------------------------------------------------------------------------------------
// Default Config erstellen
// --------------------------------------------------------------------------------------
// DKS - New function that creates a per-player default controls configuration
namespace {
// Retain the original file prefix so existing profiles and older releases can read it.
constexpr int oldActionCount = AKTION_WAFFEN_PREV + 1;
using OldBindings = std::array<BindingPair, oldActionCount>;
struct LegacyControls {
    int keys[oldActionCount]{};
    int pads[oldActionCount]{};
};
constexpr uint32_t twoSlotFormat = 0x31444e42; // BND1: mixed keyboard/controller pair
constexpr uint32_t separateAimFormat = 0x32444e42; // BND2 included two independent diagonal actions
constexpr uint32_t previousLayoutFormat = 0x33444e42; // BND3 ends after Summary
constexpr uint32_t playerLayoutFormat = 0x34444e42; // BND4 adds horizontal camera look
static_assert(sizeof(ControlLayouts) == 2 * MAX_AKTIONEN * 2 * 2 * sizeof(int32_t));
static_assert(std::is_trivially_copyable_v<ControlLayouts>);

#if SDL_VERSION_ATLEAST(2, 0, 0) && !defined(GCW)
constexpr int defaultLookPad = ControllerButton(SDL_CONTROLLER_BUTTON_RIGHTSTICK);
#else
constexpr int defaultLookPad = PadDirectionBinding(PadDirectionSource::RightStick, PadDirection::Down);
#endif

OldBindings ImportOldControls(int p, const LegacyControls &old, const int *oldCamera) {
    OldBindings bindings{};
    for (int action = 0; action < oldActionCount; ++action) {
        int pad = old.pads[action];
        if (IsDirectionalAction(action) && pad < 0)
            pad = AutomaticPadBinding(DefaultPadDirection(action));
        auto key = InputBinding::Key(old.keys[action]);
        if (!key.Valid())
            key = {};
        bindings[action] = {key, InputBinding::Pad(pad)};
    }
    // Older releases also listened for these P1 shortcuts outside the input system.
    if (p == 0)
        for (int i = 0; i < 3; ++i)
            if (old.keys[AKTION_WAFFE_SPREAD + i] == 0)
                bindings[AKTION_WAFFE_SPREAD + i][0] = InputBinding::Key(DIK_1 + i);

    auto &down = bindings[AKTION_UNTEN];
    const auto &crouch = bindings[AKTION_DUCKEN];
    for (int slot = 0; slot < 2; ++slot) {
        if (down[slot].kind != BindingKind::Empty && !down[slot].Overlaps(crouch[slot]))
            continue;
        // Keep a separately assigned Look Down. When it shared Crouch, inherit
        // the previous camera input, including an explicitly cleared camera slot.
        const int fallbackKey = p == 1 ? DIK_X : (old.keys[AKTION_JUMP] == DIK_NEXT ? DIK_V : DIK_NEXT);
        auto replacement = slot == 0 ? InputBinding::Key(oldCamera ? oldCamera[0] : fallbackKey)
                                     : InputBinding::Pad(oldCamera ? oldCamera[1] : defaultLookPad);
        if (!replacement.Valid() || replacement.Overlaps(crouch[slot]))
            replacement = {};
        down[slot] = replacement;
    }
    return bindings;
}

LegacyControls ExportLegacyControls(const ControlLayouts &layouts) {
    LegacyControls old;
    for (int action = 0; action < oldActionCount; ++action) {
        old.keys[action] = old.pads[action] = -1;
        for (auto binding : layouts.keyboard[action])
            if (binding.kind == BindingKind::Key && old.keys[action] < 0)
                old.keys[action] = binding.code;
        for (auto binding : layouts.controller[action])
            if (binding.kind == BindingKind::Pad && old.pads[action] < 0)
                old.pads[action] = binding.code;
    }
    return old;
}

ControlLayouts DefaultBindings(int p) {
    ControlLayouts layouts;
    constexpr int keys[2][MAX_AKTIONEN] = {
#if defined(PANDORA)
        {DIK_LEFT, DIK_RIGHT, DIK_DOWN, DIK_UP, DIK_V, DIK_NEXT, DIK_HOME, DIK_PRIOR, DIK_LSHIFT,
         DIK_END, DIK_RCONTROL, DIK_RETURN, DIK_1, DIK_2, DIK_3, DIK_BACK},
#else
        {DIK_LEFT, DIK_RIGHT, DIK_DOWN, DIK_UP, DIK_NEXT, DIK_LALT, DIK_LCONTROL, DIK_LSHIFT, DIK_SPACE,
         DIK_RCONTROL, DIK_RSHIFT, DIK_RETURN, DIK_1, DIK_2, DIK_3, DIK_BACK},
#endif
        {DIK_A, DIK_D, DIK_S, DIK_W, DIK_X, DIK_G, DIK_H, DIK_J, DIK_T,
         DIK_Z, DIK_U, DIK_Q, DIK_7, DIK_8, DIK_9, DIK_E}
    };
    int pads[MAX_AKTIONEN] = {
        AutomaticPadBinding(PadDirection::Left), AutomaticPadBinding(PadDirection::Right),
        AutomaticPadBinding(PadDirection::Down), AutomaticPadBinding(PadDirection::Up), defaultLookPad,
#if SDL_VERSION_ATLEAST(2, 0, 0) && !defined(GCW)
        ControllerButton(SDL_CONTROLLER_BUTTON_A), CONTROLLER_RT, CONTROLLER_LT,
        ControllerButton(SDL_CONTROLLER_BUTTON_Y), ControllerButton(SDL_CONTROLLER_BUTTON_X),
        ControllerButton(SDL_CONTROLLER_BUTTON_LEFTSTICK), ControllerButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER),
        -1, -1, -1, ControllerButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER)
#else
        0, 1, 2, 3, 4, 5, 6, -1, -1, -1, -1
#endif
    };
#if defined(GCW)
    if (p == 0) {
        const int gcwButtons[] = {3, 2, 1, 6, 7, 4, 0};
        std::copy(std::begin(gcwButtons), std::end(gcwButtons), pads + AKTION_JUMP);
    }
#endif
    for (int action = 0; action < oldActionCount; ++action) {
        const auto key = InputBinding::Key(keys[p][action]);
        layouts.keyboard[action][0] = key;
        layouts.controller[action][0] = pads[action] < 0 ? key : InputBinding::Pad(pads[action]);
    }
    for (int action : {AKTION_LINKS, AKTION_RECHTS, AKTION_DUCKEN, AKTION_OBEN}) {
        const auto dir = DefaultPadDirection(action);
        layouts.controller[action] = {InputBinding::Pad(PadDirectionBinding(PadDirectionSource::DPad, dir)),
                                      InputBinding::Pad(PadDirectionBinding(PadDirectionSource::LeftStick, dir))};
    }
    layouts.keyboard[ACTION_LOOK_LEFT][0] = InputBinding::Key(p == 0 ? DIK_COMMA : DIK_V);
    layouts.keyboard[ACTION_LOOK_RIGHT][0] = InputBinding::Key(p == 0 ? DIK_PERIOD : DIK_B);
    layouts.controller[ACTION_LOOK_LEFT][0] = InputBinding::Pad(PadDirectionBinding(PadDirectionSource::RightStick, PadDirection::Left));
    layouts.controller[ACTION_LOOK_RIGHT][0] = InputBinding::Pad(PadDirectionBinding(PadDirectionSource::RightStick, PadDirection::Right));
    layouts.keyboard[ACTION_SUMMARY][0] = InputBinding::Key(p == 0 ? DIK_F1 : DIK_F2);
#if SDL_VERSION_ATLEAST(2, 0, 0)
    layouts.controller[ACTION_SUMMARY][0] = InputBinding::Pad(ControllerButton(SDL_CONTROLLER_BUTTON_BACK));
#else
    layouts.controller[ACTION_SUMMARY][0] = InputBinding::Pad(8);
#endif
    return layouts;
}

ControlLayouts UpgradeBindings(int p, const OldBindings &saved) {
    auto layouts = DefaultBindings(p);
    for (int action = 0; action < oldActionCount; ++action) {
        const auto &pair = saved[action];
        const bool movement = action <= AKTION_OBEN;
        const bool directWeapon = action >= AKTION_WAFFE_SPREAD && action <= AKTION_WAFFE_BOUNCE;
        const auto oldKey = (p == 1 && directWeapon) ? InputBinding{} : layouts.keyboard[action][0];
        const auto oldPad = movement ? InputBinding::Pad(AutomaticPadBinding(DefaultPadDirection(action))) :
                            directWeapon ? InputBinding{} : layouts.controller[action][0];
        const auto same = [](InputBinding a, InputBinding b) { return a.kind == b.kind && a.code == b.code; };
        if (same(pair[0], oldKey) && same(pair[1], oldPad))
            continue;
        // Preserve deliberate mixed pairs, two keys, two buttons, and empty actions.
        layouts.keyboard[action] = {};
        layouts.controller[action] = {};
        int keyCount = 0, count = 0;
        int automatic = -1;
        for (auto b : pair) {
            if (b.kind == BindingKind::Empty)
                continue;
            if (b.kind == BindingKind::Key)
                layouts.keyboard[action][keyCount++] = b;
            if (b.kind == BindingKind::Pad && b.code >= PadAutomaticUpBinding && b.code < FirstPadDiagonal)
                automatic = b.code;
            layouts.controller[action][count++] = b;
        }
        if (automatic >= 0 && count == 1) {
            auto dir = automatic == PadAutomaticUpBinding ? PadDirection::Up : static_cast<PadDirection>(automatic - PadAutomaticUpBinding - 1);
            layouts.controller[action] = {InputBinding::Pad(PadDirectionBinding(PadDirectionSource::DPad, dir)),
                                          InputBinding::Pad(PadDirectionBinding(PadDirectionSource::LeftStick, dir))};
        }
    }
    return layouts;
}
} // namespace

void CreateDefaultControlsConfig(int p) {
    if (p < 0 || p > 1)
        return;
    auto &player = Player[p];
    player.controls = DefaultBindings(p);
    player.JoystickIndex = p;
    player.ControlType = CONTROLTYPE_KEYBOARD;
    player.rumbleStrength = RumbleStrength::Off;
#if defined(GCW)
    player.ControlType = CONTROLTYPE_JOY;
    if (p == 0)
        player.JoystickIndex = DirectInput.GetInternalJoystickIndex();
#elif SDL_VERSION_ATLEAST(2, 0, 0)
    if (!CommandLineParams.Arcade)
        player.ControlType = CONTROLTYPE_AUTO;
#endif
}

void CreateDefaultConfig() {
    Protokoll << "Creating new configuration file" << std::endl;

    ActualLanguage = DEFAULT_LANG_FILE;
    bool language_loaded = LoadLanguage(ActualLanguage);
    if (!language_loaded) {
        Protokoll << "ERROR: Failed to find default language file." << std::endl;
        return;
    }

    SoundManager.SetVolumes(50, 60);

    CreateDefaultControlsConfig(0);  // Load default controls for Player 1
    CreateDefaultControlsConfig(1);  // Load default controls for Player 2
    DirectInput.AssignPlayerJoysticks();

    options_Detail = DETAIL_MAXIMUM;

#if defined(GCW)
    // Max detail is too much for GCW Zero:
    options_Detail = DETAIL_HIGH;
#endif  // GCW

    SaveConfig();
}

// --------------------------------------------------------------------------------------
// Konfiguration mit den Sound-Lautstärken laden
// Existiert diese Datei nicht, so werden die Lautstärken auf den
// Defaut Wert gesetzt
// --------------------------------------------------------------------------------------

bool LoadConfig() {
    std::string filename = g_config_ext + "/" + CONFIGFILE;

    std::ifstream Datei(filename, std::ifstream::binary);  // versuchen Datei zu öffnen

    if (!Datei)
        return false;

    // Spracheinstellung laden
    char lang[256] = {};
    if (!Datei.read(lang, sizeof(lang)))
        return false;
    lang[sizeof(lang) - 1] = '\0';
    ActualLanguage = lang;
    // DKS - Made language loading default back to english if saved language not found:
    bool language_loaded = LoadLanguage(ActualLanguage);
    if (!language_loaded) {
        Protokoll << "ERROR loading " << ActualLanguage << ", reverting to default language file." << std::endl;
        ActualLanguage = DEFAULT_LANG_FILE;
        LoadLanguage(ActualLanguage);
    }

    float Sound, Musik;

    // Daten für Sound und Musik-Lautstärke auslesen
    Datei.read(reinterpret_cast<char *>(&Sound), sizeof(Sound));
    if (!Datei.read(reinterpret_cast<char *>(&Musik), sizeof(Musik)))
        return false;
    SoundManager.SetVolumes(Sound, Musik);

    // Daten für Keyboard und Joystick auslesen
    LegacyControls old[2];
    bool oldWalkAxis[2]{};
    bool oldLookAxis[2]{};
    Datei.read(reinterpret_cast<char *>(&old[0].keys), legacyBindingBytes);
    Datei.read(reinterpret_cast<char *>(&old[0].pads), legacyBindingBytes);
    Datei.read(reinterpret_cast<char *>(&oldWalkAxis[0]), sizeof(bool));
    Datei.read(reinterpret_cast<char *>(&oldLookAxis[0]), sizeof(bool));

    Datei.read(reinterpret_cast<char *>(&old[1].keys), legacyBindingBytes);
    Datei.read(reinterpret_cast<char *>(&old[1].pads), legacyBindingBytes);
    Datei.read(reinterpret_cast<char *>(&oldWalkAxis[1]), sizeof(bool));
    Datei.read(reinterpret_cast<char *>(&oldLookAxis[1]), sizeof(bool));

    bool oldRumble = false;
    Datei.read(reinterpret_cast<char *>(&oldRumble), sizeof(oldRumble));

    // Sonstige Optionen laden
    Datei.read(reinterpret_cast<char *>(&options_Detail), sizeof(options_Detail));

    int oldJoystickMode[2] = {};
    Datei.read(reinterpret_cast<char *>(&Player[0].ControlType), sizeof(Player[0].ControlType));
    Datei.read(reinterpret_cast<char *>(&oldJoystickMode[0]), sizeof(oldJoystickMode[0]));
    // This field used to store a user-adjustable stick sensitivity.
    float oldSensitivity;
    Datei.read(reinterpret_cast<char *>(&oldSensitivity), sizeof(oldSensitivity));

    Datei.read(reinterpret_cast<char *>(&Player[1].ControlType), sizeof(Player[1].ControlType));
    Datei.read(reinterpret_cast<char *>(&oldJoystickMode[1]), sizeof(oldJoystickMode[1]));
    if (!Datei.read(reinterpret_cast<char *>(&oldSensitivity), sizeof(oldSensitivity)))
        return false;

    // Older files end here. Keep their bindings and use the original slot defaults.
    int joystickSlots[2] = {0, 1};
#if defined(GCW)
    joystickSlots[0] = DirectInput.GetInternalJoystickIndex();
#endif
    int savedSlots[2];
    const auto defaultRumble = oldRumble ? RumbleStrength::High : RumbleStrength::Off;
    RumbleStrength rumbleStrengths[2] = {defaultRumble, defaultRumble};
    if (Datei.read(reinterpret_cast<char *>(savedSlots), sizeof(savedSlots))) {
        for (int i = 0; i < 2; ++i)
            if (savedSlots[i] >= 0 && savedSlots[i] < MAX_JOYSTICKS)
                joystickSlots[i] = savedSlots[i];
        int32_t strengths[2];
        if (Datei.read(reinterpret_cast<char *>(strengths), sizeof(strengths))) {
            for (int i = 0; i < 2; ++i)
                if (strengths[i] >= 0 && strengths[i] < static_cast<int>(RumbleStrength::Count))
                    rumbleStrengths[i] = static_cast<RumbleStrength>(strengths[i]);
        }
    }
    int previousBindings[2][2];
    const bool hasPreviousWeapon = static_cast<bool>(
        Datei.read(reinterpret_cast<char *>(previousBindings), sizeof(previousBindings)));
    for (int i = 0; i < 2; ++i) {
        auto &player = Player[i];
        old[i].keys[AKTION_WAFFEN_PREV] = i == 0 ? DIK_BACK : DIK_E;
        old[i].pads[AKTION_WAFFEN_PREV] = -1;
        if (hasPreviousWeapon) {
            old[i].keys[AKTION_WAFFEN_PREV] = previousBindings[i][0];
            old[i].pads[AKTION_WAFFEN_PREV] = previousBindings[i][1];
        }
#if SDL_VERSION_ATLEAST(2, 0, 0) && !defined(GCW)
        else {
            // Older profiles may already use the left bumper for Shockwave.
            const auto *end = old[i].pads + AKTION_WAFFEN_PREV;
            for (const auto button : {SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_LEFTSTICK}) {
                const int binding = ControllerButton(button);
                if (std::find(static_cast<const int *>(old[i].pads), end, binding) == end) {
                    old[i].pads[AKTION_WAFFEN_PREV] = binding;
                    break;
                }
            }
        }
#endif
        player.JoystickIndex = joystickSlots[i];
        player.rumbleStrength = rumbleStrengths[i];

        // Convert the former group-wide stick selections into individual bindings.
        for (int action = AKTION_LINKS; action <= AKTION_UNTEN; ++action) {
            const bool useStick = action <= AKTION_DUCKEN ? oldWalkAxis[i] : oldLookAxis[i];
            if (useStick && !IsPadDirectionBinding(old[i].pads[action]))
                old[i].pads[action] = PadDirectionBinding(PadDirectionSource::LeftStick, DefaultPadDirection(action));
        }
        constexpr int legacyJumpWithUp = 2;
        int &jump = old[i].pads[AKTION_JUMP];
        if (oldJoystickMode[i] == legacyJumpWithUp && !IsPadDirectionBinding(jump)) {
            const int lookUp = old[i].pads[AKTION_OBEN];
            if (oldWalkAxis[i])
                jump = PadDirectionBinding(PadDirectionSource::LeftStick, PadDirection::Up);
            else
                jump = IsPadDirectionBinding(lookUp) ? lookUp : PadAutomaticUpBinding;
        }
    }
    // New bindings are appended so all older control blocks retain their original offsets.
    int cameraBindings[2][2];
    const bool hasCameraBinding = static_cast<bool>(
        Datei.read(reinterpret_cast<char *>(cameraBindings), sizeof(cameraBindings)));
    uint32_t format = 0;
    Datei.read(reinterpret_cast<char *>(&format), sizeof(format));
    ControlLayouts saved[] = {DefaultBindings(0), DefaultBindings(1)};
    OldBindings oldPairs[2];
    bool hasLayouts = format == playerLayoutFormat && Datei.read(reinterpret_cast<char *>(saved), sizeof(saved));
    if (format == previousLayoutFormat || format == separateAimFormat) {
        std::array<BindingPair, oldActionCount + 3> previous{};
        const int count = format == previousLayoutFormat ? oldActionCount + 1 : oldActionCount + 3;
        hasLayouts = true;
        for (auto &player : saved) {
            for (auto *layout : {&player.keyboard, &player.controller}) {
                if (!Datei.read(reinterpret_cast<char *>(previous.data()), count * sizeof(BindingPair))) {
                    hasLayouts = false;
                    break;
                }
                std::copy_n(previous.begin(), oldActionCount, layout->begin());
                (*layout)[ACTION_SUMMARY] = previous[count - 1];
            }
        }
    }
    bool hasPairs = format == twoSlotFormat && Datei.read(reinterpret_cast<char *>(oldPairs), sizeof(oldPairs));
    if (hasLayouts) {
        for (const auto &player : saved)
            for (const auto *layout : {&player.keyboard, &player.controller})
                for (const auto &pair : *layout)
                    for (auto binding : pair)
                        hasLayouts &= binding.Valid();
    }
    if (hasPairs) {
        for (const auto &player : oldPairs)
            for (const auto &action : player)
                for (auto binding : action)
                    hasPairs &= binding.Valid();
    }
    for (int p = 0; p < 2; ++p) {
        if (hasLayouts)
            Player[p].controls = saved[p];
        else
            Player[p].controls = UpgradeBindings(p, hasPairs ? oldPairs[p] :
                ImportOldControls(p, old[p], hasCameraBinding ? cameraBindings[p] : nullptr));
    }
    if (format != playerLayoutFormat) {
        // An existing remap has priority over a newly introduced camera default.
        for (int p = 0; p < 2; ++p)
            for (auto *layout : {&Player[p].controls.keyboard, &Player[p].controls.controller})
                for (int action : {ACTION_LOOK_LEFT, ACTION_LOOK_RIGHT})
                    for (int old = 0; old <= ACTION_SUMMARY; ++old)
                        for (auto binding : (*layout)[old])
                            if (binding.Overlaps((*layout)[action][0]))
                                (*layout)[action][0] = {};
    }
    DirectInput.AssignPlayerJoysticks();

    Protokoll << "Config file loading successful !" << std::endl;
    PartikelSystem.SetParticleCount();

    return true;
}

// --------------------------------------------------------------------------------------
// Aktuelle Konfiguration mit den Sound-Lautstärken speichern
// --------------------------------------------------------------------------------------

void SaveConfig() {
    namespace fs = std::filesystem;
    const auto destination = fs::u8path(g_config_ext) / CONFIGFILE;
    const auto serial = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto temporary = destination.parent_path() / ("controls-" + std::to_string(serial) + ".tmp");
    std::ofstream Datei(temporary, std::ios::binary);

    if (!Datei) {
        Protokoll << "Config file saving failed !" << std::endl;
        return;
    }

    // Spracheinstellung speichern
    char lang[256] = {};
    ActualLanguage.copy(lang, sizeof(lang) - 1);
    Datei.write(reinterpret_cast<char *>(&lang), sizeof(lang));

    // Daten für Sound und Musik-Lautstärke schreiben
    float Sound = SoundManager.g_sound_vol;
    float Musik = SoundManager.g_music_vol;

    Datei.write(reinterpret_cast<char *>(&Sound), sizeof(Sound));
    Datei.write(reinterpret_cast<char *>(&Musik), sizeof(Musik));

    // Daten für Keyboard und Joystick schreiben
    // Keep the original configuration layout for existing profiles.
    const LegacyControls old[] = {ExportLegacyControls(Player[0].controls), ExportLegacyControls(Player[1].controls)};
    constexpr bool automaticDirection = false;
    constexpr int legacyJumpWithButton = 1;
    constexpr float legacySensitivity = JoystickDeadzone;
    Datei.write(reinterpret_cast<const char *>(&old[0].keys), legacyBindingBytes);
    Datei.write(reinterpret_cast<const char *>(&old[0].pads), legacyBindingBytes);
    Datei.write(reinterpret_cast<const char *>(&automaticDirection), sizeof(automaticDirection));
    Datei.write(reinterpret_cast<const char *>(&automaticDirection), sizeof(automaticDirection));

    Datei.write(reinterpret_cast<const char *>(&old[1].keys), legacyBindingBytes);
    Datei.write(reinterpret_cast<const char *>(&old[1].pads), legacyBindingBytes);
    Datei.write(reinterpret_cast<const char *>(&automaticDirection), sizeof(automaticDirection));
    Datei.write(reinterpret_cast<const char *>(&automaticDirection), sizeof(automaticDirection));

    const bool legacyRumble = Player[0].rumbleStrength != RumbleStrength::Off || Player[1].rumbleStrength != RumbleStrength::Off;
    Datei.write(reinterpret_cast<const char *>(&legacyRumble), sizeof(legacyRumble));

    // Sonstige Optionen sichern
    Datei.write(reinterpret_cast<char *>(&options_Detail), sizeof(options_Detail));

    Datei.write(reinterpret_cast<char *>(&Player[0].ControlType), sizeof(Player[0].ControlType));
    Datei.write(reinterpret_cast<const char *>(&legacyJumpWithButton), sizeof(legacyJumpWithButton));
    Datei.write(reinterpret_cast<const char *>(&legacySensitivity), sizeof(legacySensitivity));

    Datei.write(reinterpret_cast<char *>(&Player[1].ControlType), sizeof(Player[1].ControlType));
    Datei.write(reinterpret_cast<const char *>(&legacyJumpWithButton), sizeof(legacyJumpWithButton));
    Datei.write(reinterpret_cast<const char *>(&legacySensitivity), sizeof(legacySensitivity));

    const int joystickSlots[] = {Player[0].JoystickIndex, Player[1].JoystickIndex};
    Datei.write(reinterpret_cast<const char *>(joystickSlots), sizeof(joystickSlots));
    const int32_t rumbleStrengths[] = {static_cast<int32_t>(Player[0].rumbleStrength), static_cast<int32_t>(Player[1].rumbleStrength)};
    Datei.write(reinterpret_cast<const char *>(rumbleStrengths), sizeof(rumbleStrengths));
    const int previousBindings[][2] = {
        {old[0].keys[AKTION_WAFFEN_PREV], old[0].pads[AKTION_WAFFEN_PREV]},
        {old[1].keys[AKTION_WAFFEN_PREV], old[1].pads[AKTION_WAFFEN_PREV]}
    };
    Datei.write(reinterpret_cast<const char *>(previousBindings), sizeof(previousBindings));
    const int cameraBindings[][2] = {
        {old[0].keys[AKTION_UNTEN], old[0].pads[AKTION_UNTEN]},
        {old[1].keys[AKTION_UNTEN], old[1].pads[AKTION_UNTEN]}
    };
    Datei.write(reinterpret_cast<const char *>(cameraBindings), sizeof(cameraBindings));

    Datei.write(reinterpret_cast<const char *>(&playerLayoutFormat), sizeof(playerLayoutFormat));
    for (const auto &player : Player)
        Datei.write(reinterpret_cast<const char *>(&player.controls), sizeof(player.controls));

    Datei.close();
    std::error_code error;
    if (Datei)
        fs::rename(temporary, destination, error);
    if (!Datei || error) {
        Protokoll << "Could not save controls to " << destination << std::endl;
        fs::remove(temporary, error);
    }
}

// --------------------------------------------------------------------------------------
// Beim Loading Screen anzeigen, was alles geladen wurde, dabei
// mehrere Zeilen anzeigen und immer bei neuem Text eins nach oben rutschen
// --------------------------------------------------------------------------------------

bool DisplayLoadInfo(const char Text[100]) {
    if (NochKeinFullScreen || pMenu == nullptr || !pMenu->UpdateLoadingProgress(Text[0] == '\0'))
        return false;

    DirectGraphics.ClearBackBuffer();

    pMenu->ShowMenuBack();

    DirectGraphics.SetAdditiveMode();

    pMenuFont->DrawTextCenterAlign(320, 200, TextArray[TEXT::MENUE_LOADING], 0xFFFFFFFF);

    // Anzahl anzeigen
#ifndef NDEBUG
    std::string buf = std::to_string(pMenu->ItemsLoaded());

    pDefaultFont->DrawText((RENDERWIDTH - pDefaultFont->StringLength(buf.c_str())) / 2.0f, 300,
                           buf.c_str(), 0xFFFFFFFF);
#endif

    // Hint anzeigen
    // DKS - Added support for displaying hints on low-resolution devices:
    if (DisplayHintNr > -1) {
        if (CommandLineParams.LowRes) {
            const char *text = TextArray[TEXT::HINT1 + DisplayHintNr];
            constexpr float Y_POS = 270.0f;
            constexpr float Y_INC = 28.0f;
            constexpr int MAX_WIDTH = RENDERWIDTH - 20;
            if (pDefaultFont->StringLength(text, 0) > MAX_WIDTH) {
                // Split the line in two if too long to display on low-res device:
                char text1[255];
                char text2[255];
                SplitLine(text1, text2, text);
                pDefaultFont->DrawTextCenterAlign(320.0f, Y_POS, text1, 0xFFFFFFFF, 0);
                pDefaultFont->DrawTextCenterAlign(320.0f, Y_POS + Y_INC, text2, 0xFFFFFFFF, 0);
            } else {
                pDefaultFont->DrawTextCenterAlign(320.0f, Y_POS, text, 0xFFFFFFFF, 0);
            }
        } else {
            pDefaultFont->DrawTextCenterAlign(320.0f, 270.0f, TextArray[TEXT::HINT1 + DisplayHintNr], 0xFFFFFFFF, 0);
        }
    }

    pMenu->DrawProgressBar();

    DirectGraphics.ShowBackBuffer();

    Timer.wait(1);
    Timer.update();

    return true;
}

// --------------------------------------------------------------------------------------
// Stage Clear Musik dudelt und Spieler läuft aus dem Screen raus
// --------------------------------------------------------------------------------------

void StageClear(bool PlaySong) {
    for (int p = 0; p < NUMPLAYERS; p++) {
        Player[p].StageClearRunning = true;
        Player[p].CanBeDamaged = false;
        Player[p].PunisherActive = false;
    }

    GUI.HideBoxFast();

    // DKS - Stop all songs instead of just individual ones..
    // SoundManager.StopSong(MUSIC::STAGEMUSIC, false);

    //// Punisher Musik stoppen
    // if (MUSIC::IsPlaying(SoundManager.its_Songs[MUSIC::PUNISHER]->SongData)) {
    //    SoundManager.StopSong(MUSIC::PUNISHER, false);
    //}

    SoundManager.StopSongs();  // DKS - Added, see above

    if (PlaySong)
        SoundManager.PlaySong(MUSIC::STAGECLEAR, false);
}

// --------------------------------------------------------------------------------------
// Summary Screen anzeigen
// --------------------------------------------------------------------------------------

// DKS - Fixed display on low-res scaled-font devices, and made display on
//      normal-resolution devices be centered and spaced properly.
void RecordPowerBlock(PlayerClass *finder) {
    ++Player[0].BlocksFullGame;
    if (finder)
        ++finder->BlocksThisLevel;
    else
        ++unclaimedBlocks;
}

void SummaryScreen() {
    summaryOpacity = {};
    const bool reveal_cheat = !RunningTutorial && Stage > 0 && Stage <= MAX_CHEATS &&
        TileEngine.MaxDiamonds > 0 && SummaryCounts(-1)[1] == TileEngine.MaxDiamonds;

    // DKS - Added counter to prevent accidental early-exit:
    constexpr float DELAY_CAN_LEAVE = 400.0f;
    constexpr float DELAY_INC = 30.0f;
    float delay_ctr = 0.0f;

    ShowSummary = true;

    const SummaryLayout layout(1.0f, true, reveal_cheat);
    const auto frame = layout.CenteredAt({RENDERWIDTH / 2, RENDERHEIGHT / 2});
    GUI.ShowSummaryFrame(frame);

    bool all_controls_unpressed_yet = false;
    bool leave = false;
    while (!leave && GameRunning) {
        ProcessEvents();
        if (!GameRunning)
            break;

        TileEngine.CalcRenderRange();
        if (DirectGraphics.GetViewWidth() > RENDERWIDTH)
            DirectGraphics.ClearBackBuffer();
        DirectGraphics.SetHorizontalView(TileEngine.ViewLeft());
        TileEngine.UpdateCoopViews();
        TileEngine.UpdateBackground();
        TileEngine.DrawBackground();  // Level abhandeln
        TileEngine.DrawBackLevel();
        TileEngine.DrawFrontLevel();

        Gegner.RunAll();
        Gegner.RenderAll();
        Projectiles.DoProjectiles();  // Schüsse abhandeln

        // Overlay Tiles des Levels zeigen und Spieler und Objekte verdecken
        TileEngine.DrawOverlayLevel();
        PartikelSystem.DoPartikel();  // Partikel abhandeln

        TileEngine.DrawWater();
        TileEngine.DrawBackLevelOverlay();
        TileEngine.DrawOverlayLevel();
        TileEngine.DrawShadow();

        DirectGraphics.SetInterfaceView();
        HUD.DoHUD();  // HUD anhandeln

        // Blitz und andere Partikel rendern, die alles überlagern
        PartikelSystem.DoThunder();

        // Summary Screen rendern
        GUI.Run();
        D3DCOLOR color = D3DCOLOR_RGBA(0, 255, 0, static_cast<int>(GUI.GetFadingAlpha()));
        DrawSummaryContents(-1, frame, layout, color, delay_ctr >= DELAY_CAN_LEAVE, reveal_cheat);

        // Darstellung beenden

        // Backbuffer mit Frontbuffer tauschen
        DirectGraphics.ShowBackBuffer();

        Timer.update();
        Timer.wait();

        DirectInput.UpdateTastatur();
        DirectInput.UpdateJoysticks();
        bool keypressed = DirectInput.AnyKeyDown();
        bool buttonpressed = DirectInput.AnyButtonDown();

        if (!keypressed && !buttonpressed) {
            all_controls_unpressed_yet = true;
        }

        if (all_controls_unpressed_yet && delay_ctr >= DELAY_CAN_LEAVE) {
            if (buttonpressed || keypressed)
                leave = true;
        }

        // Musik zuende ?
        // DKS - Theere's no need to stop the song, as it isn't looped
        // if (MUSIC::IsFinished(SoundManager.its_Songs[MUSIC::STAGECLEAR]->SongData))
        //    SoundManager.StopSong(MUSIC::STAGECLEAR, false);

        delay_ctr += Timer.sync(DELAY_INC);
        if (delay_ctr > DELAY_CAN_LEAVE)
            delay_ctr = DELAY_CAN_LEAVE;
    }

    //    //DKS - I revamped this summary screen code quite a bit so that no delay should ever be necessary
    //    //    SDL_Delay( 3000 );
    while (GameRunning && SoundManager.SongIsPlaying(MUSIC::STAGECLEAR)) {
        ProcessEvents();
        SDL_Delay(8);
    }

    // DKS - There's no need to stop it, it will stop on its own as it's not looped:
    // SoundManager.StopSong(MUSIC::STAGECLEAR, false);

    GUI.HideBoxFast();

    ShowSummary = false;
}

// --------------------------------------------------------------------------------------
// Demo Aufnahme starten
// --------------------------------------------------------------------------------------

bool NewDemo(const char Filename[]) {
    std::string fullpath = g_save_ext + "/" + Filename;

    DEMOFile.open(fullpath, std::ofstream::binary);

    if (!DEMOFile) {
        Protokoll << "\n-> Error opening Demo File !" << std::endl;
        GameRunning = false;
        return false;
    }

    // DateiHeader schreiben
    //
    DEMOFile.write(reinterpret_cast<const char *>(&DEMO_ID), sizeof(DEMO_ID));

    // Stage Nr schreiben
    DEMOFile.write(reinterpret_cast<char *>(&Stage), sizeof(Stage));

    DEMORecording = true;
    DEMOPress = 0;

    // Tasten auf false setzen
    Player[0].Aktion.reset();

    // Level neu initialisieren und dann gehts los
    int const l = Stage;

    srand(5);

    InitNewGame();
    Stage = l;
    NewStage = l;
    InitNewGameLevel();

    // Timer auf 60 fps für Demo setzen
    Timer.SetMaxFPS(40);

    return true;

}  // NewDemo

// --------------------------------------------------------------------------------------
// Demo Laden
// --------------------------------------------------------------------------------------

bool LoadDemo(const char Filename[]) {
    TileEngine.XOffset = 0;
    TileEngine.YOffset = 0;

    std::string fullpath = g_save_ext + "/" + Filename;

    DEMOFile.open(fullpath, std::fstream::in | std::fstream::binary);

    if (!DEMOFile)
        return false;

    char Kennung[20];

    // DateiHeader lesen
    //
    DEMOFile.read(reinterpret_cast<char *>(&Kennung), sizeof(DEMO_ID));

    if (strcmp(DEMO_ID, Kennung) != 0)
        return false;

    // Stage Nr laden
    DEMOFile.read(reinterpret_cast<char *>(&Stage), sizeof(Stage));
    NewStage = Stage;

    DEMOPlaying = true;
    DEMOPress = 0;

    // Tasten auf false setzen
    Player[0].Aktion.reset();

    // Level neu initialisieren und dann gehts los
    int const l = NewStage;

    srand(5);

    InitNewGame();
    Stage = l;
    NewStage = l;
    InitNewGameLevel();

    // Timer auf 60 fps für Demo setzen
    Timer.SetMaxFPS(40);

    return true;

}  // LoadDemo

// --------------------------------------------------------------------------------------
// Demo beenden
// --------------------------------------------------------------------------------------

void EndDemo() {
    // File schliessen
    DEMOFile.close();

    DEMOPlaying = false;
    DEMORecording = false;

    Timer.SetMaxFPS(0);

    // DKS - added this to ensure RNG is always properly seeded
    srand(SDL_GetTicks());

}  // EndDemo

// --------------------------------------------------------------------------------------
// Demo aufnehmen
// --------------------------------------------------------------------------------------

void RecordDemo() {
    /*srand (DEMOPress++);
    if (DEMOPress > 32000)
        DEMOPress = 0;*/

    // Tasten speichern
    //
    for (int i = 0; i < AKTION_WAFFEN_PREV; i++) {
        char aktion = Player[0].Aktion[i];
        if (i == AKTION_WAFFEN_CYCLE && Player[0].Aktion[AKTION_WAFFEN_PREV])
            aktion |= 2;
        DEMOFile.write(reinterpret_cast<char *>(&aktion), sizeof(aktion));
    }

    // FPS speichern
    //
    int fps = static_cast<int>(Timer.getFrameRate());
    DEMOFile.write(reinterpret_cast<char *>(&fps), sizeof(fps));

}  // RecordDemo

// --------------------------------------------------------------------------------------
// Demo abspielen
// --------------------------------------------------------------------------------------

void PlayDemo() {
    /*srand (DEMOPress++);
    if (DEMOPress > 32000)
        DEMOPress = 0;*/

    // Tasten laden
    //
    for (int i = 0; i < AKTION_WAFFEN_PREV; i++) {
        char aktion = 0;
        DEMOFile.read(reinterpret_cast<char *>(&aktion), sizeof(aktion));
        if (i == AKTION_OBEN) {
            auto &actions = Player[0].Aktion;
            // BND2-era demos could aim independently of their recorded movement.
            const bool left = (aktion & 8) ? bool(aktion & 2) : (aktion && actions[AKTION_LINKS]);
            const bool right = (aktion & 8) ? bool(aktion & 4) : (aktion && actions[AKTION_RECHTS]);
            Player[0].demoAimDirection = int(right) - int(left);
            aktion &= 1;
        }
        if (i == AKTION_WAFFEN_CYCLE) {
            Player[0].Aktion[AKTION_WAFFEN_PREV] = (aktion & 2) != 0;
            aktion &= 1;
        }
        Player[0].Aktion[i] = aktion != 0;
    }

    // FPS laden
    //
    int fps;

    DEMOFile.read(reinterpret_cast<char *>(&fps), sizeof(fps));
    Timer.SetMaxFPS(fps);

    // Demo Ende ?
    if (DEMOFile.eof()) {
        EndDemo();
        LoadDemo("Demo.dem");
    }
}  // PlayDemo

// --------------------------------------------------------------------------------------
// Nach dem Boss den Screen wieder auf den Spieler zentrieren und Musik einfaden etc.
// --------------------------------------------------------------------------------------

void ScrolltoPlayeAfterBoss() {
    // Level wieder zum Spieler scrollen und dann weiterscrollen lassen
    TileEngine.Zustand = TileStateEnum::SCROLLBAR;
    TileEngine.MustCenterPlayer = true;

    // Level Musik wieder einfaden lassen (aus Pause Zustand)
    SoundManager.FadeSong(MUSIC::STAGEMUSIC, 2.0f, 100, true);
}

// --------------------------------------------------------------------------------------
// Zufällig ein Ziel aussuchen
// --------------------------------------------------------------------------------------

PlayerClass *ChooseAim() {

    PlayerClass *pAim = &Player[rand() % NUMPLAYERS];
    if (NUMPLAYERS == 1)
        return pAim;

    if (pAim == &Player[0] && Player[0].Handlung == PlayerActionEnum::TOT)
        pAim = &Player[1];

    if (pAim == &Player[1] && Player[1].Handlung == PlayerActionEnum::TOT)
        pAim = &Player[0];

    return pAim;
}

const char *convertText(const char *text) {
    // Text konvertieren
    static char temp[MAX_CHARS];

    unsigned int const len = strlen(text);
    unsigned int i = 0;
    for (i = 0; i < len; i++)
        temp[i] = text[i] ^ 64;
    temp[i] = 0;

    return temp;
}

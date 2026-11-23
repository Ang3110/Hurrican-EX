// Datei : Menu.cpp

// --------------------------------------------------------------------------------------
//
// Menu-Klasse für das
// Hurrican Hauptmenu
//
// (c) 2002 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Include Dateien
// --------------------------------------------------------------------------------------

#if !defined(__AROS__) && !defined(__MORPHOS__) && !defined(__amigaos4__)
#include <sys/timeb.h>
#endif
#include <stdio.h>
#include <time.h>
#include <array>
#include <cmath>
#include <string>
#include "Console.hpp"
#include "DX8Font.hpp"
#include "DX8Graphics.hpp"
#include "DX8Input.hpp"
#include "DX8Sound.hpp"
#include "Gameplay.hpp"
#include "GetKeyName.hpp"
#include "Logdatei.hpp"
#include "Main.hpp"
#include "Menu.hpp"
#include "Partikelsystem.hpp"
#include "Player.hpp"
#include "Timer.hpp"

// Farben für die Highscore erstellen, in der die Namen aufblinken
const std::array<D3DCOLOR, MAX_COLORS> HighscoreColors = {
    D3DCOLOR_RGBA(75, 75, 75, 255),
    D3DCOLOR_RGBA(105, 105, 105, 255),
    D3DCOLOR_RGBA(135, 135, 135, 255),
    D3DCOLOR_RGBA(165, 165, 165, 255),
    D3DCOLOR_RGBA(195, 195, 195, 255),
    D3DCOLOR_RGBA(225, 225, 225, 255),
    D3DCOLOR_RGBA(255, 255, 255, 255),
    D3DCOLOR_RGBA(255, 225, 225, 255),
    D3DCOLOR_RGBA(255, 195, 195, 255),
    D3DCOLOR_RGBA(255, 165, 165, 255),
    D3DCOLOR_RGBA(255, 135, 135, 255),
    D3DCOLOR_RGBA(255, 105, 105, 255),
    D3DCOLOR_RGBA(255, 75, 75, 255),
    D3DCOLOR_RGBA(255, 45, 45, 255),
    D3DCOLOR_RGBA(255, 15, 15, 255),
    D3DCOLOR_RGBA(255, 0, 0, 255),
    D3DCOLOR_RGBA(255, 15, 0, 255),
    D3DCOLOR_RGBA(255, 45, 0, 255),
    D3DCOLOR_RGBA(255, 75, 0, 255),
    D3DCOLOR_RGBA(255, 105, 0, 255),
    D3DCOLOR_RGBA(255, 135, 0, 255),
    D3DCOLOR_RGBA(255, 165, 0, 255),
    D3DCOLOR_RGBA(255, 195, 0, 255),
    D3DCOLOR_RGBA(255, 225, 0, 255),
    D3DCOLOR_RGBA(255, 255, 0, 255),
    D3DCOLOR_RGBA(225, 255, 0, 255),
    D3DCOLOR_RGBA(195, 255, 0, 255),
    D3DCOLOR_RGBA(165, 255, 0, 255),
    D3DCOLOR_RGBA(135, 255, 0, 255),
    D3DCOLOR_RGBA(105, 255, 0, 255),
    D3DCOLOR_RGBA(75, 255, 0, 255),
    D3DCOLOR_RGBA(45, 255, 0, 255),
    D3DCOLOR_RGBA(15, 255, 0, 255),
    D3DCOLOR_RGBA(0, 255, 0, 255),
    D3DCOLOR_RGBA(0, 255, 15, 255),
    D3DCOLOR_RGBA(0, 255, 45, 255),
    D3DCOLOR_RGBA(0, 255, 75, 255),
    D3DCOLOR_RGBA(0, 255, 105, 255),
    D3DCOLOR_RGBA(0, 255, 135, 255),
    D3DCOLOR_RGBA(0, 255, 165, 255),
    D3DCOLOR_RGBA(0, 255, 195, 255),
    D3DCOLOR_RGBA(0, 255, 225, 255),
    D3DCOLOR_RGBA(15, 255, 255, 255),
    D3DCOLOR_RGBA(45, 255, 255, 255),
    D3DCOLOR_RGBA(75, 255, 255, 255),
    D3DCOLOR_RGBA(105, 255, 255, 255),
    D3DCOLOR_RGBA(135, 255, 255, 255),
    D3DCOLOR_RGBA(165, 255, 255, 255),
    D3DCOLOR_RGBA(195, 255, 255, 255),
    D3DCOLOR_RGBA(225, 255, 255, 255),
    D3DCOLOR_RGBA(255, 255, 255, 255),
    D3DCOLOR_RGBA(255, 255, 225, 255),
    D3DCOLOR_RGBA(255, 240, 195, 255),
    D3DCOLOR_RGBA(255, 225, 165, 255),
    D3DCOLOR_RGBA(255, 210, 135, 255),
    D3DCOLOR_RGBA(255, 195, 105, 255),
    D3DCOLOR_RGBA(255, 180, 75, 255),
    D3DCOLOR_RGBA(255, 165, 45, 255),
    D3DCOLOR_RGBA(255, 165, 15, 255),
    D3DCOLOR_RGBA(255, 150, 0, 255),
    D3DCOLOR_RGBA(255, 135, 0, 255),
    D3DCOLOR_RGBA(255, 105, 0, 255),
    D3DCOLOR_RGBA(255, 75, 0, 255),
    D3DCOLOR_RGBA(255, 45, 0, 255),
    D3DCOLOR_RGBA(255, 15, 0, 255),
    D3DCOLOR_RGBA(255, 0, 0, 255),
    D3DCOLOR_RGBA(255, 0, 15, 255),
    D3DCOLOR_RGBA(255, 0, 45, 255),
    D3DCOLOR_RGBA(255, 0, 75, 255),
    D3DCOLOR_RGBA(255, 0, 105, 255),
    D3DCOLOR_RGBA(255, 0, 135, 255),
    D3DCOLOR_RGBA(255, 0, 165, 255),
    D3DCOLOR_RGBA(255, 0, 195, 255),
    D3DCOLOR_RGBA(255, 0, 225, 255),
    D3DCOLOR_RGBA(255, 0, 254, 255),
    D3DCOLOR_RGBA(225, 15, 255, 255),
    D3DCOLOR_RGBA(195, 45, 255, 255),
    D3DCOLOR_RGBA(165, 75, 255, 255),
    D3DCOLOR_RGBA(135, 105, 255, 255),
    D3DCOLOR_RGBA(105, 135, 255, 255),
    D3DCOLOR_RGBA(75, 165, 255, 255),
    D3DCOLOR_RGBA(45, 195, 255, 255),
    D3DCOLOR_RGBA(15, 225, 255, 255),
    D3DCOLOR_RGBA(0, 255, 225, 255),
    D3DCOLOR_RGBA(0, 255, 195, 255),
    D3DCOLOR_RGBA(0, 255, 165, 255),
    D3DCOLOR_RGBA(0, 255, 135, 255),
    D3DCOLOR_RGBA(0, 255, 105, 255),
    D3DCOLOR_RGBA(0, 255, 75, 255),
    D3DCOLOR_RGBA(0, 255, 45, 255),
    D3DCOLOR_RGBA(0, 255, 15, 255),
    D3DCOLOR_RGBA(0, 225, 0, 255),
    D3DCOLOR_RGBA(15, 195, 15, 255),
    D3DCOLOR_RGBA(30, 180, 30, 255),
    D3DCOLOR_RGBA(45, 165, 45, 255),
    D3DCOLOR_RGBA(60, 150, 60, 255),
    D3DCOLOR_RGBA(75, 135, 75, 255),
    D3DCOLOR_RGBA(75, 120, 75, 255),
    D3DCOLOR_RGBA(75, 105, 75, 255),
    D3DCOLOR_RGBA(75, 90, 75, 255),

    // Die ersten 20 nochmal ...

    D3DCOLOR_RGBA(75, 75, 75, 255),
    D3DCOLOR_RGBA(105, 105, 105, 255),
    D3DCOLOR_RGBA(135, 135, 135, 255),
    D3DCOLOR_RGBA(165, 165, 165, 255),
    D3DCOLOR_RGBA(195, 195, 195, 255),
    D3DCOLOR_RGBA(225, 225, 225, 255),
    D3DCOLOR_RGBA(255, 255, 255, 255),
    D3DCOLOR_RGBA(255, 225, 225, 255),
    D3DCOLOR_RGBA(255, 195, 195, 255),
    /* DKS - Commented out the rest of this, as it is writing past the end of the array: */
    /*
    D3DCOLOR_RGBA(255, 165, 165, 255),
    D3DCOLOR_RGBA(255, 135, 135, 255),
    D3DCOLOR_RGBA(255, 105, 105, 255),
    D3DCOLOR_RGBA(255,  75,  75, 255),
    D3DCOLOR_RGBA(255,  45,  45, 255),
    D3DCOLOR_RGBA(255,  15,  15, 255),
    D3DCOLOR_RGBA(255,   0,   0, 255),
    D3DCOLOR_RGBA(255,  15,   0, 255),
    D3DCOLOR_RGBA(255,  45,   0, 255),
    D3DCOLOR_RGBA(255,  75,   0, 255),
    D3DCOLOR_RGBA(255, 105,   0, 255),
    */
};

// --------------------------------------------------------------------------------------
// Die Credits
// --------------------------------------------------------------------------------------

namespace {
int pendingPlayers = 1;
int pendingSkill = SKILL_MEDIUM;
}

// --------------------------------------------------------------------------------------
// Konstruktor : Menu Grafiken laden und Werte initialisieren
// --------------------------------------------------------------------------------------

namespace {
constexpr int ControlMenuAction(int row) {
    constexpr int actions[] = {AKTION_LINKS, AKTION_RECHTS, AKTION_DUCKEN, AKTION_OBEN, AKTION_UNTEN,
                               ACTION_LOOK_LEFT, ACTION_LOOK_RIGHT, AKTION_JUMP, AKTION_SHOOT, AKTION_BLITZ,
                               AKTION_POWERLINE, AKTION_GRANATE, AKTION_SMARTBOMB, AKTION_WAFFEN_PREV,
                               AKTION_WAFFEN_CYCLE, AKTION_WAFFE_SPREAD, AKTION_WAFFE_LASER, AKTION_WAFFE_BOUNCE, ACTION_SUMMARY};
    static_assert(std::size(actions) == MENU_TASTEN_NUM_CONTROLS);
    if (row < MENU_TASTEN_NUM_NON_CONTROLS || row >= MENU_TASTEN_NUM_LINES)
        return -1;
    return actions[row - MENU_TASTEN_NUM_NON_CONTROLS];
}

const char *ControlActionName(int action) {
    switch (action) {
        case ACTION_LOOK_LEFT: return "look left (COOP)";
        case ACTION_LOOK_RIGHT: return "look right (COOP)";
        case ACTION_SUMMARY: return "summary board";
        case AKTION_WAFFEN_PREV: return "cycle weapons left";
        case AKTION_WAFFEN_CYCLE: return "cycle weapons right";
        case AKTION_WAFFE_SPREAD: return "select spread";
        case AKTION_WAFFE_LASER: return "select laser";
        case AKTION_WAFFE_BOUNCE: return "select bounce";
        default: return TextArray[TEXT::TASTEN_L + action];
    }
}

void MenuCell(DirectGraphicsFont *font, const char *text, float x, float y, float width, float height,
              D3DCOLOR color, int spacing, float align = 0.0f) {
    const float length = font->StringLength(text, spacing);
    const float scale = std::min(1.0f, width / std::max(1.0f, length));
    const auto saved = matProj;
    matProj = glm::translate(saved, glm::vec3(x + align * (width - length * scale),
                         y + (height - font->GetYCharSize() * scale) * 0.5f, 0));
    matProj = glm::scale(matProj, glm::vec3(scale, scale, 1));
#if defined(USE_GL1)
    load_matrix(GL_PROJECTION, glm::value_ptr(matProj));
#endif
    font->DrawText(0, 0, text, color, spacing);
    matProj = saved;
#if defined(USE_GL1)
    load_matrix(GL_PROJECTION, glm::value_ptr(matProj));
#endif
}
void DrawControlBindings(const PlayerClass &player, const DirectJoystickClass *pad, int action, float x, float y,
                         float width, int rowHeight, D3DCOLOR color) {
    std::string labels;
    for (const auto binding : player.BindingsFor(pad)[action]) {
        if (binding.kind == BindingKind::Empty) continue;
        if (!labels.empty()) labels += ", ";
        labels += binding.Label(pad);
    }
    MenuCell(pDefaultFont, labels.empty() ? "-" : labels.c_str(), x, y, width, rowHeight, color, 0);
}

std::string MenuPrompt(const DirectJoystickClass *pad, MenuPadButton action, int key) {
    return pad ? InputBinding::Pad(pad->MenuBinding(action)).Label(pad) : InputBinding::Key(key).Label(nullptr);
}
}  // namespace

MenuClass::MenuClass() {
    MenuNebel.LoadImage("hurrican_rund.png", 512, 512, 512, 512, 1, 1);
    MenuTitel.LoadImage("hurrican-logo.png", 400, 90, 400, 90, 1, 1);

    Skills.LoadImage("skills.png", 200, 200, 100, 100, 2, 2);

    MenuStar.LoadImage("star.png", 5, 5, 5, 5, 1, 1);
    MenuStar.SetRect(0, 0, 5, 5);

    // DKS - Made these members of MenuClass instead of globals in Main.cpp:
    LoadingScreen.LoadImage("loading.png", 360, 60, 360, 60, 1, 1);
    LoadingBar.LoadImage("loadingbar.png", 318, 19, 318, 19, 1, 1);
    LoadingProgress = 0;
    LoadingItemsToLoad = 1;
    LoadingItemsLoaded = 0;

    // Menu Werte initialisieren
    ScrollPos = 0.0f;

    xpos = (640 - 373) / 2;
    ypos = 80;

    Rotation = 0.0f;
    RotationDir = 1;

    AktuellerZustand = MENUZUSTAND_MAINMENU;
    AktuellerPunkt = MENUPUNKT_STARTGAME;

    // Sterne initialisieren
    for (auto& star: Stars) {
        star.Count = static_cast<float>(GetRandom(628)) / 100.0f;
        star.Abstand = static_cast<float>(GetRandom(400) + 20);
        star.Ebene = GetRandom(120) + 20;
    }

    BlinkOffset = 0;
    BlinkCounter = 0.75f;

    // Sprache feststellen und schauen, welche Grafik wir anzeigen müssen
    if (strcmp(TextArray[TEXT::BENUTZTE_GRAFIK], "0") == 0)
        Sprachgrafik = 0;  // deutsch ?
    else
        Sprachgrafik = 1;  // oder doch englisch ?

    // Highscore laden
    //
    LoadHighscore();

    ShowLanguageInfoCounter = 0.0f;
    control_reassignment_occuring = false;

    // DKS - Initialize NewName with just the cursor char
    strcpy_s(NewName, "_");
}

// --------------------------------------------------------------------------------------
// Konstruktor : Menu Grafiken freigeben
// --------------------------------------------------------------------------------------

MenuClass::~MenuClass() {
    // Aktuelle Highscoreliste speichern
    //
    SaveHighscore();
}

// --------------------------------------------------------------------------------------
// Hintergrund der Menüs anzeigen
// wird auch für den Progressbar benutzt
// --------------------------------------------------------------------------------------

void MenuClass::ShowMenuBack() {
    const int size = std::max(800, DirectGraphics.GetViewWidth() + 160);
    MenuNebel.RenderSpriteScaledRotated((RENDERWIDTH - size) / 2.0f, (RENDERHEIGHT - size) / 2.0f, size, size,
                                       ScrollPos, 0xFFFFFFFF);

    DirectGraphics.SetAdditiveMode();
    DirectGraphics.SetFilterMode(true);

    MenuStar.SetRect(0, 0, 5, 5);

    // Sterne anzeigen
    for (const auto& star: Stars)
        MenuStar.RenderSpriteRotated(320 - static_cast<float>(sin(star.Count) * star.Abstand),
                                     240 + static_cast<float>(cos(star.Count) * star.Abstand), star.Count,
                                     D3DCOLOR_RGBA(255, 255, 255, star.Ebene));

    DirectGraphics.SetColorKeyMode();
}

// --------------------------------------------------------------------------------------
// Menu anzeigen
// --------------------------------------------------------------------------------------

void MenuClass::ShowMenu() {

    int alpha = static_cast<int>(Rotation);
    D3DCOLOR menucolor = D3DCOLOR_RGBA(255, 255, 255, alpha);
    D3DCOLOR menucolor2 = D3DCOLOR_RGBA(255, 255, 255, alpha / 2);
    D3DCOLOR menucolor3 = D3DCOLOR_RGBA(255, 255, 255, alpha / 5);

    // Hintergrund rotieren lassen
    ScrollPos += Timer.sync(0.1f);

    clampAngle(ScrollPos);

    // Total löschen
    DirectGraphics.ClearBackBuffer();

    ShowMenuBack();

    const float viewLeft = DirectGraphics.InterfaceLeft();
    const int viewWidth = DirectGraphics.GetViewWidth();
    const float footerY = CommandLineParams.LowRes ? 458.0f : 462.0f;
    constexpr const char *releaseLabel = "Hurrican-EX 1.0";
    pDefaultFont->DrawText(viewLeft + 10.0f, footerY, "www.poke53280.de", menucolor, 0);
    pDefaultFont->DrawText(viewLeft + viewWidth - pDefaultFont->StringLength(releaseLabel, 0) - 10.0f,
                           footerY, releaseLabel, menucolor, 0);

    // Low-resolution mode has room for the two footer labels.
    if (CommandLineParams.LowRes) {
#if defined(GCW)
        // Draw port credit text for Pickle & Senquack
        if (AktuellerZustand == MENUZUSTAND_MAINMENU)
            pDefaultFont->DrawTextCenterAlign(320.0f, 428, MenuText("OpenGL and GCW Zero port by Pickle & Senquack"), menucolor,
                                              0);
#endif
    } else {
        if (AktuellerZustand != MENUPUNKT_CREDITS && AktuellerZustand != MENUZUSTAND_ENTERNAME) {
            const bool configuring = AktuellerZustand == MENUZUSTAND_TASTEN;
            const auto &selectedPlayer = Player[CurrentPlayer];
            const auto *joystick = configuring ? selectedPlayer.GetJoystick() : DirectInput.GetJoystick(lastMenuJoystick);
            if (!configuring && lastMenuJoystick == -1) joystick = Player[0].GetJoystick();
            std::string help = MenuText("menu help");
            ReplaceAll(help, "{nav}", MenuText(joystick ? "pad nav" : "key nav"));
            ReplaceAll(help, "{ok}", MenuPrompt(joystick, MenuPadButton::Select, DIK_RETURN));
            ReplaceAll(help, "{back}", MenuPrompt(joystick, MenuPadButton::Back, DIK_ESCAPE));
            const float inset = std::max(pDefaultFont->StringLength("www.poke53280.de", 0),
                                         pDefaultFont->StringLength(releaseLabel, 0)) + 16;
            MenuCell(pDefaultFont, help.c_str(), viewLeft + inset, footerY, viewWidth - 2*inset,
                     pDefaultFont->GetYCharSize(), menucolor, 0, 0.5f);
        }
    }

    RenderRect(viewLeft, 455, viewWidth, 1, menucolor2);

    // DKS - Do not draw the large "HURRICAN" title graphic when using scaled fonts and the
    //       controls reconfiguration menu is active - there is not enough room otherwise
    if (pDefaultFont->GetScaleFactor() <= 1 || AktuellerZustand != MENUZUSTAND_TASTEN) {
        MenuTitel.RenderSprite((640 - 400) / 2 + 15, 0, menucolor);
    }

    DirectGraphics.SetAdditiveMode();

    // Hintergrund des Menus anzeigen
    /*if (AktuellerZustand != MENUPUNKT_CREDITS &&
        AktuellerZustand != MENUZUSTAND_ENTERNAME)
        MenuBackground.RenderSprite(xpos, ypos, 0xFFFFFFFF);*/

    constexpr int OFFSET = 80;

    // Je nach Zustand das richtige Menu anzeigen
    switch (AktuellerZustand) {
        case MENUZUSTAND_MAINMENU: {
            constexpr int labels[] = {TEXT::MENUE_SPIEL_STARTEN, TEXT::MENUE_SPIEL_FORTSETZEN,
                TEXT::MENUE_EINSTELLUNGEN, TEXT::MENUE_HIGHSCORES, TEXT::MENUE_CREDITS,
                TEXT::MENUE_QUIT};
            int row = 0;
            for (int i = 0; i <= MENUPUNKT_END; ++i) {
                if (i == MENUPUNKT_CONTINUEGAME && Stage < 0) continue;
                pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET + (Stage < 0 ? 20 : 4) + row++ * 33, TextArray[labels[i]],
                                               i == AktuellerPunkt ? menucolor : menucolor2, 2);
            }
        } break;
        case MENUZUSTAND_STARTGAME: {
            pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET, TextArray[TEXT::MENUE_SPIEL_STARTEN], menucolor, 2);
            for (int row = 0; row < 2; ++row)
                pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET + (row + 2) * 35,
                    TextArray[row ? TEXT::MENUE_ALTES_SPIEL_FORTSETZEN : TEXT::MENUE_NEUES_SPIEL_STARTEN],
                    row == AktuellerPunkt ? menucolor : menucolor2, 2);
        } break;

        case MENUZUSTAND_VOLUMES: {
            constexpr int OFFSET2 = 20;

            float d = static_cast<float>(pMenuFont->StringLength(TextArray[TEXT::MENUE_EINSTELLUNGEN], 2));
            pMenuFont->DrawText(320 - d / 2.0f, ypos + OFFSET - OFFSET2, TextArray[TEXT::MENUE_EINSTELLUNGEN], menucolor,
                                2);

            // Sound / Musik Lautstärke
            for (int i = 0; i < 2; i++) {
                float d2 = static_cast<float>(pMenuFont->StringLength(TextArray[TEXT::SOUND + i], 2));

                if (i == 0)
                    LoadingBar.SetRect(0, 0, static_cast<int>(SoundManager.g_sound_vol / 100.0f * 318.0f), 19);
                else
                    LoadingBar.SetRect(0, 0, static_cast<int>(SoundManager.g_music_vol / 100.0f * 318.0f), 19);

                if (i == AktuellerPunkt) {
                    pMenuFont->DrawText(xpos + OFFSET - 20 - d2, ypos + OFFSET - OFFSET2 + (i + 2) * 35,
                                        TextArray[TEXT::SOUND + i], menucolor, 2);
                    LoadingScreen.RenderSprite(220, ypos + OFFSET - OFFSET2 + (i + 2) * 35 - 18, menucolor);
                    LoadingBar.RenderSprite(241, ypos + OFFSET - OFFSET2 + (i + 2) * 35 + 2, menucolor);
                } else {
                    pMenuFont->DrawText(xpos + OFFSET - 20 - d2, ypos + OFFSET - OFFSET2 + (i + 2) * 35,
                                        TextArray[TEXT::SOUND + i], menucolor2, 2);
                    LoadingScreen.RenderSprite(220, ypos + OFFSET - OFFSET2 + (i + 2) * 35 - 18, menucolor2);
                    LoadingBar.RenderSprite(241, ypos + OFFSET - OFFSET2 + (i + 2) * 35 + 2, menucolor2);
                }
            }

            // Sonstige Menu-Punkte anzeigen
            for (int i = 2; i < 4; i++) {
                float d2 = static_cast<float>(pMenuFont->StringLength(TextArray[TEXT::SOUND + i], 2));

                // Schrift anzeigen
                //
                if (i == AktuellerPunkt)
                    pMenuFont->DrawText(320.0f - d2 / 2.0f, ypos + OFFSET - OFFSET2 + (i + 2) * 35,
                                        TextArray[TEXT::SOUND + i], menucolor, 2);
                else
                    pMenuFont->DrawText(320.0f - d2 / 2.0f, ypos + OFFSET - OFFSET2 + (i + 2) * 35,
                                        TextArray[TEXT::SOUND + i], menucolor2, 2);
            }

            // Detailstufe
            d = static_cast<float>(pMenuFont->StringLength(TextArray[TEXT::DETAIL_LOW + options_Detail], 2));
            if (4 == AktuellerPunkt)
                pMenuFont->DrawText(320.0f - d / 2.0f, ypos + OFFSET - OFFSET2 + (4 + 2) * 35,
                                    TextArray[TEXT::DETAIL_LOW + options_Detail], menucolor, 2);
            else
                pMenuFont->DrawText(320.0f - d / 2.0f, ypos + OFFSET - OFFSET2 + (4 + 2) * 35,
                                    TextArray[TEXT::DETAIL_LOW + options_Detail], menucolor2, 2);

            if (DirectGraphics.SupportsDisplayMenu()) {
                const auto color = AktuellerPunkt == MENUPUNKT_VOLUMES_DISPLAY ? menucolor : menucolor2;
                pMenuFont->DrawTextCenterAlign(320.0f, ypos + OFFSET - OFFSET2 + (MENUPUNKT_VOLUMES_DISPLAY + 2) * 35,
                                               MenuText("DISPLAY"), color, 2);
            }

        } break;  // MENUZUSTAND_VOLUMES

        case MENUZUSTAND_DISPLAY: {
            constexpr const char *aspects[] = {"AUTO", "16:9", "16:10", "4:3", "STRETCHED 4:3"};
            static_assert(std::size(aspects) == static_cast<size_t>(DisplayAspect::Count));
            constexpr const char *labels[] = {"WINDOW MODE", "WIDESCREEN", "SPLITSCREEN TYPE", "RESOLUTION",
                                              "VSYNC", "TEXTURE FILTERING", "CRT FILTER", "BRIGHTNESS"};
            static_assert(std::size(labels) == DISPLAY_APPLY);
            const std::string resolution = pendingDisplay.width > 0
                ? std::to_string(pendingDisplay.width) + " X " + std::to_string(pendingDisplay.height)
                : (pendingDisplay.fullscreen ? "DESKTOP" : "DEFAULT");
            const char *values[] = {pendingDisplay.fullscreen ? "FULLSCREEN" : "WINDOWED",
                aspects[static_cast<int>(pendingDisplay.aspect)],
                pendingDisplay.splitScreenType == SplitScreenType::Dynamic ? "DYNAMIC" : "ONE SCREEN",
                resolution.c_str(), pendingDisplay.vsync ? "ON" : "OFF",
                pendingDisplay.textureFiltering ? "ON" : "OFF",
                DirectGraphics.SupportsCrtFilter() ? (pendingDisplay.crt ? "ON" : "OFF") : "UNAVAILABLE"};
            static_assert(std::size(values) == DISPLAY_BRIGHTNESS);

            pMenuFont->DrawTextCenterAlign(320, 124, MenuText("DISPLAY"), menucolor, 2);
            const auto output = DirectGraphics.GetOutputResolution();
            const std::string displayInfo = std::to_string(output.width) + " X " + std::to_string(output.height);
            pDefaultFont->DrawTextCenterAlign(320, 152, displayInfo.c_str(), menucolor, 0);

            const float margin = viewWidth * 0.12f;
            const float labelX = viewLeft + margin;
            const float valueRight = viewLeft + viewWidth - margin;
            constexpr float firstRow = 168.0f, rowHeight = 27.0f;
            for (int row = DISPLAY_MODE; row < DISPLAY_APPLY; ++row) {
                const float y = firstRow + row * rowHeight;
                const auto color = row == AktuellerPunkt ? menucolor : menucolor2;
                const float valueWidth = std::max(216.0f, (valueRight - labelX) * 0.42f);
                MenuCell(pMenuFont, MenuText(labels[row]), labelX, y, valueRight-labelX-valueWidth-8, pMenuFont->GetYCharSize(), color, 2);
                if (row == DISPLAY_BRIGHTNESS) {
                    // Use the same frame and white fill as the sound and music controls.
                    constexpr float scale = 0.60f;
                    const float x = std::round(valueRight - 360 * scale);
                    const float top = std::round(y + (pMenuFont->GetYCharSize() - 60 * scale) * 0.5f);
                    const int fill = 318 * (pendingDisplay.brightnessLevel + 1) / DisplaySettings::brightnessGains.size();
                    LoadingScreen.RenderSpriteScaled(x, top, 216, 36, color);
                    LoadingBar.SetRect(0, 0, fill, 19);
                    LoadingBar.RenderSpriteScaled(x + 13, top + 12, static_cast<int>(std::round(fill * scale)), 12, color);
                } else
                    MenuCell(pMenuFont, MenuText(values[row]), valueRight-valueWidth, y, valueWidth, pMenuFont->GetYCharSize(), color, 2, 1);
            }

            pMenuFont->DrawTextCenterAlign(320, firstRow + DISPLAY_APPLY * rowHeight + 4.0f, MenuText("APPLY"),
                                           AktuellerPunkt == DISPLAY_APPLY ? menucolor : menucolor2, 2);
            if (displayStatus && SDL_GetTicks() - displayStatusStarted < 6500u) {
                pDefaultFont->DrawTextCenterAlign(320, 422, MenuText(displayStatus), menucolor, 0);
                if (showVsyncWarning)
                    pDefaultFont->DrawTextCenterAlign(320, 436, MenuText("VSync unavailable on this driver"), menucolor, 0);
            } else if (pendingDisplay.vsync && DirectGraphics.GetDisplaySettings().vsync && !DirectGraphics.IsVSyncEnabled())
                pDefaultFont->DrawTextCenterAlign(320, 422, MenuText("VSync unavailable on this driver"), menucolor, 0);
        } break;

        case MENUZUSTAND_LANGUAGE: {
            // DKS - Menu now displays filenames without path or extensions and supports low-resolution
            int vertical_spacing = 16;
            if (CommandLineParams.LowRes) {
                vertical_spacing += 8;
            }

            float d = static_cast<float>(pMenuFont->StringLength(TextArray[TEXT::SPRACHE], 2));
            pMenuFont->DrawText(320 - d / 2.0f, ypos + OFFSET, TextArray[TEXT::SPRACHE], menucolor, 2);

            char lang_name[256] = "";
            const int num_lang = LanguageFiles.size();
            for (int i = 0; i < num_lang; i++) {
                strcpy_s(lang_name, MenuText(LanguageFiles[i].c_str()));
                pDefaultFont->DrawText(320 - pDefaultFont->StringLength(lang_name) / 2.0f,
                                       ypos + 120 + i * vertical_spacing, lang_name, 0x88FFFFFF);
                if (AktuellerPunkt == i)
                    pDefaultFont->DrawText(320 - pDefaultFont->StringLength(lang_name) / 2.0f,
                                           ypos + 120 + i * vertical_spacing, lang_name, 0x88FFFFFF);
            }

            pDefaultFont->DrawText(320 - pDefaultFont->StringLength(TextArray[TEXT::ZURUECK]) / 2.0f,
                                   ypos + 136 + vertical_spacing * LanguageFiles.size(), (TextArray[TEXT::ZURUECK]),
                                   0x88FFFFFF);
            if (AktuellerPunkt == num_lang)
                pDefaultFont->DrawText(320 - pDefaultFont->StringLength(TextArray[TEXT::ZURUECK]) / 2.0f,
                                       ypos + 136 + vertical_spacing * LanguageFiles.size(), (TextArray[TEXT::ZURUECK]),
                                       0x88FFFFFF);

        } break;  // MENUZUSTAND_LANGUAGE

        case MENUZUSTAND_TASTEN: {
            const int scale = pDefaultFont->GetScaleFactor();
            const bool large = scale > 1;
            const int line = large ? 24 : 14;
            const int firstY = large ? 8 : 117;
            constexpr int firstAction = MENU_TASTEN_NUM_NON_CONTROLS;
            const int visible = large ? 13 : MENU_TASTEN_NUM_CONTROLS;
            const int scroll = std::clamp(AktuellerPunkt - firstAction - visible + 1, 0, MENU_TASTEN_NUM_CONTROLS - visible);
            const auto rowY = [&](int row) { return firstY + (row - (row >= firstAction ? scroll : 0)) * line; };
            const auto rowColor = [&](int row) { return AktuellerPunkt == row ? 0xFFFFFFFF : 0xBBFFFFFF; };
            const float left = std::max(viewLeft + 10.0f, -120.0f);
            const float right = std::min(viewLeft + viewWidth - 10.0f, 760.0f);
            const float width = right - left;
            const float columns[] = {left + 0.28f * width, left + 0.64f * width, right};
            constexpr const char *rumbleNames[] = {"OFF", "LOW", "MEDIUM", "HIGH"};
            if (!large)
                pMenuFont->DrawTextCenterAlign(320, 82, TextArray[TEXT::STEUERUNG], menucolor, 2);
            RenderRect(left, rowY(AktuellerPunkt) - 1, width, line, 0x24FFFFFF);
            RenderRect(columns[CurrentPlayer] - 3, rowY(AktuellerPunkt) - 1, columns[1] - columns[0] - 3, line, 0x5068A9EB);
            for (int row = 0; row < MENU_TASTEN_NUM_LINES; ++row) {
                if (row >= firstAction && (row < firstAction + scroll || row >= firstAction + scroll + visible))
                    continue;
                int action = ControlMenuAction(row);
                const char *labels[] = {"Reset controls", "Player", TextArray[TEXT::TYP], "Rumble"};
                const char *label = action >= 0 ? ControlActionName(action) : labels[row];
                const auto fit = [&](const std::string &text, float x, float end, D3DCOLOR color) {
                    MenuCell(pDefaultFont, text.c_str(), x, rowY(row), end-x-5, line, color, 0);
                };
                fit(MenuText(label), left + 2, columns[0], rowColor(row));
                for (int p = 0; p < 2; ++p) {
                    const auto &player = Player[p];
                    const auto *pad = player.GetJoystick();
                    const bool focused = p == CurrentPlayer && row == AktuellerPunkt;
                    const uint32_t color = focused || (p == CurrentPlayer && row == MENU_TASTEN_PLAYER_LINE) ? 0xFFFFFFFF : 0xBBFFFFFF;
                    if (action >= 0 && !(focused && control_reassignment_occuring)) {
                        DrawControlBindings(player, pad, action, columns[p], rowY(row), columns[p + 1] - columns[p] - 6, line, color);
                        continue;
                    }
                    std::string value;
                    if (action >= 0)
                        value = MenuText("Press input...");
                    else if (row == MENU_TASTEN_PLAYER_LINE)
                        value = TextArray[p == 0 ? TEXT::PLAYER_ONE : TEXT::PLAYER_TWO];
                    else if (row == MENU_TASTEN_TYPE_LINE) {
                        if (pad) value = std::to_string(player.JoystickIndex + 1) + ". " + pad->JoystickName;
                        else if (player.ControlType == CONTROLTYPE_JOY) value = std::string(MenuText("Pad")) + " " + std::to_string(player.JoystickIndex + 1) + " (" + MenuText("disconnected") + ")";
                        else value = TextArray[TEXT::KEYBOARD];
                        if (player.ControlType == CONTROLTYPE_AUTO) value = std::string(MenuText("AUTO")) + ": " + value;
                    } else if (row == MENU_TASTEN_RUMBLE_LINE)
                        value = MenuText(rumbleNames[static_cast<int>(player.rumbleStrength)]);
                    else
                        value = MenuText("Reset");
                    fit(value, columns[p], columns[p + 1], color);
                }
            }
            const auto &player = Player[CurrentPlayer];
            const auto *pad = player.GetJoystick();
            std::string hint = MenuText(control_reassignment_occuring ?
                (bindingNotice ? bindingNotice : "cancel binding") : "binding help");
            ReplaceAll(hint, "{nav}", MenuText(pad ? "pad sides" : "key sides"));
            ReplaceAll(hint, "{ok}", MenuPrompt(pad, MenuPadButton::Select, DIK_RETURN));
            ReplaceAll(hint, "{del}", MenuPrompt(pad, MenuPadButton::Remove, DIK_DELETE));
            ReplaceAll(hint, "{back}", MenuPrompt(pad, MenuPadButton::Pause, DIK_ESCAPE));
            MenuCell(pDefaultFont, hint.c_str(), left, large ? 429 : 440, width, line, 0xDDFFFFFF, 0, 0.5f);
            pDefaultFont->SetScaleFactor(scale);
        } break;  // MENUZUSTAND_TASTEN

        case MENUPUNKT_HIGHSCORES: {
            float d = static_cast<float>(pMenuFont->StringLength(TextArray[TEXT::MENUE_HIGHSCORES], 2));
            pMenuFont->DrawText(320 - d / 2.0f, ypos, TextArray[TEXT::MENUE_HIGHSCORES], menucolor, 2);

            // pMenuFont->DrawText(xpos-120,  ypos+55, TextArray [TEXT::HIGHSCORE_PLATZ],  0xFFFFFFFF, 2);
            pMenuFont->DrawText(xpos - 120, ypos + 55, TextArray[TEXT::HIGHSCORE_NAME], 0xFFFFFFFF, 2);
            pMenuFont->DrawTextRightAlign(xpos + 270, ypos + 55, TextArray[TEXT::HIGHSCORE_PUNKTE], 0xFFFFFFFF, 2);
            pMenuFont->DrawTextCenterAlign(xpos + 350, ypos + 55, TextArray[TEXT::HIGHSCORE_STAGE], 0xFFFFFFFF, 2);
            pMenuFont->DrawText(xpos + 420, ypos + 55, TextArray[TEXT::HIGHSCORE_SKILL], 0xFFFFFFFF, 2);

            for (int i = 0; i < MAX_HIGHSCORES; i++) {

                D3DCOLOR Color = HighscoreColors[i + BlinkOffset];

                //_itoa_s(i+1, Buffer, 10);
                // pMenuFont->DrawText(xpos-90, ypos+static_cast<float>(105+i*26), Buffer, Color, 2);

                pMenuFont->DrawText(xpos - 120, ypos + static_cast<float>(105 + i * 26), (strcmp(Highscores[i].Name, "!!! CHEATER !!!") == 0 ? MenuText("!!! CHEATER !!!") : Highscores[i].Name), Color, 2);

                std::string strbuf = std::to_string(Highscores[i].Score);
                pMenuFont->DrawTextRightAlign(xpos + 270, ypos + static_cast<float>(105 + i * 26), strbuf.c_str(), Color, 2);

                strbuf = std::to_string(Highscores[i].Stage);
                pMenuFont->DrawTextCenterAlign(xpos + 350, ypos + static_cast<float>(105 + i * 26), strbuf.c_str(), Color, 2);

                Skills.RenderSpriteScaled(xpos + 422, ypos + static_cast<float>(105 + i * 26), 30, 30, Highscores[i].Skill,
                                          0xFFFFFFFF);
            }

            // Highscore-Farben blinken lassen
            BlinkCounter -= Timer.getSpeedFactor();
            if (BlinkCounter < 0.0f) {
                BlinkCounter = 0.75f;
                BlinkOffset++;
                if (BlinkOffset > MAX_COLORS - MAX_HIGHSCORES)
                    BlinkOffset = 0;
            }

        } break;  // HIGHSCORE

        case MENUZUSTAND_ENTERNAME: {
            pMenuFont->DrawTextCenterAlign(320, ypos + 90, TextArray[TEXT::WAHNSINN], D3DCOLOR_RGBA(255, 255, 255, 255),
                                           2);

            pMenuFont->DrawTextCenterAlign(320, ypos + 180, TextArray[TEXT::NAMEN_EINGEBEN],
                                           D3DCOLOR_RGBA(255, 255, 255, 255), 2);

            std::string Buffer(TextArray[TEXT::NEUE_HIGHSCORE]);
            Buffer.append(std::to_string(NewRank + 1));

            pMenuFont->DrawTextCenterAlign(320, ypos + 150, Buffer.c_str(), D3DCOLOR_RGBA(255, 255, 255, 255), 2);
        } break;  // ENTERNAME

        case MENUPUNKT_CREDITS: {
            // DKS - There are now two credits text arrays, one original, one meant for low-res display
            // (LowResCredits[])
            //      Altered this code to allow for displaying on low-res devices with new font-scaling:

            float const scale_factor = pDefaultFont->GetScaleFactor();
            int const num_lines = 40 / pDefaultFont->GetScaleFactor();
            int const yoff_inc = 12 * pDefaultFont->GetScaleFactor();
            const auto &lines = CreditLines(viewWidth - 32);
            for (int i = 0; i < num_lines; ++i) {
                const int row = CreditsOffset + i - num_lines;
                if (row < 0 || row >= static_cast<int>(lines.size())) continue;
                int alpha2 = i * yoff_inc - static_cast<int>(CreditsPosition);

                if (alpha2 > 360) {
                    alpha2 = 255 - (alpha2 - 360) * 4;

                    if (alpha2 < 0)
                        alpha2 = 0;
                } else if (alpha2 < 150) {
                    alpha2 = 255 - (150 - alpha2) * 4;

                    if (alpha2 < 0)
                        alpha2 = 0;
                }

                else
                    alpha2 = 255;

                D3DCOLOR Color = D3DCOLOR_RGBA(255, 255, 255, alpha2);

                pDefaultFont->DrawTextCenterAlign(320.0f,
                                                  static_cast<float>(i * yoff_inc - static_cast<int>(CreditsPosition)),
                                                  lines[row].c_str(), Color, 0);
            }

            // Credits scrollen
            CreditsCounter -= Timer.sync(0.7f * scale_factor);  // Credits-Delay veringern

            // Bei Taste nach unten schneller scrollen
            //
            if (KeyDown(DIK_DOWN) || KeyDown(DIK_NUMPAD2))
                CreditsCounter -= Timer.sync(2.0f * scale_factor);

            // Bei Taste nach oben langsamer werden
            //
            if (KeyDown(DIK_UP) || KeyDown(DIK_NUMPAD8))
                CreditsCounter = (0.5f * scale_factor);

            while (CreditsCounter < 0.0f)  // Ein Pixel weiterscrollen ?
            {
                CreditsCounter += (0.5f * scale_factor);  // Dann neues Delay setzen
                CreditsPosition += 1.0f * scale_factor;   // und Pixel weiterscrollen
                if (CreditsPosition >= yoff_inc)          // 12 Pixelgrenze überschritten ?
                {
                    CreditsPosition = 0;  // Dann wieder zurücksetzen
                    CreditsOffset++;      // Und im Char-Array eine zeile weiter

                    // Credits zuende ?
                    // Dann von vorne beginnen
                    //
                    if (CreditsOffset >= num_lines + static_cast<int>(lines.size()))
                        CreditsOffset = 0;
                }
            }
        } break;  // CREDITS

        case MENUZUSTAND_TUTORIAL: {
            std::string question(TextArray[TEXT::MENUE_TUTORIAL_SPIELEN]);
            question.erase(question.find_last_not_of(" \r\n?") + 1);
            question += '?';
            const char *yes = MenuText("YES"), *no = MenuText("NO");
            pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET, question.c_str(), menucolor, 2);
            for (int i = 0; i < 2; ++i)
                pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET + (i + 2) * 35, i == 0 ? yes : no,
                                                i == AktuellerPunkt ? menucolor : menucolor2, 2);
        } break;

        case MENUZUSTAND_PLAYERCOUNT: {
            pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET, TextArray[TEXT::MENUE_SPIEL_STARTEN], menucolor, 2);

            for (int i = 0; i < 2; i++)
                if (AktuellerPunkt != i)
                    pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET + (i + 2) * 35, TextArray[TEXT::ONE_PLAYER + i],
                                                   menucolor2, 2);

            pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET + (AktuellerPunkt + 2) * 35,
                                           TextArray[TEXT::ONE_PLAYER + AktuellerPunkt], menucolor, 2);

        } break;

        case MENUZUSTAND_SELECTSKILL: {
            pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET, TextArray[TEXT::MENUE_SPIEL_STARTEN], menucolor, 2);

            for (int i = 0; i < 4; i++)
                if (AktuellerPunkt != i)
                    pMenuFont->DrawText(310, ypos + OFFSET + (i + 2) * 35, TextArray[TEXT::MENUE_LEICHT + i], menucolor2,
                                        2);

            pMenuFont->DrawText(310, ypos + OFFSET + (AktuellerPunkt + 2) * 35,
                                TextArray[TEXT::MENUE_LEICHT + AktuellerPunkt], menucolor, 2);

            // Skill anzeigen
            DirectGraphics.SetColorKeyMode();
            Skills.RenderSprite(190, ypos + 165, AktuellerPunkt, 0xFFFFFFFF);
            DirectGraphics.SetAdditiveMode();

        } break;

        case MENUZUSTAND_LOADGAME: {
            // "Continue saved game"
            if (pDefaultFont->GetScaleFactor() <= 1) {
                pMenuFont->DrawTextCenterAlign(320, ypos + OFFSET, TextArray[TEXT::MENUE_ALTES_SPIEL_FORTSETZEN],
                                               menucolor, 2);
            } else {
                // DKS - When using scaled fonts, display the text higher and using the small default font
                pDefaultFont->DrawTextCenterAlign(320, ypos + 20, TextArray[TEXT::MENUE_ALTES_SPIEL_FORTSETZEN],
                                                  menucolor, 2);
            }

            ShowSavegames(AktuellerPunkt);
        } break;  // Load Game

        case MENUZUSTAND_SAVEGAME: {
            // DKS - Altered to allow scaled fonts on low-resolution devices
            const int scale_factor = pDefaultFont->GetScaleFactor();
            int line_off_y = 15;
            int continue_text_off_y = ypos + 150;

            if (scale_factor > 1) {
                continue_text_off_y = ypos + 50 + 20;
                line_off_y = 24;
            }

            ShowSavegames(AktuellerPunkt);

            // "Save Game"
            if (scale_factor <= 1) {
                pMenuFont->DrawTextCenterAlign(320.0f, ypos + OFFSET, TextArray[TEXT::MENUE_SPIEL_SPEICHERN], 0xFFFFFFFF,
                                               2);
            } else {
                // Use the smaller default font since this we're using scaled fonts and need it to fit:
                pDefaultFont->DrawTextCenterAlign(320.0f, ypos + 20, TextArray[TEXT::MENUE_SPIEL_SPEICHERN], 0xFFFFFFFF,
                                                  2);
            }

            // Aktuelle gewähltes Savegame heller anzeigen
            // "Continue"
            if (scale_factor <= 1) {
                pMenuFont->DrawTextCenterAlign(320, continue_text_off_y + (MAX_SAVEGAMES + 1) * line_off_y,
                                               TextArray[TEXT::WEITER], 0x80FFFFFF, 2);
                if (AktuellerPunkt == MAX_SAVEGAMES)
                    pMenuFont->DrawTextCenterAlign(320, continue_text_off_y + (MAX_SAVEGAMES + 1) * line_off_y,
                                                   TextArray[TEXT::WEITER], 0xFFFFFFFF, 2);
            } else {
                // If using scaled fonts, use the smaller default font:
                pDefaultFont->DrawTextCenterAlign(320, continue_text_off_y + (MAX_SAVEGAMES + 1) * line_off_y,
                                                  TextArray[TEXT::WEITER], 0x80FFFFFF, 2);
                if (AktuellerPunkt == MAX_SAVEGAMES)
                    pDefaultFont->DrawTextCenterAlign(320, continue_text_off_y + (MAX_SAVEGAMES + 1) * line_off_y,
                                                      TextArray[TEXT::WEITER], 0xFFFFFFFF, 2);
            }
        } break;  // Save Game

        default:
            break;
    }

    DirectGraphics.SetColorKeyMode();
}

// --------------------------------------------------------------------------------------
// Alles machen, was da Menu betrifft, sprich, werte ändern und dann anzeigen
// --------------------------------------------------------------------------------------

void MenuClass::DoMenu() {
    // DKS - Modified extensively while adding joystick support and fixing navigation backwards to previous menus
    bool joy_up = false;
    bool joy_down = false;
    bool joy_left = false;
    bool joy_right = false;
    bool joy_enter = false;
    bool joy_escape = false;
    //    int joy_idx = Player[0].JoystickIndex;
    static float input_counter = 0.0f;
    constexpr float INPUT_DELAY = 40.0f;  // Only accept joy input once every time counter reaches this value
    input_counter += Timer.sync(30.0f);
    if (input_counter > INPUT_DELAY) {
        input_counter = INPUT_DELAY;
    }

    // Einheitsmatrix setzen, damit das Menu richtig angezeigt wird
    //
    glm::mat4x4 matView = glm::mat4x4(1.0f);
    g_matView = matView;

    // Wird noch keine Menu Musik gespielt ?
    //
    // DKS - Added function SongIsPlaying() to SoundManagerClass:
    // if (SoundManager.CurrentSongNr != MUSIC::MENU &&
    //        SoundManager.CurrentSongNr != MUSIC::HIGHSCORE &&
    //        SoundManager.CurrentSongNr != MUSIC::CREDITS)
    //    SoundManager.PlaySong(MUSIC::MENU, false);
    if (!SoundManager.SongIsPlaying(MUSIC::MENU) && !SoundManager.SongIsPlaying(MUSIC::HIGHSCORE) &&
        !SoundManager.SongIsPlaying(MUSIC::CREDITS))
        SoundManager.PlaySong(MUSIC::MENU, false);

    // Logo einfaden
    if (RotationDir > 0)
        Rotation += Timer.sync(static_cast<float>(RotationDir * 10));

    if (Rotation >= 255.0f) {
        Rotation = 255.0f;
        RotationDir = 0;
    }

    // Sterne bewegen
    //
    for (auto& star: Stars) {
        star.Count += Timer.sync(star.Ebene / 20000.0f);

        if (star.Count > TWO_PI)
            star.Count -= TWO_PI;
    }

    ShowMenu();

    if (Console.Showing)
        return;

    //----- Tasten abfragen

    bool selected = false;

    const bool musicSelected = AktuellerZustand == MENUZUSTAND_VOLUMES && AktuellerPunkt == MENUPUNKT_VOLUMES_MUSIC;
    if (!musicSelected)
        musicAdjustmentDirection = 0;

    bool menuControllersReleased = true;
    int navigationSlot = -1;
    for (int slot = DirectInput.NextJoystick(-1); slot >= 0; slot = DirectInput.NextJoystick(slot)) {
        const auto &joystick = DirectInput.Joysticks[slot];
        bool right = false, left = false, down = false, up = false;
        const int pov = joystick.JoystickPOV;
        if (pov != -1) {
            right = pov >= 4500 && pov <= 13500;
            left  = pov >= 22500 && pov <= 31500;
            down  = pov == 18000;
            up    = pov == 0;
        }
#if !defined(GCW)
        else if (std::abs(joystick.JoystickX) > JoystickDeadzone &&
                 std::abs(joystick.JoystickX) >= std::abs(joystick.JoystickY)) {
            right = joystick.JoystickX > 0;
            left  = joystick.JoystickX < 0;
        } else if (std::abs(joystick.JoystickY) > JoystickDeadzone) {
            down = joystick.JoystickY > 0;
            up   = joystick.JoystickY < 0;
        }
#endif
        const bool enter = joystick.ButtonEnterPressed();
        const bool escape = joystick.ButtonEscapePressed();
        const bool erase = joystick.ButtonDeletePressed();
        if (joystick.ButtonStartPressed())
            menuControllersReleased = false;
        if (!(right || left || down || up || enter || escape || erase))
            continue;

        menuControllersReleased = false;
        if (navigationSlot >= 0 || control_reassignment_occuring ||
            (input_counter < INPUT_DELAY && !(musicSelected && (left || right))))
            continue;
        navigationSlot = slot;
        lastMenuJoystick = slot;
        joy_right = right;
        joy_left = left;
        joy_down = down;
        joy_up = up;
        joy_enter = enter;
        joy_escape = escape;
    }

    if ((KeyDown(DIK_SPACE) || KeyDown(DIK_RETURN) || joy_enter) && AuswahlPossible && !control_reassignment_occuring) {
        input_counter = 0.0f;
        selected = true;
    }

    bool KeyOK = false;

    if (!KeyDown(DIK_NUMPAD4) && !KeyDown(DIK_NUMPAD6) && !KeyDown(DIK_NUMPAD2) && !KeyDown(DIK_NUMPAD8) &&
        !KeyDown(DIK_LEFT) && !KeyDown(DIK_RIGHT) && !KeyDown(DIK_UP) && !KeyDown(DIK_DOWN) && !KeyDown(DIK_RETURN) &&
        !KeyDown(DIK_SPACE) && !KeyDown(DIK_ESCAPE) &&
        (AktuellerZustand != MENUZUSTAND_TASTEN || (!KeyDown(DIK_DELETE) && !KeyDown(DIK_BACK)))) {
        KeyOK = true;
        if (CommandLineParams.Arcade) {
            for (int i = 0; i < 2; i++) {
                if (Player[i].ControlType != CONTROLTYPE_KEYBOARD)
                    continue;
                if (Player[i].ActionPressed(AKTION_RECHTS, nullptr) || Player[i].ActionPressed(AKTION_LINKS, nullptr) ||
                    Player[i].ActionPressed(AKTION_OBEN, nullptr) || Player[i].ActionPressed(AKTION_DUCKEN, nullptr) ||
                    Player[i].ActionPressed(AKTION_SHOOT, nullptr) || Player[i].ActionPressed(AKTION_WAFFEN_CYCLE, nullptr))
                    KeyOK = false;
            }
        }
    }

    if (!KeyOK && navigationSlot < 0) lastMenuJoystick = -2;
    if (KeyOK && menuControllersReleased && !control_reassignment_occuring)
        AuswahlPossible = true;

    if ((ShowLanguageInfoCounter > 256.0f && ShowLanguageInfoCounter < 1600.0f) &&
        (KeyDown(DIK_ESCAPE) || KeyDown(DIK_RETURN) || joy_escape || joy_enter)) {
        ShowLanguageInfoCounter = 256.0f;
        SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
        AuswahlPossible = false;
        input_counter = 0.0f;
    }

    // LanguageFile Info Fenster Counter runterzählen und Fenster ggf anzeigen
    //
    if (ShowLanguageInfoCounter > 0.0f) {
        ShowLanguageInfoCounter -= Timer.sync(50.0f);
        ShowLanguageInfo();
        return;  // Do not accept menu input until language counter reaches 0
    }

    if (AuswahlPossible && !control_reassignment_occuring && (KeyDown(DIK_NUMPAD8) || KeyDown(DIK_UP) || joy_up)) {
        input_counter = 0.0f;
        AuswahlPossible = false;
        AktuellerPunkt--;
        SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);

        // Spieler will auf "spiel fortsetzen" wechseln, aber es läuft noch kein spiel
        //
        if (Stage == -1 && AktuellerZustand == MENUZUSTAND_MAINMENU && AktuellerPunkt == 1) {
            AktuellerPunkt = 0;
        }

        if (AktuellerZustand == MENUZUSTAND_TASTEN) {
            if (AktuellerPunkt < 0)
                AktuellerPunkt = MENU_TASTEN_NUM_LINES - 1;
        }
    }

    if (AuswahlPossible && !control_reassignment_occuring && (KeyDown(DIK_NUMPAD2) || KeyDown(DIK_DOWN) || joy_down)) {
        input_counter = 0.0f;
        AuswahlPossible = false;
        AktuellerPunkt++;
        SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);

        // Spieler will auf "spiel fortsetzen" wechseln, aber es läuft noch kein spiel
        //
        if (Stage == -1 && AktuellerZustand == MENUZUSTAND_MAINMENU && AktuellerPunkt == 1) {
            AktuellerPunkt = 2;
        }

        if (AktuellerZustand == MENUZUSTAND_TASTEN) {
            if (AktuellerPunkt >= MENU_TASTEN_NUM_LINES)
                AktuellerPunkt = 0;
        }
    }

    if (CommandLineParams.Arcade) {
        for (int i = 0; i < 2; i++) {
            if (Player[i].ControlType != CONTROLTYPE_KEYBOARD)
                continue;
            if (Player[i].ActionPressed(AKTION_WAFFEN_CYCLE, nullptr))
                joy_escape = true;
        }
    }

    //----- Je nach Menu-Zustand die Aktionen auswerten

    switch (AktuellerZustand) {
        // Haupt-Menu
        case MENUZUSTAND_MAINMENU: {
            // zurück zum Game ?
            if ((KeyDown(DIK_ESCAPE) || joy_escape) && AuswahlPossible == true && Stage != -1) {
                input_counter = 0.0f;
                AktuellerPunkt = MENUPUNKT_CONTINUEGAME;
                selected = true;

                while (DirectInput.AreAllKeysReleased() == false)
                    DirectInput.UpdateTastatur();
            }

            if (AktuellerPunkt < MENUPUNKT_STARTGAME)
                AktuellerPunkt = MENUPUNKT_END;

            if (AktuellerPunkt > MENUPUNKT_END)
                AktuellerPunkt = MENUPUNKT_STARTGAME;

            // Menu-Punkt im Hauptmenu ausgewählt ?
            if (selected) {
                AuswahlPossible = false;
                switch (AktuellerPunkt) {
                    case MENUPUNKT_STARTGAME:
                        AktuellerZustand = MENUZUSTAND_STARTGAME;
                        AktuellerPunkt = 0;
                        break;
                    case MENUPUNKT_CONTINUEGAME:
                        if (Stage > -1) {
                            AktuellerPunkt = 0;
                            SoundManager.StopSong(MUSIC::MENU, false);
                            SpielZustand = GameStateEnum::GAMELOOP;
                            SoundManager.UnpauseSongs();
                            SoundManager.UnpauseSounds();
                            Player[0].PowerLinePossible = false;
                            Player[0].ShotDelay = 2.0f;
                        }
                        break;
                    case MENUPUNKT_VOLUMES:
                        AktuellerZustand = MENUZUSTAND_VOLUMES;
                        AktuellerPunkt = 0;
                        break;
                    case MENUPUNKT_HIGHSCORES:
                        SoundManager.SetSongVolume(MUSIC::HIGHSCORE, 0);
                        SoundManager.FadeSong(MUSIC::MENU, -5.0f, 0, false);
                        SoundManager.FadeSong(MUSIC::HIGHSCORE, 5.0f, 100, false);
                        AktuellerZustand = MENUPUNKT_HIGHSCORES;
                        AktuellerPunkt = MENUZUSTAND_ENTERNAME;
                        break;
                    case MENUPUNKT_CREDITS:
                        SoundManager.StopSong(MUSIC::MENU, false);
                        SoundManager.PlaySong(MUSIC::CREDITS, false);
                        SoundManager.SetSongVolume(MUSIC::CREDITS, 100);
                        CreditsCounter = 0.0f;
                        AktuellerZustand = MENUPUNKT_CREDITS;
                        break;
                    case MENUPUNKT_END:
                        GameRunning = false;
                        break;
                }
                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);

                // Highscore wieder von Anfang an blinken lassen
                BlinkOffset = 0;

                // Credits von Anfang an scrollen lassen
                CreditsOffset = 0;
                CreditsPosition = 0;
            }
        } break;  // MAINMENU

        // Sound / Musik Lautstärke wählen
        case MENUZUSTAND_VOLUMES: {
            const int last = DirectGraphics.SupportsDisplayMenu() ? MENUPUNKT_VOLUMES_DISPLAY : MENUPUNKT_VOLUMES_DETAILS;
            if (AktuellerPunkt < 0)
                AktuellerPunkt = last;
            if (AktuellerPunkt > last)
                AktuellerPunkt = 0;

            if (AktuellerPunkt == MENUPUNKT_VOLUMES_SOUND || AktuellerPunkt == MENUPUNKT_VOLUMES_MUSIC) {
                const bool lower = KeyDown(DIK_NUMPAD4) || KeyDown(DIK_LEFT) || joy_left;
                const bool raise = KeyDown(DIK_NUMPAD6) || KeyDown(DIK_RIGHT) || joy_right;
                if (AktuellerPunkt == MENUPUNKT_VOLUMES_MUSIC) {
                    const int direction = static_cast<int>(raise) - static_cast<int>(lower);
                    const Uint32 now = SDL_GetTicks();
                    if (direction != 0 && (direction != musicAdjustmentDirection || now - musicAdjustmentTime >= 150)) {
                        constexpr float step = 5.0f;
                        const float position = SoundManager.g_music_vol / step;
                        const float index = direction > 0 ? std::floor(position) : std::ceil(position);
                        SoundManager.SetVolumes(SoundManager.g_sound_vol, (index + direction) * step);
                        musicAdjustmentTime = now;
                    }
                    musicAdjustmentDirection = direction;
                } else if (lower != raise) {
                    const float change = Timer.sync(lower ? -5.0f : 5.0f);
                    SoundManager.SetVolumes(SoundManager.g_sound_vol + change, SoundManager.g_music_vol);
                }
            }

            // Punkt gewählt
            //
            if (selected) {
                AuswahlPossible = false;

                // Keyboard konfigurieren
                //
                if (AktuellerPunkt == 2) {
                    CurrentPlayer = 0;
                    AktuellerZustand = MENUZUSTAND_TASTEN;
                    AktuellerPunkt = 0;
                    control_reassignment_occuring = false;
                }

                // DKS - This was already commented out:
                /*
                                // Joystick Optionen
                                //
                                if (AktuellerPunkt == 3 && JoystickFound == true)
                                {
                                    AktuellerZustand = MENUZUSTAND_BUTTONS;
                                    AktuellerPunkt   = 0;
                                    locked			 = false;
                                }
                */

                // Sprache wählen
                //
                if (AktuellerPunkt == 3) {
                    AktuellerZustand = MENUZUSTAND_LANGUAGE;
                    AktuellerPunkt = 0;
                }

                // Sprache wählen
                //
                if (AktuellerPunkt == MENUPUNKT_VOLUMES_DISPLAY) {
                    pendingDisplay = DirectGraphics.GetDisplaySettings();
                    displayStatus = nullptr;
                    AktuellerZustand = MENUZUSTAND_DISPLAY;
                    AktuellerPunkt = DISPLAY_MODE;
                } else if (AktuellerPunkt == MENUPUNKT_VOLUMES_DETAILS) {
                    options_Detail++;

                    if (options_Detail > DETAIL_MAXIMUM)
                        options_Detail = DETAIL_LOW;

                    PartikelSystem.SetParticleCount();
                }
            }

            // zurück zum Hauptmenu ?
            if ((KeyDown(DIK_ESCAPE) || joy_escape) && AuswahlPossible == true) {
                input_counter = 0.0f;
                SaveConfig();  // Sound Config speichern
                AuswahlPossible = false;
                AktuellerZustand = MENUZUSTAND_MAINMENU;
                AktuellerPunkt = MENUPUNKT_VOLUMES;
                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
            }
        } break;  // Sound / Musik Lautstärke

        case MENUZUSTAND_DISPLAY: {
            constexpr int rowCount = DISPLAY_LAST + 1;
            AktuellerPunkt = (AktuellerPunkt + rowCount) % rowCount;

            if ((KeyDown(DIK_ESCAPE) || joy_escape) && AuswahlPossible) {
                input_counter = 0.0f;
                AuswahlPossible = false;
                AktuellerZustand = MENUZUSTAND_VOLUMES;
                AktuellerPunkt = MENUPUNKT_VOLUMES_DISPLAY;
                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
                break;
            }

            const bool left = KeyDown(DIK_NUMPAD4) || KeyDown(DIK_LEFT) || joy_left;
            const bool right = KeyDown(DIK_NUMPAD6) || KeyDown(DIK_RIGHT) || joy_right;
            if (!selected && !(AuswahlPossible && (left || right)))
                break;
            if (AktuellerPunkt == DISPLAY_APPLY && !selected)
                break;

            input_counter = 0.0f;
            AuswahlPossible = false;
            displayStatus = nullptr;
            const int step = left ? -1 : 1;
            switch (AktuellerPunkt) {
                case DISPLAY_MODE:
                    pendingDisplay.fullscreen = !pendingDisplay.fullscreen;
                    pendingDisplay.width = pendingDisplay.height = 0;
                    break;
                case DISPLAY_ASPECT: {
                    const int count = static_cast<int>(DisplayAspect::Count);
                    const int current = static_cast<int>(pendingDisplay.aspect);
                    pendingDisplay.aspect = static_cast<DisplayAspect>((current + step + count) % count);
                    break;
                }
                case DISPLAY_RESOLUTION: {
                    const auto modes = DirectGraphics.DisplayResolutions(pendingDisplay.fullscreen);
                    const auto current = std::find_if(modes.begin(), modes.end(), [this](const auto &mode) {
                        return mode.width == pendingDisplay.width && mode.height == pendingDisplay.height;
                    });
                    const int count = static_cast<int>(modes.size());
                    const int index = current == modes.end() ? 0 : static_cast<int>(current - modes.begin());
                    const auto &next = modes[(index + step + count) % count];
                    pendingDisplay.width = next.width;
                    pendingDisplay.height = next.height;
                    break;
                }
                case DISPLAY_VSYNC:
                    pendingDisplay.vsync = !pendingDisplay.vsync;
                    break;
                case DISPLAY_TEXTURE_FILTERING:
                    pendingDisplay.textureFiltering = !pendingDisplay.textureFiltering;
                    break;
                case DISPLAY_CRT_FILTER:
                    if (DirectGraphics.SupportsCrtFilter())
                        pendingDisplay.crt = !pendingDisplay.crt;
                    break;
                case DISPLAY_BRIGHTNESS:
                    pendingDisplay.brightnessLevel = std::clamp(pendingDisplay.brightnessLevel + step, 0,
                        static_cast<int>(DisplaySettings::brightnessGains.size()) - 1);
                    break;
                case DISPLAY_SPLITSCREEN_TYPE:
                    pendingDisplay.splitScreenType = pendingDisplay.splitScreenType == SplitScreenType::Dynamic
                        ? SplitScreenType::OneScreen : SplitScreenType::Dynamic;
                    break;
                case DISPLAY_APPLY: {
                    const bool typeChanged = pendingDisplay.splitScreenType != DirectGraphics.GetDisplaySettings().splitScreenType;
                    displayStatusStarted = SDL_GetTicks();
                    showVsyncWarning = false;
                    if (!DirectGraphics.ApplyDisplaySettings(pendingDisplay)) {
                        pendingDisplay = DirectGraphics.GetDisplaySettings();
                        displayStatus = "Display mode unavailable";
                    } else if (!SaveDisplaySettings(pendingDisplay)) {
                        displayStatus = "Applied. Could not save settings.";
                    } else {
                        const bool deferred = Stage >= 0
                            ? pendingDisplay.splitScreenType != TileEngine.LevelSplitScreenType() : typeChanged;
                        const bool vsyncUnavailable = DirectGraphics.IsVSyncEnabled() != pendingDisplay.vsync;
                        displayStatus = "Display settings saved";
                        if (deferred) {
                            displayStatus = "Some settings apply on level load or restart.";
                            showVsyncWarning = vsyncUnavailable;
                        } else if (vsyncUnavailable) {
                            displayStatus = "Saved. VSync unavailable on this driver.";
                        }
                    }
                    break;
                }
            }
            SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
        } break;

        case MENUZUSTAND_LANGUAGE: {
            const int num_lang = LanguageFiles.size();
            if (AktuellerPunkt < 0)
                AktuellerPunkt = num_lang;

            if (AktuellerPunkt > num_lang)
                AktuellerPunkt = 0;

            // Punkt gewählt
            //
            if (selected) {
                AuswahlPossible = false;

                // Language File gewählt ?
                if (AktuellerPunkt < num_lang) {
                    ActualLanguage = LanguageFiles[AktuellerPunkt];
                    LoadLanguage(ActualLanguage);
                    SaveConfig();
                    ShowLanguageInfoCounter = 2000.0f;

                    // Sprache feststellen und schauen, welche Grafik wir anzeigen müssen
                    if (strcmp(TextArray[TEXT::BENUTZTE_GRAFIK], "0") == 0)
                        Sprachgrafik = 0;  // deutsch ?
                    else
                        Sprachgrafik = 1;  // oder doch englisch ?

                    AktuellerZustand = MENUZUSTAND_VOLUMES;
                    AktuellerPunkt = MENUPUNKT_VOLUMES_LANGUAGE;
                    SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
                }

                // oder nur zurück ?
                else {
                    AktuellerZustand = MENUZUSTAND_VOLUMES;
                    AktuellerPunkt = MENUPUNKT_VOLUMES_LANGUAGE;
                    SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
                }
            }

            // zurück zum Hauptmenu ?
            if ((KeyDown(DIK_ESCAPE) || joy_escape) && AuswahlPossible == true) {
                input_counter = 0.0f;
                SaveConfig();  // Sound Config speichern
                AuswahlPossible = false;
                AktuellerZustand = MENUZUSTAND_VOLUMES;
                AktuellerPunkt = MENUPUNKT_VOLUMES_LANGUAGE;
                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
            }

        } break;  // Languages

        case MENUZUSTAND_TASTEN: {
            auto &player = Player[CurrentPlayer];
            const auto *joystick = player.GetJoystick();
            const int action = ControlMenuAction(AktuellerPunkt);
            auto &layout = joystick ? player.controls.controller : player.controls.keyboard;

            if (control_reassignment_occuring) {
                bool deviceChanged = joystick != remapJoystick;
#if SDL_VERSION_ATLEAST(2, 0, 0)
                deviceChanged |= joystick && joystick->Instance() != remapInstance;
#endif
                if (KeyDown(DIK_ESCAPE) || DirectInput.AnyPauseButtonDown() || deviceChanged) {
                    control_reassignment_occuring = false;
                    AuswahlPossible = false;
                    input_counter = 0.0f;
                    break;
                }

                const int button = joystick ? joystick->PressedBinding() : -1;
                const int padDirection = joystick ? joystick->PressedDirection() : -1;
                // Release the input which opened the editor, even if it came from the other player.
                if (!AuswahlPossible) {
                    AuswahlPossible = menuControllersReleased && button < 0 && padDirection < 0 && DirectInput.AreAllKeysReleased();
                    break;
                }

                // Capture a complete stick/hat gesture. A thumb can cross one axis first.
                if (padDirection >= 0) {
                    if (pendingDirection < 0 || padDirection >= FirstPadDiagonal)
                        pendingDirection = padDirection;
                    break;
                }
                InputBinding next = InputBinding::Pad(pendingDirection >= 0 ? pendingDirection : button);
                pendingDirection = -1;
                if (next.kind == BindingKind::Empty) {
                    for (int i = 1; i < DirectInput.NumberOfKeys; ++i) {
                        if (!TastaturPuffer[i])
                            continue;
                        const int key = KeyCode(i);
                        if (key == DIK_TAB || key == DIK_ESCAPE)
                            continue;
#if defined(GCW)
                        if (key == DIK_HOME)
                            continue;
#endif
                        next = InputBinding::Key(key);
                        if (next.kind != BindingKind::Empty)
                            break;
                    }
                }
                if (next.kind == BindingKind::Empty)
                    break;
                const auto conflicts = [&](int other) {
                    const auto &slots = layout[other];
                    return next.Overlaps(slots[0]) || next.Overlaps(slots[1]);
                };
                if ((action == AKTION_UNTEN && conflicts(AKTION_DUCKEN)) || (action == AKTION_DUCKEN && conflicts(AKTION_UNTEN))) {
                    bindingNotice = "Crouch and Look Down need separate inputs";
                } else {
                    auto &pair = layout[action];
                    // The most recent input is first. A third input drops the oldest.
                    if (!next.Overlaps(pair[0]))
                        pair = {next, pair[0]};
                    control_reassignment_occuring = false;
                    bindingNotice = nullptr;
                }
                AuswahlPossible = false;
                input_counter = 0.0f;
                break;
            }

            const bool remove = joystick ? joystick->ButtonDeletePressed() : KeyDown(DIK_DELETE);
            if (action >= 0 && AuswahlPossible && remove) {
                auto &pair = layout[action];
                pair = {pair[1], InputBinding{}};
                input_counter = 0.0f;
                AuswahlPossible = false;
                break;
            }

            const bool right = KeyDown(DIK_NUMPAD6) || KeyDown(DIK_RIGHT) || joy_right;
            const bool left = KeyDown(DIK_NUMPAD4) || KeyDown(DIK_LEFT) || joy_left;
            if (!selected && AuswahlPossible && (left || right)) {
                CurrentPlayer = right ? 1 : 0;
                AuswahlPossible = false;
            }

            if (selected) {
                AuswahlPossible = false;
                input_counter = 0.0f;
                switch (AktuellerPunkt) {
                    case MENU_TASTEN_RUMBLE_LINE: {
                        const int count = static_cast<int>(RumbleStrength::Count);
                        player.rumbleStrength = static_cast<RumbleStrength>((static_cast<int>(player.rumbleStrength) + 1) % count);
                        if (player.rumbleStrength != RumbleStrength::Off)
                            player.PlayFeedback(FFE_MEDIUMRUMBLE);
                        else if (joystick)
                            joystick->StopForceFeedbackEffect(FFE_MEDIUMRUMBLE);
                        break;
                    }
                    case MENU_TASTEN_DEFAULTS_LINE:
                        CreateDefaultControlsConfig(CurrentPlayer);
                        DirectInput.AssignPlayerJoysticks();
                        break;
                    case MENU_TASTEN_PLAYER_LINE:
                        CurrentPlayer = 1 - CurrentPlayer;
                        break;
                    case MENU_TASTEN_TYPE_LINE:
                        DirectInput.CyclePlayerJoystick(CurrentPlayer);
                        break;
                    default:
                        if (action < 0)
                            break;
                        control_reassignment_occuring = true;
                        bindingNotice = nullptr;
                        pendingDirection = -1;
                        remapJoystick = joystick;
#if SDL_VERSION_ATLEAST(2, 0, 0)
                        remapInstance = joystick ? joystick->Instance() : -1;
#endif
                        break;
                }
            }

            if ((KeyDown(DIK_ESCAPE) || joy_escape) && AuswahlPossible) {
                input_counter = 0.0f;
                InvalidateTutorialText();
                SaveConfig();
                AuswahlPossible = false;
                AktuellerZustand = MENUZUSTAND_VOLUMES;
                AktuellerPunkt = MENUPUNKT_VOLUMES_TASTEN;
                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
            }
        } break;  // Tasten

        // Die Credits und die Highscore
        case MENUPUNKT_CREDITS:
        case MENUPUNKT_HIGHSCORES: {
            // zurück zum Hauptmenu ?
            if (((!CommandLineParams.Arcade && KeyDown(DIK_ESCAPE)) || joy_escape) && AuswahlPossible == true) {
                input_counter = 0.0f;

                if (AktuellerZustand == MENUPUNKT_CREDITS) {
                    SoundManager.SetSongVolume(MUSIC::MENU, 0);
                    SoundManager.FadeSong(MUSIC::CREDITS, -5.0f, 0, false);
                    SoundManager.FadeSong(MUSIC::MENU, 5.0f, 100, false);
                    CreditsCounter = 0.0f;
                    AktuellerPunkt = 4;
                }

                if (AktuellerZustand == MENUPUNKT_HIGHSCORES) {
                    SoundManager.SetSongVolume(MUSIC::MENU, 0);
                    SoundManager.FadeSong(MUSIC::HIGHSCORE, -5.0f, 0, false);
                    SoundManager.FadeSong(MUSIC::MENU, 5.0f, 100, false);
                    AktuellerPunkt = 3;
                }

                AuswahlPossible = false;
                AktuellerZustand = MENUZUSTAND_MAINMENU;
                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
            }
        } break;  // Credits und Highscore

        case MENUZUSTAND_ENTERNAME: {
            // DKS - Modified extensively to allow joystick name entry
            static unsigned char Taste;
            static bool possible = false;

            int newname_len = strlen(NewName);
            int newname_maxlen = 16;

            if (CommandLineParams.Arcade)
            {
                for (int i = 0; i < 2; i++)
                {
                    if (Player[i].ControlType != CONTROLTYPE_KEYBOARD)
                        continue;
                    if (Player[i].ActionPressed(AKTION_RECHTS, nullptr))
                        joy_right = true;
                    if (Player[i].ActionPressed(AKTION_LINKS, nullptr))
                        joy_left = true;
                    if (Player[i].ActionPressed(AKTION_OBEN, nullptr))
                        joy_up = true;
                    if (Player[i].ActionPressed(AKTION_DUCKEN, nullptr))
                        joy_down = true;
                    if (Player[i].ActionPressed(AKTION_SHOOT, nullptr))
                        joy_enter = true;
                }
            }
            if (joy_right) {
                input_counter = 0.0f;
                if (newname_len < newname_maxlen && newname_len > 0 && NewName[newname_len - 1] != '_') {
                    NewName[newname_len] = '_';  // Add prompt char
                    NewName[newname_len + 1] = '\0';
                    newname_len++;
                }
            } else if (joy_left) {
                input_counter = 0.0f;
                if (newname_len > 1) {
                    NewName[newname_len - 2] = '_';
                    NewName[newname_len - 1] = '\0';
                    newname_len--;
                }
            } else if (joy_up) {
                input_counter = 0.0f;
                if (newname_len > 0) {
                    // Chars 97-122 are a..z, Chars 48-57 are 0..9, Space is 32
                    if (NewName[newname_len - 1] == 'Z') {
                        NewName[newname_len - 1] = '0';  // Number '0'
                    } else if (NewName[newname_len - 1] == '9' || NewName[newname_len - 1] == '_') {
                        NewName[newname_len - 1] = 'A';
                    } else {
                        NewName[newname_len - 1]++;
                    }
                }
            } else if (joy_down) {
                input_counter = 0.0f;
                if (newname_len > 0) {
                    // Chars 97-122 are a..z, Chars 48-57 are 0..9, Space is 32
                    if (NewName[newname_len - 1] == 'A' || NewName[newname_len - 1] == '_') {
                        NewName[newname_len - 1] = '9';
                    } else if (NewName[newname_len - 1] == '0') {
                        NewName[newname_len - 1] = 'Z';
                    } else {
                        NewName[newname_len - 1]--;
                    }
                }
            }

            if (!KeyDown(DIK_RETURN) && !joy_enter)
                AuswahlPossible = true;

            if (((!CommandLineParams.Arcade && KeyDown(DIK_RETURN)) || joy_enter) && AuswahlPossible == true) {
                input_counter = 0.0f;
                // Strip trailing cursor char
                if (newname_len > 0 && NewName[newname_len - 1] == '_') {
                    NewName[newname_len - 1] = '\0';
                    newname_len--;
                }

                // Neuen Namen an die neue Position setzen
                strcpy_s(Highscores[NewRank].Name, 32-1, NewName);

                Highscores[NewRank].Stage = NewStage;
                Highscores[NewRank].Skill = NewSkill;

                // Und zurück ins Hauptmenu und dort die Highscores anzeigen
                SpielZustand = GameStateEnum::MAINMENU;
                AktuellerZustand = MENUPUNKT_HIGHSCORES;

                SoundManager.SetSongVolume(MUSIC::MENU, 0.0f);

                // Highscore noch sichern
                SaveHighscore();
            }

            DirectInput.UpdateTastatur();

            if (possible == false) {
                possible = true;

                // Test whether the pressed key was released
                if ((!CommandLineParams.Arcade && (KeyDown(Taste) || KeyDown(DIK_BACK) || KeyDown(DIK_SPACE))) || joy_left || joy_right || joy_up ||
                    joy_down)
                    possible = false;
            }

            for (int i = 0; i < 256; i++)  // Puffer durchgehen
                if (!CommandLineParams.Arcade && KeyDown(i) && possible == true) {
                    possible = false;

                    if (KeyDown(DIK_BACK)) {
                        // DKS - added in missing/disabled backspace support
                        if (newname_len > 1) {
                            SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
                            NewName[newname_len - 2] = '_';
                            NewName[newname_len - 1] = '\0';
                            newname_len--;
                        }
                    } else if (KeyDown(DIK_SPACE)) {
                        // DKS - added in missing/disabled space support
                        // Only insert a space if the string is not empty
                        if (newname_len > 1 && NewName[newname_len - 1] == '_' && newname_len < newname_maxlen) {
                            SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
                            NewName[newname_len - 1] = ' ';
                            NewName[newname_len] = '_';
                            NewName[newname_len + 1] = '\0';
                            newname_len++;
                        }

                    } else {
                        Taste = i;
                        if (strlen(GetKeyName(Taste)) == 1 && newname_len < newname_maxlen) {
                            char keyname[2];
                            strcpy_s(keyname, GetKeyName(Taste));
                            keyname[0] = toupper(keyname[0]);
                            if ((keyname[0] >= 'A' && keyname[0] <= 'Z') || (keyname[0] >= '0' && keyname[0] <= '9')) {
                                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
                                // NewName should already have a trailing underscore, indicating the cursor..
                                //  replace it with the new char and a new underscore
                                if (newname_len > 0) {
                                    NewName[newname_len - 1] = keyname[0];
                                    NewName[newname_len] = '_';
                                    NewName[newname_len + 1] = '\0';
                                    newname_len++;
                                } else {
                                    NewName[0] = keyname[0];
                                    NewName[1] = '_';
                                    NewName[2] = '\0';
                                    newname_len = 2;
                                }
                            }
                        }
                    }
                }

            // Aktuellen Namen azeigen
            char Buffer[32];
            snprintf(Buffer, 32, "%s", NewName);
            pMenuFont->DrawTextCenterAlign(319, ypos + 230, Buffer, D3DCOLOR_RGBA(0, 0, 255, 255), 2);
            pMenuFont->DrawTextCenterAlign(321, ypos + 230, Buffer, D3DCOLOR_RGBA(0, 0, 255, 255), 2);
            pMenuFont->DrawTextCenterAlign(320, ypos + 229, Buffer, D3DCOLOR_RGBA(0, 0, 255, 255), 2);
            pMenuFont->DrawTextCenterAlign(320, ypos + 231, Buffer, D3DCOLOR_RGBA(0, 0, 255, 255), 2);
            pMenuFont->DrawTextCenterAlign(320, ypos + 230, Buffer, D3DCOLOR_RGBA(255, 255, 255, 255), 2);

        } break;  // Namen eingeben

        // Neues Spiel starten / Savegame laden
        case MENUZUSTAND_STARTGAME:
        case MENUZUSTAND_SELECTSKILL:
        case MENUZUSTAND_PLAYERCOUNT:
        case MENUZUSTAND_TUTORIAL: {
            const int count = AktuellerZustand == MENUZUSTAND_SELECTSKILL ? 4 : 2;
            AktuellerPunkt = (AktuellerPunkt + count) % count;
            if (((!CommandLineParams.Arcade && KeyDown(DIK_ESCAPE)) || joy_escape) && AuswahlPossible) {
                input_counter = 0.0f;
                AuswahlPossible = false;
                if (AktuellerZustand == MENUZUSTAND_TUTORIAL) {
                    AktuellerZustand = MENUZUSTAND_PLAYERCOUNT;
                    AktuellerPunkt = pendingPlayers - 1;
                } else if (AktuellerZustand == MENUZUSTAND_PLAYERCOUNT) {
                    AktuellerZustand = MENUZUSTAND_SELECTSKILL;
                    AktuellerPunkt = pendingSkill;
                } else if (AktuellerZustand == MENUZUSTAND_SELECTSKILL) {
                    AktuellerZustand = MENUZUSTAND_STARTGAME;
                    AktuellerPunkt = 0;
                } else {
                    AktuellerZustand = MENUZUSTAND_MAINMENU;
                    AktuellerPunkt = MENUPUNKT_STARTGAME;
                }
            } else if (selected) {
                AuswahlPossible = false;
                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);
                if (AktuellerZustand == MENUZUSTAND_STARTGAME) {
                    if (AktuellerPunkt == 0) {
                        pendingSkill = SKILL_MEDIUM;
                        pendingPlayers = 1;
                        AktuellerZustand = MENUZUSTAND_SELECTSKILL;
                        AktuellerPunkt = pendingSkill;
                    } else {
                        LoadSavegames();
                        AktuellerZustand = MENUZUSTAND_LOADGAME;
                        AktuellerPunkt = 0;
                    }
                } else if (AktuellerZustand == MENUZUSTAND_SELECTSKILL) {
                    pendingSkill = AktuellerPunkt;
                    AktuellerZustand = MENUZUSTAND_PLAYERCOUNT;
                    AktuellerPunkt = pendingPlayers - 1;
                } else if (AktuellerZustand == MENUZUSTAND_PLAYERCOUNT) {
                    pendingPlayers = AktuellerPunkt + 1;
                    AktuellerZustand = MENUZUSTAND_TUTORIAL;
                    AktuellerPunkt = 1;
                } else {
                    NUMPLAYERS = pendingPlayers;
                    Skill = pendingSkill;
                    RunningTutorial = AktuellerPunkt == 0;
                    SoundManager.StopSongs();
                    DisplayHintNr = -1;
                    if (RunningTutorial) {
                        InitNewGame();
                        InitNewGameLevel();
                    } else {
                        StartIntro();
                    }
                }
            }
        } break;

        // Savegame zum Laden wählen
        case MENUZUSTAND_LOADGAME: {
            if (AktuellerPunkt < 0)
                AktuellerPunkt = MAX_SAVEGAMES - 1;

            if (AktuellerPunkt > MAX_SAVEGAMES - 1)
                AktuellerPunkt = 0;

            // Zurück ins New Game Menu ?
            if (((!CommandLineParams.Arcade && KeyDown(DIK_ESCAPE)) || joy_escape) && AuswahlPossible == true) {
                input_counter = 0.0f;
                AktuellerZustand = MENUZUSTAND_STARTGAME;
                AktuellerPunkt = 1;
                AuswahlPossible = false;
            }

            // Menu-Punkt ausgewählt ?
            // Dann prüfen ob das aktuelle Savegame ein leerer Slot ist
            // und wenn das nicht der Fall ist, dann das Savegame laden und
            // das Spiel starten
            if (selected) {

                AuswahlPossible = false;
                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);

                // Aktuell gewähltes Savegame laden, wenn es existiert
                // bzw die Werte aus den schon geladenen Savegames übernehmen
                if (Savegames[AktuellerPunkt].Stage >= 0) {
                    RunningTutorial = false;
                    // DKS - Original order was like so:
                    // InitNewGame();
                    // NUMPLAYERS = Savegames[AktuellerPunkt].Players;

                    // DKS - But I have reversed it here, because InitNewGame() depended on
                    //      NUMPLAYERS being set correctly.
                    NUMPLAYERS = Savegames[AktuellerPunkt].Players;
                    InitNewGame();

                    // Werte von Spieler auf das Savegame übertragen
                    Player[0].Score = Savegames[AktuellerPunkt].Score;
                    Stage = Savegames[AktuellerPunkt].Stage;
                    Skill = Savegames[AktuellerPunkt].Skill;
                    NewStage = Savegames[AktuellerPunkt].NewStage;
                    Player[0].CollectedDiamonds = Savegames[AktuellerPunkt].CollectedDiamonds;
                    Player[0].PowerLines = Savegames[AktuellerPunkt].PowerLines;
                    Player[0].Grenades = Savegames[AktuellerPunkt].Grenades;
                    Player[0].SmartBombs = Savegames[AktuellerPunkt].SmartBombs;

                    for (int p = 0; p < 2; p++) {
                        Player[p].Handlung = PlayerActionEnum::STEHEN;
                        Player[p].Energy = Savegames[AktuellerPunkt].Energy[p];
                        Player[p].Armour = Savegames[AktuellerPunkt].Armour[p];
                        Player[p].Shield = Savegames[AktuellerPunkt].Shield[p];
                        Player[p].SelectedWeapon = Savegames[AktuellerPunkt].SelectedWeapon[p];
                        Player[p].BlitzLength = Savegames[AktuellerPunkt].BlitzLength[p];
                        Player[p].AutoFireExtra = Savegames[AktuellerPunkt].AutoFire[p];
                        Player[p].RiesenShotExtra = Savegames[AktuellerPunkt].RiesenShot[p];
                        Player[p].Lives = Savegames[AktuellerPunkt].Lives[p];

                        for (int i = 0; i < 4; i++)
                            Player[p].CurrentWeaponLevel[i] = Savegames[AktuellerPunkt].CurrentWeaponLevel[p][i];
                    }

                    // An der Stelle im Savegame weiterspielen
                    AktuellerZustand = 3; // MAINMENU ???
                    AktuellerPunkt = 0;
                    //					SoundManager.StopSong(MUSIC::MENU, false);
                    InitNewGameLevel();  // Neues level laden
                    SpielZustand = GameStateEnum::GAMELOOP;     // Weiterspielen
                }
            }
        } break;  // Load Game

        // Savegame zum Speichern wählen
        case MENUZUSTAND_SAVEGAME: {
            if (AktuellerPunkt < 0)
                AktuellerPunkt = MAX_SAVEGAMES;

            if (AktuellerPunkt > MAX_SAVEGAMES)
                AktuellerPunkt = 0;

            // Menu-Punkt ausgewählt ?
            // Dann das Spiel an dem aktuell gewählten Slot speichern
            // oder weiterspielen ohne zu speichern (je nach Auswahl)
            if (selected) {
                AuswahlPossible = false;
                SoundManager.PlayWave(100, 128, 11025, SOUND::CLICK);

                // Bestimmtes Savegame ausgewählt
                // Dann hier speichern
                if (AktuellerPunkt < MAX_SAVEGAMES) {
                    // Werte von Spieler auf das Savegame übertragen
                    // Name des Save games aus aktuellem Datum und aktueller Zeit erstelen

                    time_t seconds = time(nullptr);
                    struct tm *ptm = localtime(&seconds);

                    /*
                                        sprintf_s(timestr,"%02i-%02i-%04i %02i:%02i",
                                               (int)ptm->tm_mday,
                                               (int)ptm->tm_mon+1,
                                               (int)ptm->tm_year+1900,

                                               (int)ptm->tm_hour,
                                               (int)ptm->tm_min);
                    */
                    std::string timestr(asctime(ptm));

                    strcpy_s(Savegames[AktuellerPunkt].Name, 1, "");
                    strcpy_s(Savegames[AktuellerPunkt].Name, 40-1, timestr.c_str());

                    Savegames[AktuellerPunkt].Players = NUMPLAYERS;
                    Savegames[AktuellerPunkt].Score = Player[0].Score;
                    Savegames[AktuellerPunkt].Stage = Stage;
                    Savegames[AktuellerPunkt].Skill = Skill;
                    Savegames[AktuellerPunkt].NewStage = NewStage;
                    Savegames[AktuellerPunkt].CollectedDiamonds = Player[0].CollectedDiamonds;
                    Savegames[AktuellerPunkt].PowerLines = Player[0].PowerLines;
                    Savegames[AktuellerPunkt].Grenades = Player[0].Grenades;
                    Savegames[AktuellerPunkt].SmartBombs = Player[0].SmartBombs;

                    for (int p = 0; p < 2; p++) {
                        Savegames[AktuellerPunkt].Energy[p] = Player[0].Energy;
                        Savegames[AktuellerPunkt].Armour[p] = Player[0].Armour;
                        Savegames[AktuellerPunkt].Shield[p] = Player[0].Shield;
                        Savegames[AktuellerPunkt].SelectedWeapon[p] = Player[0].SelectedWeapon;
                        Savegames[AktuellerPunkt].Lives[p] = Player[p].Lives;
                        Savegames[AktuellerPunkt].AutoFire[p] = Player[p].AutoFireExtra;
                        Savegames[AktuellerPunkt].RiesenShot[p] = Player[p].RiesenShotExtra;

                        for (int i = 0; i < 4; i++)
                            Savegames[AktuellerPunkt].CurrentWeaponLevel[p][i] = Player[p].CurrentWeaponLevel[i];

                        Savegames[AktuellerPunkt].BlitzLength[p] = Player[p].BlitzLength;
                    }

                    // Prüfsumme gegen Savegame-Manipulation errechnen

                    Savegames[AktuellerPunkt].Pruefsumme =
                        static_cast<int>(Player[0].Energy) + static_cast<int>(Player[0].Armour) +
                        static_cast<int>(Player[0].Shield) + Player[0].Score + Stage + Skill + NewStage +
                        Player[0].CollectedDiamonds + Player[0].SelectedWeapon;

                    for (int i = 0; i < 4; i++)
                        Savegames[AktuellerPunkt].Pruefsumme += Player[0].CurrentWeaponLevel[i];

                    Savegames[AktuellerPunkt].Pruefsumme += Player[0].BlitzLength + Player[0].PowerLines +
                                                            Player[0].Grenades + Player[0].SmartBombs + NUMPLAYERS;

                    // Und Savegame in Datei schreiben

                    // Name des Savegames erstellen
                    std::string Buffer = std::to_string(AktuellerPunkt);

                    // Versuchen, die Datei zu erstellen
                    // nur weitermachen falls es keinen Fehler gibt
                    std::string Name = g_save_ext + "/Savegame" + Buffer + ".save";
                    std::ofstream Datei(Name, std::ofstream::binary);

                    // Fehler beim Öffnen ? Dann leeren Slot erzeugen
                    if (!Datei) {
                        // TODO log message
                    }

                    // Ansonsten Slot speichern
                    else
                        Datei.write(reinterpret_cast<char *>(&Savegames[AktuellerPunkt]),
                                    sizeof(Savegames[AktuellerPunkt]));

                    // Und Datei wieder schliessen
                    Datei.close();
                }  // Select Slot Auswahl

                // oder Weiter/Continue  ?
                //				if (AktuellerPunkt == MAX_SAVEGAMES)
                {
                    AktuellerZustand = 3; // MAINMENU ???
                    AktuellerPunkt = 0;
                    InitNewGameLevel();  // Neues level laden
                }
            }
        } break;  // Save Game

        default:
            break;
    }  // switch
}

// --------------------------------------------------------------------------------------
// Savegames laden
// --------------------------------------------------------------------------------------

void MenuClass::LoadSavegames() {

    // Versuchen, die einzelnen Savegames zu laden
    for (int i = 0; i < MAX_SAVEGAMES; i++) {
        // Name des Savegames erstellen
        std::string Name = g_save_ext + "/Savegame" + std::to_string(i) + ".save";

        // Versuchen, die Datei zu öffnen
        // falls sie nicht existiert oder es eine Fehler gibt, ist der Slot noch leer
        std::ifstream Datei(Name, std::ifstream::binary);

        Protokoll << "save games loaded" << std::endl;

        // Fehler beim Öffnen ? Dann leeren Slot erzeugen
        //
        if (!Datei)
            Savegames[i].Stage = -1;

        else {
            // Ansonsten Slot auslesen
            //
            Datei.read(reinterpret_cast<char *>(&Savegames[i]), sizeof(Savegames[i]));

            // Und Datei wieder schliessen
            //
            Datei.close();
            // Checken ob die Pruefsumme noch stimmt
            // Prüfsumme gegen Savegame-Manipulation errechnen
            // und nur wenn diese stimmt das Savegame übernehmen
            long Pruefsumme;

            Pruefsumme = static_cast<int>(Savegames[i].Energy[0]) + static_cast<int>(Savegames[i].Armour[0]) +
                         static_cast<int>(Savegames[i].Shield[0]) + Savegames[i].Score + Savegames[i].Stage +
                         Savegames[i].Skill + Savegames[i].NewStage + Savegames[i].CollectedDiamonds +
                         Savegames[i].SelectedWeapon[0];

            for (int j = 0; j < 4; j++)
                Pruefsumme += Savegames[i].CurrentWeaponLevel[0][j];

            Pruefsumme += Savegames[i].BlitzLength[0] + Savegames[i].PowerLines + Savegames[i].Grenades +
                          Savegames[i].SmartBombs + Savegames[i].Players;

            // Pruefsumme stimmt nicht mehr ? Dann Savegame zum leeren Slot machen
            //
            if (Pruefsumme != Savegames[i].Pruefsumme)
                Savegames[i].Stage = -1;
        }
    }
}

// --------------------------------------------------------------------------------------
// Savegames anzeigen
// --------------------------------------------------------------------------------------

// DKS - Altered to allow scaled fonts on low resolution devices
void MenuClass::ShowSavegames(int Highlight) {
    const int scale_factor = pDefaultFont->GetScaleFactor();
    int line_off_y = 14;
    int title_bar_off_y = ypos + 120;
    int savegames_off_y = ypos + 150;
    int col1_off_x = xpos + 100;
    int col2_off_x = xpos + 150;
    int col3_off_x = xpos + 250;

    if (scale_factor > 1) {
        // Things get shifted higher and spaced farther when using a scaled font, so there's room for everything
        line_off_y = 24;
        title_bar_off_y = ypos + 50;
        savegames_off_y = title_bar_off_y + 30;
        col1_off_x = xpos / 2 + 50;
        col2_off_x = xpos / 2 + 150;
        col3_off_x = xpos / 2 + 350;
    }

    pDefaultFont->DrawTextCenterAlign(col1_off_x, title_bar_off_y, TextArray[TEXT::SAVE_STAGE], 0xFFFFFFFF, 0);
    pDefaultFont->DrawTextCenterAlign(col2_off_x, title_bar_off_y, TextArray[TEXT::SAVE_SPIELER], 0xFFFFFFFF, 0);
    pDefaultFont->DrawTextCenterAlign(col3_off_x, title_bar_off_y, TextArray[TEXT::SAVE_DATUM], 0xFFFFFFFF, 0);

    // Alle Savegames anzeigen
    for (int i = 0; i < MAX_SAVEGAMES; i++) {
        D3DCOLOR col;
        if (i == Highlight)
            col = 0xFFFFFFFF;
        else
            col = 0x80FFFFFF;
        // Stage anzeigen
        //
        if (Savegames[i].Stage >= 0) {
            std::string buffer = std::to_string(Savegames[i].Stage);
            pDefaultFont->DrawTextCenterAlign(col1_off_x, savegames_off_y + i * line_off_y, buffer.c_str(), col, 0);
        }

        // Spieler anzeigen
        //
        if (Savegames[i].Stage >= 0) {
            std::string buffer = std::to_string(Savegames[i].Players);
            pDefaultFont->DrawTextCenterAlign(col2_off_x, savegames_off_y + i * line_off_y, buffer.c_str(), col, 0);
        }

        // DKS - This was duplicate code that I removed, it did nothing different than above.. obvious bug
        /*
        // Skill anzeigen
        //
        if (Savegames[i].Stage >= 0)
        {
            _itoa_s(Savegames[i].Players, buffer, 10);
            pDefaultFont->DrawTextCenterAlign(xpos + 150, ypos + 150 + i*14, buffer, col, 0);
        }
        */

        // Ist ein Savegame gefunden worden ?
        if (Savegames[i].Stage >= 0) {
            pDefaultFont->DrawTextCenterAlign(col3_off_x, savegames_off_y + i * line_off_y, Savegames[i].Name, col, 0);
        }

        // oder ist dort noch ein leerer Slot ?
        else {
            pDefaultFont->DrawTextCenterAlign(320, savegames_off_y + i * line_off_y, TextArray[TEXT::SAVE_LEER], col, 0);
        }
    }
}

// --------------------------------------------------------------------------------------
// Highscore checksum
// --------------------------------------------------------------------------------------
inline long CalcChecksum(const HighscoreStruct& Highscore) {

    long Pruefsumme = 0;
    unsigned int const nameLen = strlen(Highscore.Name);
    for (unsigned int j = 0; j < nameLen; j++)
        Pruefsumme += Highscore.Name[j];

    Pruefsumme += nameLen + Highscore.Score + Highscore.Stage + Highscore.Skill;
    return Pruefsumme;
}

// --------------------------------------------------------------------------------------
// Highscore-Liste laden
// --------------------------------------------------------------------------------------

void MenuClass::LoadHighscore() {

    // Versuchen, die Highscore Datei zu öffnen
    // falls sie nicht existiert oder es eine Fehler gibt, wird die Standard
    // Highscore gesetzt
    std::string name = g_save_ext + "/Hurrican.hsl";
    std::ifstream Datei(name, std::ifstream::binary);

    // Fehler beim Öffnen ? Dann standard Highscore setzen
    //
    if (!Datei)
        ResetHighscore();

    else {
        // Ansonsten Scores auslesen
        //
        for (int i = 0; i < MAX_HIGHSCORES; i++)
            Datei.read(reinterpret_cast<char *>(&Highscores[i]), sizeof(Highscores[i]));

        // Und Datei wieder schliessen
        //
        Datei.close();

        // Checken ob die Pruefsumme noch stimmt
        // und nur wenn diese stimmt die Highscore übernehmen
        // ansonsten wieder standard Highscore setzen

        for (int i = 0; i < MAX_HIGHSCORES; i++) {

            Highscores[i].Score = FixEndian(Highscores[i].Score);
            Highscores[i].Stage = FixEndian(Highscores[i].Stage);
            Highscores[i].Skill = FixEndian(Highscores[i].Skill);
            Highscores[i].Pruefsumme = FixEndian(Highscores[i].Pruefsumme);

            // Sanity check
            if (Highscores[i].Skill < SKILL_EASY ||
                Highscores[i].Skill > SKILL_HURRICAN) {
                Highscores[i].Skill = SKILL_EASY;
            }
            
            long Pruefsumme = CalcChecksum(Highscores[i]);

            // Pruefsumme stimmt nicht mehr ?
            //
            if (Pruefsumme != Highscores[i].Pruefsumme) {
                strcpy_s(Highscores[i].Name, "!!! CHEATER !!!");
                Highscores[i].Stage = 0;
            }
        }
    }
}

// --------------------------------------------------------------------------------------
// Highscore-Liste speichern
// --------------------------------------------------------------------------------------

void MenuClass::SaveHighscore() {

    // Highscore Datei öffnen
    std::string name = g_save_ext + "/Hurrican.hsl";
    std::ofstream Datei(name, std::ofstream::binary);

    // Fehler beim Öffnen ? Dann standard Highscore setzen
    //
    if (!Datei)
        ResetHighscore();

    else {
        // Ansonsten Scores speichern
        //
        for (int i = 0; i < MAX_HIGHSCORES; i++) {
            // Prüfsumme erstellen
            //
            Highscores[i].Pruefsumme = CalcChecksum(Highscores[i]);

            // SixK - SWAP TO LITTLE ENDIAN before saving (Tweaked by DKS)
            Highscores[i].Score = FixEndian(Highscores[i].Score);
            Highscores[i].Stage = FixEndian(Highscores[i].Stage);
            Highscores[i].Skill = FixEndian(Highscores[i].Skill);
            Highscores[i].Pruefsumme = FixEndian(Highscores[i].Pruefsumme);

            // Und Eintrag speichern
            //
            Datei.write(reinterpret_cast<char *>(&Highscores[i]), sizeof(Highscores[i]));

            // SixK - SWAP TO BIG ENDIAN Again once datas have been written
            Highscores[i].Score = FixEndian(Highscores[i].Score);
            Highscores[i].Stage = FixEndian(Highscores[i].Stage);
            Highscores[i].Skill = FixEndian(Highscores[i].Skill);
            Highscores[i].Pruefsumme = FixEndian(Highscores[i].Pruefsumme);
        }
    }

    // Und Datei wieder schliessen
    //
    Datei.close();
}

// --------------------------------------------------------------------------------------
// Highscore-Liste mit Standardwerten resetten
// --------------------------------------------------------------------------------------

void MenuClass::ResetHighscore() {
    char HighscoreNames[MAX_HIGHSCORES][30] = {
        "EISWUXE", "TURRI", "MANFRED", "CHRIS", "ANDREAS", "JULIAN", "RAMIRO", "PETER", "CELAL", "MARKUS",
    };

    for (int i = 0; i < MAX_HIGHSCORES; i++) {
        Highscores[i].Score = (MAX_HIGHSCORES - i) * 10000;
        Highscores[i].Skill = i % 4;
        Highscores[i].Stage = MAX_HIGHSCORES - i;

        if (Highscores[i].Stage > 9)
            Highscores[i].Stage = 9;

        strcpy_s(Highscores[i].Name, HighscoreNames[i]);

        Highscores[i].Pruefsumme = CalcChecksum(Highscores[i]);
    }
}

// --------------------------------------------------------------------------------------
// Infos aus der Language Datei anzeigen lassen (Zeile 1 - 8)
// --------------------------------------------------------------------------------------

// DKS - Fixed displaying language info with scaled fonts, made rectangle drawing smarter and sized dynamically
void MenuClass::ShowLanguageInfo() {

    if (ShowLanguageInfoCounter < 0.0f)
        return;

    int a1 = static_cast<int>(ShowLanguageInfoCounter);

    if (a1 > 255)
        a1 = 255;

    int a2 = a1 / 2;

    // Determine size and location of background rectangle:
    unsigned int longest_line = 0;
    for (int i = 0; i < 9; i++) {
        unsigned int const line_len = strlen(TextArray[i]);
        if (line_len > longest_line) {
            longest_line = line_len;
        }
    }
    int const xoff_inc = 8 * pDefaultFont->GetScaleFactor();
    int const yoff_inc = 12 * pDefaultFont->GetScaleFactor();
    int const border = 20 / pDefaultFont->GetScaleFactor();
    int const rect_w = longest_line * xoff_inc + border * 2;
    int const rect_h = 11 * yoff_inc + border * 2;
    int const rect_x = 320 - rect_w / 2;
    int const rect_y = 240 - rect_h / 2;
    RenderRect(rect_x - 2, rect_y - 2, rect_w + 4, rect_h + 4, D3DCOLOR_RGBA(64, 128, 255, a2));
    RenderRect(rect_x - 1, rect_y - 1, rect_w + 2, rect_h + 2, D3DCOLOR_RGBA(0, 0, 64, a2));
    RenderRect(rect_x, rect_y, rect_w, rect_h, D3DCOLOR_RGBA(0, 0, 64, a2));
    pDefaultFont->DrawText(static_cast<float>(320 - pDefaultFont->StringLength(TextArray[TEXT::BENUTZTES_FILE]) / 2),
                           static_cast<float>(rect_y + border), TextArray[TEXT::BENUTZTES_FILE], D3DCOLOR_RGBA(255, 255, 255, a1));

    for (int i = 0; i < 9; i++) {
        unsigned int const len = strlen(TextArray[i]);
        int xoff = 320 - (len - 1) * xoff_inc / 2;
        for (unsigned int j = 0; j < len; j++) {
            char c[2];
            c[0] = TextArray[i][j];
            c[1] = '\0';
            pDefaultFont->DrawText(static_cast<float>(xoff), static_cast<float>(rect_y + border + ((2 + i) * yoff_inc)), c,
                                   D3DCOLOR_RGBA(255, 255, 255, a1));
            xoff += xoff_inc;
        }
    }
}  // ShowLanguageInfo

// --------------------------------------------------------------------------------------
// Prüfen obs eine neue Highscore gibt
// --------------------------------------------------------------------------------------

void MenuClass::CheckForNewHighscore() {
    AktuellerZustand = MENUPUNKT_HIGHSCORES;
    SoundManager.StopSongs();
    SoundManager.StopSounds();
    SoundManager.PlaySong(MUSIC::HIGHSCORE, false);

    // Feststellen ob wir eine neue Highscore erreicht haben
    //
    NewScore = 0;
    NewRank = 0;
    AktuellerPunkt = 0;
    // DKS - Initialize NewName with just the cursor char
    strcpy_s(NewName, "_");

    // Alle Highscores durchgehen und vergleichen
    //
    bool besser = false;

    for (int j = 0; j < MAX_HIGHSCORES; j++)
        if (Player[0].Score >= Highscores[j].Score && besser == false) {
            besser = true;

            // Ab der Stelle wo ein Namen eingefügt wird eine Zeile runterscrollen
            //
            for (int k = MAX_HIGHSCORES - 1; k > j; k--) {
                Highscores[k].Score = Highscores[k - 1].Score;
                Highscores[k].Stage = Highscores[k - 1].Stage;
                Highscores[k].Skill = Highscores[k - 1].Skill;
                strcpy_s(Highscores[k].Name, Highscores[k - 1].Name);
            }

            Highscores[j].Score = Player[0].Score;
            NewScore = Player[0].Score;
            NewStage = Stage;
            NewSkill = Skill;
            Player[0].Score = 0;

            // Werte für den neuen Eintrag löschen
            NewRank = j;
            // DKS - Initialize NewName with just the cursor char
            strcpy_s(NewName, "_");
            strcpy_s(Highscores[NewRank].Name, "");
            AktuellerZustand = MENUZUSTAND_ENTERNAME;
            break;
        }

    // The high-score entry has captured the stage. A finished game cannot be resumed.
    Stage = -1;
    SpielZustand = GameStateEnum::MAINMENU;
}

// --------------------------------------------------------------------------------------
// Progress bar
// --------------------------------------------------------------------------------------

void MenuClass::ResetProgressBar() {
    LoadingProgress = 318.0f;
}

void MenuClass::StartProgressBar(int items) {
    LoadingProgress = 0.0f;
    LoadingItemsToLoad = static_cast<float>(items);
    LoadingItemsLoaded = 0;
}

bool MenuClass::UpdateLoadingProgress(bool finished) {
    if (!finished) {
        ++LoadingItemsLoaded;
        if (LoadingItemsToLoad > 0)
            LoadingProgress = std::min(318.0f, LoadingItemsLoaded * 318.0f / LoadingItemsToLoad);
    }

    const Uint32 now = SDL_GetTicks();
    if (!finished && LoadingItemsLoaded != 1 && now - LastLoadingFrame < 33)
        return false;
    LastLoadingFrame = now;
    return true;
}

void MenuClass::DrawProgressBar() {
    LoadingScreen.RenderSprite((RENDERWIDTH - 360) / 2, (RENDERHEIGHT - 60) / 2 + 5, 0x88FFFFFF);
    LoadingBar.SetRect(0, 0, static_cast<int>(LoadingProgress), 19);
    LoadingBar.RenderSprite((RENDERWIDTH - 318) / 2, (RENDERHEIGHT - 19) / 2 + 5, 0x88FFFFFF);
}

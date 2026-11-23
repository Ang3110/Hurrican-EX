// --------------------------------------------------------------------------------------
// Der Tutorial Text
//
// Befindet sich der Spieler nahe genug an dem Trigger, so wird
// der TutorialText mit der ID "Value1" eingefadet. Ist der Spieler wieder weit genug
// entfernt, wird dieser wieder ausgeblendet.
// --------------------------------------------------------------------------------------

#include "stdafx.hpp"

#include "Trigger_TutorialText.hpp"
#include "enemies/Gegner_PowerBlock.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerTutorialText::GegnerTutorialText(int Wert1, int Wert2, bool Light) {
    Handlung = GEGNER::STEHEN;
    BlickRichtung = DirectionEnum::LINKS;
    Energy = 100;
    Value1 = Wert1;
    Value2 = Wert2;
    AnimPhase = 0;
    ChangeLight = Light;
    Destroyable = false;
    Active = true;
    DontMove = true;
}

// --------------------------------------------------------------------------------------
// "Bewegungs KI"
// --------------------------------------------------------------------------------------

void GegnerTutorialText::DoKI() {
    bool near = false, usedShockwave = false, upgraded = false;
    int lines = 0, grenades = 0;
    const auto &rect = GegnerRect[GegnerArt];
    for (int p = 0; p < NUMPLAYERS; ++p) {
        auto &player = Player[p];
        if (!CoopPlayerReady(player) || player.StageClearRunning)
            continue;
        lines += player.PowerLines;
        grenades += player.Grenades;
        upgraded |= player.CurrentWeaponLevel[0] >= 2 && player.BlinkCounter <= 3.0f;
        const float dx = player.xpos + 35.0f - (xPos + rect.right * 0.5f);
        const float dy = player.ypos + 40.0f - (yPos + rect.bottom * 0.5f);
        const float distance = dx * dx + dy * dy;
        if (distance >= 120.0f * 120.0f)
            continue;
        near = true;
        if (Value1 == 28) {
            if (!receivedShockwave[p]) {
                player.SmartBombs = std::max(1, player.SmartBombs);
                receivedShockwave[p] = true;
            } else if (player.SmartBombs == 0) {
                usedShockwave = true;
            }
        }
        if (Value1 != 9)
            OfferTutorialLesson(p, Value1, distance);
    }
    if (!near)
        return;
    // Diamonds are shared by the game. Equipment is stored on its actual collector.
    if (Value1 == 7 && Player[0].CollectedDiamonds < 5)
        Value2 = Player[0].CollectedDiamonds = 5;
    bool stillAlive = false;
    switch (Value1) {
        // die ersten 5 Diamanten eingesammelt?
        //
        case 2:
            if (Player[0].CollectedDiamonds >= 5) {
                Value1 = 3;
            }
            break;

        // die nächsten 3 Diamanten eingesammelt?
        //
        case 7:
            if (Player[0].CollectedDiamonds >= 8) {
                Value1 = 8;
            }
            break;

        // die ersten drei augen plattgemacht?
        //
        case 9: {
            for (auto& pEnemy: Gegner.enemies) {
                // noch mind. ein Auge am Leben=
                //
                if (pEnemy->GegnerArt == AUGE && pEnemy->Value2 == 99)
                    stillAlive = true;
            }

            if (stillAlive == false) {
                Value1 = 10;
            }
        } break;

        // Spreadshot das erste mal aufgepowert
        //
        case 10: {
            if (upgraded) {
                Value1 = 11;
            }
        } break;

        // Pfeil auf Spreadshot Stufe anzeigen
        case 11: {
            HUD.ShowArrow(256, 60);
        } break;

        // Pfeil auf Blitz Stufe anzeigen
        case 15: {
            HUD.ShowArrow(351, 60);
        } break;

        // ersten Powerblock angeschoßen?
        case 16: {
            for (auto& pEnemy: Gegner.enemies) {
                if (pEnemy->GegnerArt == POWERBLOCK) {
                    // angeschoßenen PowerBlock suchen
                    //
                    GegnerPowerBlock *pPower = reinterpret_cast<GegnerPowerBlock *>(pEnemy.get());

                    if (pPower->ExtraType == 7 && pPower->AnimPhase == 1) {
                        Value1 = 17;
                        break;
                    }
                }
            }
        } break;

        // alle PowerLines gesammelt?
        case 17: {
            if (lines >= 5)
                Value1 = 18;
        } break;

        // Pfeil auf PowerLines
        case 18: {
            HUD.ShowArrow(391, 60);
        } break;

        // alle Granaten gesammelt?
        case 20: {
            if (grenades >= 2) {
                for (int p = 0; p < NUMPLAYERS; ++p)
                    Player[p].PowerLines = 0;
                Value1 = 21;
            }
        } break;

        // Pfeil auf Rad-Energie
        case 24: {
            HUD.ShowArrow(548, 70);
        } break;

        // Meldung nach der Schockwaver
        case 28: {
            if (usedShockwave)
                Value1 = 29;
        } break;

        // Pfeil auf Shockwave
        case 29: {
            HUD.ShowArrow(434, 42);
        } break;

        default:
            break;
    }
}

// --------------------------------------------------------------------------------------
// Trigger explodiert
// --------------------------------------------------------------------------------------

void GegnerTutorialText::GegnerExplode() {}

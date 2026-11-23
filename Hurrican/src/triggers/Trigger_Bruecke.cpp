// --------------------------------------------------------------------------------------
// Die Hängebrücke
//
// Ein Teil der Brücke
// Hängt je nach Abstand zum Spieler oder einem Objekt unterschiedlich weit runter
// --------------------------------------------------------------------------------------

#include "Trigger_Bruecke.hpp"
#include "stdafx.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerBruecke::GegnerBruecke(int Wert1, int Wert2, bool Light) {
    Handlung = GEGNER::LAUFEN;
    BlickRichtung = DirectionEnum::RECHTS;
    Energy = 100;
    Value1 = Wert1;
    Value2 = Wert2;
    ChangeLight = Light;
    Destroyable = false;
    AnimCount = 0.0f;
    TestBlock = false;
}

// --------------------------------------------------------------------------------------
// "Bruecke KI"
// --------------------------------------------------------------------------------------

void GegnerBruecke::DoKI() {
    const float rest = static_cast<float>(Value1);
    const auto &link = GegnerRect[BRUECKE];
    if (!TileEngine.IsInPlayerViews(xPos, rest, xPos + link.right, rest + link.bottom + 32.0f))
        return;

    // Compare every load to the undeformed chain. Feet can touch the platform
    // exactly without the two collision rectangles overlapping.
    const auto bearsWeight = [&](float x, float y, const RECT_struct &body) {
        const float feet = y + body.bottom;
        return x + body.right > xPos && x + body.left < xPos + link.right &&
               feet >= rest - 2.0f && feet <= rest + link.bottom + 2.0f;
    };
    float sag = 0.0f;
    for (int p = 0; p < NUMPLAYERS; ++p) {
        const auto &player = Player[p];
        if (CoopPlayerReady(player) && player.yspeed >= 0.0f &&
            bearsWeight(player.xpos, player.ypos, player.CollideRect)) {
            const float distance = std::hypot(player.xpos + 35.0f - (xPos + 5.0f),
                                              player.ypos + 40.0f - (rest + 5.0f));
            sag = std::max(sag, 32.0f - distance / 1.5f);
        }
    }
    for (const auto &enemy : Gegner.enemies) {
        if (!enemy->Active || enemy->DontMove || enemy->Energy <= 0.0f || enemy->ySpeed != 0.0f ||
            enemy->GegnerArt == BRUECKE || enemy->GegnerArt == STAHLMUECKE)
            continue;
        const auto &body = GegnerRect[enemy->GegnerArt];
        if (!bearsWeight(enemy->xPos, enemy->yPos, body))
            continue;
        const float w = (body.right - body.left) * 0.5f;
        const float h = enemy->GegnerArt == DIAMANT ? 2.0f : (body.bottom - body.top) * 0.5f;
        float distance = std::min(40.0f, std::hypot(enemy->xPos + w - (xPos + 5.0f),
                                                   enemy->yPos + h - (rest + 5.0f)));
        if (enemy->GegnerArt == SPITTER)
            distance += 13.0f;
        sag = std::max(sag, enemy->yPos + body.bottom - rest + 13.0f - distance / 2.5f);
    }
    yPos = rest + sag;
}

// --------------------------------------------------------------------------------------
// Bruecke explodiert (nicht)
// --------------------------------------------------------------------------------------

void GegnerBruecke::GegnerExplode() {}

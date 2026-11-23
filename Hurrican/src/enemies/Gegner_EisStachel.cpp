// --------------------------------------------------------------------------------------
// Der sich drehende Eisstachel
//
// Bewegt sich sinusförmig (auf der x-achse) hin und her und dreht sich dabei
// --------------------------------------------------------------------------------------

#include "Gegner_EisStachel.hpp"
#include "stdafx.hpp"
#include <algorithm>
#include <cmath>

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerEisStachel::GegnerEisStachel(float x, float y, int Wert1, int Wert2, bool Light) {
    Handlung = GEGNER::LAUFEN;
    Energy = 100;
    Value1 = Wert1;
    Value2 = Wert2;
    ChangeLight = Light;
    Destroyable = false;
    xSpeed = 0.0f;
    ySpeed = 0.0f;
    xAcc = 1.7f;
    AnimSpeed = 1.0f;
    AnimEnde = 19;
    Active = true;

    // The 78-pixel sprite's base meets the floor below its damage rectangle.
    const auto &body = GegnerRect[EISSTACHEL];
    const int row = static_cast<int>(std::floor((y + 78) / TILESIZE_Y));
    const int column = static_cast<int>(std::floor((x + (body.left + body.right) * 0.5f) / TILESIZE_X));
    if (row < 0 || row >= TileEngine.LEVELSIZE_Y || row * TILESIZE_Y < y + body.bottom - 1)
        return;
    const auto hasFloor = [row](int tile) {
        return tile >= 0 && tile < TileEngine.LEVELSIZE_X &&
               (TileEngine.TileAt(tile, row).Block & (BLOCKWERT_WAND | BLOCKWERT_PLATTFORM));
    };
    if (!hasFloor(column))
        return; // A map may deliberately place a free-moving spike in the air.

    int first = column, last = column;
    while (hasFloor(first - 1))
        --first;
    while (hasFloor(last + 1))
        ++last;

    float left = static_cast<float>(first * TILESIZE_X - body.left);
    float right = static_cast<float>((last + 1) * TILESIZE_X - body.right);
    if (left < right) {
        const float start = std::clamp(x, left, right);
        constexpr auto walls = BLOCKWERT_WAND | BLOCKWERT_GEGNERWAND;
        left = TileEngine.ClipWallMotion(start, left, y, body, true, walls);
        right = TileEngine.ClipWallMotion(start, right, y, body, true, walls);
    }
    patrolCenter = (left + right) * 0.5f;
    patrolRadius = std::max(0.0f, (right - left) * 0.5f);
    if (patrolRadius > 0.0f)
        patrolPhase = std::acos(std::clamp((patrolCenter - x) / patrolRadius, -1.0f, 1.0f));
    xAcc = 0.0f; // The bounded patrol supplies its own position.
}

// --------------------------------------------------------------------------------------
// "Bewegungs KI"
// --------------------------------------------------------------------------------------

void GegnerEisStachel::DoKI() {
    SimpleAnimation();

    // Je nach Handlung richtig verhalten
    switch (Handlung) {
        case GEGNER::LAUFEN:  // Umherdrehen
        {
            if (patrolRadius >= 0.0f) {
                // Keep the authored peak speed and ease through each turnaround.
                // Absolute positions stay on this platform across variable frame times.
                if (patrolRadius > 0.0f)
                    patrolPhase = std::fmod(patrolPhase + Timer.sync(static_cast<float>(Value1)) / patrolRadius,
                                            TWO_PI);
                xPos = patrolCenter - patrolRadius * cosf(patrolPhase);
            } else if ((xSpeed > Value1 && xAcc > 0.0f) || (xSpeed < -Value1 && xAcc < 0.0f)) {
                xAcc = -xAcc;
            }
        } break;

        default:
            break;
    }  // switch

    Wegschieben(GegnerRect[GegnerArt], 10.0f);
}

// --------------------------------------------------------------------------------------
// EisStachel explodiert
// --------------------------------------------------------------------------------------

void GegnerEisStachel::GegnerExplode() {}

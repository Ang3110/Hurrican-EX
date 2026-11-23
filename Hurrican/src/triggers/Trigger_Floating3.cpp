// --------------------------------------------------------------------------------------
// Die große schwebende Plattform, die sich dreht, wenn man draufhopst
//
// Geht ein Stück nach unten, wenn der Hurri draufhopst
// Je schneller die Fallgeschwindigkeit beim aufkommen, desto weiter geht sie nach unten
// --------------------------------------------------------------------------------------

#include "Trigger_Floating3.hpp"
#include "stdafx.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerFloating3::GegnerFloating3(int Wert1, int Wert2, bool Light) {
    Handlung = GEGNER::STEHEN;
    BlickRichtung = DirectionEnum::RECHTS;
    Energy = 100;
    Value1 = Wert1;
    Value2 = Wert2;
    if (Value1 == 0)
        Value1 = 16;
    ChangeLight = Light;
    Destroyable = false;
    AnimCount = 0.0f;
    winkel = 0.0f;
    turncount = 0.0f;
    drehdir = 0.0f;

    OwnDraw = true;
}

// --------------------------------------------------------------------------------------
// Eigene Drawfunktion
// --------------------------------------------------------------------------------------

void GegnerFloating3::DoDraw() {
    // Standfläche rendern
    //
    pGegnerGrafix[GegnerArt]->itsRect = pGegnerGrafix[GegnerArt]->itsPreCalcedRects[0];
    pGegnerGrafix[GegnerArt]->RenderSpriteRotatedOffset(xPos - TileEngine.XOffset,
                                                        yPos - TileEngine.YOffset, winkel, 0.0f,
                                                        -6.0f, 0xFFFFFFFF);

    // Mittelteil rendern
    //
    pGegnerGrafix[GegnerArt]->itsRect = pGegnerGrafix[GegnerArt]->itsPreCalcedRects[1];
    pGegnerGrafix[GegnerArt]->RenderSprite(xPos - TileEngine.XOffset,
                                           yPos - TileEngine.YOffset, 1, 0xFFFFFFFF);
}

// --------------------------------------------------------------------------------------
// Floating3 KI
// --------------------------------------------------------------------------------------

void GegnerFloating3::DoKI() {
    switch (Handlung) {
        case GEGNER::STEHEN: {
            if (auto *landing = PlattformTest(GegnerRect[GegnerArt])) {
                if (turncount == 0.0f) {
                    turncount = 0.01f;
                    drehdir = landing->xpos + 35 < xPos + 50 ? 1.0f : -1.0f;
                }
                if (ySpeed == 0.0f) {
                    Value2 = static_cast<int>(yPos);
                    ySpeed = landing->yspeed / 5.0f;
                    yAcc = -3.0f;
                }
            }

            // Spieler war drauf? Dann Counter hochzählen, dass die Plattform sich bald dreht
            if (turncount > 0.0f)
                turncount += Timer.sync(1.0f);

            // drehen? -> Spieler fällt runter
            if (turncount > 8.0f) {
                // Noch drauf? Dann nochmal Drehrichtung rausfinden
                for (int p = 0; p < NUMPLAYERS; p++) {
                    if (Player[p].AufPlattform == this) {
                        if (Player[p].xpos + 35 < xPos + 50)
                            drehdir = 1.0f;
                        else
                            drehdir = -1.0f;
                        Player[p].AufPlattform = nullptr;
                    }
                }
                turncount = 0.0f;
                Handlung = GEGNER::DREHEN;
            }
        } break;

        // plattform dreht sich
        case GEGNER::DREHEN: {
            winkel += Timer.sync(25.0f * drehdir);

            if (winkel > 180.0f || winkel < -180.0f) {
                winkel = 0.0f;
                Handlung = GEGNER::STEHEN;
            }
        } break;
    }

    // Plattform wieder an Ausgangspos ?
    //
    if (ySpeed < 0.0f && static_cast<int>(yPos) <= Value2) {
        ySpeed = 0.0f;
        yAcc = 0.0f;
        yPos = static_cast<float>(Value2);
    }

    // Counter für Partikel runterzählen
    //
    AnimCount -= Timer.sync(1.0f);

    // Partikel erzeugen und counter wieder hochsetzen
    //
    if (AnimCount <= 0.0f) {
        AnimCount = 0.15f;
        PartikelSystem.PushPartikel(xPos + 44.0f, yPos + 21.0f, FLOATSMOKE);
        PartikelSystem.PushPartikel(xPos + 27.0f, yPos + 10.0f, FLUGSACKSMOKE);
        PartikelSystem.PushPartikel(xPos + 57.0f, yPos + 10.0f, FLUGSACKSMOKE2);
    }
}

// --------------------------------------------------------------------------------------
// Floating3 explodiert (nicht)
// --------------------------------------------------------------------------------------

void GegnerFloating3::GegnerExplode() {}

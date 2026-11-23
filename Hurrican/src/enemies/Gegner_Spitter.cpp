// --------------------------------------------------------------------------------------
// Der Spitter
//
// Bewegt sich nur von links nach rechts und dreht um, wenn er an eine Wand gelangt
// und schiesst in regelmässigen Abständen senkrecht nach oben
// --------------------------------------------------------------------------------------

#include "Gegner_Spitter.hpp"
#include "stdafx.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerSpitter::GegnerSpitter(int Wert1, int Wert2, bool Light) {
    Handlung = GEGNER::LAUFEN;
    AnimEnde = 4;
    AnimSpeed = 1.25f;
    Energy = 20;
    Value1 = Wert1;
    Value2 = Wert2;
    ChangeLight = Light;
    Destroyable = true;
}

// --------------------------------------------------------------------------------------
// "Bewegungs KI"
// --------------------------------------------------------------------------------------

void GegnerSpitter::DoKI() {
    // Animieren
    if (AnimEnde > 0)  // Soll überhaupt anmiert werden ?
    {
        AnimCount += Timer.getSpeedFactor();   // Animationscounter weiterzählen
        if (AnimCount > AnimSpeed)  // Grenze überschritten ?
        {
            AnimCount = 0;  // Dann wieder auf Null setzen
            AnimPhase++;    // Und nächste Animationsphase

            if (AnimPhase == 2) {
                Projectiles.PushProjectile(xPos + 8.0f, yPos, STRAIGHTSCHUSS);
                PartikelSystem.PushPartikel(xPos, yPos - 8.0f, SMOKE);
            }

            if (AnimPhase >= AnimEnde)  // Animation von zu Ende	?
                AnimPhase = 0;          // Dann wieder von vorne beginnen
        }
    }  // animieren

    // Spieler verfolgen
    //

    if (PlayerAbstand() > 100) {
        if (pAim->xpos < xPos) {
            BlickRichtung = DirectionEnum::LINKS;
            xAcc = -2.0f;
        } else {
            BlickRichtung = DirectionEnum::RECHTS;
            xAcc = 2.0f;
        }
    }

    if (xSpeed < -7.0f) {
        xSpeed = -7.0f;
        xAcc = 0.0f;
    }

    if (xSpeed > 7.0f) {
        xSpeed = 7.0f;
        xAcc = 0.0f;
    }

    if (blocku & (BLOCKWERT_WAND | BLOCKWERT_PLATTFORM | BLOCKWERT_SCHRAEGE_L | BLOCKWERT_SCHRAEGE_R)) {
        ySpeed = 0.0f;
        yAcc = 0.0f;
    } else {
        if (ySpeed == 0.0f)
            ySpeed = 5.0f;
        yAcc = 3.0f;
    }

    // An der Wand umdrehen
    //

    if (xSpeed < 0.0f) {
        if (!(blocku & BLOCKWERT_SCHRAEGE_L) && (blockl & BLOCKWERT_WAND || blockl & BLOCKWERT_GEGNERWAND))
            xSpeed *= -1.0f;
    }

    if (xSpeed > 0.0f) {
        if (!(blocku & BLOCKWERT_SCHRAEGE_R) && (blockr & BLOCKWERT_WAND || blockr & BLOCKWERT_GEGNERWAND))
            xSpeed *= -1.0f;
    }

    // Testen, ob der Spieler den Spitter berührt hat
    //
    TestDamagePlayers(Timer.sync(1.0f));
}

// --------------------------------------------------------------------------------------
// Spitter explodiert
// --------------------------------------------------------------------------------------

void GegnerSpitter::GegnerExplode() {
    Player[0].Score += 50;

    PartikelSystem.PushPartikel(xPos - 10.0f, yPos - 10.0f, EXPLOSION_MEDIUM);

    SoundManager.PlayWave(100, 128, -GetRandom(2000) + 11025, SOUND::EXPLOSION1);  // Sound ausgeben
}

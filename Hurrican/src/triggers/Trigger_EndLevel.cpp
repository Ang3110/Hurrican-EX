// --------------------------------------------------------------------------------------
// Der End Level Trigger
//
// Berührt ihn der Spieler so kommt er in
// die Stage, die bei Value1 angegeben ist
// --------------------------------------------------------------------------------------

#include "Trigger_EndLevel.hpp"
#include "stdafx.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

GegnerEndLevel::GegnerEndLevel(int Wert1, int Wert2, bool Light) {
    Handlung = GEGNER::STEHEN;
    Value1 = Wert1;
    Value2 = Wert2;
    ChangeLight = Light;
    Destroyable = false;
    Energy = 100;
    DontMove = true;
}

// --------------------------------------------------------------------------------------
// "Bewegungs KI"
// --------------------------------------------------------------------------------------

void GegnerEndLevel::DoKI() {
    if (Energy <= 0.0f || (NewStage == Value1 && !RunningTutorial) || Player[0].StageClearRunning)
        return;

    const auto &rect = GegnerRect[ENDLEVEL];
    const bool needsPartner = CoopNeedsPartner();
    if (!needsPartner && !RunningTutorial && !TileEngine.IsInPlayerViews(xPos, yPos, xPos + rect.right, yPos + rect.bottom, false))
        return;

    // Notice a co-op arrival before the suit crosses the doorway. The original
    // trigger still starts the exit sequence once the party has gathered.
    auto approach = rect;
    if (needsPartner) {
        switch (Value2) {
            case AKTION_LINKS:  approach.right += 4 * TILESIZE_X; break;
            case AKTION_RECHTS: approach.left -= 4 * TILESIZE_X; break;
            case AKTION_OBEN:   approach.bottom += 4 * TILESIZE_Y; break;
            case AKTION_UNTEN:  approach.top -= 4 * TILESIZE_Y; break;
        }
    }
    const auto overlaps = [](const PlayerClass &player, const GegnerClass &exit, const RECT_struct &area) {
        return CoopPlayerReady(player) &&
               SpriteCollision(exit.xPos, exit.yPos, area, player.xpos, player.ypos, player.CollideRect);
    };
    int first = 0;
    while (first < NUMPLAYERS && !overlaps(Player[first], *this, approach))
        ++first;
    if (first == NUMPLAYERS)
        return;

    const GegnerClass *exitLift = nullptr;
    if (needsPartner && Value2 == AKTION_DUCKEN) {
        for (const auto &object : Gegner.enemies) {
            if (object->GegnerArt == FAHRSTUHL && object->Energy > 0.0f &&
                std::abs(object->yPos - yPos) <= TILESIZE_Y &&
                object->xPos < xPos + rect.right && object->xPos + GegnerRect[FAHRSTUHL].right > xPos) {
                exitLift = object.get();
                break;
            }
        }
    }

    if (needsPartner) {
        const auto &partner = Player[1 - first];
        auto partnerArea = approach;
        if (RunningTutorial) {
            partnerArea.left = std::min(partnerArea.left, rect.left - 3 * TILESIZE_X);
            partnerArea.right = std::max(partnerArea.right, rect.right + 3 * TILESIZE_X);
            partnerArea.top = std::min(partnerArea.top, rect.top - 2 * TILESIZE_Y);
            partnerArea.bottom = std::max(partnerArea.bottom, rect.bottom + 2 * TILESIZE_Y);
        }
        bool partnerArrived = overlaps(partner, *this, partnerArea);
        if (exitLift) {
            // The Temple exit sets both players to crouch as the lift descends.
            partnerArrived = CoopPlayerReady(partner) && Player[first].AufPlattform == exitLift &&
                             partner.AufPlattform == exitLift;
        } else if (!partnerArrived) {
            // A wide entrance can be built out of multiple neighboring exit triggers.
            partnerArrived = std::any_of(Gegner.enemies.begin(), Gegner.enemies.end(), [&](const auto &exit) {
                return exit->GegnerArt == ENDLEVEL && exit->Energy > 0.0f &&
                       exit->Value1 == Value1 && exit->Value2 == Value2 &&
                       std::abs(exit->xPos - xPos) <= rect.right * 2 &&
                       std::abs(exit->yPos - yPos) <= rect.bottom * 2 && overlaps(partner, *exit, approach);
            });
        }

        if (!partnerArrived) {
            // Keep three tiles between the waiting suit and the doorway.
            const auto &body = Player[first].CollideRect;
            switch (Value2) {
                case AKTION_LINKS:  WaitForCoopPartner(first, CoopGateSide::Left, xPos + approach.right - body.left - TILESIZE_X); break;
                case AKTION_RECHTS: WaitForCoopPartner(first, CoopGateSide::Right, xPos + approach.left - body.right + TILESIZE_X); break;
                case AKTION_OBEN:   WaitForCoopPartner(first, CoopGateSide::Top, yPos + approach.bottom - body.top - TILESIZE_Y); break;
                case AKTION_UNTEN:  WaitForCoopPartner(first, CoopGateSide::Bottom, yPos + approach.top - body.bottom + TILESIZE_Y); break;
                default: WaitForCoopPartner(first); break;
            }
            return;
        }

        if (!overlaps(Player[first], *this, rect) && !overlaps(partner, *this, rect))
            return;

        if (!exitLift) {
            const auto camera = TileEngine.ClampCameraOrigin({
                (Player[0].xpos + Player[1].xpos) * 0.5f + 35.0f - RENDERWIDTH / 2.0f,
                (Player[0].ypos + Player[1].ypos) * 0.5f + 40.0f - RENDERHEIGHT / 2.0f
            });
            TileEngine.XOffset = camera.x;
            TileEngine.YOffset = camera.y;
        }
    }

    if (!exitLift || TileEngine.Zustand == TileStateEnum::SCROLLBAR)
        TileEngine.ScrollLevel(TileEngine.XOffset, TileEngine.YOffset, TileStateEnum::SCROLLTOLOCK);

    Energy = 0.0f;
    for (int p = 0; p < NUMPLAYERS; ++p) {
        Player[p].FesteAktion = Value2;
        Player[p].DoFesteAktion = true;
    }
    NewStage = Value1;

    if (RunningTutorial && Value1 <= 9) {
        SoundManager.PlayWave(100, 128, 11025, SOUND::MARIO);
        StageClear(false);
    } else
        StageClear(true);
}

// --------------------------------------------------------------------------------------
// EndLevel explodiert (nicht *g*)
// --------------------------------------------------------------------------------------

void GegnerEndLevel::GegnerExplode() {}

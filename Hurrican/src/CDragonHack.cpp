// Datei : CDragonHack.cpp

// --------------------------------------------------------------------------------------
//
// Klasse für den Drache, der im Hintergrund beim Turmlevel rumfliegt
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------------

#include "CDragonHack.hpp"
#include "Tileengine.hpp"
#include "Timer.hpp"

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

CDragonHack::CDragonHack() {
    m_GFX.LoadImage("drache_klein.png", 160, 160, 160, 40, 1, 4);

    m_AnimCount = 0.0f;
    m_xPos = 0.0f;
    m_yPos = 0.0f;
    m_xSpeed = 0.0f;
    m_ySpeed = 0.0f;
    m_State = DragonState::WAIT;

    m_AppearCount = 5.0f;
}

// --------------------------------------------------------------------------------------
// Destruktor
// --------------------------------------------------------------------------------------

CDragonHack::~CDragonHack() {}

// --------------------------------------------------------------------------------------
// rumfliegen lassen
// --------------------------------------------------------------------------------------

void CDragonHack::Run() {
    m_AnimCount += Timer.sync(3.0f);

    if (m_AnimCount >= 4.0f)
        m_AnimCount = 0.0f;

    switch (m_State) {
        // nicht zu sehen
        case DragonState::WAIT: {
            if (m_AppearCount > 0.0f)
                m_AppearCount -= Timer.sync(1.0f);
            else {
                m_AppearCount = 20.0f;
                m_State = DragonState::FLY;
                const float left = TileEngine.IsCoopSplit() ? TileEngine.CoopView(0).x : TileEngine.XOffset;
                const float top = TileEngine.IsCoopSplit() ? TileEngine.CoopView(0).y : TileEngine.YOffset;
                const int width = TileEngine.IsCoopSplit() ? TileEngine.CoopView(0).w : RENDERWIDTH;

                // von links
                if (GetRandom(2) == 0) {
                    m_xPos = left - 160.0f;
                    m_xSpeed = 10.0f;
                }

                // von rechts
                else {
                    m_xPos = left + width + 160.0f;
                    m_xSpeed = -10.0f;
                }

                m_yPos = top + 200.0f;
                m_ySpeed = static_cast<float>(GetRandom(9) - 4) / 5.0f;
            }
        } break;

        // fliegen lassen, bis er aus dem Bild ist
        case DragonState::FLY: {
            m_xPos += Timer.sync(m_xSpeed);
            m_yPos += Timer.sync(m_ySpeed);

            // Ausserhalb des sichtbaren Bereichs? Dann verschwinden lassen
            if (!TileEngine.IsInPlayerViews(m_xPos - 260.0f, m_yPos - 240.0f, m_xPos + 260.0f, m_yPos + 240.0f, false)) {
                m_State = DragonState::WAIT;
                m_AppearCount = GetRandom(100) + 50.0f;
            }
        } break;
    }
}

void CDragonHack::Draw() {
    if (m_State == DragonState::FLY)
        m_GFX.RenderSprite(m_xPos - TileEngine.XOffset, m_yPos - TileEngine.YOffset,
                           static_cast<int>(m_AnimCount), 0xDDFFFFFF, m_xSpeed > 0.0f);
}

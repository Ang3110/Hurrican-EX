#ifndef _GUISYSTEM_HPP_
#define _GUISYSTEM_HPP_

#include "DX8Font.hpp"
#include "DX8Graphics.hpp"
#include "DX8Sprite.hpp"

// --------------------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------------------

constexpr int TILESIZE = 20;

constexpr int BOXTEXTLENGTH = 2000;

enum class FadeMode {
  INVISIBLE,
  VISIBLE,
  FADEIN,
  FADEOUT,
};

// --------------------------------------------------------------------------------------
// GUI Klasse
// --------------------------------------------------------------------------------------

class CGUISystem {
  private:
    float m_xPos, m_yPos;
    DirectGraphicsSprite m_Rahmen;
    RECT_struct m_BoxSize;

    FadeMode m_FadeMode;
    float m_FadingAlpha;
    void DrawBoxFrame(float x, float y, int width, int height, D3DCOLOR color);

  public:
    static constexpr int panelOpacity = 200;
    CGUISystem();
    ~CGUISystem();

    void InitGUISystem();

    void Run();
    void RenderBox();
    void DrawCompactFrame(const SDL_Rect &bounds, D3DCOLOR color);
    void DrawTutorialCard(int lesson, int owner, int profile, float opacity, const SDL_Rect &space, bool shared);
    void ShowSummaryFrame(const SDL_Rect &bounds);
    void HideBox();
    void HideBoxFast();

    FadeMode GetFadeMode() const { return m_FadeMode; }
    float GetFadingAlpha() const { return m_FadingAlpha; }
};

// --------------------------------------------------------------------------------------
// Externals
// --------------------------------------------------------------------------------------

extern CGUISystem GUI;

#endif

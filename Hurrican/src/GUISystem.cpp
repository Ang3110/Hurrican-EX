#include "GUISystem.hpp"
#include "Main.hpp"
#include "Timer.hpp"
#include "Texts.hpp"

// --------------------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------------------

constexpr float FADESPEED = 40.0f;


//
// Konstruktor
//

CGUISystem::CGUISystem() {}

//
// Destruktor
//

CGUISystem::~CGUISystem() {}

//
// GUISystem initialisieren
//

void CGUISystem::InitGUISystem() {
    m_Rahmen.LoadImage("guiback.png", 60, 60, 20, 20, 3, 3);

    m_BoxSize.top = 0;
    m_BoxSize.left = 0;
    m_BoxSize.right = 0;
    m_BoxSize.bottom = 0;

    m_FadingAlpha = 0.0f;
    m_FadeMode = FadeMode::INVISIBLE;

}

//
// Hinweisbox rendern
//

void CGUISystem::DrawCompactFrame(const SDL_Rect &bounds, D3DCOLOR color) {
    // Use the edging and scanlines from the middle column of the original guiback.
    // Build the outline in the pane's pixel grid so resizing a panel never thins one side.
    constexpr int cornerInsets[] = {4, 2, 1, 0};
    constexpr int cornerSize = static_cast<int>(std::size(cornerInsets));
    if (bounds.w < 2 * cornerSize || bounds.h < 2 * cornerSize)
        return;
    const float u = (TILESIZE + 0.5f) * m_Rahmen.itsXTexScale;
    DirectGraphics.SetTexture(m_Rahmen.itsTexIdx);
    DirectGraphics.SetFilterMode(true);
    for (int y = 0; y < bounds.h;) {
        const int distance = std::min(y, bounds.h - 1 - y);
        const int rows = distance >= cornerSize ? std::min(TILESIZE, bounds.h - y - cornerSize) : 1;
        const auto inset = [&](int layer) -> float {
            if (distance < layer)
                return bounds.w * 0.5f;
            const int d = distance - layer;
            return static_cast<float>(d < cornerSize ? std::max(layer, cornerInsets[d]) : layer);
        };
        const float outside = inset(0), rim = inset(1), inside = inset(2);
        std::array<VERTEX2D, 30> vertices;
        int count = 0;
        const auto span = [&](float left, float right, float srcTop, float srcBottom) {
            if (right <= left)
                return;
            const float l = bounds.x + left, r = bounds.x + right;
            const float t = static_cast<float>(bounds.y + y), b = t + rows;
            const float v0 = srcTop * m_Rahmen.itsYTexScale, v1 = srcBottom * m_Rahmen.itsYTexScale;
            const VERTEX2D tl{l, t, color, u, v0}, tr{r, t, color, u, v0};
            const VERTEX2D bl{l, b, color, u, v1}, br{r, b, color, u, v1};
            for (const auto &vertex : {tl, tr, bl, bl, tr, br})
                vertices[count++] = vertex;
        };
        const float scanline = static_cast<float>(TILESIZE + y % 2);
        span(outside, rim, 0.5f, 0.5f);
        span(rim, inside, 1.5f, 1.5f);
        span(inside, bounds.w - inside, scanline, scanline + rows);
        span(bounds.w - inside, bounds.w - rim, 1.5f, 1.5f);
        span(bounds.w - rim, bounds.w - outside, 0.5f, 0.5f);
        DirectGraphics.RendertoBuffer(GL_TRIANGLES, count / 3, vertices.data());
        y += rows;
    }
    DirectGraphics.SetFilterMode(false);
}

void CGUISystem::DrawBoxFrame(float x, float y, int width, int height, D3DCOLOR color) {
    // Ecken
    //
    m_Rahmen.RenderSprite(x, y, 0, color);

    m_Rahmen.RenderSprite(x + width + TILESIZE, y, 2, color);

    m_Rahmen.RenderSprite(x, y + height + TILESIZE, 6, color);

    m_Rahmen.RenderSprite(x + width + TILESIZE, y + height + TILESIZE, 8, color);

    // Rand oben/unten
    for (int i = 0; i < (width) / TILESIZE; i++) {
        m_Rahmen.RenderSprite(x + (i + 1) * TILESIZE, y, 1, color);
        m_Rahmen.RenderSprite(x + (i + 1) * TILESIZE, y + height + 20, 7, color);
    }

    // Rand links/rechts
    for (int i = 0; i < (height) / TILESIZE; i++) {
        m_Rahmen.RenderSprite(x, y + (i + 1) * TILESIZE, 3, color);
        m_Rahmen.RenderSprite(x + width + 20, y + (i + 1) * TILESIZE, 5, color);
    }

    // Hintergrund in der Mitte
    for (int i = 0; i < (width) / TILESIZE; i++)
        for (int j = 0; j < (height) / TILESIZE; j++)
            m_Rahmen.RenderSprite(x + (i + 1) * TILESIZE, y + (j + 1) * TILESIZE, 4, color);
}

void CGUISystem::RenderBox() {
    const int alpha = static_cast<int>(m_FadingAlpha);
    if (alpha < 0)
        return;
    const D3DCOLOR color = D3DCOLOR_RGBA(0, 255, 0, alpha);
    DrawCompactFrame({static_cast<int>(m_xPos), static_cast<int>(m_yPos), m_BoxSize.right, m_BoxSize.bottom}, color);
}

void CGUISystem::DrawTutorialCard(int lesson, int owner, int profile, float opacity, const SDL_Rect &space, bool shared) {
    if (space.w < 110 || space.h < 48 || opacity <= 0.0f)
        return;
    const auto &words = TutorialWords(lesson);
    if (words.empty())
        return;
    auto &font = *pDefaultFont;
    const int oldScale = font.GetScaleFactor();
    if (!shared)
        font.SetScaleFactor(1);
    const int pad = shared ? 21 : 8;
    const int gap = font.StringLength(" ");
    const int rowHeight = font.GetYCharSize() + (shared ? 6 : 2);
    struct PlacedWord { int x, y; };
    std::vector<PlacedWord> positions;
    positions.reserve(words.size());
    const int limit = std::min(shared ? 420 : 345, space.w - 2 * pad);
    int cursor = 0, row = 0, widest = 0;
    for (const auto &word : words) {
        int width = font.StringLength(word[profile].c_str());
        if (owner < 0)
            width = std::max(width, font.StringLength(word[1 - profile].c_str()));
        if (cursor && cursor + width > limit) {
            ++row;
            cursor = 0;
        }
        positions.push_back({cursor, row * rowHeight});
        widest = std::max(widest, cursor + width);
        cursor += width + gap;
    }
    int w = widest + 2 * pad, h = row * rowHeight + font.GetYCharSize() + 2 * pad;
    if (shared) {
        w = ((w + TILESIZE - 1) / TILESIZE) * TILESIZE;
        h = ((h + TILESIZE - 1) / TILESIZE) * TILESIZE;
    }
    const float scale = std::min({1.0f, static_cast<float>(space.w) / w, static_cast<float>(space.h) / h});
    const int width = static_cast<int>(std::round(w * scale)), height = static_cast<int>(std::round(h * scale));
    const int x = space.x + (space.w - width) / 2;
    const int y = shared ? std::clamp(100, space.y, space.y + space.h - height) : space.y + (space.h - height) / 4;
    const auto frameColor = D3DCOLOR_RGBA(0, 255, 0, static_cast<int>(panelOpacity * opacity));
    const auto ink = D3DCOLOR_RGBA(0, 255, 0, static_cast<int>(255 * opacity));
    if (shared && scale == 1.0f)
        DrawBoxFrame(x, y, width - 2 * TILESIZE, height - 2 * TILESIZE, frameColor);
    else
        DrawCompactFrame({x, y, width, height}, frameColor);

    const auto previous = g_matModelView;
    g_matModelView = glm::scale(glm::translate(previous, glm::vec3(x, y, 0)), glm::vec3(scale, scale, 1));
#if defined(USE_GL1)
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
    for (std::size_t i = 0; i < words.size(); ++i)
        font.DrawText(pad + positions[i].x, pad + positions[i].y, words[i][profile].c_str(), ink);
    g_matModelView = previous;
#if defined(USE_GL1)
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
    if (!shared)
        font.SetScaleFactor(oldScale);
}

//
// Hinweisbox an x/y mit breite/höhe w/h setzen
//

void CGUISystem::ShowSummaryFrame(const SDL_Rect &bounds) {
    m_xPos = static_cast<float>(bounds.x);
    m_yPos = static_cast<float>(bounds.y);
    m_BoxSize.right = bounds.w;
    m_BoxSize.bottom = bounds.h;
    m_FadeMode = FadeMode::FADEIN;
}

//
// Hinweisbox für Tutorial Level ausblenden
//

void CGUISystem::HideBox() {
    m_FadeMode = FadeMode::FADEOUT;
}

//
// Hinweisbox für Tutorial Level gleich komplett abschalten
//

void CGUISystem::HideBoxFast() {
    m_FadingAlpha = 0.0f;
    m_FadeMode = FadeMode::INVISIBLE;
}

//
// GUI anzeigen und evtl faden
//

void CGUISystem::Run() {
    // nicht faden? dann gleich abhauen
    //
    if (m_FadeMode == FadeMode::INVISIBLE)
        return;

    // Box rendern
    //

    RenderBox();

    // Einfaden
    //

    if (m_FadeMode == FadeMode::FADEIN) {
        if (m_FadingAlpha < panelOpacity)
            m_FadingAlpha = std::min(static_cast<float>(panelOpacity), m_FadingAlpha + Timer.sync(FADESPEED));
        else {
            m_FadingAlpha = panelOpacity;
            m_FadeMode = FadeMode::VISIBLE;
        }
    }

    // Ausfaden
    //

    if (m_FadeMode == FadeMode::FADEOUT) {
        if (m_FadingAlpha > 0.0f)
            m_FadingAlpha -= Timer.sync(FADESPEED);
        else {
            m_FadingAlpha = 0.0f;
            m_FadeMode = FadeMode::INVISIBLE;
        }
    }
}

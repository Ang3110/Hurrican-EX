// Datei : CCracktro.cpp

// --------------------------------------------------------------------------------------
//
// Klasse für das Cracktro am Anfang
//
// (c) 2004 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Includes
// --------------------------------------------------------------------------------------

#include "CCracktro.hpp"
#include <cstring>
#include <iterator>
#include "DX8Input.hpp"
#include "DX8Sound.hpp"

char CrackText[] = {
    "                                              "
    "yeehaaw! poke is back with another fine release of a "
    "retro-remake! this time, its -hurrican-, a remake of the great "
    "classic turrican made by manfred trenz, chris huelsbeck, andreas escher,"
    " jeroen tel, peter thierolf, ramiro vaca, markus siebold, julian eggebrecht,"
    " celal kandemiroglu and many other people. it took us a loooong time to crack "
    "this damn game, but at least it's here. hope you like it... "
    "for updates and more visit www.hurrican-game.de or www.poke53280.de "
    "and now have fun with this game!"
    "                                               "
    ""};

char StaticText[] = {
    "                                   "
    "poke53280 proudly presents -hurrican- cracked on 05-31-2007 by eiswuxe and turri. "
    "greetings fly to all turrican fans worldwide. "
    "visit www.hurrican-game.de for more infos, updates, downloads. check out the "
    "create section to learn how to make your own hurrican levels and add-ons! "
    "we hope you like this game!"
    "                                     "};

char BlinkText[][30] = {"poke53280",  "brings you",      "-hurrican-", "cracked on",
                        "05-31-2007", "always remember", "shoot...",   "or die!"};

D3DCOLOR ScrollCol[] = {0xFF000088, 0xFF000088, 0xFF0000AA, 0xFF0000AA, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF,
                        0xFF0088FF, 0xFF0088FF, 0xFF0088FF, 0xFF0088FF, 0xFFAAFFEE, 0xFFAAFFEE, 0xFFAAFFEE, 0xFFAAFFEE,
                        0xFFEEEE77, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFEEEE77, 0xFFAAFFEE, 0xFFAAFFEE, 0xFFAAFFEE, 0xFFAAFFEE,
                        0xFF0088FF, 0xFF0088FF, 0xFF0088FF, 0xFF0088FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF, 0xFF0000FF,
                        0xFF000088, 0xFF000088, 0xFF000088, 0xFF000088};

D3DCOLOR BlinkCol[] = {0xFF664400, 0xFF664400, 0xFF880000, 0xFF880000, 0xFFCC44CC, 0xFFCC44CC, 0xFFDD8855, 0xFFDD8855,
                       0xFFEEEE77, 0xFFEEEE77, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFEEEE77, 0xFFEEEE77, 0xFFDD8855, 0xFFDD8855,
                       0xFFCC44CC, 0xFFCC44CC, 0xFF880000, 0xFF880000, 0xFF000000, 0xFF000000, 0xFF333333, 0xFF333333,
                       0xFF777777, 0xFF777777, 0xFFBBBBBB, 0xFFBBBBBB, 0xFFAAFF66, 0xFFAAFF66, 0xFFEEEE77, 0xFFEEEE77,
                       0xFFFFFFFF, 0xFFFFFFFF, 0xFFEEEE77, 0xFFEEEE77, 0xFFAAFF66, 0xFFAAFF66, 0xFFBBBBBB, 0xFFBBBBBB,
                       0xFF777777, 0xFF777777, 0xFF333333, 0xFF333333, 0xFF000000, 0xFF000000};

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

CCracktro::CCracktro() {
    b_running = true;
    pFont = new (DirectGraphicsFont);
    pFont->LoadFont("demofont.png", 288, 256, 18, 16, 16, 8, demofont_charwidths);

    // DKS - Corrected dimensions from 32x36 to 32x34, to match actual image file:
    Bars[0].LoadImage("copper1.png", 32, 34, 32, 34, 1, 1);
    Bars[1].LoadImage("copper2.png", 32, 18, 32, 18, 1, 1);
    Bars[2].LoadImage("copper3.png", 32, 100, 32, 100, 1, 1);

    Zahlen.LoadImage("demozahlen.png", 160, 40, 32, 40, 5, 1);

    Logo[0].LoadImage("demologo.png", 341, 80, 341, 80, 1, 1);

    Star.LoadImage("star.png", 5, 5, 5, 5, 1, 1);

    ScrollOffset = 0.0f;
    ScrollOffset2 = 0.0f;
    ScrollPos = 0;
    ScrollPos2 = 0;
    SinPos = 0.0f;

    for (auto& star: Stars) {
        star.Count = static_cast<float>(GetRandom(RENDERWIDTH)) / RENDERWIDTH;
        star.Abstand = static_cast<float>(GetRandom(140) + 340);
        star.Ebene = GetRandom(200) + 55;
    }

    SoundManager.LoadSong("Cracktro.it", MUSIC::CRACKTRO);
    SoundManager.PlaySong(MUSIC::CRACKTRO, false);

    DirectGraphics.SetAdditiveMode();
    DirectGraphics.SetColorKeyMode();

    State = StateEnum::MAIN;

}  // Konstruktor

// --------------------------------------------------------------------------------------
// Destruktor
// --------------------------------------------------------------------------------------

CCracktro::~CCracktro() {
    // DKS - Game was not freeing music data, added this:
    SoundManager.UnloadSong(MUSIC::CRACKTRO);

    delete (pFont);
}  // Destruktor

// --------------------------------------------------------------------------------------
// Hauptteil
// --------------------------------------------------------------------------------------

void CCracktro::Main() {
    DirectGraphics.ClearBackBuffer();
    const float left = DirectGraphics.InterfaceLeft();
    const int width = DirectGraphics.GetViewWidth();

    // --------------------------------------------------------------------------------------
    // Sterne
    // --------------------------------------------------------------------------------------

    for (auto& star: Stars) {
        star.Count -= Timer.sync(star.Ebene * 0.1f / width);

        if (star.Count < 0.0f)
            star.Count += 1.0f;

        RenderRect(left + star.Count * width, star.Abstand, 4, 2, D3DCOLOR_RGBA(255, 255, 255, star.Ebene));
    }

    // --------------------------------------------------------------------------------------
    // Sinus Scroller
    // --------------------------------------------------------------------------------------
    static float colpos = 0.0f;
    static float LogoPos = 0.0f;

    colpos += Timer.sync(6.0f);

    const auto drawScroller = [&](const char *line, int &position, float &offset, float y, float speed, bool rainbow) {
        const auto advance = [this](char ch) {
            return ch == ' ' ? pFont->GetXCharSize() : pFont->GetCharLength(ch - '!') + 2;
        };
        const int length = static_cast<int>(std::strlen(line));
        float x = left - offset;
        for (int i = 0; x < left + width; ++i) {
            const char ch = line[(position + i) % length];
            const auto color = rainbow ? ScrollCol[static_cast<int>(colpos + i) % std::size(ScrollCol)] : 0xFF000000;
            pFont->DrawDemoChar(x, y, ch, color);
            x += advance(ch);
        }
        offset += Timer.sync(speed);
        while (offset > advance(line[position])) {
            offset -= advance(line[position]);
            position = (position + 1) % length;
        }
    };
    drawScroller(CrackText, ScrollPos, ScrollOffset, 450.0f - abs(static_cast<int>(sinf(SinPos) * 80.0f)), 12.0f, true);

    SinPos += Timer.sync(0.3f);
    if (SinPos > TWO_PI)
        SinPos -= TWO_PI;

    LogoPos += Timer.sync(0.2f);
    if (LogoPos > TWO_PI)
        LogoPos -= TWO_PI;

    // --------------------------------------------------------------------------------------
    // Logo (hinter Copper Bars)
    // --------------------------------------------------------------------------------------

    static float numsin = 0.0f;

    if (SinPos > 0.7f && SinPos < PI + 1.0f) {
        DirectGraphics.SetColorKeyMode();
        Logo[0].RenderSprite((RENDERWIDTH - 341) / 2 + sin(LogoPos) * 100.0f, 50, 0, 0xFFFFFFFF);

        numsin += Timer.sync(0.4f);

        if (numsin > TWO_PI)
            numsin -= TWO_PI;

        // Zahlen
        for (int i = 0; i < 5; i++) {
            float off = sin(numsin + i / 3.0f) * 10.0f;
            float off2 = sin(numsin + i / 3.0f + HALF_PI) * 20.0f;

            Zahlen.RenderSprite(75 + i * 40 - off + (RENDERWIDTH - 341) / 2 + sin(LogoPos) * 100.0f, 110 - off2, i, 0xFFFFFFFF);
        }
    }

    // --------------------------------------------------------------------------------------
    // Copper Balken
    // --------------------------------------------------------------------------------------

    for (int i = 0; i < 7; i++)
        Bars[1].RenderSpriteScaled(left, 95 - sin(SinPos + i / 5.0f) * 90.0f, width, 6 + i * 3, 0, 0xFFFFFFFF);

    Bars[2].RenderSpriteScaled(left, 255, width, 100, 0, 0xFFFFFFFF);

    // --------------------------------------------------------------------------------------
    // Scroller 2
    // --------------------------------------------------------------------------------------
    drawScroller(StaticText, ScrollPos2, ScrollOffset2, 295, 10.0f, false);

    // --------------------------------------------------------------------------------------
    // Logo (vor Copper Bars)
    // --------------------------------------------------------------------------------------

    if (!(SinPos > 0.7f && SinPos < PI + 1.0f)) {
        DirectGraphics.SetColorKeyMode();
        Logo[0].RenderSprite((RENDERWIDTH - 341) / 2 + sin(LogoPos) * 100.0f, 50, 0, 0xFFFFFFFF);

        numsin += Timer.sync(0.4f);

        if (numsin > TWO_PI)
            numsin -= TWO_PI;

        // Zahlen
        for (int i = 0; i < 5; i++) {
            float off = sin(numsin + i / 3.0f) * 10.0f;
            float off2 = sin(numsin + i / 3.0f + HALF_PI) * 20.0f;

            Zahlen.RenderSprite(75 + i * 40 - off + (RENDERWIDTH - 341) / 2 + sin(LogoPos) * 100.0f, 110 - off2, i, 0xFFFFFFFF);
        }
    }

    // --------------------------------------------------------------------------------------
    // Static Blink Text
    // --------------------------------------------------------------------------------------

    static float blinkpos = 0.0f;
    static int fontoff = 0;

    blinkpos += Timer.sync(5.0f);

    fontoff = static_cast<int>(blinkpos / 100.0f);

    if (fontoff >= static_cast<int>(sizeof(BlinkText) / sizeof(BlinkText[0]))) {
        blinkpos = 0.0f;
        fontoff = 0;
    }

    float yo = 220 - cos(LogoPos) * 10.0f;
    D3DCOLOR col = BlinkCol[static_cast<int>(blinkpos) % (sizeof(BlinkCol) / sizeof(D3DCOLOR))];

    RenderRect(left, yo, width, 2, col);
    RenderRect(left, yo + 20.0f, width, 2, col);

    pFont->DrawDemoText(
        -sin(LogoPos) * 140.0f + static_cast<float>(626 - pFont->DemoStringLength(BlinkText[fontoff])) / 2.0f,
        yo + 2.0f, BlinkText[fontoff], col);

    // --------------------------------------------------------------------------------------
    // Kringel
    // --------------------------------------------------------------------------------------

    for (int i = 0; i < 16; i++) {
        RenderRect(-sin(LogoPos) * 140.0f + 320 + sin(SinPos + i * (TWO_PI / 16)) * 140,
                   yo + 10 + cos(SinPos + i * (TWO_PI / 16)) * 20, 4, 2, 0xFFFFFFFF);
    }

    // beenden
    //
    if (DirectInput.AnyKeyDown() || DirectInput.AnyButtonDown()) {
        SoundManager.StopSong(MUSIC::CRACKTRO, false);
        State = StateEnum::LOAD;
    }
}

// --------------------------------------------------------------------------------------
// Ladebalken
// --------------------------------------------------------------------------------------

void CCracktro::Load() {
    static float count = 10.0f;

    // farbige Balken
    for (int i = 0; i < (RENDERHEIGHT / 2); i++)
        RenderRect(DirectGraphics.InterfaceLeft(), static_cast<float>(i * 2), DirectGraphics.GetViewWidth(), 2,
                   ScrollCol[rand() % (static_cast<int>(sizeof(ScrollCol) / sizeof(D3DCOLOR)))]);

    count -= Timer.sync(1.0f);

    if (count <= 0.0f)
        b_running = false;
}

// --------------------------------------------------------------------------------------
// Laufen lassen
// --------------------------------------------------------------------------------------

void CCracktro::Run() {
    switch (State) {
        case StateEnum::MAIN:
            Main();
            break;

        case StateEnum::LOAD:
            Load();
            break;
    }
}  // RunCracktro

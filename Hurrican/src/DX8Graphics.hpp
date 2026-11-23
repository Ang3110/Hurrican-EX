// Datei : DX8Graphics.hpp

// --------------------------------------------------------------------------------------
//
// Direct Graphics Klasse
// zum initialisieren von DirectX8
// beinhaltet zudem verschiedene Grafik-Funktionen zum Speichern von Screenshots usw
//
// (c) 2002 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

#ifndef _DX8GRAPHICS_HPP_
#define _DX8GRAPHICS_HPP_

// --------------------------------------------------------------------------------------
// Include Dateien
// --------------------------------------------------------------------------------------

#include "SDL_port.hpp"

#include <array>
#include <vector>
#if defined(USE_GL2) || defined(USE_GL3)
#  include "cshader.hpp"
#  if defined(USE_FBO)
#    include "cfbo.hpp"
#  endif /* USE_FBO */
#endif /* USE_GL2 || USE_GL3 */

// --------------------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------------------


enum class DisplayAspect { Automatic, Wide16x9, Wide16x10, Classic, Stretch, Count };
enum class SplitScreenType { Dynamic, OneScreen };
enum class TextureLayout { Sprite, Font, Repeat, MirrorRepeat, Parallax, VerticalParallax };

struct DisplayResolution {
    int width;
    int height;
};

struct DisplaySettings {
    static constexpr std::array<int, 10> brightnessGains{50, 63, 75, 88, 100, 110, 120, 130, 140, 150};
    int brightnessLevel = 4;
    int BrightnessGain() const { return brightnessGains[brightnessLevel]; }
    void LoadBrightnessGain(int gain);
    bool fullscreen = true;
    DisplayAspect aspect = DisplayAspect::Automatic;
    int width = 0;
    int height = 0;
    bool vsync = true;
    bool crt = false;
    bool textureFiltering = false;
    SplitScreenType splitScreenType = SplitScreenType::Dynamic;
};

enum class BlendModeEnum {
  ADDITIV,
  COLORKEY,
  WHITE,
  BRIGHTNESS
};

#if defined(USE_GL2) || defined(USE_GL3)
enum { PROGRAM_COLOR = 0, PROGRAM_TEXTURE, PROGRAM_RENDER, PROGRAM_TOTAL, PROGRAM_NONE };
#endif

// --------------------------------------------------------------------------------------
// Strukturen
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Struktur für einen 2D Vertex
// --------------------------------------------------------------------------------------

// DKS - Removed unnecessary z-coordinate:
struct VERTEX2D {
    float x, y;      // x,y Koordinaten
    D3DCOLOR color;  // Vertex-Color
    float tu, tv;    // Textur-Koordinaten
};

// DKS - Added
struct QUAD2D {
    VERTEX2D v1, v2, v3, v4;
};

// --------------------------------------------------------------------------------------
// Klassendeklaration
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// DirectGraphics Klasse
// --------------------------------------------------------------------------------------

class DirectGraphicsClass {
  private:
    enum class shader_t {COLOR, TEXTURE, RENDER};

  private:
    DisplaySettings display;
    bool VSyncEnabled = false;
    void SetVSync(bool enabled);
    void ApplyFrameBrightness(int gainPercent);
#if SDL_VERSION_ATLEAST(2, 0, 0)
    bool SetWindowMode(const DisplaySettings &settings);
#endif
    bool FilterMode;    // Linearer Filter an/aus?
    const char *glextensions;
    shader_t use_shader;
    BlendModeEnum BlendMode;  // Additiv, Colorkey oder White mode aktiviert?
    int MaxTextureUnits;
    bool SupportedETC1;
    bool SupportedPVRTC;
#if defined(USE_GL2) || defined(USE_GL3)
    GLuint ProgramCurrent;
    GLint NameTime;
    GLint NameOutputWidth;
    GLint NameOutputHeight;
    CShader Shaders[PROGRAM_TOTAL];
#endif
    glm::mat4x4 matProjWindow;

#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_Window *Window;
    SDL_GLContext GLcontext;
#else /* SDL 1.2 */
    SDL_Surface *Screen;
#endif
    SDL_Rect WindowView{};
    SDL_Rect RenderView{};
    SDL_Rect RenderRect{};
#if defined(USE_FBO)
    CFbo RenderBuffer;
    SDL_Rect FramebufferView{};
    SDL_Rect RequestedFramebufferView{};
    bool interfaceRendering = true;
    bool framebufferFullColor = false;
#endif
    void UpdateRenderTarget();

  public:
    void ShowBackBuffer();  // Present aufrufen
    void ShowBackBuffer(int brightnessGain);
    void DisplayBuffer() { ShowBackBuffer(); }

    DirectGraphicsClass();   // Konstruktor
    ~DirectGraphicsClass();  // Desktruktor

    bool Init(const DisplaySettings &settings);
    const DisplaySettings &GetDisplaySettings() const { return display; }
    bool ApplyDisplaySettings(const DisplaySettings &settings);
    bool IsVSyncEnabled() const { return VSyncEnabled; }
    bool SupportsDisplayMenu() const;
    bool SupportsCrtFilter() const;
    std::vector<DisplayResolution> DisplayResolutions(bool fullscreen) const;
    bool Exit();  // D3D beenden
    bool SetDeviceInfo();

    bool TakeScreenshot(const char *filename);   // Screenshot machen
    void SetColorKeyMode();         // Alpha für Colorkey oder
    void SetAdditiveMode();         // Additive-Blending nutzen
    void SetWhiteMode();            // Komplett weiss rendern
    void SetFilterMode(bool filteron);  // Linearer Textur Filter ein/aus

    void RendertoBuffer(GLenum PrimitiveType,          // Rendert in den Buffer, der am Ende
                        std::uint32_t PrimitiveCount,  // eines jeden Frames komplett in
                        void *pVertexStreamZeroData);  // den Backbuffer gerendert wird

    // DKS - SetTexture is now used for both GL and DirectX, and uses new TexturesystemClass:
    void SetTexture(int idx);
    bool ExtensionSupported(const char *ext);
    void SetupFramebuffers(bool force = false);
    void SetInterfaceRendering(bool enabled);
    int GetViewWidth() const { return RenderView.w; }
    DisplayResolution GetOutputResolution() const { return {RenderRect.w, RenderRect.h}; }
    float InterfaceLeft() const;
    void SetInterfaceView();
    void SetHorizontalView(float left);
    void SetGameViewport(const SDL_Rect &region, float left, float worldScale = 1.0f);
    bool EnsureGameStencil();
    void ClearBackBuffer();
#if (defined(USE_GL2) || defined(USE_GL3)) && defined(USE_FBO)
    void SelectBuffer(bool active);
#endif

    inline BlendModeEnum GetBlendMode() const { return BlendMode; }
    inline bool IsETC1Supported() const { return SupportedETC1; }
    inline bool IsPVRTCSupported() const { return SupportedPVRTC; }

#if SDL_VERSION_ATLEAST(2, 0, 0)
    void FlipSurface(SDL_Surface* surface);
#endif
};



// --------------------------------------------------------------------------------------
// Externals
// --------------------------------------------------------------------------------------

extern DirectGraphicsClass DirectGraphics;  // DirectGraphics Klasse
extern glm::mat4x4 matProj;                  // Projektionsmatrix
extern glm::mat4x4 matWorld;                 // Weltmatrix
extern float DegreetoRad[360];              // Tabelle mit Rotationswerten

#endif

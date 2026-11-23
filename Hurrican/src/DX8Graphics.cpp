// Datei : DX8Graphics.cpp

// --------------------------------------------------------------------------------------
//
// Direct Graphics Klasse
// zum initialisieren von DirectX8
// beinhaltet zudem verschiedene Grafik-Funktionen zum Speichern von Screenshots usw
//
// (c) 2002 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Include Dateien
// --------------------------------------------------------------------------------------

#include "DX8Graphics.hpp"
#include <time.h>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <string>
#include "DX8Font.hpp"
#include "DX8Texture.hpp"
#include "Gameplay.hpp"
#include "Globals.hpp"
#include "Logdatei.hpp"
#include "Main.hpp"
#include "Tileengine.hpp"
#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW) || defined(USE_EGL_RPI)
#include "eglport.hpp"
#endif

// --------------------------------------------------------------------------------------
// sonstige Variablen
// --------------------------------------------------------------------------------------

glm::mat4x4 matProj;   // Projektionsmatrix
glm::mat4x4 matWorld;  // Weltmatrix
float DegreetoRad[360];  // Tabelle mit Rotationswerten

// --------------------------------------------------------------------------------------
// Klassenfunktionen
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// DirectGraphics Klasse
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

DirectGraphicsClass::DirectGraphicsClass() {
    SupportedETC1 = false;
    SupportedPVRTC = false;
    use_shader = shader_t::COLOR;

#if defined(USE_GL2) || defined(USE_GL3)
    ProgramCurrent = PROGRAM_NONE;
#endif
}

// --------------------------------------------------------------------------------------
// Desktruktor
// --------------------------------------------------------------------------------------

DirectGraphicsClass::~DirectGraphicsClass() {}

// --------------------------------------------------------------------------------------
// D3D Initialisieren
// --------------------------------------------------------------------------------------
bool DirectGraphicsClass::Init(const DisplaySettings &settings) {
    display = settings;
    const bool isFullscreen = display.fullscreen;
    int ScreenWidth = SCREENWIDTH;
    int ScreenHeight = SCREENHEIGHT;

    if (CommandLineParams.LowRes) {
        ScreenWidth = LOWRES_SCREENWIDTH;
        ScreenHeight = LOWRES_SCREENHEIGHT;
    } else if (!isFullscreen && display.width > 0) {
        ScreenWidth = display.width;
        ScreenHeight = display.height;
    } else if (!isFullscreen && display.aspect != DisplayAspect::Classic) {
        ScreenWidth = 960;
        ScreenHeight = 540;
    }

    int const ScreenDepth = CommandLineParams.ScreenDepth;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    uint32_t flags = SDL_WINDOW_OPENGL | SDL_WINDOW_ALLOW_HIGHDPI;
    if (!CommandLineParams.LowRes)
        flags |= SDL_WINDOW_RESIZABLE;
#else /* SDL 1.2 */
#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW) || defined(USE_EGL_RPI)
    uint32_t flags = SDL_SWSURFACE;
#else
    uint32_t flags = SDL_SWSURFACE | SDL_OPENGL;
#endif
#endif

#if defined(RPI)
    ScreenWidth = 0;
    ScreenHeight = 0;
#endif

    Protokoll << "\n--> SDL/OpenGL init <--\n";
    Protokoll << "---------------------\n" << std::endl;

    // Initialize defaults, Video and Audio subsystems
    Protokoll << "Initializing SDL." << std::endl;
#if SDL_VERSION_ATLEAST(2, 0, 14)
    SDL_SetHintWithPriority(SDL_HINT_JOYSTICK_HIDAPI_PS4_RUMBLE, "1", SDL_HINT_DEFAULT);
    SDL_SetHintWithPriority(SDL_HINT_JOYSTICK_HIDAPI_PS5_RUMBLE, "1", SDL_HINT_DEFAULT);
#endif
#ifdef SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS
    SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS, "0");
#endif
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_JOYSTICK) == -1) {
        Protokoll << "Failed to initialize SDL: " << SDL_GetError() << "." << std::endl;
        return false;
    }
    Protokoll << "SDL initialized." << std::endl;

#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (isFullscreen && SupportsDisplayMenu()) {
        SDL_DisplayMode desktop{};
        if (SDL_GetDesktopDisplayMode(0, &desktop) == 0) {
            // Size the window before creating the GL drawable, including under compositors
            // that handle fullscreen geometry asynchronously.
            ScreenWidth = desktop.w;
            ScreenHeight = desktop.h;
        }
    }
#endif

#if !defined(USE_EGL_SDL) && !defined(USE_EGL_RAW) && !defined(USE_EGL_RPI)
    if (ScreenDepth > 16) {     // DKS - Screen depth is now default 16 under GLES, 32 others
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);  //      (Can now be changed via command line switch)
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);

        if (ScreenDepth >= 32) {
            SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
            SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
        } else {
            SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
            SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 24);
        }
    } else {
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
        SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 6);
        SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
        SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 16);
    }
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);          // DKS - No need for a depth buffer in this game
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);  // DKS - Changed this to 0 (Game would not load w/ GL1.2 laptop)
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
#if !SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, display.vsync);
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
#if defined(USE_GLES1)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#elif defined(USE_GLES2)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#elif defined(USE_GLES3)
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
#elif defined(USE_GL2)
    // The GL2 path uses GLSL 1.00-style shaders and client-side vertex arrays.
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif /* defined(USE_GLES1) */
#endif /* SDL_VERSION_ATLEAST(2,0,0) */
#endif // !EGL

    // Setup SDL Screen
    if (isFullscreen) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
#else /* SDL 1.2 */
        flags |= SDL_FULLSCREEN;
#endif
    }

#if SDL_VERSION_ATLEAST(2, 0, 0)
    // Create a window. Window mode MUST include SDL_WINDOW_OPENGL for use with OpenGL.
    Window =
        SDL_CreateWindow(HURRICAN_EX_NAME, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, ScreenWidth, ScreenHeight, flags);
    if (Window == nullptr) {
        Protokoll << "Failed to create " << ScreenWidth << "x" << ScreenHeight
                  << " window: " << SDL_GetError() << std::endl;
        return false;
    }

    if (SupportsDisplayMenu() && display.fullscreen && display.width > 0 && !SetWindowMode(display)) {
        Protokoll << "Could not restore saved fullscreen resolution: " << SDL_GetError() << std::endl;
        display.width = display.height = 0;
        if (!SetWindowMode(display))
            return false;
    }

    // Create an OpenGL context associated with the window.
    GLcontext = SDL_GL_CreateContext(Window);
    if (GLcontext == nullptr) {
        Protokoll << "Failed to create GL context: " << SDL_GetError() << std::endl;
        return false;
    }
#else /* SDL 1.2 */
    // SDL_WM_SetCaption("Hurrican", "Hurrican");

    Screen = SDL_SetVideoMode(ScreenWidth, ScreenHeight, ScreenDepth, flags);
    if (Screen == nullptr) {
        Protokoll << "Failed to set " << ScreenWidth << "x" << ScreenHeight << "x" << ScreenDepth
                  << " video mode: " << SDL_GetError() << std::endl;
        return false;
    }
#endif

    SDL_ShowCursor(SDL_DISABLE);

#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW) || defined(USE_EGL_RPI)
    int actual_w, actual_h;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_GetWindowSize(Window, &actual_w, &actual_h);
#else
    actual_w = Screen->w;
    actual_h = Screen->h;
#endif

    // DKS - EGL_Open now takes additional parameters to specify desired screen
    //      depth and vsync. If eglport.cfg file is present, it will override
    //      whatever is passed here, however:
    if (EGL_Open(actual_w, actual_h, CommandLineParams.ScreenDepth == 16 ? 16 : 32, display.vsync) != 0) {
        return false;
    }
#endif

    SetVSync(display.vsync);

    if (!SetDeviceInfo())
        return false;

    Protokoll << "\n-> OpenGL init successful!\n" << std::endl;

    // DegreetoRad-Tabelle füllen
    for (int i = 0; i < 360; i++)
        DegreetoRad[i] = PI * static_cast<float>(i) / 180.0f;

    SetColorKeyMode();

    return true;
}

bool DirectGraphicsClass::SupportsDisplayMenu() const {
#if SDL_VERSION_ATLEAST(2, 0, 0) && !defined(USE_EGL_SDL) && !defined(USE_EGL_RAW) && !defined(USE_EGL_RPI)
    return !CommandLineParams.LowRes;
#else
    return false;
#endif
}

void DirectGraphicsClass::SetVSync(bool enabled) {
#if SDL_VERSION_ATLEAST(2, 0, 0) && !defined(USE_EGL_SDL) && !defined(USE_EGL_RAW) && !defined(USE_EGL_RPI)
    if (SDL_GL_SetSwapInterval(enabled ? 1 : 0) < 0)
        Protokoll << "VSync request " << (enabled ? "on" : "off") << ": " << SDL_GetError() << std::endl;
    VSyncEnabled = SDL_GL_GetSwapInterval() != 0;
    Protokoll << "VSync requested " << (enabled ? "on" : "off") << ", active "
              << (VSyncEnabled ? "on" : "off") << std::endl;
#else
    VSyncEnabled = enabled;
#endif
}

std::vector<DisplayResolution> DirectGraphicsClass::DisplayResolutions(bool fullscreen) const {
    std::vector<DisplayResolution> modes{{0, 0}};
#if SDL_VERSION_ATLEAST(2, 0, 0)
    const int monitor = SDL_GetWindowDisplayIndex(Window);
    const auto add = [&modes](int w, int h) {
        if (w >= 320 && h >= 240 && std::none_of(modes.begin(), modes.end(), [=](const auto &size) {
            return size.width == w && size.height == h;
        }))
            modes.push_back({w, h});
    };
    if (fullscreen) {
        const int modeCount = SDL_GetNumDisplayModes(monitor);
        for (int i = 0; i < modeCount; ++i) {
            SDL_DisplayMode mode{};
            if (SDL_GetDisplayMode(monitor, i, &mode) == 0)
                add(mode.w, mode.h);
        }
    } else {
        constexpr DisplayResolution presets[] = {{640, 480}, {960, 540}, {1024, 768}, {1280, 720}, {1280, 800},
            {1366, 768}, {1600, 900}, {1920, 1080}, {1920, 1200}, {2560, 1440}, {2560, 1600}, {3840, 2160}};
        SDL_DisplayMode desktop{};
        if (SDL_GetDesktopDisplayMode(monitor, &desktop) == 0) {
            for (const auto &size : presets)
                if (size.width <= desktop.w && size.height <= desktop.h)
                    add(size.width, size.height);
            add(desktop.w, desktop.h);
        }
    }
    std::sort(modes.begin() + 1, modes.end(), [](const auto &a, const auto &b) {
        return a.width != b.width ? a.width < b.width : a.height < b.height;
    });
#endif
    return modes;
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
bool DirectGraphicsClass::SetWindowMode(const DisplaySettings &settings) {
    SDL_DisplayMode mode{};
    const bool exclusive = settings.fullscreen && settings.width > 0;
    if (exclusive) {
        SDL_DisplayMode desired{};
        desired.w = settings.width;
        desired.h = settings.height;
        if (!SDL_GetClosestDisplayMode(SDL_GetWindowDisplayIndex(Window), &desired, &mode) ||
            mode.w != desired.w || mode.h != desired.h) {
            SDL_SetError("Display resolution %dx%d is unavailable", desired.w, desired.h);
            return false;
        }
    }

    if ((SDL_GetWindowFlags(Window) & SDL_WINDOW_FULLSCREEN) && SDL_SetWindowFullscreen(Window, 0) < 0)
        return false;
    if (SDL_SetWindowDisplayMode(Window, exclusive ? &mode : nullptr) < 0)
        return false;
    if (settings.fullscreen)
        return SDL_SetWindowFullscreen(Window, exclusive ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_FULLSCREEN_DESKTOP) == 0;

    const bool classic = settings.aspect == DisplayAspect::Classic;
    const int width = settings.width > 0 ? settings.width : (classic ? SCREENWIDTH : 960);
    const int height = settings.height > 0 ? settings.height : (classic ? SCREENHEIGHT : 540);
    SDL_SetWindowSize(Window, width, height);
    return true;
}
#endif

bool DirectGraphicsClass::ApplyDisplaySettings(const DisplaySettings &settings) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    const bool modeChanged = display.fullscreen != settings.fullscreen ||
                             display.width != settings.width || display.height != settings.height;
    if (modeChanged && !SetWindowMode(settings)) {
        Protokoll << "Could not apply display settings: " << SDL_GetError() << std::endl;
        if (!SetWindowMode(display))
            SDL_SetWindowFullscreen(Window, display.fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SetupFramebuffers(true);
        return false;
    }

    const bool aspectChanged = display.aspect != settings.aspect;
    const bool vsyncChanged = display.vsync != settings.vsync;
    display = settings;
    if (modeChanged || vsyncChanged)
        SetVSync(display.vsync);
    if (modeChanged || aspectChanged)
        SetupFramebuffers(true);
    return true;
#else
    return false;
#endif
}

bool DirectGraphicsClass::SupportsCrtFilter() const {
#if (defined(USE_GL2) || defined(USE_GL3)) && defined(USE_FBO)
#if defined(USE_ETC1)
    if (SupportedETC1)
        return false;
#endif
    return RenderBuffer.IsEnabled();
#else
    return false;
#endif
}

// --------------------------------------------------------------------------------------
// Direct3D beenden
// --------------------------------------------------------------------------------------

bool DirectGraphicsClass::Exit() {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (!GLcontext) {
        SDL_DestroyWindow(Window);
        SDL_Quit();
        return true;
    }
#endif
#if defined(USE_GL2) || defined(USE_GL3)
    Shaders[PROGRAM_COLOR].Close();
    Shaders[PROGRAM_TEXTURE].Close();
    Shaders[PROGRAM_RENDER].Close();
#if (defined(USE_GL2) || defined(USE_GL3)) && defined(USE_FBO)
    RenderBuffer.Close();
#endif /* USE_FBO */
#endif /* USE_GL2 || USE_GL3 */
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_GL_DeleteContext(GLcontext);
    SDL_DestroyWindow(Window);
#endif
#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW) || defined(USE_EGL_RPI)
    EGL_Close();
#endif
    SDL_Quit();
    Protokoll << "-> SDL/OpenGL shutdown successfully completed !" << std::endl;
    return true;
}

// --------------------------------------------------------------------------------------
// Infos für Device Objekt setzen
// Für Init und nach Task Wechsel
// --------------------------------------------------------------------------------------

bool DirectGraphicsClass::SetDeviceInfo() {
#if defined(USE_GL2) || defined(USE_GL3)
    std::string vert;
    std::string frag;
#if defined(USE_GL3)
    const char* glsl_version = "320";
#else /* USE_GL2 */
    const char* glsl_version = "100";
#endif
#endif

    SetupFramebuffers();

    /* OpenGL Information */
    Protokoll << "GL_VENDOR: " << glGetString(GL_VENDOR) << std::endl;
    Protokoll << "GL_RENDERER: " << glGetString(GL_RENDERER) << std::endl;
    Protokoll << "GL_VERSION: " << glGetString(GL_VERSION) << std::endl;
#if defined(USE_GL2) || defined(USE_GL3)
    Protokoll << "GL_SHADING_LANGUAGE_VERSION: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;
#endif
    glextensions = reinterpret_cast<const char *>(glGetString(GL_EXTENSIONS));
    if (glextensions)
        Protokoll << "GL_EXTENSIONS: " << glextensions << std::endl;

#if defined(USE_ETC1)
    SupportedETC1 = ExtensionSupported("GL_OES_compressed_ETC1_RGB8_texture");
#endif
#if defined(USE_PVRTC)
    SupportedPVRTC = ExtensionSupported("GL_IMG_texture_compression_pvrtc");
#endif

    /* Init OpenGL */
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); /* Set the background black */

#if defined(USE_GLES1) || defined(USE_GLES2) || defined(USE_GLES3)
    glClearDepthf(1.0f);                   /* Depth buffer setup */
#else
    glClearDepth(1.0);                     /* Depth buffer setup */
#endif

    glDisable(GL_DEPTH_TEST); /* No Depth Testing */
    glEnable(GL_BLEND);

#if defined(USE_GL1)
    glDisable(GL_LIGHTING);                            /* No light */
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); /* Really Nice Perspective Calculations */
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#endif

#if defined(USE_GL2) || defined(USE_GL3)
    // Compile the shader code and link into a program
    vert = g_storage_ext + "/data/shaders/" + glsl_version + "/shader_color.vert";
    frag = g_storage_ext + "/data/shaders/" + glsl_version + "/shader_color.frag";

    if (!Shaders[PROGRAM_COLOR].Load(vert, frag)) {
        return false;
    }

    vert = g_storage_ext + "/data/shaders/" + glsl_version + "/shader_texture.vert";
#if defined(USE_ETC1)
    if (SupportedETC1) {
        frag = g_storage_ext + "/data/shaders/" + glsl_version + "/shader_etc1_texture.frag";
    } else {
#endif
    frag = g_storage_ext + "/data/shaders/" + glsl_version + "/shader_texture.frag";
#if defined(USE_ETC1)
    }
#endif

    if (!Shaders[PROGRAM_TEXTURE].Load(vert, frag)) {
        return false;
    }

    vert = g_storage_ext + "/data/shaders/" + glsl_version + "/shader_render.vert";
    frag = g_storage_ext + "/data/shaders/" + glsl_version + "/shader_render.frag";

    // Compile the usual CRT preset, or the exact effects selected on the command line.
    const bool usePreset = !CommandLineParams.HasCrtEffects();
    Shaders[PROGRAM_RENDER].AddConstant("c_curvature", usePreset || CommandLineParams.ScreenCurvature ? 1 : 0);
    Shaders[PROGRAM_RENDER].AddConstant("c_color_bleed", usePreset || CommandLineParams.ColorBleed ? 1 : 0);
    Shaders[PROGRAM_RENDER].AddConstant("c_scanlines", usePreset || CommandLineParams.Scanlines ? 1 : 0);
    Shaders[PROGRAM_RENDER].AddConstant("c_noise", CommandLineParams.ScreenNoise ? 1 : 0);

    if (!Shaders[PROGRAM_RENDER].Load(vert, frag)) {
        return false;
    }

    // Get names for attributes and uniforms
    Shaders[PROGRAM_COLOR].NamePos = Shaders[PROGRAM_COLOR].GetAttribute("a_Position");
    Shaders[PROGRAM_COLOR].NameClr = Shaders[PROGRAM_COLOR].GetAttribute("a_Color");
    Shaders[PROGRAM_COLOR].NameMvp = Shaders[PROGRAM_COLOR].GetUniform("u_MVPMatrix");

    Shaders[PROGRAM_TEXTURE].NamePos = Shaders[PROGRAM_TEXTURE].GetAttribute("a_Position");
    Shaders[PROGRAM_TEXTURE].NameClr = Shaders[PROGRAM_TEXTURE].GetAttribute("a_Color");
    Shaders[PROGRAM_TEXTURE].NameTex = Shaders[PROGRAM_TEXTURE].GetAttribute("a_Texcoord0");
    Shaders[PROGRAM_TEXTURE].NameMvp = Shaders[PROGRAM_TEXTURE].GetUniform("u_MVPMatrix");

#if defined(USE_ETC1)
    if (SupportedETC1) {
        Shaders[PROGRAM_TEXTURE].texUnit0 = Shaders[PROGRAM_TEXTURE].GetUniform("u_Texture0");
        Shaders[PROGRAM_TEXTURE].texUnit1 = Shaders[PROGRAM_TEXTURE].GetUniform("u_Texture1");
    }
#endif
    Shaders[PROGRAM_RENDER].NamePos = Shaders[PROGRAM_RENDER].GetAttribute("a_Position");
    Shaders[PROGRAM_RENDER].NameClr = Shaders[PROGRAM_RENDER].GetAttribute("a_Color");
    Shaders[PROGRAM_RENDER].NameTex = Shaders[PROGRAM_RENDER].GetAttribute("a_Texcoord0");
    Shaders[PROGRAM_RENDER].NameMvp = Shaders[PROGRAM_RENDER].GetUniform("u_MVPMatrix");
    NameOutputWidth = Shaders[PROGRAM_RENDER].GetUniform("u_OutputWidth");
    NameOutputHeight = Shaders[PROGRAM_RENDER].GetUniform("u_OutputHeight");
    if (CommandLineParams.ScreenNoise)
        NameTime = Shaders[PROGRAM_RENDER].GetUniform("u_Time");
#endif /* USE_GL2 || USE_GL3 */

    /* Matrices setup */
    g_matView = glm::mat4x4(1.0f);
    g_matModelView = glm::mat4x4(1.0f);

    return true;
}
// --------------------------------------------------------------------------------------
// Take a screenshot
// Copies the screen to a bitmap file
// --------------------------------------------------------------------------------------

bool DirectGraphicsClass::TakeScreenshot(const char *filename) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_Rect area = RenderRect;
#if defined(USE_FBO)
    if (RenderBuffer.IsEnabled())
        area = FramebufferView;
    else
#endif
        area.y = WindowView.h - area.y - area.h;
    SDL_Surface *image = SDL_CreateRGBSurface(SDL_SWSURFACE, area.w, area.h, 24, 0x000000FF, 0x0000FF00, 0x00FF0000, 0);
    if (!image)
        return false;
    glReadPixels(area.x, area.y, area.w, area.h, GL_RGB, GL_UNSIGNED_BYTE, image->pixels);
    FlipSurface(image);
    const bool saved = SDL_SaveBMP(image, filename) == 0;
    SDL_FreeSurface(image);
    return saved;
#else

#ifndef NDEBUG
    Protokoll << "DirectGraphicsClass::TakeScreenshot not implemented in this backend!" << std::endl;
#endif
    return false;

#endif
}

// --------------------------------------------------------------------------------------
// Renderstates für Sprites mit ColorKey setzen
// --------------------------------------------------------------------------------------

void DirectGraphicsClass::SetColorKeyMode() {
    if (BlendMode == BlendModeEnum::COLORKEY)
        return;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    BlendMode = BlendModeEnum::COLORKEY;
}

// --------------------------------------------------------------------------------------
// Renderstates für Sprites setzen, die komplett weiss gerendert werden
// --------------------------------------------------------------------------------------

void DirectGraphicsClass::SetWhiteMode() {
    if (BlendMode == BlendModeEnum::WHITE)
        return;

    glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_DST_ALPHA);

    BlendMode = BlendModeEnum::WHITE;
}

// --------------------------------------------------------------------------------------
// Renderstates für Sprites mit Additivem Alphablending setzen
// --------------------------------------------------------------------------------------

void DirectGraphicsClass::SetAdditiveMode() {
    if (BlendMode == BlendModeEnum::ADDITIV)
        return;

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    BlendMode = BlendModeEnum::ADDITIV;
}

// --------------------------------------------------------------------------------------
// Renderstates für linearen Texturfilter ein/ausschalten
// --------------------------------------------------------------------------------------

void DirectGraphicsClass::SetFilterMode(bool filteron) {
    FilterMode = filteron;
}

// --------------------------------------------------------------------------------------
// Rendert in den Buffer, der am Ende eines jeden Frames komplett
// in den Backbuffer gerendert wird
// --------------------------------------------------------------------------------------

void DirectGraphicsClass::RendertoBuffer(GLenum PrimitiveType,
                                         std::uint32_t PrimitiveCount,
                                         void *pVertexStreamZeroData) {
    constexpr int STRIDE = sizeof(VERTEX2D);
    constexpr size_t CLR_OFFSET = offsetof(VERTEX2D, color);
    constexpr size_t TEX_OFFSET = offsetof(VERTEX2D, tu);

#if defined(USE_GL2) || defined(USE_GL3)
    uint8_t program_next;
    bool is_texture;

    // Determine the shader program to use
    switch (use_shader) {
    case shader_t::TEXTURE:
        program_next = PROGRAM_TEXTURE;
        is_texture = true;
        break;
    case shader_t::RENDER:
        program_next = PROGRAM_RENDER;
        is_texture = true;
        break;
    default:
        program_next = PROGRAM_COLOR;
        is_texture = false;
    }

    // Check if the program is already in use
    if (ProgramCurrent != program_next) {
        Shaders[program_next].Use();
        ProgramCurrent = program_next;
    }
    if (program_next == PROGRAM_RENDER) {
        glUniform1i(NameOutputWidth, RenderRect.w);
        glUniform1i(NameOutputHeight, RenderRect.h);
        if (CommandLineParams.ScreenNoise)
            glUniform1i(NameTime, 50 * SDL_GetTicks() / 1000);
    }
#endif

    if (PrimitiveType == GL_LINES) {
        PrimitiveCount *= 2;
    } else if (PrimitiveType == GL_LINE_STRIP) {
        PrimitiveCount += 2;
    } else if (PrimitiveType == GL_TRIANGLES) {
        PrimitiveCount *= 3;
    } else if (PrimitiveType == GL_TRIANGLE_STRIP) {
        PrimitiveCount += 2;
    } else {
        Protokoll << "Add type to count indinces" << std::endl;
        return;
    }

#if defined(USE_GL1)
    // Enable the client states for transfer
    if (use_shader == shader_t::TEXTURE) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, STRIDE, reinterpret_cast<uint8_t *>(pVertexStreamZeroData) + TEX_OFFSET);
    }

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, STRIDE, pVertexStreamZeroData);

    glEnableClientState(GL_COLOR_ARRAY);
    glColorPointer(4, GL_UNSIGNED_BYTE, STRIDE, reinterpret_cast<uint8_t *>(pVertexStreamZeroData) + CLR_OFFSET);
#elif defined(USE_GL2) || defined(USE_GL3)
        // Enable attributes and uniforms for transfer
        if (is_texture) {
#if defined(USE_ETC1)
            if (SupportedETC1) {
                glUniform1i(Shaders[ProgramCurrent].texUnit0, 0);
                glUniform1i(Shaders[ProgramCurrent].texUnit1, 1);
            }
#endif
            glEnableVertexAttribArray(Shaders[ProgramCurrent].NameTex);
            glVertexAttribPointer(Shaders[ProgramCurrent].NameTex, 2, GL_FLOAT, GL_FALSE, STRIDE,
                                  reinterpret_cast<uint8_t *>(pVertexStreamZeroData) + TEX_OFFSET);
        }

        glEnableVertexAttribArray(Shaders[ProgramCurrent].NamePos);
        glVertexAttribPointer(Shaders[ProgramCurrent].NamePos, 2, GL_FLOAT, GL_FALSE, STRIDE, pVertexStreamZeroData);

        glEnableVertexAttribArray(Shaders[ProgramCurrent].NameClr);
        glVertexAttribPointer(Shaders[ProgramCurrent].NameClr, 4, GL_UNSIGNED_BYTE, GL_TRUE, STRIDE,
                              reinterpret_cast<uint8_t *>(pVertexStreamZeroData) + CLR_OFFSET);

        glm::mat4x4 matMVP = matProj * g_matModelView;
        glUniformMatrix4fv(Shaders[ProgramCurrent].NameMvp, 1, GL_FALSE, glm::value_ptr(matMVP));
#endif

    glDrawArrays(PrimitiveType, 0, PrimitiveCount);

#if defined(USE_GL1)
    // Disable the client states
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    if (use_shader == shader_t::TEXTURE) {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
#elif defined(USE_GL2) || defined(USE_GL3)
        // Disable attributes and uniforms
        glDisableVertexAttribArray(Shaders[ProgramCurrent].NamePos);
        glDisableVertexAttribArray(Shaders[ProgramCurrent].NameClr);

        if (is_texture) {
            glDisableVertexAttribArray(Shaders[ProgramCurrent].NameTex);
        }
#endif
}

// --------------------------------------------------------------------------------------
// Read the gain saved by previous builds into the ten-position slider.
// --------------------------------------------------------------------------------------

void DisplaySettings::LoadBrightnessGain(int gain) {
    // Comparing midpoints also handles values outside the old range.
    brightnessLevel = 0;
    while (brightnessLevel + 1 < static_cast<int>(brightnessGains.size()) &&
           gain >= (brightnessGains[brightnessLevel] + brightnessGains[brightnessLevel + 1] + 1) / 2)
        ++brightnessLevel;
}

bool DirectGraphicsClass::ExtensionSupported(const char *ext) {
    if (glextensions && strstr(glextensions, ext) != nullptr) {
        Protokoll << ext << " is supported" << std::endl;
        return true;
    }

    Protokoll << ext << " is not supported" << std::endl;
    return false;
}

// DKS - Supports new TexturesystemClass and is now used for both GL and DirectX
#if 0
void DirectGraphicsClass::SetTexture( int32_t index )
{
    if (index >= 0)
    {
        use_shader = shader_t::TEXTURE;
        glBindTexture( GL_TEXTURE_2D, textures.at(index) );
#if defined(USE_GL1)
        glEnable( GL_TEXTURE_2D );
#endif
#if defined(USE_ETC1)
        if (SupportedETC1)
        {
            glActiveTexture( GL_TEXTURE1 );
            glBindTexture( GL_TEXTURE_2D, alphatexs.at(index) );
            glActiveTexture( GL_TEXTURE0 );
        }
#endif
    }
    else
    {
        use_shader = shader_t::COLOR;
        glBindTexture( GL_TEXTURE_2D, 0 );
#if defined(USE_GL1)
        glDisable( GL_TEXTURE_2D );
#endif

#if defined(USE_ETC1)
        if (SupportedETC1)
        {
            glActiveTexture( GL_TEXTURE1 );
            glBindTexture( GL_TEXTURE_2D, 0 );
            glActiveTexture( GL_TEXTURE0 );
        }
#endif
    }
}
#endif  // 0
void DirectGraphicsClass::SetTexture(int idx) {
    if (idx >= 0) {
        use_shader = shader_t::TEXTURE;
        TextureHandle &th = Textures[idx];
        glBindTexture(GL_TEXTURE_2D, th.tex);
        const GLenum filter = display.textureFiltering ? GL_LINEAR : GL_NEAREST;
        if (th.minFilter != filter) {
            th.minFilter = filter;
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
        }
#if defined(USE_ETC1)
        if (SupportedETC1) {
            // Bind the alpha-channel texture when using ETC1-compressed textures:
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, th.alphatex);
            glActiveTexture(GL_TEXTURE0);
        }
#endif

#if defined(USE_GL1)
        glEnable(GL_TEXTURE_2D);
#endif
    } else {
        use_shader = shader_t::COLOR;
        // DKS - There is no need to call glBindTexture():
        // glBindTexture( GL_TEXTURE_2D, 0 );
#if defined(USE_GL1)
        glDisable(GL_TEXTURE_2D);
#endif

        // DKS - There is no need to call glBindTexture():
#if 0
#if defined(USE_ETC1)
        if (SupportedETC1)
        {
            glActiveTexture( GL_TEXTURE1 );
            glBindTexture( GL_TEXTURE_2D, 0 );
            glActiveTexture( GL_TEXTURE0 );
        }
#endif
#endif  // 0
    }
}

// --------------------------------------------------------------------------------------
// Present aufrufen
// --------------------------------------------------------------------------------------

void DirectGraphicsClass::ApplyFrameBrightness(int gainPercent) {
    if (gainPercent == 100)
        return;

    // Multiplicative gain on the completed image works on both rendering paths
    // and leaves the monitor's gamma settings alone.
    const auto projection = matProj;
    const auto model = g_matModelView;
    const auto shader = use_shader;
    const auto blend = BlendMode;
    matProj = g_matModelView = glm::mat4(1.0f);
#if defined(USE_GL1)
    load_matrix(GL_PROJECTION, glm::value_ptr(matProj));
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
    SetTexture(-1);
    const int gain = (gainPercent * 255 + 100) / 200;
    const auto color = D3DCOLOR_RGBA(gain, gain, gain, 255);
    VERTEX2D quad[] = {{-1, -1, color, 0, 0}, {1, -1, color, 0, 0},
                       {-1, 1, color, 0, 0}, {1, 1, color, 0, 0}};
    glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
    BlendMode = BlendModeEnum::BRIGHTNESS;
    RendertoBuffer(GL_TRIANGLE_STRIP, 2, quad);
    if (blend == BlendModeEnum::ADDITIV)
        SetAdditiveMode();
    else if (blend == BlendModeEnum::WHITE)
        SetWhiteMode();
    else
        SetColorKeyMode();
    use_shader = shader;
    matProj = projection;
    g_matModelView = model;
#if defined(USE_GL1)
    if (shader == shader_t::TEXTURE)
        glEnable(GL_TEXTURE_2D);
    load_matrix(GL_PROJECTION, glm::value_ptr(matProj));
    load_matrix(GL_MODELVIEW, glm::value_ptr(g_matModelView));
#endif
}

void DirectGraphicsClass::ShowBackBuffer() {
    ShowBackBuffer(display.BrightnessGain());
}

void DirectGraphicsClass::ShowBackBuffer(int brightnessGain) {
#if (defined(USE_GL2) || defined(USE_GL3)) && defined(USE_FBO)
    if (RenderBuffer.IsEnabled()) {
        VERTEX2D vertices[4];

        // Protokoll << std::dec << RenderRect.w << "x" << RenderRect.h << " at " << RenderRect.x << "x" << RenderRect.y
        // << std::endl;

        vertices[0].x = RenderRect.x;
        vertices[0].y = RenderRect.y + RenderRect.h; /* lower left */
        vertices[1].x = RenderRect.x;
        vertices[1].y = RenderRect.y; /* upper left */
        vertices[2].x = RenderRect.x + RenderRect.w;
        vertices[2].y = RenderRect.y + RenderRect.h; /* lower right */
        vertices[3].x = RenderRect.x + RenderRect.w;
        vertices[3].y = RenderRect.y; /* upper right */
        vertices[0].color = vertices[1].color = vertices[2].color = vertices[3].color = 0xFFFFFFFF;

        vertices[0].tu = 0;
        vertices[0].tv = 0; /* lower left */
        vertices[1].tu = 0;
        vertices[1].tv = 1; /* upper left */
        vertices[2].tu = 1;
        vertices[2].tv = 0; /* lower right */
        vertices[3].tu = 1;
        vertices[3].tv = 1; /* upper right */

        SelectBuffer(false);

        use_shader = display.crt && SupportsCrtFilter() ? shader_t::RENDER : shader_t::TEXTURE;
        RenderBuffer.BindTexture(true, display.textureFiltering);

        glDisable(GL_BLEND);
        RendertoBuffer(GL_TRIANGLE_STRIP, 2, &vertices[0]);
        glEnable(GL_BLEND);

        use_shader = shader_t::COLOR;
        RenderBuffer.BindTexture(false);

    }
#endif

    ApplyFrameBrightness(brightnessGain);

#if defined(USE_EGL_SDL) || defined(USE_EGL_RAW) || defined(USE_EGL_RPI)
    EGL_SwapBuffers();
#else
// DKS - TODO: Test with SDL2.0 and ensure it wouldn't be better to
//      include this in the new VSyncEnabled test below, calling
//      glFlush() instead of SDL_GL_SwapWindow() when vsync is either
//      disabled or unavailable.
#if SDL_VERSION_ATLEAST(2, 0, 0)
        SDL_GL_SwapWindow(Window);
#else  // SDL1.2:
        glFlush();
        SDL_GL_SwapBuffers();
#endif
#endif

#ifndef NDEBUG
    int error = glGetError();

    if (error != 0) {
        Protokoll << "GL Error " << std::hex << error << " file " << __FILE__ << ": line " << std::dec << __LINE__
                  << std::endl;
    }
#endif

#if (defined(USE_GL2) || defined(USE_GL3)) && defined(USE_FBO)
    SelectBuffer(true);
#endif
}

void DirectGraphicsClass::SetupFramebuffers(bool force) {
    int width, height;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_GL_GetDrawableSize(Window, &width, &height);
#else
    width = Screen->w;
    height = Screen->h;
#endif
    if (width <= 0 || height <= 0 || (!force && width == WindowView.w && height == WindowView.h))
        return;

    WindowView = {0, 0, width, height};
    const bool stretched = display.aspect == DisplayAspect::Stretch;
    constexpr double classicAspect = static_cast<double>(RENDERWIDTH) / RENDERHEIGHT;
    double aspect = classicAspect;
#if SDL_VERSION_ATLEAST(2, 0, 0) && !defined(USE_EGL_RAW) && !defined(USE_EGL_RPI)
    if (!CommandLineParams.LowRes) {
        switch (display.aspect) {
            case DisplayAspect::Automatic: {
                GLint maxSize = 0;
                glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxSize);
                const double maxAspect = static_cast<double>(std::max(RENDERWIDTH, maxSize & ~1)) / RENDERHEIGHT;
                aspect = std::clamp(static_cast<double>(width) / height, classicAspect, maxAspect);
                break;
            }
            case DisplayAspect::Wide16x9:  aspect = 16.0 / 9.0; break;
            case DisplayAspect::Wide16x10: aspect = 16.0 / 10.0; break;
            default: break;
        }
    }
#endif

    const int renderWidth = 2 * static_cast<int>(std::lround(RENDERHEIGHT * aspect / 2));
    RenderView = {0, 0, renderWidth, RENDERHEIGHT};

    const int fittedWidth = stretched ? width : std::min(width, static_cast<int>(std::lround(height * aspect)));
    const int fittedHeight = stretched ? height : std::min(height, static_cast<int>(std::lround(width / aspect)));
    RenderRect = {(width - fittedWidth) / 2, (height - fittedHeight) / 2, fittedWidth, fittedHeight};

    matProjWindow = glm::ortho(0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 0.0f, 1.0f);
    UpdateRenderTarget();
    ClearBackBuffer();
    Protokoll << "Drawable " << width << "x" << height << ", game view " << renderWidth << "x" << RENDERHEIGHT
              << ", output " << RenderRect.w << "x" << RenderRect.h << std::endl;
}

void DirectGraphicsClass::SetInterfaceRendering(bool enabled) {
#if defined(USE_FBO)
    if (interfaceRendering != enabled) {
        interfaceRendering = enabled;
        UpdateRenderTarget();
        ClearBackBuffer();
    }
#endif
}

void DirectGraphicsClass::UpdateRenderTarget() {
#if defined(USE_FBO)
    // Rasterize menus and cutscenes at output resolution while keeping their logical coordinates.
    const bool fullColor = interfaceRendering && !CommandLineParams.LowRes;
    const SDL_Rect requested = fullColor ? SDL_Rect{0, 0, RenderRect.w, RenderRect.h} : RenderView;
    if (requested.w != RequestedFramebufferView.w || requested.h != RequestedFramebufferView.h ||
        fullColor != framebufferFullColor) {
        RequestedFramebufferView = requested;
        framebufferFullColor = fullColor;
        FramebufferView = requested;
        RenderBuffer.Close();
        if (!RenderBuffer.Open(requested.w, requested.h, fullColor) && fullColor) {
            FramebufferView = RenderView;
            RenderBuffer.Open(RenderView.w, RenderView.h);
        }
    }
    if (RenderBuffer.IsEnabled()) {
        SelectBuffer(true);
        return;
    }
#endif
    glViewport(RenderRect.x, WindowView.h - RenderRect.y - RenderRect.h, RenderRect.w, RenderRect.h);
    SetInterfaceView();
}

float DirectGraphicsClass::InterfaceLeft() const {
    return (RENDERWIDTH - RenderView.w) * 0.5f;
}

void DirectGraphicsClass::SetInterfaceView() {
    SetHorizontalView(InterfaceLeft());
}

void DirectGraphicsClass::SetHorizontalView(float left) {
    matProj = glm::ortho(left, left + RenderView.w, static_cast<float>(RENDERHEIGHT), 0.0f, 0.0f, 1.0f);
#if defined(USE_GL1)
    load_matrix(GL_PROJECTION, glm::value_ptr(matProj));
#endif
}

void DirectGraphicsClass::SetGameViewport(const SDL_Rect &region, float left, float worldScale) {
    SDL_Rect area = RenderRect;
    int targetHeight = WindowView.h;
#if defined(USE_FBO)
    if (RenderBuffer.IsEnabled()) {
        area = FramebufferView;
        targetHeight = FramebufferView.h;
    }
#endif
    // Scale edges so adjacent viewports share the same boundary at odd output sizes.
    const int x = area.x + region.x * area.w / RenderView.w;
    const int y = area.y + region.y * area.h / RenderView.h;
    const int right = area.x + (region.x + region.w) * area.w / RenderView.w;
    const int bottom = area.y + (region.y + region.h) * area.h / RenderView.h;
    glViewport(x, targetHeight - bottom, right - x, bottom - y);
    matProj = glm::ortho(left, left + region.w * worldScale, region.h * worldScale, 0.0f, 0.0f, 1.0f);
#if defined(USE_GL1)
    load_matrix(GL_PROJECTION, glm::value_ptr(matProj));
#endif
}

bool DirectGraphicsClass::EnsureGameStencil() {
#if defined(USE_FBO)
    if (RenderBuffer.IsEnabled())
        return RenderBuffer.EnsureStencil(FramebufferView.w, FramebufferView.h);
#endif
    GLint stencilBits = 0;
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    return stencilBits > 0;
}

void DirectGraphicsClass::ClearBackBuffer() {
#if defined(USE_FBO)
    if (RenderBuffer.IsEnabled())
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
#endif
    glClear(GL_COLOR_BUFFER_BIT);
#if defined(USE_FBO)
    if (RenderBuffer.IsEnabled()) {
        glBindFramebuffer(GL_FRAMEBUFFER, RenderBuffer.GetFramebuffer());
        glClear(GL_COLOR_BUFFER_BIT);
    }
#endif
}

#if defined(USE_FBO)
void DirectGraphicsClass::SelectBuffer(bool active) {
    if (!RenderBuffer.IsEnabled())
        return;
    if (active) {
        glBindFramebuffer(GL_FRAMEBUFFER, RenderBuffer.GetFramebuffer());
        glViewport(FramebufferView.x, FramebufferView.y, FramebufferView.w, FramebufferView.h);
        SetInterfaceView();
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(WindowView.x, WindowView.y, WindowView.w, WindowView.h);
        g_matModelView = glm::mat4x4(1.0f);
        matProj = matProjWindow;
    }
}
#endif

// --------------------------------------------------------------------------------------
// Flip Surface
// Takes a SDL surface and flips it vertically
// --------------------------------------------------------------------------------------

#if SDL_VERSION_ATLEAST(2, 0, 0)
void DirectGraphicsClass::FlipSurface(SDL_Surface* surface) {
    SDL_LockSurface(surface);
    
    int pitch = surface->pitch; // row size
    char* temp = new char[pitch]; // intermediate buffer
    char* pixels = static_cast<char*>(surface->pixels);
    
    for(int i = 0; i < surface->h / 2; ++i) {
        // get pointers to the two rows to swap
        char* row1 = pixels + i * pitch;
        char* row2 = pixels + (surface->h - i - 1) * pitch;
        
        // swap rows
        memcpy(temp, row1, pitch);
        memcpy(row1, row2, pitch);
        memcpy(row2, temp, pitch);
    }
    
    delete[] temp;

    SDL_UnlockSurface(surface);
}
#endif

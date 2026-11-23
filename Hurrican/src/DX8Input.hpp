// Datei : DX8Input.hpp

// --------------------------------------------------------------------------------------
//
// DirectInput Klasse
// zum Erfassen von Eingaben durch Tastaur, Maus und Joystick/Joypad
//
// (c) 2002 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

#ifndef _DX8INPUT_HPP_
#define _DX8INPUT_HPP_

// --------------------------------------------------------------------------------------
// Include Dateien
// --------------------------------------------------------------------------------------

#include "DX8Joystick.hpp"

#include <bitset>

// --------------------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------------------

constexpr int MAX_MOUSEBUTTONS = 4;
constexpr int DINPUT_BUFFERSIZE = 32;
constexpr int MAX_KEYS = 256;

#if SDL_VERSION_ATLEAST(2, 0, 0)
#  define KeyDown(Taste) (TastaturPuffer[SDL_GetScancodeFromKey(Taste)] > 0)
#  define KeyCode(Taste) (SDL_GetKeyFromScancode(static_cast<SDL_Scancode>(Taste)))
#else
#  define KeyDown(Taste) (TastaturPuffer[Taste] > 0)
#  define KeyCode(Taste) (Taste)
#endif

// ForceFeedback Effekte
//
enum {
  FFE_SMALLRUMBLE = 0,
  FFE_MEDIUMRUMBLE = 1,
  FFE_BIGRUMBLE = 2,
  FFE_BLITZ = 3
};

constexpr int MAX_JOYSTICKS = 4;

constexpr int CONTROLTYPE_KEYBOARD = 0;
constexpr int CONTROLTYPE_JOY = 1;
constexpr int CONTROLTYPE_AUTO = 2;

// --------------------------------------------------------------------------------------
// Klassendeklaration
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// DirectInput Klasse
// --------------------------------------------------------------------------------------

class DirectInputClass {
  private:
#if defined(GCW)
    int InternalJoystickIndex;  // If the system has built-in joystick controls,
#endif                          // GCW
                                // this is their index, -1 otherwise.
  public:
    int JoysticksFound;
    DirectJoystickClass Joysticks[MAX_JOYSTICKS];

#if 0
    int MausX;  // Koordinaten der
    int MausY;  // der Maus und ihre Buttons
    std::bitset<MAX_MOUSEBUTTONS> MausButtons;
#endif

    DirectInputClass();   // Konstruktor
    ~DirectInputClass();  // Desktruktor

    bool Init();                      // DirectInput initialisieren
    void Exit();                      // DirectInput beenden
    bool UpdateTastatur();            // Tasten abfragen und im Puffer speichern
#if 0
    bool UpdateMaus(bool gepuffert);  // Maus abfragen gepuffert/ungepuffert
    void AcquireKeyboard();           // Keyboard akquirierens
#endif
    bool AreAllKeysReleased() const;  // Checken ob keine Taste mehr gedrückt ist
    bool AnyKeyDown() const;
    bool AnyButtonDown() const;
    void UpdateJoysticks();
    void RefreshJoysticks();
    void AssignPlayerJoysticks();
    void CyclePlayerJoystick(int player);
    const DirectJoystickClass *GetJoystick(int slot) const;
    bool AnyPauseButtonDown() const;
    int NextJoystick(int after) const;
    const DirectJoystickClass &MenuJoystick() const;
    const char *MapButtonToString(int joy_idx, int button);  // Returns char string "1" for button 0, etc..
                                                       // But when used on GCW Zero's internal controls
                                                       // (or other special cases) will return the custom
                                                       // string for the devices's buttons ("X", "Select", etc)

#if defined(GCW)
    // If the system has built-in joystick controls, this is their index. 0 is default.
    // On embedded systems like GCW Zero, this is set to the device's internal controls
    // so that a player is never 'locked out' and also so the game is not dependent on
    // the system always setting the internal controls to device index 0. Likely useful on other devices, too. -DKS
    int GetInternalJoystickIndex() { return InternalJoystickIndex; }

    // On GCW Zero, Main menu button is permanently mapped to START (button 5):
    int GetInternalJoystickMainMenuButton() { return 5; }

    bool InternalJoystickMainMenuButtonDown();
#endif  // GCW
    int NumberOfKeys;
};

// --------------------------------------------------------------------------------------
// Externals
// --------------------------------------------------------------------------------------

extern DirectInputClass DirectInput;
extern const Uint8 *TastaturPuffer;

#endif

// Datei : DX8Joystick.h

// --------------------------------------------------------------------------------------
//
// Joystick Klasse
//
// (c) 2002 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

#ifndef _DX8JOYSTICK_HPP_
#define _DX8JOYSTICK_HPP_

// --------------------------------------------------------------------------------------
// Defines
// --------------------------------------------------------------------------------------

constexpr int MAX_RAW_JOYSTICKBUTTONS = 128;
constexpr int CONTROLLER_BUTTON_BASE = MAX_RAW_JOYSTICKBUTTONS;
constexpr int CONTROLLER_LT = CONTROLLER_BUTTON_BASE + 32;
constexpr int CONTROLLER_RT = CONTROLLER_LT + 1;
constexpr int MAX_JOYSTICKBUTTONS = CONTROLLER_RT + 1;
constexpr int JoystickDeadzone = 500;

enum class PadDirection { Left, Right, Down, Up, Count };
enum class PadDirectionSource { DPad, LeftStick, RightStick, Count };
enum class RumbleStrength { Off, Low, Medium, High, Count };
enum class PlayStationSymbol { None, Cross, Circle, Square, Triangle };

constexpr int PadDirectionBinding(PadDirectionSource source, PadDirection direction) {
    return MAX_JOYSTICKBUTTONS + static_cast<int>(source) * static_cast<int>(PadDirection::Count) + static_cast<int>(direction);
}

// Existing profiles can use D-pad or stick Up as a single Jump binding.
constexpr int PadAutomaticUpBinding = PadDirectionBinding(PadDirectionSource::Count, PadDirection::Left);

constexpr int AutomaticPadBinding(PadDirection direction) {
    return direction == PadDirection::Up ? PadAutomaticUpBinding : PadAutomaticUpBinding + 1 + static_cast<int>(direction);
}

// Append the diagonals after all historical button and direction codes.
enum class PadDiagonal { UpLeft, UpRight, DownLeft, DownRight, Count };
constexpr int FirstPadDiagonal = AutomaticPadBinding(PadDirection::Down) + 1;
constexpr int PadDiagonalBinding(PadDirectionSource source, PadDiagonal direction) {
    return FirstPadDiagonal + static_cast<int>(source) * 4 + static_cast<int>(direction);
}
constexpr int LastPadBinding = PadDiagonalBinding(PadDirectionSource::RightStick, PadDiagonal::DownRight);
constexpr bool IsPadDirectionBinding(int binding) {
    return binding >= MAX_JOYSTICKBUTTONS && binding <= LastPadBinding;
}

// --------------------------------------------------------------------------------------
// Include Dateien
// --------------------------------------------------------------------------------------

#include "SDL_port.hpp"
#if SDL_VERSION_ATLEAST(2, 0, 0)
#  include <SDL_gamecontroller.h>
constexpr int ControllerButton(SDL_GameControllerButton button) {
    return CONTROLLER_BUTTON_BASE + button;
}
#endif

#include <array>
#include <bitset>
#include <string>

// --------------------------------------------------------------------------------------
// Klassendeklaration
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Joystick Klasse
// --------------------------------------------------------------------------------------

enum class MenuPadButton { Select, Back, Remove, Pause };

class DirectJoystickClass {
  private:
    LPDIRECTINPUTDEVICE8 lpDIJoystick;  // Joystick Device Interface

#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_GameController *controller = nullptr;
    SDL_JoystickID instance = -1;
#endif
    int NumButtons;                             // How many buttons joystick supports
    int NumAxis;                                // How many axis joystick supports
    int NumHats;                                // How many hats joystick supports

    bool CanForceFeedback;
    std::array<Uint8, static_cast<int>(PadDirectionSource::Count)> directions{};

  public:
    bool Active;
    int JoystickX;                              // Joystick x-Koordinaten
    int JoystickY;                              // Joystick y-Koordinaten
    int JoystickPOV;                            // POV (für coolie hat)
    std::string JoystickName;                   // Joystick Produktname

    DirectJoystickClass();
    ~DirectJoystickClass();

    void ForceFeedbackEffect(int nr, RumbleStrength strength, float intensity = 1.0f) const;
    void StopForceFeedbackEffect(int nr) const;

    bool Init(int joy);

    void Exit(int joy);

    bool Update();

    bool AnyButtonDown() const { return buttons.any(); }
    bool ButtonPressed(int binding) const;
    int PressedBinding() const;
    int PressedDirection() const;
    bool DirectionPressed(int binding) const;
    bool AutomaticDirectionPressed(PadDirection direction) const;
    static const char *DirectionName(int binding);
    bool HasControllerMapping() const;
    bool IsAttached() const;
#if SDL_VERSION_ATLEAST(2, 0, 0)
    SDL_JoystickID Instance() const { return instance; }
#endif
    bool HasNintendoLayout() const;
    bool HasPlayStationLayout() const;
    PlayStationSymbol FaceButtonSymbol(int binding) const;
    const char *BindingName(int binding) const;

    // DKS-Added these three for better joystick support, esp in menus
    // Returns true if button(s) serving as Enter key  are pressed (For menus)
    int MenuBinding(MenuPadButton action) const;
    bool ButtonEnterPressed() const;

    // Returns true if button(s) serving as Escape key are pressed (For menus)
    bool ButtonEscapePressed() const;

    // Returns true if button(s) serving as Delete key are pressed (For menus, esp. button mapping menu)
    bool ButtonDeletePressed() const;

    // Returns true if button(s) serving as Start key are pressed (For game)
    bool ButtonStartPressed() const;

  private:
    std::bitset<MAX_JOYSTICKBUTTONS> buttons;
};
#endif

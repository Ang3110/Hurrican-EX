// Datei : DX8Input.cpp

// --------------------------------------------------------------------------------------
//
// DirectInput Klasse
// zum Erfassen von Eingaben durch Tastaur, Maus und Joystick/Joypad
//
// (c) 2002 Jörg M. Winterstein
//
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Include Dateien
// --------------------------------------------------------------------------------------

#include "DX8Input.hpp"
#include <algorithm>
#include "Gameplay.hpp"
#include "Globals.hpp"
#include "Logdatei.hpp"
#include "Main.hpp"

// --------------------------------------------------------------------------------------
// Variablen
// --------------------------------------------------------------------------------------

const Uint8 *TastaturPuffer;

#if 0
// --------------------------------------------------------------------------------------
// Joystick mit ForceFeedback Enumeration Callback-Funktion
// --------------------------------------------------------------------------------------

bool EnumForceFeedbackDevices(int lpddi, void *pv) {
    return true;
}
// --------------------------------------------------------------------------------------
// Joystick Enumeration Callback-Funktion
// --------------------------------------------------------------------------------------

bool EnumJoystickDevices(int lpddi, void *pv) {
    return true;
}
#endif


// --------------------------------------------------------------------------------------
// Klassenfunktionen
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// DirectInput Klasse
// --------------------------------------------------------------------------------------

// --------------------------------------------------------------------------------------
// Konstruktor
// --------------------------------------------------------------------------------------

DirectInputClass::DirectInputClass() {
#if 0
    // Zu Beginn alle Eingabegeräte zurücksetzen
    MausX = 0;
    MausY = 0;
    MausButtons.reset();
#endif

    JoysticksFound = 0;
#if defined(GCW)
    InternalJoystickIndex = 0;  // DKS: Set to the device's internal joystick controls, default is 0.
#endif                          // GCW
}

// --------------------------------------------------------------------------------------
// Destruktor
// --------------------------------------------------------------------------------------

DirectInputClass::~DirectInputClass() = default;

// --------------------------------------------------------------------------------------
// DirectInput initialisieren
// Keyboard und Maus initialisieren und Joystick, falls vorhanden
// --------------------------------------------------------------------------------------
bool DirectInputClass::Init() {
#if SDL_VERSION_ATLEAST(2,0,0)
    if (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) < 0)
        Protokoll << "SDL game controller initialization: " << SDL_GetError() << std::endl;

    // Poll input state while keeping the device-change events enabled.
    SDL_EventState(SDL_JOYAXISMOTION, SDL_IGNORE);
    SDL_EventState(SDL_JOYBALLMOTION, SDL_IGNORE);
    SDL_EventState(SDL_JOYHATMOTION, SDL_IGNORE);
    SDL_EventState(SDL_JOYBUTTONDOWN, SDL_IGNORE);
    SDL_EventState(SDL_JOYBUTTONUP, SDL_IGNORE);
    SDL_EventState(SDL_CONTROLLERAXISMOTION, SDL_IGNORE);
    SDL_EventState(SDL_CONTROLLERBUTTONDOWN, SDL_IGNORE);
    SDL_EventState(SDL_CONTROLLERBUTTONUP, SDL_IGNORE);
#endif

#if SDL_VERSION_ATLEAST(2, 0, 0)
    TastaturPuffer = SDL_GetKeyboardState(&NumberOfKeys);
#else
    TastaturPuffer = SDL_GetKeyState(&NumberOfKeys);
#endif
    Protokoll << "DirectInput8 polling for " << NumberOfKeys << " keys!" << std::endl;

#  if SDL_VERSION_ATLEAST(2,0,2)
    std::string gcdb(g_storage_ext);
    gcdb.append("/data/gamecontrollerdb.txt");
    int res = SDL_GameControllerAddMappingsFromFile(gcdb.c_str());
    if (res > 0) {
        Protokoll << "Loaded " << res << " mappings from game controller db" << std::endl;
    }
#  endif
    RefreshJoysticks();
    return true;
}

void DirectInputClass::RefreshJoysticks() {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    for (int slot = 0; slot < MAX_JOYSTICKS; ++slot)
        if (Joysticks[slot].Active && !Joysticks[slot].IsAttached())
            Joysticks[slot].Exit(slot);
#endif

    // Retain assigned slots when a gamepad is disconnected or a new pad appears.
    const int devices = SDL_NumJoysticks();
    for (int device = 0; device < devices; ++device) {
#if SDL_VERSION_ATLEAST(2, 0, 6)
        const SDL_JoystickID id = SDL_JoystickGetDeviceInstanceID(device);
        if (id < 0 || std::any_of(std::begin(Joysticks), std::end(Joysticks), [id](const auto &joystick) {
            return joystick.Instance() == id;
        }))
            continue;
#elif SDL_VERSION_ATLEAST(2, 0, 0)
        if (JoystickFound)
            break;
#else
        if (JoysticksFound > 0)
            break;
#endif
        auto slot = std::find_if(std::begin(Joysticks), std::end(Joysticks), [](const auto &joystick) {
            return !joystick.Active;
        });
        if (slot == std::end(Joysticks))
            break;
        if (!slot->Init(device))
            continue;
#if defined(GCW)
        if (slot->JoystickName == "linkdev device (Analog 2-axis 8-button 2-hat)") {
            slot->JoystickName = "GCW Zero";
            InternalJoystickIndex = static_cast<int>(slot - std::begin(Joysticks));
        }
#endif
    }

    JoysticksFound = MAX_JOYSTICKS;
    while (JoysticksFound > 0 && !Joysticks[JoysticksFound - 1].Active)
        --JoysticksFound;
    JoystickFound = JoysticksFound > 0;
    AssignPlayerJoysticks();
    UpdateJoysticks();
}

void DirectInputClass::AssignPlayerJoysticks() {
    std::bitset<MAX_JOYSTICKS> claimed;

    // An explicit choice owns its slot even while the controller is unplugged.
    for (auto &player : Player) {
        if (player.ControlType != CONTROLTYPE_JOY)
            continue;
        const int slot = player.JoystickIndex;
        if (slot >= 0 && slot < MAX_JOYSTICKS && !claimed[slot])
            claimed.set(slot);
        else
            player.ControlType = CONTROLTYPE_AUTO;  // Repair old duplicate assignments.
    }

    // Keep each automatic assignment that still has its controller.
    for (auto &player : Player) {
        if (player.ControlType != CONTROLTYPE_AUTO)
            continue;
        const int slot = player.JoystickIndex;
        if (GetJoystick(slot) && !claimed[slot])
            claimed.set(slot);
        else
            player.JoystickIndex = -1;
    }

    for (auto &player : Player) {
        if (player.ControlType != CONTROLTYPE_AUTO || player.JoystickIndex >= 0)
            continue;
        for (int slot = NextJoystick(-1); slot >= 0; slot = NextJoystick(slot)) {
            if (!claimed[slot]) {
                player.JoystickIndex = slot;
                claimed.set(slot);
                break;
            }
        }
    }
}

void DirectInputClass::CyclePlayerJoystick(int player) {
    auto &current = Player[player];
    const auto &other = Player[1 - player];
    if (current.ControlType == CONTROLTYPE_AUTO) {
        current.ControlType = CONTROLTYPE_KEYBOARD;
    } else {
        int slot = NextJoystick(current.ControlType == CONTROLTYPE_JOY ? current.JoystickIndex : -1);
        if (slot >= 0 && other.ControlType == CONTROLTYPE_JOY && slot == other.JoystickIndex)
            slot = NextJoystick(slot);
        current.ControlType = slot >= 0 ? CONTROLTYPE_JOY : CONTROLTYPE_AUTO;
        current.JoystickIndex = slot >= 0 ? slot : player;
    }
    AssignPlayerJoysticks();
}

const DirectJoystickClass *DirectInputClass::GetJoystick(int slot) const {
    return slot >= 0 && slot < JoysticksFound && Joysticks[slot].Active ? &Joysticks[slot] : nullptr;
}

bool DirectInputClass::AnyPauseButtonDown() const {
    for (int slot = NextJoystick(-1); slot >= 0; slot = NextJoystick(slot))
        if (Joysticks[slot].ButtonStartPressed())
            return true;
    return false;
}

int DirectInputClass::NextJoystick(int after) const {
    for (int slot = after + 1; slot < JoysticksFound; ++slot)
        if (Joysticks[slot].Active)
            return slot;
    return -1;
}

const DirectJoystickClass &DirectInputClass::MenuJoystick() const {
    if (const auto *assigned = Player[0].GetJoystick())
        return *assigned;
    const int first = NextJoystick(-1);
    return Joysticks[first >= 0 ? first : 0];
}

// --------------------------------------------------------------------------------------
// DirectInput beenden
// --------------------------------------------------------------------------------------

void DirectInputClass::Exit() {
    for (int i = 0; i < JoysticksFound; i++) {
        Joysticks[i].Exit(i);
    }

    JoystickFound = false;
    JoysticksFound = 0;

    Protokoll << "-> SDL/OpenGL input shutdown successfully completed !" << std::endl;
}

// --------------------------------------------------------------------------------------
// Tasten abfragen und im Puffer speichern
// --------------------------------------------------------------------------------------

bool DirectInputClass::UpdateTastatur() {
    SDL_PumpEvents();
    return true;
}

#if 0
// --------------------------------------------------------------------------------------
// Maus abfragen
// true  = gepuffert
// false = ungepuffert
// --------------------------------------------------------------------------------------

bool DirectInputClass::UpdateMaus(bool gepuffert) {
    // Position
    uint8_t buttons = SDL_GetMouseState(&MausX, &MausY);

    // Buttons
    MausButtons[0] = ((buttons & SDL_BUTTON(1)) == SDL_BUTTON(1));
    MausButtons[1] = ((buttons & SDL_BUTTON(2)) == SDL_BUTTON(2));
    MausButtons[2] = ((buttons & SDL_BUTTON(3)) == SDL_BUTTON(3));

    MausX = std::clamp(MausX, 0, RENDERWIDTH);
    MausY = std::clamp(MausY, 0, RENDERHEIGHT);

    return true;
}

// --------------------------------------------------------------------------------------
// Keyboard wieder akquirieren (wenn Fenster gewechselt zB)
// --------------------------------------------------------------------------------------

void DirectInputClass::AcquireKeyboard() {}
#endif

// --------------------------------------------------------------------------------------
// Alle Joysticks updaten
// --------------------------------------------------------------------------------------

void DirectInputClass::UpdateJoysticks() {
    SDL_PumpEvents();
    SDL_JoystickUpdate();
    for (int i = 0; i < JoysticksFound; i++)
        if (Joysticks[i].Active)
            Joysticks[i].Update();
}

// --------------------------------------------------------------------------------------
// Checken ob keine Taste mehr gedrückt ist
// --------------------------------------------------------------------------------------

bool DirectInputClass::AreAllKeysReleased() const {
    for (int i = 0; i < NumberOfKeys; i++)
        if (TastaturPuffer[i] != 0)
            return false;

    return true;
}

// --------------------------------------------------------------------------------------
// Checken ob irgendeine Taste gedrückt ist
// --------------------------------------------------------------------------------------

bool DirectInputClass::AnyKeyDown() const {
    return !AreAllKeysReleased();
}

// --------------------------------------------------------------------------------------
// Checken ob irgendein Button gedrückt ist
// --------------------------------------------------------------------------------------

bool DirectInputClass::AnyButtonDown() const {
    if (!JoystickFound)
        return false;

    for (int i = 0; i < JoysticksFound; i++)
        if (Joysticks[i].AnyButtonDown())
            return true;

    return false;
}

// DKS - GCW Zero handheld-specific button strings
#if defined(GCW)
#define GCW_MAX_BUTTONS 8
static char InternalButtonNumToStringMap[8][10] = {
    "B",  // Button 0
    "A", "Y", "X", "Select", "Start", "L",
    "R"  // Button 7
};
#endif  // GCW

// DKS - Added helper function to facilitate customized naming of joystick buttons and
//      when not customized, to report joy buttons as ranging 1..99 instead of 0..98
const char *DirectInputClass::MapButtonToString(int joy_idx, int button) {
    static char buf[60];

    if (button < 0) {
        return TextArray[TEXT::NICHT_DEFINIERT];
    } else {
        if (const auto *joystick = GetJoystick(joy_idx))
            if (const char *name = joystick->BindingName(button))
                return name;
#ifdef GCW
        // Special case for GCW Zero's internal controls:
        if (joy_idx == GetInternalJoystickIndex() && button < GCW_MAX_BUTTONS) {
            return InternalButtonNumToStringMap[button];
        } else
#endif  // GCW
        // Non-platform-specific generic code for all joysticks:
        {
            // Report button numbers as ranging from 1..99 instead of 0..98
            snprintf(buf, 20, "%d", button + 1);
            return buf;
        }
    }
}

// DKS - Convenience function for when in the actual game
#if defined(GCW)
bool DirectInputClass::InternalJoystickMainMenuButtonDown() {
    int joy_idx = GetInternalJoystickIndex();
    int button_idx = GetInternalJoystickMainMenuButton();
    return joy_idx >= 0 && joy_idx < JoysticksFound && Joysticks[joy_idx].ButtonPressed(button_idx);
}
#endif  // GCW

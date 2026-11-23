// Joystick input (original class by Jörg M. Winterstein).

#include "DX8Joystick.hpp"
#include "Gameplay.hpp"
#include "Logdatei.hpp"

#include <algorithm>
#include <utility>

namespace {
constexpr Uint16 rumble[] = {20000, 40000, 60000, 30000};
constexpr int RAW_START = 7;
constexpr int RAW_ENTER = 0;
constexpr int RAW_BACK = 1;
constexpr int RAW_DELETE = 4;

int HatAngle(Uint8 directions) {
    switch (directions) {
        case SDL_HAT_UP:        return 0;
        case SDL_HAT_RIGHTUP:   return 4500;
        case SDL_HAT_RIGHT:     return 9000;
        case SDL_HAT_RIGHTDOWN: return 13500;
        case SDL_HAT_DOWN:      return 18000;
        case SDL_HAT_LEFTDOWN:  return 22500;
        case SDL_HAT_LEFT:      return 27000;
        case SDL_HAT_LEFTUP:    return 31500;
        default:                return -1;
    }
}

constexpr float STICK_SCALE = 1000.0f / 32768.0f;
constexpr Uint8 directionBits[] = {SDL_HAT_LEFT, SDL_HAT_RIGHT, SDL_HAT_DOWN, SDL_HAT_UP};

Uint8 StickDirections(int x, int y) {
    return (x < -JoystickDeadzone ? SDL_HAT_LEFT : 0) | (x > JoystickDeadzone ? SDL_HAT_RIGHT : 0) |
           (y < -JoystickDeadzone ? SDL_HAT_UP : 0) | (y > JoystickDeadzone ? SDL_HAT_DOWN : 0);
}

#if SDL_VERSION_ATLEAST(2, 0, 0)
bool TriggerPressed(Sint16 position, bool wasPressed) {
    // Hysteresis keeps the digital action steady near the actuation point.
    return position >= (wasPressed ? 5000 : 8000);
}
#endif
}

DirectJoystickClass::DirectJoystickClass()
    : lpDIJoystick(nullptr), NumButtons(0), NumAxis(0), NumHats(0),
      CanForceFeedback(false), Active(false), JoystickX(0), JoystickY(0), JoystickPOV(-1) {}

DirectJoystickClass::~DirectJoystickClass() = default;

void DirectJoystickClass::ForceFeedbackEffect(int nr, RumbleStrength strength, float intensity) const {
#if SDL_VERSION_ATLEAST(2, 0, 9)
    const int level = static_cast<int>(strength);
    if (IsAttached() && CanForceFeedback && level > 0 && level < static_cast<int>(RumbleStrength::Count) &&
        nr >= 0 && nr < static_cast<int>(std::size(rumble))) {
        const auto amplitude = static_cast<Uint16>(rumble[nr] * std::clamp(intensity, 0.0f, 1.0f) * level / static_cast<int>(RumbleStrength::High));
        SDL_JoystickRumble(lpDIJoystick, amplitude, amplitude, 100 * (nr + 1));
    }
#endif
}

void DirectJoystickClass::StopForceFeedbackEffect(int) const {
#if SDL_VERSION_ATLEAST(2, 0, 9)
    if (IsAttached() && CanForceFeedback)
        SDL_JoystickRumble(lpDIJoystick, 0, 0, 0);
#endif
}

bool DirectJoystickClass::Init(int joy) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (SDL_IsGameController(joy))
        controller = SDL_GameControllerOpen(joy);
    lpDIJoystick = controller ? SDL_GameControllerGetJoystick(controller) : SDL_JoystickOpen(joy);
#else
    lpDIJoystick = SDL_JoystickOpen(joy);
#endif

    if (!lpDIJoystick) {
        Protokoll << "Cannot open joystick " << joy << ": " << SDL_GetError() << std::endl;
        return false;
    }

    Active = true;
    NumButtons = std::clamp(SDL_JoystickNumButtons(lpDIJoystick), 0, MAX_RAW_JOYSTICKBUTTONS);
    NumAxis = SDL_JoystickNumAxes(lpDIJoystick);
    NumHats = SDL_JoystickNumHats(lpDIJoystick);
#if SDL_VERSION_ATLEAST(2, 0, 0)
    instance = SDL_JoystickInstanceID(lpDIJoystick);
    const char *name = controller ? SDL_GameControllerName(controller) : SDL_JoystickName(lpDIJoystick);
#else
    const char *name = SDL_JoystickName(joy);
#endif
    JoystickName = name ? name : "Joystick";

#if SDL_VERSION_ATLEAST(2, 0, 18)
    CanForceFeedback = SDL_JoystickHasRumble(lpDIJoystick) == SDL_TRUE;
#elif SDL_VERSION_ATLEAST(2, 0, 9)
    CanForceFeedback = SDL_JoystickRumble(lpDIJoystick, 0, 0, 0) == 0;
#endif
    Protokoll << "Joystick " << joy << ": " << JoystickName
              << (HasControllerMapping() ? " (SDL game controller)" : " (raw joystick)") << std::endl;
    return true;
}

void DirectJoystickClass::Exit(int) {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (controller) {
        SDL_GameControllerClose(controller);
        controller = nullptr;
    } else
#endif
    if (lpDIJoystick) {
        SDL_JoystickClose(lpDIJoystick);
    }
#if SDL_VERSION_ATLEAST(2, 0, 0)
    instance = -1;
#endif
    lpDIJoystick = nullptr;
    Active = false;
    CanForceFeedback = false;
    NumButtons = 0;
    NumAxis = 0;
    NumHats = 0;
    JoystickName.clear();
    JoystickX = JoystickY = 0;
    JoystickPOV = -1;
    buttons.reset();
    directions.fill(SDL_HAT_CENTERED);
}

bool DirectJoystickClass::HasControllerMapping() const {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    return controller != nullptr;
#else
    return false;
#endif
}

bool DirectJoystickClass::IsAttached() const {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    return Active && SDL_JoystickGetAttached(lpDIJoystick);
#else
    return Active;
#endif
}

bool DirectJoystickClass::Update() {
    const bool wasLT = buttons[CONTROLLER_LT];
    const bool wasRT = buttons[CONTROLLER_RT];
    buttons.reset();
    directions.fill(SDL_HAT_CENTERED);

    if (!IsAttached()) {
        JoystickX = JoystickY = 0;
        JoystickPOV = -1;
        return false;
    }

    for (int i = 0; i < NumButtons; ++i)
        buttons[i] = SDL_JoystickGetButton(lpDIJoystick, i) != 0;

#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (controller) {
        static_assert(SDL_CONTROLLER_BUTTON_MAX <= CONTROLLER_LT - CONTROLLER_BUTTON_BASE);
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            auto button = static_cast<SDL_GameControllerButton>(i);
            buttons[ControllerButton(button)] = SDL_GameControllerGetButton(controller, button) != 0;
        }
        buttons[CONTROLLER_LT] = TriggerPressed(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT), wasLT);
        buttons[CONTROLLER_RT] = TriggerPressed(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT), wasRT);

        // Old configuration files store trigger axes as buttons appended after the physical ones.
        int legacy = NumButtons;
        if (legacy < MAX_RAW_JOYSTICKBUTTONS &&
            SDL_GameControllerGetBindForAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT).bindType == SDL_CONTROLLER_BINDTYPE_AXIS)
            buttons[legacy++] = buttons[CONTROLLER_LT];
        if (legacy < MAX_RAW_JOYSTICKBUTTONS &&
            SDL_GameControllerGetBindForAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT).bindType == SDL_CONTROLLER_BINDTYPE_AXIS)
            buttons[legacy] = buttons[CONTROLLER_RT];

        JoystickX = static_cast<int>(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) * STICK_SCALE);
        JoystickY = static_cast<int>(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) * STICK_SCALE);

        Uint8 dpad = SDL_HAT_CENTERED;
        if (buttons[ControllerButton(SDL_CONTROLLER_BUTTON_DPAD_UP)])    dpad |= SDL_HAT_UP;
        if (buttons[ControllerButton(SDL_CONTROLLER_BUTTON_DPAD_DOWN)])  dpad |= SDL_HAT_DOWN;
        if (buttons[ControllerButton(SDL_CONTROLLER_BUTTON_DPAD_LEFT)])  dpad |= SDL_HAT_LEFT;
        if (buttons[ControllerButton(SDL_CONTROLLER_BUTTON_DPAD_RIGHT)]) dpad |= SDL_HAT_RIGHT;
        JoystickPOV = HatAngle(dpad);
        directions[static_cast<int>(PadDirectionSource::DPad)] = dpad;
        directions[static_cast<int>(PadDirectionSource::LeftStick)] = StickDirections(JoystickX, JoystickY);
        directions[static_cast<int>(PadDirectionSource::RightStick)] = StickDirections(
            static_cast<int>(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) * STICK_SCALE),
            static_cast<int>(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) * STICK_SCALE));
        return true;
    }
#endif

    if (NumAxis >= 2) {
        JoystickX = static_cast<int>(SDL_JoystickGetAxis(lpDIJoystick, 0) * STICK_SCALE);
        JoystickY = static_cast<int>(SDL_JoystickGetAxis(lpDIJoystick, 1) * STICK_SCALE);
    }
    if (NumAxis >= 4)
        directions[static_cast<int>(PadDirectionSource::RightStick)] = StickDirections(
            static_cast<int>(SDL_JoystickGetAxis(lpDIJoystick, 2) * STICK_SCALE),
            static_cast<int>(SDL_JoystickGetAxis(lpDIJoystick, 3) * STICK_SCALE));
    const Uint8 dpad = NumHats > 0 ? SDL_JoystickGetHat(lpDIJoystick, 0) : SDL_HAT_CENTERED;
    JoystickPOV = HatAngle(dpad);
    directions[static_cast<int>(PadDirectionSource::DPad)] = dpad;
    directions[static_cast<int>(PadDirectionSource::LeftStick)] = StickDirections(JoystickX, JoystickY);
    return true;
}

bool DirectJoystickClass::DirectionPressed(int binding) const {
    if (!IsPadDirectionBinding(binding))
        return false;
    if (binding >= FirstPadDiagonal) {
        constexpr Uint8 masks[] = {SDL_HAT_LEFTUP, SDL_HAT_RIGHTUP, SDL_HAT_LEFTDOWN, SDL_HAT_RIGHTDOWN};
        const int index = binding - FirstPadDiagonal;
        const auto mask = masks[index % 4];
        return (directions[index / 4] & mask) == mask;
    }
    if (binding >= PadAutomaticUpBinding) {
        const auto dir = binding == PadAutomaticUpBinding ? PadDirection::Up
            : static_cast<PadDirection>(binding - PadAutomaticUpBinding - 1);
        return AutomaticDirectionPressed(dir);
    }
    const int index = binding - MAX_JOYSTICKBUTTONS;
    return (directions[index / static_cast<int>(PadDirection::Count)] &
            directionBits[index % static_cast<int>(PadDirection::Count)]) != 0;
}

bool DirectJoystickClass::AutomaticDirectionPressed(PadDirection direction) const {
    const auto source = directions[static_cast<int>(PadDirectionSource::DPad)] ? PadDirectionSource::DPad : PadDirectionSource::LeftStick;
    return DirectionPressed(PadDirectionBinding(source, direction));
}

int DirectJoystickClass::PressedDirection() const {
    for (int binding = FirstPadDiagonal; binding <= LastPadBinding; ++binding)
        if (DirectionPressed(binding))
            return binding;
    for (int binding = MAX_JOYSTICKBUTTONS; binding < PadAutomaticUpBinding; ++binding)
        if (DirectionPressed(binding))
            return binding;
    return -1;
}

const char *DirectJoystickClass::DirectionName(int binding) {
    constexpr const char *names[] = {"D-pad Left", "D-pad Right", "D-pad Down", "D-pad Up",
                                     "LS Left", "LS Right", "LS Down", "LS Up",
                                     "RS Left", "RS Right", "RS Down", "RS Up"};
    static_assert(std::size(names) == static_cast<int>(PadDirectionSource::Count) * static_cast<int>(PadDirection::Count));
    constexpr const char *autoNames[] = {"D-pad/LS Up", "D-pad/LS Left", "D-pad/LS Right", "D-pad/LS Down"};
    constexpr const char *diagonals[] = {"D-pad Up-Left", "D-pad Up-Right", "D-pad Down-Left", "D-pad Down-Right",
                                         "LS Up-Left", "LS Up-Right", "LS Down-Left", "LS Down-Right",
                                         "RS Up-Left", "RS Up-Right", "RS Down-Left", "RS Down-Right"};
    if (!IsPadDirectionBinding(binding))
        return "";
    if (binding >= FirstPadDiagonal)
        return diagonals[binding - FirstPadDiagonal];
    return binding < PadAutomaticUpBinding ? names[binding - MAX_JOYSTICKBUTTONS] : autoNames[binding - PadAutomaticUpBinding];
}

bool DirectJoystickClass::ButtonPressed(int binding) const {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (!controller) {
        // Keep sensible out-of-the-box bindings for joysticks with no SDL controller mapping.
        switch (binding) {
            case ControllerButton(SDL_CONTROLLER_BUTTON_A):             binding = 0; break;
            case CONTROLLER_RT:                                         binding = 1; break;
            case CONTROLLER_LT:                                         binding = 2; break;
            case ControllerButton(SDL_CONTROLLER_BUTTON_Y):             binding = 3; break;
            case ControllerButton(SDL_CONTROLLER_BUTTON_X):             binding = 4; break;
            case ControllerButton(SDL_CONTROLLER_BUTTON_BACK):          binding = 8; break;
            case ControllerButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER):  binding = 5; break;
            case ControllerButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER): binding = 6; break;
            default: break;
        }
    }
#endif
    return binding >= 0 && binding < MAX_JOYSTICKBUTTONS && buttons[binding];
}

int DirectJoystickClass::PressedBinding() const {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (controller) {
        if (buttons[CONTROLLER_LT]) return CONTROLLER_LT;
        if (buttons[CONTROLLER_RT]) return CONTROLLER_RT;
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            if (i == SDL_CONTROLLER_BUTTON_START || i == SDL_CONTROLLER_BUTTON_GUIDE ||
                (i >= SDL_CONTROLLER_BUTTON_DPAD_UP && i <= SDL_CONTROLLER_BUTTON_DPAD_RIGHT))
                continue;
            if (buttons[CONTROLLER_BUTTON_BASE + i])
                return CONTROLLER_BUTTON_BASE + i;
        }
        return -1;
    }
#endif
    for (int i = 0; i < NumButtons; ++i)
        if (i != RAW_START && buttons[i])
            return i;
    return -1;
}

bool DirectJoystickClass::HasNintendoLayout() const {
#if SDL_VERSION_ATLEAST(2, 0, 12)
    if (controller) {
        const auto type = SDL_GameControllerGetType(controller);
        if (type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_PRO) return true;
#if SDL_VERSION_ATLEAST(2, 24, 0)
        return type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_LEFT ||
               type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT || type == SDL_CONTROLLER_TYPE_NINTENDO_SWITCH_JOYCON_PAIR;
#endif
    }
#endif
    return false;
}

bool DirectJoystickClass::HasPlayStationLayout() const {
#if SDL_VERSION_ATLEAST(2, 0, 14)
    if (controller) {
        const auto type = SDL_GameControllerGetType(controller);
        return type == SDL_CONTROLLER_TYPE_PS3 || type == SDL_CONTROLLER_TYPE_PS4 || type == SDL_CONTROLLER_TYPE_PS5;
    }
#endif
    return false;
}

PlayStationSymbol DirectJoystickClass::FaceButtonSymbol(int binding) const {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (binding < 0 || !HasPlayStationLayout())
        return PlayStationSymbol::None;

    constexpr std::pair<SDL_GameControllerButton, PlayStationSymbol> faceButtons[] = {
        {SDL_CONTROLLER_BUTTON_A, PlayStationSymbol::Cross},
        {SDL_CONTROLLER_BUTTON_B, PlayStationSymbol::Circle},
        {SDL_CONTROLLER_BUTTON_X, PlayStationSymbol::Square},
        {SDL_CONTROLLER_BUTTON_Y, PlayStationSymbol::Triangle},
    };
    for (const auto &[button, symbol] : faceButtons) {
        if (binding == ControllerButton(button))
            return symbol;
        if (binding < MAX_RAW_JOYSTICKBUTTONS) {
            const auto raw = SDL_GameControllerGetBindForButton(controller, button);
            if (raw.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON && raw.value.button == binding)
                return symbol;
        }
    }
#endif
    return PlayStationSymbol::None;
}

const char *DirectJoystickClass::BindingName(int binding) const {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (controller && binding >= 0 && binding < MAX_RAW_JOYSTICKBUTTONS) {
        for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
            const auto btn = static_cast<SDL_GameControllerButton>(i);
            const auto raw = SDL_GameControllerGetBindForButton(controller, btn);
            if (raw.bindType == SDL_CONTROLLER_BINDTYPE_BUTTON && raw.value.button == binding) {
                binding = ControllerButton(btn);
                break;
            }
        }
    }
    if (HasNintendoLayout()) {
        switch (binding) {
            case ControllerButton(SDL_CONTROLLER_BUTTON_A): return "B";
            case ControllerButton(SDL_CONTROLLER_BUTTON_B): return "A";
            case ControllerButton(SDL_CONTROLLER_BUTTON_X): return "Y";
            case ControllerButton(SDL_CONTROLLER_BUTTON_Y): return "X";
            case ControllerButton(SDL_CONTROLLER_BUTTON_START): return "+";
            case ControllerButton(SDL_CONTROLLER_BUTTON_BACK): return "-";
            case ControllerButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER): return "L";
            case ControllerButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER): return "R";
            case CONTROLLER_LT: return "ZL";
            case CONTROLLER_RT: return "ZR";
            default: break;
        }
    }
    switch (FaceButtonSymbol(binding)) {
        case PlayStationSymbol::Cross:    return "\xad";
        case PlayStationSymbol::Circle:   return "\xae";
        case PlayStationSymbol::Square:   return "\xaf";
        case PlayStationSymbol::Triangle: return "\xb0";
        case PlayStationSymbol::None:     break;
    }
    if (HasPlayStationLayout()) {
        switch (binding) {
            case ControllerButton(SDL_CONTROLLER_BUTTON_START):
#if SDL_VERSION_ATLEAST(2, 0, 14)
                return SDL_GameControllerGetType(controller) == SDL_CONTROLLER_TYPE_PS3 ? "Start" : "Options";
#else
                return "Start";
#endif
            case ControllerButton(SDL_CONTROLLER_BUTTON_BACK):
#if SDL_VERSION_ATLEAST(2, 0, 14)
                return SDL_GameControllerGetType(controller) == SDL_CONTROLLER_TYPE_PS3 ? "Select" :
                       SDL_GameControllerGetType(controller) == SDL_CONTROLLER_TYPE_PS5 ? "Create" : "Share";
#else
                return "Select";
#endif
            case ControllerButton(SDL_CONTROLLER_BUTTON_LEFTSTICK):    return "L3";
            case ControllerButton(SDL_CONTROLLER_BUTTON_RIGHTSTICK):   return "R3";
            case CONTROLLER_LT:                                        return "L2";
            case CONTROLLER_RT:                                        return "R2";
            case ControllerButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER): return "L1";
            case ControllerButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER):return "R1";
            default: break;
        }
    }
    switch (binding) {
        case CONTROLLER_LT:                                         return "LT";
        case CONTROLLER_RT:                                         return "RT";
        case ControllerButton(SDL_CONTROLLER_BUTTON_A):             return "A";
        case ControllerButton(SDL_CONTROLLER_BUTTON_B):             return "B";
        case ControllerButton(SDL_CONTROLLER_BUTTON_X):             return "X";
        case ControllerButton(SDL_CONTROLLER_BUTTON_Y):             return "Y";
        case ControllerButton(SDL_CONTROLLER_BUTTON_START):
#if SDL_VERSION_ATLEAST(2, 0, 12)
            if (controller && SDL_GameControllerGetType(controller) == SDL_CONTROLLER_TYPE_XBOX360)
                return "Start";
#endif
            return "Menu";
        case ControllerButton(SDL_CONTROLLER_BUTTON_BACK):
#if SDL_VERSION_ATLEAST(2, 0, 12)
            if (controller && SDL_GameControllerGetType(controller) == SDL_CONTROLLER_TYPE_XBOX360)
                return "Back";
#endif
            return "View";
        case ControllerButton(SDL_CONTROLLER_BUTTON_LEFTSTICK):     return "LS click";
        case ControllerButton(SDL_CONTROLLER_BUTTON_RIGHTSTICK):    return "RS click";
        case ControllerButton(SDL_CONTROLLER_BUTTON_LEFTSHOULDER):  return "LB";
        case ControllerButton(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER): return "RB";
        default: break;
    }
    if (binding >= CONTROLLER_BUTTON_BASE && binding < CONTROLLER_BUTTON_BASE + SDL_CONTROLLER_BUTTON_MAX)
        return SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(binding - CONTROLLER_BUTTON_BASE));
#endif
    return nullptr;
}

int DirectJoystickClass::MenuBinding(MenuPadButton action) const {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (controller) {
        switch (action) {
            case MenuPadButton::Select: return ControllerButton(HasNintendoLayout() ? SDL_CONTROLLER_BUTTON_B : SDL_CONTROLLER_BUTTON_A);
            case MenuPadButton::Back:   return ControllerButton(HasNintendoLayout() ? SDL_CONTROLLER_BUTTON_A : SDL_CONTROLLER_BUTTON_B);
            case MenuPadButton::Remove: return ControllerButton(SDL_CONTROLLER_BUTTON_Y);
            case MenuPadButton::Pause:  return ControllerButton(SDL_CONTROLLER_BUTTON_START);
        }
    }
#endif
    switch (action) {
        case MenuPadButton::Select: return RAW_ENTER;
        case MenuPadButton::Back:   return RAW_BACK;
        case MenuPadButton::Remove: return RAW_DELETE;
        case MenuPadButton::Pause:
#if defined(GCW)
            if (JoystickName == "GCW Zero") return 5;
#endif
            return RAW_START;
    }
    return RAW_BACK;
}

bool DirectJoystickClass::ButtonEnterPressed() const {
    return buttons[MenuBinding(MenuPadButton::Select)];
}

bool DirectJoystickClass::ButtonEscapePressed() const {
#if SDL_VERSION_ATLEAST(2, 0, 0)
    if (controller && buttons[ControllerButton(SDL_CONTROLLER_BUTTON_BACK)])
        return true;
#endif
    return buttons[MenuBinding(MenuPadButton::Back)];
}

bool DirectJoystickClass::ButtonDeletePressed() const {
    return buttons[MenuBinding(MenuPadButton::Remove)];
}

bool DirectJoystickClass::ButtonStartPressed() const {
    return buttons[MenuBinding(MenuPadButton::Pause)];
}

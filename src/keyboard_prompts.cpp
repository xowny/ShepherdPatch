#include "keyboard_prompts.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <optional>
#include <sstream>
#include <vector>

namespace shh
{
namespace
{
std::string Upper(std::string_view value)
{
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
        return static_cast<char>(std::toupper(character));
    });
    return result;
}

std::string DisplayKeyboardKey(std::string_view value)
{
    std::string key = Upper(value);
    if (key.starts_with("KEY_"))
    {
        key.erase(0, 4);
    }

    static const std::unordered_map<std::string, std::string> names{
        {"SPACE", "Space"},       {"ESCAPE", "Esc"},
        {"RETURN", "Enter"},      {"NUMPADENTER", "Numpad Enter"},
        {"LSHIFT", "Left Shift"}, {"RSHIFT", "Right Shift"},
        {"LCONTROL", "Left Ctrl"}, {"RCONTROL", "Right Ctrl"},
        {"LMENU", "Left Alt"},    {"RMENU", "Right Alt"},
        {"LBRACKET", "Left Bracket"}, {"RBRACKET", "Right Bracket"},
        {"LEFT", "Left Arrow"},   {"RIGHT", "Right Arrow"},
        {"UP", "Up Arrow"},       {"DOWN", "Down Arrow"},
        {"DELETE", "Delete"},     {"BACK", "Backspace"},
        {"CAPITAL", "Caps Lock"}, {"TAB", "Tab"},
    };

    const auto match = names.find(key);
    if (match != names.end())
    {
        return match->second;
    }

    if (key.size() == 1 || key.starts_with("F") || key.starts_with("NUMPAD"))
    {
        return key;
    }

    std::replace(key.begin(), key.end(), '_', ' ');
    bool capitalize = true;
    for (char& character : key)
    {
        if (capitalize && std::isalpha(static_cast<unsigned char>(character)))
        {
            character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
            capitalize = false;
        }
        else
        {
            character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
        }
        if (character == ' ')
        {
            capitalize = true;
        }
    }
    return key;
}

std::string DisplayMouseInput(std::string_view value)
{
    const std::string input = Upper(value);
    if (input == "BUTTON_0")
        return "LMB";
    if (input == "BUTTON_1")
        return "RMB";
    if (input == "BUTTON_2")
        return "MMB";
    static const std::unordered_map<std::string, std::string_view> extraButtons{
        {"BUTTON_3", "Mouse 4"}, {"BUTTON_4", "Mouse 5"},
        {"BUTTON_5", "Mouse 6"}, {"BUTTON_6", "Mouse 7"},
        {"BUTTON_7", "Mouse 8"},
    };
    if (const auto match = extraButtons.find(input); match != extraButtons.end())
        return std::string(match->second);
    if (input.starts_with("AXIS_"))
        return "Mouse";
    return {};
}

std::string DisplayKeyboardScanCode(std::uint8_t scanCode)
{
    static const std::unordered_map<std::uint8_t, std::string_view> names{
        {0x01, "Esc"}, {0x02, "1"}, {0x03, "2"}, {0x04, "3"}, {0x05, "4"},
        {0x06, "5"}, {0x07, "6"}, {0x08, "7"}, {0x09, "8"}, {0x0A, "9"},
        {0x0B, "0"}, {0x0C, "-"}, {0x0D, "="}, {0x0E, "Backspace"},
        {0x0F, "Tab"}, {0x10, "Q"}, {0x11, "W"}, {0x12, "E"}, {0x13, "R"},
        {0x14, "T"}, {0x15, "Y"}, {0x16, "U"}, {0x17, "I"}, {0x18, "O"},
        {0x19, "P"}, {0x1A, "Left Bracket"}, {0x1B, "Right Bracket"},
        {0x1C, "Enter"}, {0x1D, "Left Ctrl"}, {0x1E, "A"}, {0x1F, "S"},
        {0x20, "D"}, {0x21, "F"}, {0x22, "G"}, {0x23, "H"}, {0x24, "J"},
        {0x25, "K"}, {0x26, "L"}, {0x27, ";"}, {0x28, "'"}, {0x29, "`"},
        {0x2A, "Left Shift"}, {0x2B, "\\"}, {0x2C, "Z"}, {0x2D, "X"},
        {0x2E, "C"}, {0x2F, "V"}, {0x30, "B"}, {0x31, "N"}, {0x32, "M"},
        {0x33, ","}, {0x34, "."}, {0x35, "/"}, {0x36, "Right Shift"},
        {0x37, "Numpad *"}, {0x38, "Left Alt"}, {0x39, "Space"},
        {0x3A, "Caps Lock"},
        {0x3B, "F1"}, {0x3C, "F2"}, {0x3D, "F3"}, {0x3E, "F4"},
        {0x3F, "F5"}, {0x40, "F6"}, {0x41, "F7"}, {0x42, "F8"},
        {0x43, "F9"}, {0x44, "F10"}, {0x45, "Num Lock"},
        {0x46, "Scroll Lock"}, {0x47, "Numpad 7"}, {0x48, "Numpad 8"},
        {0x49, "Numpad 9"}, {0x4A, "Numpad -"}, {0x4B, "Numpad 4"},
        {0x4C, "Numpad 5"}, {0x4D, "Numpad 6"}, {0x4E, "Numpad +"},
        {0x4F, "Numpad 1"}, {0x50, "Numpad 2"}, {0x51, "Numpad 3"},
        {0x52, "Numpad 0"}, {0x53, "Numpad ."}, {0x56, "OEM 102"},
        {0x57, "F11"}, {0x58, "F12"}, {0x9C, "Numpad Enter"},
        {0x9D, "Right Ctrl"}, {0xB5, "Numpad /"}, {0xB8, "Right Alt"},
        {0xC5, "Pause"},
        {0xC7, "Home"}, {0xC8, "Up Arrow"}, {0xC9, "Page Up"},
        {0xCB, "Left Arrow"}, {0xCD, "Right Arrow"}, {0xCF, "End"},
        {0xD0, "Down Arrow"}, {0xD1, "Page Down"}, {0xD2, "Insert"},
        {0xD3, "Delete"}, {0xDB, "Left Windows"}, {0xDC, "Right Windows"},
        {0xDD, "Menu"},
    };
    const auto match = names.find(scanCode);
    return match != names.end() ? std::string(match->second) : std::string{};
}

std::optional<std::string> FindBinding(
    const KeyboardPromptBindings& bindings, std::string_view command)
{
    const auto match = bindings.find(Upper(command));
    if (match == bindings.end() || match->second.empty())
    {
        return std::nullopt;
    }
    return match->second;
}

std::optional<std::string> JoinBindings(
    const KeyboardPromptBindings& bindings,
    std::initializer_list<std::string_view> commands,
    bool compactCharacters)
{
    std::vector<std::string> labels;
    for (const std::string_view command : commands)
    {
        const auto label = FindBinding(bindings, command);
        if (!label)
        {
            return std::nullopt;
        }
        labels.push_back(*label);
    }

    std::string result;
    const bool compact = compactCharacters &&
                         std::all_of(labels.begin(), labels.end(), [](const std::string& label) {
                             return label.size() == 1;
                         });
    for (std::size_t index = 0; index < labels.size(); ++index)
    {
        if (index != 0 && !compact)
        {
            result += '/';
        }
        result += labels[index];
    }
    return result;
}

std::optional<std::string_view> ResolveAliasCommand(std::string_view token)
{
    static const std::unordered_map<std::string, std::string_view> aliases{
        {"ACTION", "COMMAND_KICK"},
        {"FIRE", "COMMAND_KICK"},
        {"HEAVY_ATTACK", "COMMAND_PUNCH"},
        {"DEFENSE", "COMMAND_JUMP"},
        {"TARGETING", "COMMAND_STALK"},
        {"COMBAT_WHEEL", "COMMAND_RELOAD"},
        {"INVENTORY_WHEEL", "COMMAND_BLOCK"},
        {"UI_TOGGLE_MAP", "COMMAND_USE"},
    };
    const auto alias = aliases.find(Upper(token));
    return alias != aliases.end() ? std::optional<std::string_view>(alias->second)
                                  : std::nullopt;
}

std::optional<std::string> ResolveToken(
    const KeyboardPromptBindings& bindings, std::string_view token)
{
    const std::string upperToken = Upper(token);
    if (upperToken.starts_with("COMMAND_") &&
        upperToken != "COMMAND_MOVEMENT" &&
        upperToken != "COMMAND_CAMERA" &&
        upperToken != "COMMAND_LEFT_STICK_LEFTRIGHT" &&
        upperToken != "COMMAND_LEFT_STICK_UPDOWN")
    {
        return FindBinding(bindings, upperToken);
    }

    if (const auto alias = ResolveAliasCommand(upperToken))
    {
        return FindBinding(bindings, *alias);
    }

    if (upperToken == "COMMAND_MOVEMENT")
    {
        return JoinBindings(bindings,
                            {"COMMAND_FORWARD", "COMMAND_LEFT", "COMMAND_BACKWARD",
                             "COMMAND_RIGHT"},
                            true);
    }
    if (upperToken == "COMMAND_LEFT_STICK_LEFTRIGHT")
    {
        return JoinBindings(bindings, {"COMMAND_LEFT", "COMMAND_RIGHT"}, false);
    }
    if (upperToken == "COMMAND_LEFT_STICK_UPDOWN")
    {
        return JoinBindings(bindings, {"COMMAND_FORWARD", "COMMAND_BACKWARD"}, false);
    }
    if (upperToken == "COMMAND_CAMERA")
    {
        return std::string("Mouse");
    }
    return std::nullopt;
}
} // namespace

KeyboardPromptBindings ParseKeyboardPromptBindings(std::string_view text)
{
    KeyboardPromptBindings bindings;
    std::istringstream stream{std::string(text)};
    std::string line;
    while (std::getline(stream, line))
    {
        const std::size_t first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#')
        {
            continue;
        }

        std::istringstream fields(line.substr(first));
        std::string operation;
        int profile = -1;
        std::string command;
        std::string device;
        int deviceIndex = -1;
        std::string input;
        if (!(fields >> operation >> profile >> command >> device >> deviceIndex >> input) ||
            profile != 0)
        {
            continue;
        }

        operation = Upper(operation);
        command = Upper(command);
        device = Upper(device);
        std::string display;
        if (device == "KEYBOARD")
        {
            display = DisplayKeyboardKey(input);
        }
        else if (device == "MOUSE")
        {
            display = DisplayMouseInput(input);
        }
        if (display.empty())
        {
            continue;
        }

        if (operation == "SETBIND")
        {
            bindings[command] = std::move(display);
        }
        else if (operation == "ADDBIND" && !bindings.contains(command))
        {
            bindings.emplace(std::move(command), std::move(display));
        }
    }
    return bindings;
}

void ApplyKeyboardPromptScanCodeOverrides(
    KeyboardPromptBindings& bindings, const KeyboardScanCodeBindings& scanCodes)
{
    static const std::unordered_map<std::uint8_t, std::string_view> commandNames{
        {0, "COMMAND_FORWARD"}, {1, "COMMAND_BACKWARD"}, {2, "COMMAND_LEFT"},
        {3, "COMMAND_RIGHT"}, {4, "COMMAND_PUNCH"}, {5, "COMMAND_KICK"},
        {6, "COMMAND_JUMP"}, {7, "COMMAND_STALK"}, {8, "COMMAND_BLOCK"},
        {9, "COMMAND_USE"}, {10, "COMMAND_RELOAD"},
    };

    for (const auto& [commandId, scanCode] : scanCodes)
    {
        const auto command = commandNames.find(commandId);
        const std::string label = DisplayKeyboardScanCode(scanCode);
        if (command != commandNames.end() && !label.empty())
        {
            bindings[std::string(command->second)] = label;
        }
    }
}

std::string ReplaceKeyboardPromptTokens(
    std::string_view text, const KeyboardPromptBindings& bindings)
{
    std::string output;
    output.reserve(text.size());
    std::size_t cursor = 0;
    while (cursor < text.size())
    {
        const std::size_t opening = text.find('<', cursor);
        if (opening == std::string_view::npos)
        {
            output.append(text.substr(cursor));
            break;
        }
        output.append(text.substr(cursor, opening - cursor));
        const std::size_t closing = text.find('>', opening + 1);
        if (closing == std::string_view::npos)
        {
            output.append(text.substr(opening));
            break;
        }

        const std::string_view token = text.substr(opening + 1, closing - opening - 1);
        const auto replacement = ResolveToken(bindings, token);
        if (replacement)
        {
            output += '[';
            output += *replacement;
            output += ']';
        }
        else
        {
            output.append(text.substr(opening, closing - opening + 1));
        }
        cursor = closing + 1;
    }

    return output;
}

bool IsLocalizedStringsFileName(std::string_view fileName)
{
    const std::string name = Upper(fileName);
    return name == "STRINGS.STR" ||
           (name.starts_with("STRINGS_") && name.ends_with(".STR"));
}

bool IsGenericPcActionResource(std::string_view resourceName)
{
    const std::string name = Upper(resourceName);
    return name == "SHV_BUTTONACTIONPC" || name == "PC_DODGEFLASH";
}

const char* ResolvePcPromptResourceOverride(std::uint32_t commandId)
{
    if (commandId == 19)
        return "shv_buttonpcmouseleft_32";
    if (commandId == 20)
        return "shv_buttonpcmouseright_32";
    return nullptr;
}

std::uint32_t ResolvePcPromptCommand(std::uint32_t commandId,
                                     std::string_view resourceName)
{
    constexpr std::uint32_t kMouseLeftCommand = 5;
    if (resourceName.empty())
    {
        return commandId;
    }

    if (!IsGenericPcActionResource(resourceName))
    {
        return commandId;
    }

    return commandId == 34 || commandId == 46 ? kMouseLeftCommand : commandId;
}
} // namespace shh

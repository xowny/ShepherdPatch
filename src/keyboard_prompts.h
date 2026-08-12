#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

namespace shh
{
using KeyboardPromptBindings = std::unordered_map<std::string, std::string>;
using KeyboardScanCodeBindings = std::unordered_map<std::uint8_t, std::uint8_t>;

KeyboardPromptBindings ParseKeyboardPromptBindings(std::string_view text);
void ApplyKeyboardPromptScanCodeOverrides(
    KeyboardPromptBindings& bindings, const KeyboardScanCodeBindings& scanCodes);
std::string ReplaceKeyboardPromptTokens(
    std::string_view text, const KeyboardPromptBindings& bindings);
bool IsLocalizedStringsFileName(std::string_view fileName);
bool IsGenericPcActionResource(std::string_view resourceName);
std::uint32_t ResolvePcPromptCommand(std::uint32_t commandId,
                                     std::string_view resourceName);
} // namespace shh

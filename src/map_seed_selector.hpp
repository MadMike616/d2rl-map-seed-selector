#pragma once

#include <D2RLPlugin/context.h>
#include <D2RLPlugin/shared_events.h>
#include <D2RLPlugin/widgets.h>

#include <string>
#include <string_view>
#include <cstdint>
#include <cstddef>

namespace MapSeedSelector {

[[nodiscard]] auto Initialize(const D2RL::PluginContext* context) noexcept -> bool;
[[nodiscard]] auto IsAvailable() noexcept -> bool;
void SetWidgetService(const D2RL::WidgetService* service,
    bool nativeDropdownAvailable) noexcept;
void SetPanelGeneration(std::uint32_t generation) noexcept;
void SetOfflineFlowActive(bool active) noexcept;
void LogSeedInitializationSummary() noexcept;
[[nodiscard]] auto BuildLayoutChildren() -> std::string;
[[nodiscard]] auto BuildOptionsLayoutChildren() -> std::string;
void GetPanelPosition(std::int32_t& x, std::int32_t& y) noexcept;
void GetAddSeedButtonPosition(std::int32_t& x, std::int32_t& y) noexcept;
[[nodiscard]] auto GetPanelWidth() noexcept -> std::int32_t;
[[nodiscard]] auto GetPanelHeight() noexcept -> std::int32_t;
[[nodiscard]] auto GetOptionsPanelHeight() noexcept -> std::int32_t;
void HandleDropdownChanged() noexcept;
void SelectChoice(std::size_t choice) noexcept;
[[nodiscard]] auto AddSeedEntry(std::string_view name,
    std::string_view seedText, std::string& message) noexcept -> bool;
void RefreshUi() noexcept;
void Shutdown() noexcept;

} // namespace MapSeedSelector

extern "C" {
extern volatile long gDrlgSeedInput;
extern volatile long gDrlgSeedInputState;
extern volatile long gDrlgSeedWritten;
extern volatile long gDrlgSeedObservedCalls;
extern volatile long gDrlgSeedObservedStored;
extern volatile long gDrlgSeedObservedLcg;
extern volatile long gGameSeedIngressCalls;
extern volatile long gGameSeedIngressInput;
extern volatile long gGameSeedIngressWritten;
void MapDrlgSeedInitializationHook() noexcept;
void MapDrlgSeedPostWriteHook() noexcept;
void MapGameSeedIngressHook() noexcept;
void MapSavedSeedCopyAHook() noexcept;
void MapSavedSeedCopyBHook() noexcept;
void MapSavedSeedCopyCHook() noexcept;
}

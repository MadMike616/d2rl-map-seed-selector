#include <D2RLPlugin/api.h>
#include <D2RLPlugin/lifecycle_events.h>
#include <D2RLPlugin/logging.h>
#include <D2RLPlugin/panels.h>
#include <D2RLPlugin/resources.h>
#include <D2RLPlugin/shared_events.h>
#include <D2RLPlugin/threads.h>
#include <D2RLPlugin/widgets.h>
#include <windows.h>

#include "map_seed_selector.hpp"
#include "native_dropdown_art.hpp"
#include "add_seed_button_sprite.hpp"

#include <algorithm>
#include <atomic>
#include <array>
#include <charconv>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <vector>
#include <string>
#include <string_view>

namespace {

constexpr std::size_t SelectorPanelSlotCount = 17;
constexpr std::size_t KeptClosedSelectorPanels = 8;
// The loader limits each plugin to 64 panels. The selector and auxiliary UI
// use 36, leaving 28 rotating Add Seed views while text changes.
constexpr std::size_t AddSeedDialogSlotCount = 28;
constexpr auto PanelUnloadCooldown = std::chrono::milliseconds(1500);
struct SelectorPanelCacheEntry {
    bool loaded{};
    std::uint64_t lastUsed{};
    std::chrono::steady_clock::time_point closedAt{};
};
std::array<SelectorPanelCacheEntry, SelectorPanelSlotCount>
    SelectorPanelCache{};
std::array<SelectorPanelCacheEntry, SelectorPanelSlotCount>
    OptionsPanelCache{};
std::uint64_t SelectorPanelUseSequence{};
std::string PanelLocalId{"MapSeeds0"};
std::string QualifiedPanelName{"map-seed-selector/MapSeeds0"};
std::string OptionsPanelLocalId{"SeedOptions0"};
std::string QualifiedOptionsPanelName{"map-seed-selector/SeedOptions0"};
constexpr char AddSeedButtonPanelLocalId[] = "AddSeedButton";
constexpr char QualifiedAddSeedButtonPanelName[] =
    "map-seed-selector/AddSeedButton";
constexpr char DropdownToggleMessage[] = "map-seed-selector/ToggleDropdown";
constexpr char OpenAddSeedMessage[] = "map-seed-selector/OpenAddSeed";
constexpr char CommitAddSeedMessage[] = "map-seed-selector/CommitAddSeed";
constexpr char CancelAddSeedMessage[] = "map-seed-selector/CancelAddSeed";
constexpr char ShowAddSeedHelpMessage[] = "map-seed-selector/ShowAddSeedHelp";
constexpr char CopyCurrentSeedMessage[] = "map-seed-selector/CopyCurrentSeed";
constexpr char CloseCurrentSeedMessage[] = "map-seed-selector/CloseCurrentSeed";
constexpr char FocusSeedNameMessage[] = "map-seed-selector/FocusSeedName";
constexpr char FocusSeedValueMessage[] = "map-seed-selector/FocusSeedValue";
constexpr char ChoiceMessagePrefix[] = "map-seed-selector/Choose_";
std::string LayoutResourcePath{
    "data/global/ui/layouts/map-seed-selector/MapSeeds0hd.json"};
std::string OptionsLayoutResourcePath{
    "data/global/ui/layouts/map-seed-selector/SeedOptions0hd.json"};
constexpr char AddSeedButtonLayoutResourcePath[] =
    "data/global/ui/layouts/map-seed-selector/AddSeedButtonhd.json";
constexpr char CurrentSeedPanelLocalId[] = "CurrentSeed";
constexpr char QualifiedCurrentSeedPanelName[] =
    "map-seed-selector/CurrentSeed";
constexpr char CurrentSeedPanelLayoutResourcePath[] =
    "data/global/ui/layouts/map-seed-selector/CurrentSeedhd.json";
constexpr char AddSeedButtonSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/addseedbutton.sprite";
constexpr char AddSeedButtonLowEndSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/addseedbutton.lowend.sprite";
constexpr char QuestionButtonSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/questionbutton.sprite";
constexpr char QuestionButtonLowEndSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/questionbutton.lowend.sprite";
constexpr char ModalButtonSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/modalbutton.sprite";
constexpr char ModalButtonLowEndSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/modalbutton.lowend.sprite";
constexpr char CurrentSeedPanelSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/currentseedpanel.sprite";
constexpr char CurrentSeedPanelLowEndSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/currentseedpanel.lowend.sprite";
constexpr char CurrentSeedPanelButtonSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/currentseedpanelbutton.sprite";
constexpr char CurrentSeedPanelButtonLowEndSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/currentseedpanelbutton.lowend.sprite";
constexpr char BlankSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/blank.sprite";
constexpr char BlankSpriteLowEndResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/blank.lowend.sprite";
constexpr char DropdownSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/dropdown.sprite";
constexpr char DropdownSpriteLowEndResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/dropdown.lowend.sprite";
constexpr char OptionTileSpriteResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/optiontile.sprite";
constexpr char OptionTileSpriteLowEndResourcePath[] =
    "data/hd/global/ui/d2rloader/map-seed-selector/optiontile.lowend.sprite";
constexpr std::int32_t PopupCanvasWidth = 8192;
constexpr std::int32_t PopupCanvasHeight = 4320;
constexpr char LayoutTemplate[] = R"json({
  "type": "Panel",
  "name": "@panel_name@",
  "fields": {
    "priority": 9005,
    "anchor": { "x": 0.0, "y": 0.0 },
    "rect": { "x": @panel_x@, "y": @panel_y@, "width": @panel_width@, "height": @panel_height@ }
  },
  "children": [@seed_children@]
})json";
constexpr char AddSeedButtonLayoutTemplate[] = R"json({
  "type": "Panel",
  "name": "map-seed-selector/AddSeedButton",
  "fields": {
    "priority": 9004,
    "anchor": { "x": 0.0, "y": 0.0 },
    "rect": { "x": 0, "y": 0, "width": 8192, "height": 4320 }
  },
  "children": [
    {
      "type": "ButtonWidget",
      "name": "OpenAddSeedPrompt",
      "fields": {
        "rect": { "x": @button_x@, "y": @button_y@, "width": 56, "height": 56 },
        "filename": "d2rloader\\map-seed-selector\\addseedbutton",
        "normalFrame": 0,
        "pressedFrame": 0,
        "disabledFrame": 0,
        "hoveredFrame": 1,
        "onClickMessage": "PanelManager:ClosePanel:map-seed-selector/OpenAddSeed"
      }
    }
  ]
})json";
constexpr char CurrentSeedPanelLayoutTemplate[] = R"json({
  "type": "Panel",
  "name": "map-seed-selector/CurrentSeed",
    "fields": { "priority": 9010, "anchor": { "x": 0.5, "y": 0.5 } },
    "children": [
    { "type": "ImageWidget", "name": "Frame",
      "fields": { "rect": { "x": 627, "y": -270, "width": 473, "height": 227 }, "filename": "d2rloader\\map-seed-selector\\currentseedpanel" } },
    { "type": "TextBoxWidget", "name": "CurrentSeedTitle", "fields": {
      "rect": { "x": 645, "y": -260, "width": 440, "height": 42 },
      "text": "Map Seed", "style": { "fontColor": "$FontColorGoldYellow", "pointSize": "$MediumFontSize", "dropShadow": "$DefaultDropShadow" } } },
    { "type": "TextBoxWidget", "name": "CurrentSeedValue", "fields": {
      "rect": { "x": 645, "y": -206, "width": 440, "height": 42 },
      "text": "@seed_text@", "style": { "fontColor": "$FontColorWhite", "pointSize": "$LargeFontSize", "dropShadow": "$DefaultDropShadow", "alignment": { "h": "center", "v": "center" } } } },
    { "type": "TextBoxWidget", "name": "CurrentSeedStatus", "fields": {
      "rect": { "x": 700, "y": -155, "width": 331, "height": 24 },
      "text": "@seed_status@", "style": { "fontColor": "$FontColorLightGold", "pointSize": "$SmallFontSize", "dropShadow": "$DefaultDropShadow", "alignment": { "h": "center", "v": "center" } } } },
    { "type": "ButtonWidget", "name": "CopyCurrentSeed", "fields": {
      "rect": { "x": 714, "y": -110, "width": 133, "height": 49 },
      "filename": "d2rloader\\map-seed-selector\\currentseedpanelbutton", "focusIndicatorFilename": "Controller/HoverImages/ModalButton_Hover",
      "pressedFrame": 1, "disabledFrame": 2, "hoveredFrame": 3, "textString": "Copy", "pointSize": 24, "textColor": "$FontColorWhite",
      "onClickMessage": "PanelManager:ClosePanel:map-seed-selector/CopyCurrentSeed" } },
    { "type": "ButtonWidget", "name": "CloseCurrentSeed", "fields": {
      "rect": { "x": 884, "y": -110, "width": 133, "height": 49 },
      "filename": "d2rloader\\map-seed-selector\\currentseedpanelbutton", "focusIndicatorFilename": "Controller/HoverImages/ModalButton_Hover",
      "pressedFrame": 1, "disabledFrame": 2, "hoveredFrame": 3, "textString": "Close", "pointSize": 24, "textColor": "$FontColorWhite",
      "onClickMessage": "PanelManager:ClosePanel:map-seed-selector/CloseCurrentSeed" } }
  ]
})json";
auto BuildCurrentSeedPanelLayout(bool hasSeed, std::uint32_t seed)
        -> std::string {
    std::string layout = CurrentSeedPanelLayoutTemplate;
    const auto replace = [&layout](std::string_view token,
            std::string_view value) {
        std::size_t position{};
        while ((position = layout.find(token, position))
                != std::string::npos) {
            layout.replace(position, token.size(), value);
            position += value.size();
        }
    };
    const std::string seedText = hasSeed
        ? std::to_string(seed) : "Not captured";
    const std::string status = hasSeed
        ? ""
        : "Enter game";
    replace("@seed_text@", seedText);
    replace("@seed_status@", status);
    return layout;
}
constexpr char DefaultConfig[] = R"toml(# Map Seed Selector
# Add [[seeds]] blocks below for the offline map seeds you want to choose.

config_version = 1
selected_seed = "character" # Stored compatibility value for the visible "Off" option.

[ui]
# Screen coordinates of the Add Seed button's upper-left corner, in UI pixels.
# The selector and "Map Seed" text move with this anchor, 56 pixels to its right.
# The screen's upper-left corner is (0, 0); X increases to the right, Y down.
position_x = 800
position_y = 0
# Place the "Map Seed" label above, below, left, or right of the selector.
text_position = "right"
# Expand the seed list above or below the selector. Keep the anchor low enough when using "above".
expand_direction = "below"

# Seed values are unsigned 32-bit integers in decimal or hexadecimal.
# Names are optional. Blank or non-ASCII names display the seed instead.
)toml";

constexpr D2RL::PluginInfo Info{
    .infoSize = D2RL::PluginInfoSize,
    .abiVersion = D2RL_PLUGIN_ABI_VERSION,
    .id = "map-seed-selector",
    .name = "Map Seed Selector",
    .version = "0.1.52",
    .author = "MadMike",
    .description = "Selects configured offline map seeds on the character screen.",
    .flags = D2RL::PluginFlags::Client | D2RL::PluginFlags::NativeHooks,
};

const D2RL::PluginContext* Context{};
const D2RL::ThreadService* ThreadService{};
const D2RL::ResourceService* ResourceService{};
const D2RL::PanelService* PanelService{};
const D2RL::SharedEventService* SharedEventService{};
const D2RL::WidgetService* WidgetService{};
const D2RL::LifecycleService* LifecycleService{};

D2RL::Resources::RegistrationHandle LayoutHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle OptionsLayoutHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle AddSeedButtonLayoutHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle AddSeedButtonSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle AddSeedButtonLowEndSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle QuestionButtonSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle QuestionButtonLowEndSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle ModalButtonSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle ModalButtonLowEndSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle CurrentSeedPanelSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle CurrentSeedPanelLowEndSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle CurrentSeedPanelButtonSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle CurrentSeedPanelButtonLowEndSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle BlankSpriteHandle{D2RL::Resources::InvalidHandle};
D2RL::Resources::RegistrationHandle BlankSpriteLowEndHandle{D2RL::Resources::InvalidHandle};
std::array<D2RL::Resources::RegistrationHandle, 4> CompositeSpriteHandles{
    D2RL::Resources::InvalidHandle, D2RL::Resources::InvalidHandle,
    D2RL::Resources::InvalidHandle, D2RL::Resources::InvalidHandle,
};
D2RL::Panels::RegistrationHandle PanelHandle{D2RL::Panels::InvalidHandle};
D2RL::Panels::RegistrationHandle OptionsPanelHandle{D2RL::Panels::InvalidHandle};
D2RL::Panels::RegistrationHandle AddSeedButtonPanelHandle{D2RL::Panels::InvalidHandle};
D2RL::Resources::RegistrationHandle CurrentSeedPanelLayoutHandle{D2RL::Resources::InvalidHandle};
D2RL::Panels::RegistrationHandle CurrentSeedPanelHandle{D2RL::Panels::InvalidHandle};
std::string CurrentSeedPanelLayoutJson{};
std::atomic_bool CurrentSeedPanelLayoutLoaded{};
std::array<D2RL::Resources::RegistrationHandle, SelectorPanelSlotCount>
    SelectorLayoutHandles{};
std::array<D2RL::Resources::RegistrationHandle, SelectorPanelSlotCount>
    SelectorOptionsLayoutHandles{};
std::array<D2RL::Panels::RegistrationHandle, SelectorPanelSlotCount>
    SelectorPanelHandles{};
std::array<D2RL::Panels::RegistrationHandle, SelectorPanelSlotCount>
    SelectorOptionsPanelHandles{};
struct AddSeedDialogPanelSlot {
    D2RL::Resources::RegistrationHandle layout{
        D2RL::Resources::InvalidHandle};
    D2RL::Panels::RegistrationHandle panel{D2RL::Panels::InvalidHandle};
    std::string localId{};
    std::string qualifiedName{};
    std::string resourcePath{};
    std::string layoutJson{};
    bool loaded{};
    std::uint64_t lastUsed{};
    std::chrono::steady_clock::time_point closedAt{};
};
std::array<AddSeedDialogPanelSlot, AddSeedDialogSlotCount>
    AddSeedDialogSlots{};
std::size_t AddSeedDialogCursor{AddSeedDialogSlotCount - 1};
std::uint64_t AddSeedDialogUseSequence{};
std::size_t ActiveAddSeedDialogSlot{AddSeedDialogSlotCount};
std::size_t ActivePanelSlot{};
D2RL::SharedEvents::ListenerHandle UiMessageHandle{D2RL::SharedEvents::InvalidHandle};
D2RL::Lifecycle::ListenerHandle GameJoinedHandle{D2RL::Lifecycle::InvalidHandle};
D2RL::Lifecycle::ListenerHandle GameLeftHandle{D2RL::Lifecycle::InvalidHandle};
D2RL::Lifecycle::ListenerHandle DataTablesLoadedHandle{D2RL::Lifecycle::InvalidHandle};

std::string LayoutJson{};
std::string OptionsLayoutJson{};
std::string AddSeedButtonLayoutJson{};
std::string AddSeedStatus{};
std::string AddSeedNameText{};
std::string AddSeedValueText{};
std::mutex AddSeedInputMutex{};
std::wstring AddSeedNameWide{};
std::wstring AddSeedValueWide{};
std::atomic_int PendingAddSeedAction{-1};
std::atomic_bool AddSeedActionPending{};
std::atomic_bool AddSeedDialogOpen{};
std::atomic_int AddSeedFocusedField{};
std::atomic_bool AddSeedTypingEnabled{};
std::atomic_bool CurrentSeedPanelOpen{};
std::atomic_bool AddSeedTextChanged{};
std::atomic_bool AddSeedTextRefreshPending{};
std::array<std::uint8_t, 44> BlankSpriteBytes{};
MapSeedSelector::NativeDropdownArt::SpriteBundle CompositeSprites{};
bool NativeDropdownAvailable{};
std::atomic_bool Operational{};
std::atomic_bool DesiredPanelOpen{};
std::atomic_bool OnlineFlowActive{};
std::atomic_bool PanelSyncPending{};
std::atomic_bool DropdownSyncPending{};
std::atomic_bool PickerActionPending{};
std::atomic_int PendingPickerAction{-1};
std::atomic_bool UiRefreshPending{};
std::atomic_bool UiRegistrationPending{};
std::atomic_bool FallbackArtworkReady{};

void SchedulePanelSync() noexcept;
void ScheduleDropdownChanged() noexcept;
void SchedulePickerAction(int action) noexcept;
void ScheduleAddSeedAction(int action) noexcept;
void ScheduleAddSeedTextRefresh() noexcept;
void CloseCurrentSeedPanel(const D2RL::PluginContext* context) noexcept;
void StopAddSeedKeyboard() noexcept;
auto StartAddSeedKeyboard() noexcept -> bool;
auto RefreshAddSeedDialog(const D2RL::PluginContext* context) noexcept
    -> bool;
void MarkSelectorPanelOpened(std::size_t slot, bool options) noexcept;
void MarkSelectorPanelClosed(std::size_t slot, bool options) noexcept;
void TrimClosedSelectorPanels(const D2RL::PluginContext* context) noexcept;
void SetActivePanelSlot(std::size_t slot) noexcept;
auto RefreshSelectorLayouts(const D2RL::PluginContext* context) noexcept
    -> bool;
void __cdecl OnDataTablesLoaded(const D2RL::PluginContext* context,
    const D2RL::Lifecycle::DataTablesLoadedEvent* event, void*) noexcept;

auto RegisterCompositeSprites(const D2RL::PluginContext* context) noexcept
        -> bool {
    const std::array<const char*, 4> paths{
        DropdownSpriteResourcePath,
        DropdownSpriteLowEndResourcePath,
        OptionTileSpriteResourcePath,
        OptionTileSpriteLowEndResourcePath,
    };
    const std::array<const std::vector<std::uint8_t>*, 4> bytes{
        &CompositeSprites.dropdown,
        &CompositeSprites.dropdownLowEnd,
        &CompositeSprites.optionTile,
        &CompositeSprites.optionTileLowEnd,
    };
    for (std::size_t index = 0; index < paths.size(); ++index) {
        if (bytes[index]->empty()) {
            D2RL::LogWarnF(context,
                "MapSeedSelector: generated sprite %s is empty.", paths[index]);
            return false;
        }
        const D2RL::Resources::ResourceRegistration resource{
            .structSize = D2RL::Resources::ResourceRegistrationSize,
            .flags = 0,
            .path = paths[index],
            .bytes = bytes[index]->data(),
            .byteCount = bytes[index]->size(),
        };
        const auto result = ResourceService->registerResource(context,
            &resource, &CompositeSpriteHandles[index]);
        if (result != D2RL::Resources::Result::Success
                || CompositeSpriteHandles[index]
                    == D2RL::Resources::InvalidHandle) {
            D2RL::LogWarnF(context,
                "MapSeedSelector: registering generated sprite %s failed (result=%u, handle=%llu).",
                paths[index], static_cast<unsigned>(result),
                static_cast<unsigned long long>(CompositeSpriteHandles[index]));
            for (std::size_t registered = 0; registered < index; ++registered) {
                if (CompositeSpriteHandles[registered]
                        != D2RL::Resources::InvalidHandle) {
                    (void)ResourceService->unregisterResource(context,
                        CompositeSpriteHandles[registered]);
                    CompositeSpriteHandles[registered]
                        = D2RL::Resources::InvalidHandle;
                }
            }
            return false;
        }
    }
    return true;
}

auto RegisterBlankSprite(const D2RL::PluginContext* context) noexcept -> bool {
    BlankSpriteBytes.fill(0);
    BlankSpriteBytes[0] = 'S';
    BlankSpriteBytes[1] = 'p';
    BlankSpriteBytes[2] = 'A';
    BlankSpriteBytes[3] = '1';
    const auto put = [](std::array<std::uint8_t, 44>& bytes,
            std::size_t offset, std::uint32_t value, std::size_t count) {
        for (std::size_t index = 0; index < count; ++index) {
            bytes[offset + index] = static_cast<std::uint8_t>(
                (value >> (8 * index)) & 0xffU);
        }
    };
    put(BlankSpriteBytes, 4, 31, 2);
    put(BlankSpriteBytes, 6, 1, 2);
    put(BlankSpriteBytes, 8, 1, 4);
    put(BlankSpriteBytes, 12, 1, 4);
    put(BlankSpriteBytes, 20, 1, 4);

    const D2RL::Resources::ResourceRegistration standard{
        .structSize = D2RL::Resources::ResourceRegistrationSize,
        .flags = 0,
        .path = BlankSpriteResourcePath,
        .bytes = BlankSpriteBytes.data(),
        .byteCount = BlankSpriteBytes.size(),
    };
    const auto standardResult = ResourceService->registerResource(context,
        &standard, &BlankSpriteHandle);
    if (standardResult != D2RL::Resources::Result::Success
            || BlankSpriteHandle == D2RL::Resources::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: blank popup sprite registration failed (result=%u, handle=%llu).",
            static_cast<unsigned>(standardResult),
            static_cast<unsigned long long>(BlankSpriteHandle));
        return false;
    }

    const D2RL::Resources::ResourceRegistration lowEnd{
        .structSize = D2RL::Resources::ResourceRegistrationSize,
        .flags = 0,
        .path = BlankSpriteLowEndResourcePath,
        .bytes = BlankSpriteBytes.data(),
        .byteCount = BlankSpriteBytes.size(),
    };
    const auto lowEndResult = ResourceService->registerResource(context,
        &lowEnd, &BlankSpriteLowEndHandle);
    if (lowEndResult != D2RL::Resources::Result::Success
            || BlankSpriteLowEndHandle == D2RL::Resources::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: low-end blank popup sprite registration failed (result=%u, handle=%llu).",
            static_cast<unsigned>(lowEndResult),
            static_cast<unsigned long long>(BlankSpriteLowEndHandle));
        (void)ResourceService->unregisterResource(context, BlankSpriteHandle);
        BlankSpriteHandle = D2RL::Resources::InvalidHandle;
        return false;
    }
    return true;
}

void __cdecl ApplyDeferredUiRefresh(const D2RL::PluginContext* context,
        void*) noexcept {
    UiRefreshPending.store(false, std::memory_order_release);
    if (context == nullptr || context != Context
            || !Operational.load(std::memory_order_acquire)) {
        return;
    }
    MapSeedSelector::RefreshUi();
}

void ScheduleUiRefresh() noexcept {
    if (Context == nullptr || ThreadService == nullptr
            || ThreadService->runOnUiThread == nullptr
            || !Operational.load(std::memory_order_acquire)) {
        return;
    }
    bool expected = false;
    if (!UiRefreshPending.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel)) {
        return;
    }
    const auto result = ThreadService->runOnUiThread(
        Context, ApplyDeferredUiRefresh, nullptr);
    if (result != D2RL::Threads::Result::Success) {
        UiRefreshPending.store(false, std::memory_order_release);
        D2RL::LogWarnF(Context,
            "MapSeedSelector: deferred UI synchronization could not be scheduled (result=%u).",
            static_cast<unsigned>(result));
    }
}

void MarkSelectorPanelOpened(std::size_t slot, bool options) noexcept {
    if (slot >= SelectorPanelSlotCount) return;
    auto& cache = options ? OptionsPanelCache[slot]
        : SelectorPanelCache[slot];
    cache.loaded = true;
    cache.lastUsed = ++SelectorPanelUseSequence;
    cache.closedAt = {};
}

void MarkSelectorPanelClosed(std::size_t slot, bool options) noexcept {
    if (slot >= SelectorPanelSlotCount) return;
    auto& cache = options ? OptionsPanelCache[slot]
        : SelectorPanelCache[slot];
    if (cache.loaded
            && cache.closedAt == std::chrono::steady_clock::time_point{}) {
        cache.closedAt = std::chrono::steady_clock::now();
    }
}

// D2R retains a panel's built widget tree after it closes. Keep the newest
// eight closed selector/popup trees ready and let older ones go, following
// Grail On's KeptLoaded policy. A short grace period is required because the
// game's layout loader can crash if a just-closed panel is unloaded.
void TrimClosedSelectorPanels(
        const D2RL::PluginContext* context) noexcept {
    if (context == nullptr || context != Context || PanelService == nullptr
            || WidgetService == nullptr
            || WidgetService->dispatchUiAction == nullptr) {
        return;
    }
    struct Candidate {
        std::size_t slot{};
        bool options{};
        std::uint64_t lastUsed{};
    };
    std::array<Candidate, SelectorPanelSlotCount * 2> candidates{};
    std::size_t candidateCount{};
    std::size_t closedCount{};
    const auto now = std::chrono::steady_clock::now();

    const auto inspect = [&](std::size_t slot, bool options,
            D2RL::Panels::RegistrationHandle handle,
            SelectorPanelCacheEntry& cache) {
        if (!cache.loaded || handle == D2RL::Panels::InvalidHandle) return;
        D2RL::Panels::PanelInfo info{
            .structSize = D2RL::Panels::PanelInfoSize,
        };
        if (PanelService->getPanelInfo(context, handle, &info)
                != D2RL::Panels::Result::Success
                || info.presentationState
                    != D2RL::Panels::PresentationState::Closed) {
            return;
        }
        if (cache.closedAt == std::chrono::steady_clock::time_point{}) {
            cache.closedAt = now;
        }
        ++closedCount;
        if (now - cache.closedAt >= PanelUnloadCooldown) {
            candidates[candidateCount++] = {
                .slot = slot,
                .options = options,
                .lastUsed = cache.lastUsed,
            };
        }
    };

    for (std::size_t slot = 0; slot < SelectorPanelSlotCount; ++slot) {
        inspect(slot, false, SelectorPanelHandles[slot],
            SelectorPanelCache[slot]);
        inspect(slot, true, SelectorOptionsPanelHandles[slot],
            OptionsPanelCache[slot]);
    }
    if (closedCount <= KeptClosedSelectorPanels) return;

    std::sort(candidates.begin(), candidates.begin() + candidateCount,
        [](const Candidate& lhs, const Candidate& rhs) {
            return lhs.lastUsed < rhs.lastUsed;
        });
    for (std::size_t index = 0; index < candidateCount
            && closedCount > KeptClosedSelectorPanels; ++index) {
        const auto& candidate = candidates[index];
        auto& cache = candidate.options
            ? OptionsPanelCache[candidate.slot]
            : SelectorPanelCache[candidate.slot];
        std::string panelName = "map-seed-selector/";
        panelName += candidate.options ? "SeedOptions" : "MapSeeds";
        panelName += std::to_string(candidate.slot);
        const D2RL::Widgets::UiAction unload{
            .structSize = D2RL::Widgets::UiActionSize,
            .flags = 0,
            .target = "PanelManager",
            .command = "UnloadPanel",
            .text = panelName.c_str(),
        };
        if (WidgetService->dispatchUiAction(context, &unload)
                == D2RL::Widgets::Result::Success) {
            cache.loaded = false;
            cache.lastUsed = 0;
            cache.closedAt = {};
            --closedCount;
        } else {
            D2RL::LogWarnF(context,
                "MapSeedSelector: could not unload closed panel %s.",
                panelName.c_str());
        }
    }
}

void __cdecl ApplyDropdownChanged(const D2RL::PluginContext* context,
        void*) noexcept {
    if (context == nullptr || context != Context
            || !Operational.load(std::memory_order_acquire)) {
        DropdownSyncPending.store(false, std::memory_order_release);
        return;
    }
    DropdownSyncPending.store(false, std::memory_order_release);
    MapSeedSelector::HandleDropdownChanged();
}

void ScheduleDropdownChanged() noexcept {
    if (Context == nullptr || ThreadService == nullptr
            || ThreadService->runOnUiThread == nullptr
            || !Operational.load(std::memory_order_acquire)) {
        return;
    }
    bool expected = false;
    if (!DropdownSyncPending.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel)) {
        return;
    }
    const auto result = ThreadService->runOnUiThread(
        Context, ApplyDropdownChanged, nullptr);
    if (result != D2RL::Threads::Result::Success) {
        DropdownSyncPending.store(false, std::memory_order_release);
        D2RL::LogWarnF(Context,
            "MapSeedSelector: UI-thread scheduling for the native dropdown change failed (result=%u).",
            static_cast<unsigned>(result));
    }
}

void __cdecl ApplyPickerAction(const D2RL::PluginContext* context,
        void*) noexcept {
    const int action = PendingPickerAction.exchange(-1,
        std::memory_order_acq_rel);
    if (context == nullptr || context != Context
            || !Operational.load(std::memory_order_acquire)) {
        PickerActionPending.store(false, std::memory_order_release);
        return;
    }

    if (action == 0) {
        if (OptionsPanelHandle == D2RL::Panels::InvalidHandle) {
            context->LogWarn("MapSeedSelector: the seed popup panel is unavailable.");
        } else {
            D2RL::Panels::PanelInfo info{
                .structSize = D2RL::Panels::PanelInfoSize,
            };
            const auto infoResult = PanelService->getPanelInfo(context,
                OptionsPanelHandle, &info);
            if (infoResult != D2RL::Panels::Result::Success) {
                D2RL::LogWarnF(context,
                    "MapSeedSelector: popup state could not be read (result=%u).",
                    static_cast<unsigned>(infoResult));
            } else if (info.presentationState
                    == D2RL::Panels::PresentationState::Open) {
                if (PanelService->closePanel(context, OptionsPanelHandle)
                        == D2RL::Panels::Result::Success) {
                    MarkSelectorPanelClosed(ActivePanelSlot, true);
                }
            } else {
                const auto openResult = PanelService->openPanel(context,
                    OptionsPanelHandle);
                if (openResult != D2RL::Panels::Result::Success) {
                    D2RL::LogWarnF(context,
                        "MapSeedSelector: popup could not be opened (result=%u).",
                        static_cast<unsigned>(openResult));
                } else {
                    MarkSelectorPanelOpened(ActivePanelSlot, true);
                }
            }
        }
    } else if (action > 0) {
        MapSeedSelector::SelectChoice(static_cast<std::size_t>(action - 1));
        if (OptionsPanelHandle != D2RL::Panels::InvalidHandle) {
            if (PanelService->closePanel(context, OptionsPanelHandle)
                    == D2RL::Panels::Result::Success) {
                MarkSelectorPanelClosed(ActivePanelSlot, true);
            }
        }
    }

    TrimClosedSelectorPanels(context);
    PickerActionPending.store(false, std::memory_order_release);
    if (PendingPickerAction.load(std::memory_order_acquire) >= 0) {
        SchedulePickerAction(PendingPickerAction.load(std::memory_order_acquire));
    }
}

void SchedulePickerAction(int action) noexcept {
    if (Context == nullptr || ThreadService == nullptr
            || ThreadService->runOnUiThread == nullptr
            || !Operational.load(std::memory_order_acquire)) {
        return;
    }
    PendingPickerAction.store(action, std::memory_order_release);
    bool expected = false;
    if (!PickerActionPending.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel)) {
        return;
    }
    const auto result = ThreadService->runOnUiThread(Context,
        ApplyPickerAction, nullptr);
    if (result != D2RL::Threads::Result::Success) {
        PendingPickerAction.store(-1, std::memory_order_release);
        PickerActionPending.store(false, std::memory_order_release);
        D2RL::LogWarnF(Context,
            "MapSeedSelector: popup input could not be scheduled (result=%u).",
            static_cast<unsigned>(result));
    }
}



constexpr UINT AddSeedLoadFieldMessage = WM_APP + 74;
constexpr UINT AddSeedShutdownMessage = WM_APP + 75;

HWND AddSeedEdit{};
HHOOK AddSeedKeyboardHook{};
HANDLE AddSeedKeyboardThread{};
std::atomic<DWORD> AddSeedKeyboardThreadId{};
std::atomic_bool AddSeedKeyboardReady{};
std::atomic_bool AddSeedKeyboardFailed{};
std::array<bool, 256> AddSeedForwardedKeys{};
std::array<bool, 256> AddSeedConsumedKeys{};

auto JsonEscape(std::string_view value) -> std::string {
    static constexpr char hex[] = "0123456789ABCDEF";
    std::string escaped;
    escaped.reserve(value.size() + 8);
    for (const unsigned char c : value) {
        switch (c) {
        case '"': escaped += "\\\""; break;
        case '\\': escaped += "\\\\"; break;
        case '\b': escaped += "\\b"; break;
        case '\f': escaped += "\\f"; break;
        case '\n': escaped += "\\n"; break;
        case '\r': escaped += "\\r"; break;
        case '\t': escaped += "\\t"; break;
        default:
            if (c < 0x20) {
                escaped += "\\u00";
                escaped += hex[c >> 4];
                escaped += hex[c & 0x0f];
            } else {
                escaped += static_cast<char>(c);
            }
            break;
        }
    }
    return escaped;
}

auto WideToUtf8(std::wstring_view value) -> std::string {
    if (value.empty()) return {};
    const int bytes = WideCharToMultiByte(CP_UTF8, 0, value.data(),
        static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (bytes <= 0) return {};
    std::string result(static_cast<std::size_t>(bytes), '\0');
    if (WideCharToMultiByte(CP_UTF8, 0, value.data(),
            static_cast<int>(value.size()), result.data(), bytes,
            nullptr, nullptr) != bytes) {
        return {};
    }
    return result;
}

auto DialogLocalId(std::size_t slot) -> std::string {
    return "AddSeedDialog" + std::to_string(slot);
}

auto BuildAddSeedDialogLayout(std::size_t slot, std::string_view name,
        std::string_view seed, std::string_view status, bool nameFocused)
        -> std::string {
    const std::string localId = DialogLocalId(slot);
    const std::string qualifiedName = "map-seed-selector/" + localId;
    std::string nameValue(name);
    std::string seedValue(seed);
    const bool nameEmpty = nameValue.empty();
    const bool seedEmpty = seedValue.empty();
    if (nameFocused) nameValue += '|';
    else seedValue += '|';
    if (nameEmpty && !nameFocused) nameValue = "Enter a name";
    if (seedEmpty && nameFocused) seedValue = "Enter a seed";

    std::string layout = R"json({
  "type": "Panel",
  "name": "@panel_name@",
  "fields": { "priority": 9008, "anchor": { "x": 0.5, "y": 0.5 } },
  "children": [{
    "type": "ImageWidget", "name": "Frame",
    "fields": { "rect": { "x": -627, "y": -270 }, "filename": "Panel\\Modals\\Modal_No_Title_BG" },
    "children": [
      { "type": "TextBoxWidget", "name": "Title", "fields": {
        "rect": { "x": 78, "y": 52, "width": 1076, "height": 68 },
        "text": "Add Map Seed", "style": "$StyleSettingsTitle" } },
      { "type": "ButtonWidget", "name": "AddSeedHelp", "fields": {
        "rect": { "x": 1102, "y": 58, "width": 56, "height": 56 },
        "filename": "d2rloader\\map-seed-selector\\questionbutton",
        "normalFrame": 0, "pressedFrame": 1, "disabledFrame": 2, "hoveredFrame": 3,
        "sound": "cursor_button_click_hd",
        "tooltipString": "Show the last map seed used and copy it.",
        "tooltipStyle": "$StyHlpTt",
        "onClickMessage": "PanelManager:ClosePanel:map-seed-selector/ShowAddSeedHelp" } },
      { "type": "TextBoxWidget", "name": "NameLabel", "fields": {
        "rect": { "x": 277, "y": 135, "width": 700, "height": 30 },
        "text": "Name", "style": { "fontColor": "$FontColorGoldYellow", "pointSize": "$MediumFontSize", "dropShadow": "$DefaultDropShadow", "alignment": { "h": "center", "v": "center" } } } },
      { "type": "TextBoxWidget", "name": "SeedNameDisplay", "fields": {
        "rect": { "x": 277, "y": 170, "width": 700, "height": 52 },
        "text": "@name_text@", "style": { "fontColor": "@name_color@", "pointSize": "$SmallFontSize", "dropShadow": "$DefaultDropShadow", "alignment": { "h": "center", "v": "center" } } } },
      { "type": "ButtonWidget", "name": "SeedNameHitTarget", "fields": {
        "rect": { "x": 277, "y": 170, "width": 700, "height": 52 },
        "filename": "d2rloader\\map-seed-selector\\blank", "normalFrame": 0, "pressedFrame": 0, "disabledFrame": 0, "hoveredFrame": 0,
        "onClickMessage": "PanelManager:ClosePanel:map-seed-selector/FocusSeedName" } },
      { "type": "TextBoxWidget", "name": "SeedValueLabel", "fields": {
        "rect": { "x": 277, "y": 236, "width": 700, "height": 30 },
        "text": "Seed (Decimal or 0x Hexadecimal)", "style": { "fontColor": "$FontColorGoldYellow", "pointSize": "$MediumFontSize", "dropShadow": "$DefaultDropShadow", "alignment": { "h": "center", "v": "center" } } } },
      { "type": "TextBoxWidget", "name": "SeedValueDisplay", "fields": {
        "rect": { "x": 277, "y": 271, "width": 700, "height": 52 },
        "text": "@seed_text@", "style": { "fontColor": "@seed_color@", "pointSize": "$SmallFontSize", "dropShadow": "$DefaultDropShadow", "alignment": { "h": "center", "v": "center" } } } },
      { "type": "ButtonWidget", "name": "SeedValueHitTarget", "fields": {
        "rect": { "x": 277, "y": 271, "width": 700, "height": 52 },
        "filename": "d2rloader\\map-seed-selector\\blank", "normalFrame": 0, "pressedFrame": 0, "disabledFrame": 0, "hoveredFrame": 0,
        "onClickMessage": "PanelManager:ClosePanel:map-seed-selector/FocusSeedValue" } },
      { "type": "TextBoxWidget", "name": "AddSeedStatus", "fields": {
        "rect": { "x": 150, "y": 325, "width": 956, "height": 23 },
        "text": "@status_text@", "style": { "fontColor": "$FontColorLightGold", "pointSize": "$SmallFontSize", "dropShadow": "$DefaultDropShadow", "alignment": { "h": "center", "v": "center" } } } },
      { "type": "ButtonWidget", "name": "AddSeedConfirm", "fields": {
        "rect": { "x": 342, "y": 350, "width": 265, "height": 70 },
        "filename": "d2rloader\\map-seed-selector\\modalbutton", "focusIndicatorFilename": "Controller/HoverImages/ModalButton_Hover",
        "pressedFrame": 1, "disabledFrame": 2, "hoveredFrame": 3, "textString": "Add Seed", "pointSize": "$MediumFontSize", "textColor": "$FontColorWhite",
        "onClickMessage": "PanelManager:ClosePanel:map-seed-selector/CommitAddSeed" } },
      { "type": "ButtonWidget", "name": "AddSeedCancel", "fields": {
        "rect": { "x": 647, "y": 350, "width": 265, "height": 70 },
        "filename": "d2rloader\\map-seed-selector\\modalbutton", "focusIndicatorFilename": "Controller/HoverImages/ModalButton_Hover",
        "pressedFrame": 1, "disabledFrame": 2, "hoveredFrame": 3, "textString": "Cancel", "pointSize": "$MediumFontSize", "textColor": "$FontColorWhite",
        "onClickMessage": "PanelManager:ClosePanel:map-seed-selector/CancelAddSeed" } }
    ]
  }]
})json";
    const auto replace = [&layout](std::string_view token,
            std::string_view value) {
        std::size_t position{};
        while ((position = layout.find(token, position))
                != std::string::npos) {
            layout.replace(position, token.size(), value);
            position += value.size();
        }
    };
    replace("@panel_name@", JsonEscape(qualifiedName));
    replace("@name_text@", JsonEscape(nameValue));
    replace("@seed_text@", JsonEscape(seedValue));
    replace("@status_text@", JsonEscape(status));
    replace("@name_color@", nameEmpty && !nameFocused
        ? "$FontColorLightGold" : "$FontColorWhite");
    replace("@seed_color@", seedEmpty && nameFocused
        ? "$FontColorLightGold" : "$FontColorWhite");
    return layout;
}

void __cdecl ApplyAddSeedTextRefresh(const D2RL::PluginContext* context,
        void*) noexcept {
    AddSeedTextRefreshPending.store(false, std::memory_order_release);
    if (context == nullptr || context != Context
            || !Operational.load(std::memory_order_acquire)
            || !AddSeedDialogOpen.load(std::memory_order_acquire)) {
        return;
    }
    std::wstring name;
    std::wstring seed;
    {
        std::lock_guard lock(AddSeedInputMutex);
        name = AddSeedNameWide;
        seed = AddSeedValueWide;
    }
    AddSeedNameText = WideToUtf8(name);
    AddSeedValueText = WideToUtf8(seed);
    AddSeedStatus.clear();
    (void)RefreshAddSeedDialog(context);
    if (AddSeedTextChanged.exchange(false, std::memory_order_acq_rel)) {
        ScheduleAddSeedTextRefresh();
    }
}

void ScheduleAddSeedTextRefresh() noexcept {
    if (Context == nullptr || ThreadService == nullptr
            || ThreadService->runOnUiThread == nullptr
            || !Operational.load(std::memory_order_acquire)
            || !AddSeedDialogOpen.load(std::memory_order_acquire)) {
        return;
    }
    bool expected = false;
    if (!AddSeedTextRefreshPending.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel)) {
        AddSeedTextChanged.store(true, std::memory_order_release);
        return;
    }
    const auto result = ThreadService->runOnUiThread(Context,
        ApplyAddSeedTextRefresh, nullptr);
    if (result != D2RL::Threads::Result::Success) {
        AddSeedTextRefreshPending.store(false, std::memory_order_release);
        D2RL::LogWarnF(Context,
            "MapSeedSelector: text update could not be scheduled (result=%u).",
            static_cast<unsigned>(result));
    }
}

void PostAddSeedFieldLoad() noexcept {
    const DWORD threadId = AddSeedKeyboardThreadId.load(
        std::memory_order_acquire);
    if (threadId != 0) {
        (void)PostThreadMessageW(threadId, AddSeedLoadFieldMessage,
            static_cast<WPARAM>(AddSeedFocusedField.load(
                std::memory_order_acquire)), 0);
    }
}

void __cdecl AddSeedActionOnUi(const D2RL::PluginContext* context,
        void*) noexcept;

void ScheduleAddSeedAction(int action) noexcept {
    if (Context == nullptr || ThreadService == nullptr
            || ThreadService->runOnUiThread == nullptr
            || !Operational.load(std::memory_order_acquire)) {
        return;
    }
    PendingAddSeedAction.store(action, std::memory_order_release);
    bool expected = false;
    if (!AddSeedActionPending.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel)) {
        return;
    }
    const auto result = ThreadService->runOnUiThread(Context,
        AddSeedActionOnUi, nullptr);
    if (result != D2RL::Threads::Result::Success) {
        PendingAddSeedAction.store(-1, std::memory_order_release);
        AddSeedActionPending.store(false, std::memory_order_release);
        D2RL::LogWarnF(Context,
            "MapSeedSelector: Add Seed UI action could not be scheduled (result=%u).",
            static_cast<unsigned>(result));
    }
}

auto UnloadCurrentSeedPanelLayout(
        const D2RL::PluginContext* context) noexcept -> bool {
    if (!CurrentSeedPanelLayoutLoaded.load(std::memory_order_acquire)) return true;
    if (context == nullptr || context != Context || WidgetService == nullptr
            || WidgetService->dispatchUiAction == nullptr) {
        return false;
    }
    const D2RL::Widgets::UiAction unload{
        .structSize = D2RL::Widgets::UiActionSize,
        .flags = 0,
        .target = "PanelManager",
        .command = "UnloadPanel",
        .text = QualifiedCurrentSeedPanelName,
    };
    const auto result = WidgetService->dispatchUiAction(context, &unload);
    if (result != D2RL::Widgets::Result::Success) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: current-seed panel could not be unloaded (result=%u).",
            static_cast<unsigned>(result));
        return false;
    }
    CurrentSeedPanelLayoutLoaded.store(false, std::memory_order_release);
    return true;
}

auto RefreshCurrentSeedPanelLayout(const D2RL::PluginContext* context,
        bool hasSeed, std::uint32_t seed) noexcept -> bool {
    if (context == nullptr || context != Context || ResourceService == nullptr
            || ResourceService->registerResource == nullptr
            || ResourceService->unregisterResource == nullptr
            || !UnloadCurrentSeedPanelLayout(context)) {
        return false;
    }
    if (CurrentSeedPanelLayoutHandle != D2RL::Resources::InvalidHandle) {
        const auto result = ResourceService->unregisterResource(context,
            CurrentSeedPanelLayoutHandle);
        if (result != D2RL::Resources::Result::Success) {
            D2RL::LogWarnF(context,
                "MapSeedSelector: current-seed layout could not be refreshed (result=%u).",
                static_cast<unsigned>(result));
            return false;
        }
        CurrentSeedPanelLayoutHandle = D2RL::Resources::InvalidHandle;
    }
    CurrentSeedPanelLayoutJson = BuildCurrentSeedPanelLayout(hasSeed, seed);
    const D2RL::Resources::ResourceRegistration layout{
        .structSize = D2RL::Resources::ResourceRegistrationSize,
        .flags = 0,
        .path = CurrentSeedPanelLayoutResourcePath,
        .bytes = CurrentSeedPanelLayoutJson.data(),
        .byteCount = CurrentSeedPanelLayoutJson.size(),
    };
    const auto result = ResourceService->registerResource(context, &layout,
        &CurrentSeedPanelLayoutHandle);
    if (result != D2RL::Resources::Result::Success
            || CurrentSeedPanelLayoutHandle
                == D2RL::Resources::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: current-seed layout could not be registered (result=%u).",
            static_cast<unsigned>(result));
        return false;
    }
    return true;
}

auto CopySeedToClipboard(std::uint32_t seed) noexcept -> bool {
    const std::wstring text = std::to_wstring(seed);
    const SIZE_T bytes = (text.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (memory == nullptr) return false;
    auto* destination = static_cast<wchar_t*>(GlobalLock(memory));
    if (destination == nullptr) {
        (void)GlobalFree(memory);
        return false;
    }
    std::copy(text.begin(), text.end(), destination);
    destination[text.size()] = L'\0';
    (void)GlobalUnlock(memory);
    if (!OpenClipboard(nullptr)) {
        (void)GlobalFree(memory);
        return false;
    }
    const bool emptied = EmptyClipboard() != FALSE;
    const HANDLE clipboardData = emptied
        ? SetClipboardData(CF_UNICODETEXT, memory) : nullptr;
    (void)CloseClipboard();
    if (clipboardData == nullptr) {
        (void)GlobalFree(memory);
        return false;
    }
    return true;
}

void OpenCurrentSeedPanel(const D2RL::PluginContext* context) noexcept {
    if (context == nullptr || context != Context
            || CurrentSeedPanelHandle == D2RL::Panels::InvalidHandle
            || PanelService == nullptr || ResourceService == nullptr
            || WidgetService == nullptr) {
        if (context == Context) {
            context->LogWarn("MapSeedSelector: current-seed display services are unavailable.");
        }
        return;
    }
    std::uint32_t seed{};
    const bool hasSeed = MapSeedSelector::GetLastObservedMapSeed(seed);
    if (!RefreshCurrentSeedPanelLayout(context, hasSeed, seed)) {
        context->LogWarn("MapSeedSelector: current-seed panel could not be prepared.");
        return;
    }
    const bool resumeTyping = AddSeedDialogOpen.load(std::memory_order_acquire);
    if (resumeTyping) AddSeedTypingEnabled.store(false,
        std::memory_order_release);
    const auto openResult = PanelService->openPanel(context,
        CurrentSeedPanelHandle);
    if (openResult != D2RL::Panels::Result::Success) {
        if (resumeTyping) AddSeedTypingEnabled.store(true,
            std::memory_order_release);
        D2RL::LogWarnF(context,
            "MapSeedSelector: current-seed panel could not be opened (result=%u).",
            static_cast<unsigned>(openResult));
        return;
    }
    CurrentSeedPanelOpen.store(true, std::memory_order_release);
    CurrentSeedPanelLayoutLoaded.store(true, std::memory_order_release);
}

void CloseCurrentSeedPanel(const D2RL::PluginContext* context) noexcept {
    if (context != nullptr && context == Context
            && PanelService != nullptr
            && CurrentSeedPanelHandle != D2RL::Panels::InvalidHandle) {
        (void)PanelService->closePanel(context, CurrentSeedPanelHandle);
    }
    const bool wasOpen = CurrentSeedPanelOpen.exchange(false,
        std::memory_order_acq_rel);
    if (wasOpen && AddSeedDialogOpen.load(std::memory_order_acquire)) {
        AddSeedTypingEnabled.store(true, std::memory_order_release);
        PostAddSeedFieldLoad();
    }
}

auto AddSeedGameIsForeground() noexcept -> bool {
    const HWND foreground = GetForegroundWindow();
    DWORD processId{};
    return foreground != nullptr
        && GetWindowThreadProcessId(foreground, &processId) != 0
        && processId == GetCurrentProcessId();
}

LRESULT CALLBACK AddSeedKeyboardProc(int code, WPARAM message,
        LPARAM parameter) {
    if (code < 0 || parameter == 0) {
        return CallNextHookEx(AddSeedKeyboardHook, code, message, parameter);
    }
    const auto* key = reinterpret_cast<const KBDLLHOOKSTRUCT*>(parameter);
    if (key->vkCode > 0xffU
            || (message != WM_KEYDOWN && message != WM_KEYUP)) {
        return CallNextHookEx(AddSeedKeyboardHook, code, message, parameter);
    }
    const auto virtualKey = static_cast<std::size_t>(key->vkCode);
    if (message == WM_KEYUP) {
        if (AddSeedConsumedKeys[virtualKey]) {
            AddSeedConsumedKeys[virtualKey] = false;
            return 1;
        }
        if (AddSeedForwardedKeys[virtualKey]) {
            AddSeedForwardedKeys[virtualKey] = false;
            if (AddSeedEdit != nullptr) {
                const LPARAM keyData = static_cast<LPARAM>(1
                    | (static_cast<std::uint64_t>(static_cast<std::uint8_t>(
                        key->scanCode)) << 16)
                    | ((key->flags & LLKHF_EXTENDED) ? 0x01000000ULL : 0)
                    | 0xC0000000ULL);
                (void)PostMessageW(AddSeedEdit, WM_KEYUP, key->vkCode,
                    keyData);
            }
            return 1;
        }
        return CallNextHookEx(AddSeedKeyboardHook, code, message, parameter);
    }
    if (AddSeedConsumedKeys[virtualKey]) return 1;
    if (CurrentSeedPanelOpen.load(std::memory_order_acquire)
            && AddSeedGameIsForeground()) {
        AddSeedConsumedKeys[virtualKey] = true;
        if (key->vkCode == VK_ESCAPE) ScheduleAddSeedAction(7);
        return 1;
    }
    if (!AddSeedTypingEnabled.load(std::memory_order_acquire)
            || !AddSeedDialogOpen.load(std::memory_order_acquire)
            || !AddSeedGameIsForeground()) {
        return CallNextHookEx(AddSeedKeyboardHook, code, message, parameter);
    }

    switch (key->vkCode) {
    case VK_SHIFT: case VK_LSHIFT: case VK_RSHIFT:
    case VK_CONTROL: case VK_LCONTROL: case VK_RCONTROL:
    case VK_MENU: case VK_LMENU: case VK_RMENU:
    case VK_LWIN: case VK_RWIN: case VK_CAPITAL:
    case VK_NUMLOCK: case VK_SCROLL:
        return CallNextHookEx(AddSeedKeyboardHook, code, message, parameter);
    default: break;
    }
    const bool control = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    const bool alt = (key->flags & LLKHF_ALTDOWN) != 0;
    if (alt && !control) {
        return CallNextHookEx(AddSeedKeyboardHook, code, message, parameter);
    }
    if (key->vkCode == VK_TAB || key->vkCode == VK_ESCAPE
            || key->vkCode == VK_RETURN) {
        AddSeedConsumedKeys[virtualKey] = true;
        if (key->vkCode == VK_ESCAPE) {
            AddSeedTypingEnabled.store(false, std::memory_order_release);
            ScheduleAddSeedAction(1);
        } else if (key->vkCode == VK_TAB) {
            AddSeedFocusedField.store(1
                    - AddSeedFocusedField.load(std::memory_order_acquire),
                std::memory_order_release);
            PostAddSeedFieldLoad();
            AddSeedTextChanged.store(true, std::memory_order_release);
            ScheduleAddSeedTextRefresh();
        } else {
            ScheduleAddSeedAction(2);
        }
        return 1;
    }
    if (AddSeedEdit == nullptr) {
        return CallNextHookEx(AddSeedKeyboardHook, code, message, parameter);
    }
    const LPARAM keyData = static_cast<LPARAM>(1
        | (static_cast<std::uint64_t>(static_cast<std::uint8_t>(
            key->scanCode)) << 16)
        | ((key->flags & LLKHF_EXTENDED) ? 0x01000000ULL : 0));
    UINT editMessage = WM_KEYDOWN;
    WPARAM editKey = key->vkCode;
    if (control && !alt && (key->vkCode == 'A' || key->vkCode == 'C'
            || key->vkCode == 'V' || key->vkCode == 'X'
            || key->vkCode == 'Z')) {
        editMessage = WM_CHAR;
        editKey = key->vkCode - 'A' + 1;
    }
    if (!PostMessageW(AddSeedEdit, editMessage, editKey, keyData)) {
        return CallNextHookEx(AddSeedKeyboardHook, code, message, parameter);
    }
    AddSeedForwardedKeys[virtualKey] = true;
    return 1;
}

void SyncAddSeedKeyboardState() noexcept {
    const HWND foreground = GetForegroundWindow();
    DWORD processId{};
    if (foreground != nullptr
            && GetWindowThreadProcessId(foreground, &processId) != 0
            && processId == GetCurrentProcessId()) {
        const DWORD gameThread = GetWindowThreadProcessId(foreground, nullptr);
        if (const HKL layout = GetKeyboardLayout(gameThread);
                layout != nullptr && layout != GetKeyboardLayout(0)) {
            (void)ActivateKeyboardLayout(layout, 0);
        }
    }
    BYTE state[256]{};
    if (!GetKeyboardState(state)) return;
    constexpr std::array<int, 9> modifiers{
        VK_SHIFT, VK_LSHIFT, VK_RSHIFT, VK_CONTROL, VK_LCONTROL,
        VK_RCONTROL, VK_MENU, VK_LMENU, VK_RMENU,
    };
    for (const int key : modifiers) {
        state[key] = (GetAsyncKeyState(key) & 0x8000) != 0 ? 0x80 : 0;
    }
    constexpr std::array<int, 2> toggles{VK_CAPITAL, VK_NUMLOCK};
    for (const int key : toggles) {
        state[key] = static_cast<BYTE>((GetKeyState(key) & 1)
            | ((GetAsyncKeyState(key) & 0x8000) != 0 ? 0x80 : 0));
    }
    (void)SetKeyboardState(state);
}

DWORD WINAPI AddSeedKeyboardMain(void*) {
    MSG message{};
    (void)PeekMessageW(&message, nullptr, WM_USER, WM_USER, PM_NOREMOVE);
    AddSeedKeyboardThreadId.store(GetCurrentThreadId(),
        std::memory_order_release);
    HMODULE module{};
    (void)GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
        | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&AddSeedKeyboardProc), &module);
    AddSeedEdit = CreateWindowExW(0, L"EDIT", L"", ES_AUTOHSCROLL,
        0, 0, 0, 0, HWND_MESSAGE, nullptr, module, nullptr);
    if (AddSeedEdit != nullptr) SetFocus(AddSeedEdit);
    AddSeedKeyboardHook = SetWindowsHookExW(WH_KEYBOARD_LL,
        AddSeedKeyboardProc, module, 0);
    if (AddSeedEdit == nullptr || AddSeedKeyboardHook == nullptr) {
        AddSeedKeyboardFailed.store(true, std::memory_order_release);
    }
    AddSeedKeyboardReady.store(true, std::memory_order_release);
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (message.message == AddSeedShutdownMessage) break;
        if (message.message == AddSeedLoadFieldMessage) {
            const bool seedField = message.wParam != 0;
            std::wstring text;
            {
                std::lock_guard lock(AddSeedInputMutex);
                text = seedField ? AddSeedValueWide : AddSeedNameWide;
            }
            if (AddSeedEdit != nullptr) {
                SendMessageW(AddSeedEdit, EM_LIMITTEXT,
                    seedField ? 32 : 48, 0);
                SetWindowTextW(AddSeedEdit, text.c_str());
                SendMessageW(AddSeedEdit, EM_SETSEL, text.size(),
                    text.size());
            }
            continue;
        }
        const bool editKey = message.message == WM_KEYDOWN
            || message.message == WM_KEYUP || message.message == WM_CHAR;
        if (message.message == WM_KEYDOWN || message.message == WM_KEYUP) {
            SyncAddSeedKeyboardState();
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
        if (editKey && AddSeedEdit != nullptr) {
            std::wstring text(static_cast<std::size_t>(
                GetWindowTextLengthW(AddSeedEdit)) + 1, L'\0');
            text.resize(static_cast<std::size_t>(GetWindowTextW(AddSeedEdit,
                text.data(), static_cast<int>(text.size()))));
            const bool seedField = AddSeedFocusedField.load(
                std::memory_order_acquire) != 0;
            {
                std::lock_guard lock(AddSeedInputMutex);
                auto& field = seedField ? AddSeedValueWide : AddSeedNameWide;
                if (field == text) continue;
                field = std::move(text);
            }
            AddSeedTextChanged.store(true, std::memory_order_release);
            ScheduleAddSeedTextRefresh();
        }
    }
    if (AddSeedKeyboardHook != nullptr) {
        (void)UnhookWindowsHookEx(AddSeedKeyboardHook);
        AddSeedKeyboardHook = nullptr;
    }
    if (AddSeedEdit != nullptr) {
        DestroyWindow(AddSeedEdit);
        AddSeedEdit = nullptr;
    }
    AddSeedKeyboardThreadId.store(0, std::memory_order_release);
    return 0;
}

auto StartAddSeedKeyboard() noexcept -> bool {
    if (AddSeedKeyboardThread != nullptr) {
        return !AddSeedKeyboardFailed.load(std::memory_order_acquire);
    }
    AddSeedKeyboardReady.store(false, std::memory_order_release);
    AddSeedKeyboardFailed.store(false, std::memory_order_release);
    AddSeedForwardedKeys.fill(false);
    AddSeedConsumedKeys.fill(false);
    AddSeedKeyboardThread = CreateThread(nullptr, 0, AddSeedKeyboardMain,
        nullptr, 0, nullptr);
    if (AddSeedKeyboardThread == nullptr) return false;
    for (int attempt = 0; attempt < 100
            && !AddSeedKeyboardReady.load(std::memory_order_acquire);
            ++attempt) {
        Sleep(10);
    }
    return AddSeedKeyboardReady.load(std::memory_order_acquire)
        && !AddSeedKeyboardFailed.load(std::memory_order_acquire);
}

void StopAddSeedKeyboard() noexcept {
    AddSeedTypingEnabled.store(false, std::memory_order_release);
    const DWORD threadId = AddSeedKeyboardThreadId.load(
        std::memory_order_acquire);
    if (threadId != 0) {
        (void)PostThreadMessageW(threadId, AddSeedShutdownMessage, 0, 0);
    }
    if (AddSeedKeyboardThread != nullptr) {
        (void)WaitForSingleObject(AddSeedKeyboardThread, INFINITE);
        CloseHandle(AddSeedKeyboardThread);
        AddSeedKeyboardThread = nullptr;
    }
    AddSeedKeyboardReady.store(false, std::memory_order_release);
    AddSeedKeyboardFailed.store(false, std::memory_order_release);
}

auto RegisterDialogLayout(const D2RL::PluginContext* context,
        std::size_t slotIndex) noexcept -> bool {
    if (context == nullptr || slotIndex >= AddSeedDialogSlotCount) return false;
    auto& slot = AddSeedDialogSlots[slotIndex];
    slot.layoutJson = BuildAddSeedDialogLayout(slotIndex,
        AddSeedNameText, AddSeedValueText, AddSeedStatus,
        AddSeedFocusedField.load(std::memory_order_acquire) == 0);
    if (slot.layout != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(context, slot.layout);
        slot.layout = D2RL::Resources::InvalidHandle;
    }
    const D2RL::Resources::ResourceRegistration resource{
        .structSize = D2RL::Resources::ResourceRegistrationSize,
        .flags = 0,
        .path = slot.resourcePath.c_str(),
        .bytes = slot.layoutJson.data(),
        .byteCount = slot.layoutJson.size(),
    };
    return ResourceService->registerResource(context, &resource,
        &slot.layout) == D2RL::Resources::Result::Success
        && slot.layout != D2RL::Resources::InvalidHandle;
}

auto UnloadAddSeedDialogSlot(const D2RL::PluginContext* context,
        std::size_t slotIndex) noexcept -> bool {
    if (context == nullptr || slotIndex >= AddSeedDialogSlotCount) return false;
    auto& slot = AddSeedDialogSlots[slotIndex];
    if (slot.loaded) {
        const D2RL::Widgets::UiAction unload{
            .structSize = D2RL::Widgets::UiActionSize,
            .flags = 0,
            .target = "PanelManager",
            .command = "UnloadPanel",
            .text = slot.qualifiedName.c_str(),
        };
        if (WidgetService == nullptr || WidgetService->dispatchUiAction == nullptr
                || WidgetService->dispatchUiAction(context, &unload)
                    != D2RL::Widgets::Result::Success) {
            return false;
        }
    }
    if (slot.layout != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(context, slot.layout);
        slot.layout = D2RL::Resources::InvalidHandle;
    }
    slot.loaded = false;
    slot.lastUsed = 0;
    slot.closedAt = {};
    return true;
}

void TrimClosedAddSeedDialogPanels(
        const D2RL::PluginContext* context) noexcept {
    if (context == nullptr || context != Context) return;
    struct Candidate { std::size_t slot{}; std::uint64_t lastUsed{}; };
    std::array<Candidate, AddSeedDialogSlotCount> candidates{};
    std::size_t count{};
    std::size_t closed{};
    const auto now = std::chrono::steady_clock::now();
    for (std::size_t index = 0; index < AddSeedDialogSlotCount; ++index) {
        auto& slot = AddSeedDialogSlots[index];
        if (!slot.loaded || index == ActiveAddSeedDialogSlot) continue;
        D2RL::Panels::PanelInfo info{
            .structSize = D2RL::Panels::PanelInfoSize,
        };
        if (PanelService->getPanelInfo(context, slot.panel, &info)
                != D2RL::Panels::Result::Success
                || info.presentationState
                    != D2RL::Panels::PresentationState::Closed) continue;
        ++closed;
        if (slot.closedAt == std::chrono::steady_clock::time_point{}) {
            slot.closedAt = now;
        }
        if (now - slot.closedAt >= PanelUnloadCooldown) {
            candidates[count++] = {index, slot.lastUsed};
        }
    }
    if (closed <= KeptClosedSelectorPanels) return;
    std::sort(candidates.begin(), candidates.begin() + count,
        [](const Candidate& lhs, const Candidate& rhs) {
            return lhs.lastUsed < rhs.lastUsed;
        });
    for (std::size_t i = 0; i < count
            && closed > KeptClosedSelectorPanels; ++i) {
        if (UnloadAddSeedDialogSlot(context, candidates[i].slot)) --closed;
    }
}

auto RefreshAddSeedDialog(const D2RL::PluginContext* context) noexcept
        -> bool {
    if (context == nullptr || context != Context || PanelService == nullptr
            || ResourceService == nullptr
            || !AddSeedDialogOpen.load(std::memory_order_acquire)) {
        return false;
    }
    std::size_t target = AddSeedDialogSlotCount;
    const auto now = std::chrono::steady_clock::now();
    for (std::size_t offset = 1; offset <= AddSeedDialogSlotCount; ++offset) {
        const auto index = (AddSeedDialogCursor + offset)
            % AddSeedDialogSlotCount;
        if (index == ActiveAddSeedDialogSlot) continue;
        auto& slot = AddSeedDialogSlots[index];
        if (!slot.loaded) {
            target = index;
            break;
        }
        if (slot.closedAt != std::chrono::steady_clock::time_point{}
                && now - slot.closedAt >= PanelUnloadCooldown) {
            target = index;
            break;
        }
    }
    if (target == AddSeedDialogSlotCount) {
        context->LogWarn("MapSeedSelector: the Add Seed view pool is cooling down; the current text remains saved for submission.");
        return false;
    }

    auto& next = AddSeedDialogSlots[target];
    if (next.loaded && !UnloadAddSeedDialogSlot(context, target)) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: previous Add Seed view %s could not be unloaded.",
            next.qualifiedName.c_str());
        return false;
    }
    if (!RegisterDialogLayout(context, target)) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: Add Seed view layout registration failed for slot %u.",
            static_cast<unsigned>(target));
        return false;
    }
    if (PanelService->openPanel(context, next.panel)
            != D2RL::Panels::Result::Success) {
        context->LogWarn("MapSeedSelector: the Add Seed prompt could not be opened.");
        return false;
    }
    const auto previous = ActiveAddSeedDialogSlot;
    ActiveAddSeedDialogSlot = target;
    AddSeedDialogCursor = target;
    next.loaded = true;
    next.lastUsed = ++AddSeedDialogUseSequence;
    next.closedAt = {};
    if (previous < AddSeedDialogSlotCount) {
        auto& previousSlot = AddSeedDialogSlots[previous];
        if (previousSlot.panel != D2RL::Panels::InvalidHandle) {
            (void)PanelService->closePanel(context, previousSlot.panel);
            previousSlot.closedAt = std::chrono::steady_clock::now();
        }
    }
    TrimClosedAddSeedDialogPanels(context);
    return true;
}

void CloseAddSeedDialog(const D2RL::PluginContext* context) noexcept {
    AddSeedTypingEnabled.store(false, std::memory_order_release);
    AddSeedDialogOpen.store(false, std::memory_order_release);
    CloseCurrentSeedPanel(context);
    if (context == nullptr || context != Context
            || ActiveAddSeedDialogSlot >= AddSeedDialogSlotCount) return;
    auto& slot = AddSeedDialogSlots[ActiveAddSeedDialogSlot];
    if (PanelService->closePanel(context, slot.panel)
            == D2RL::Panels::Result::Success) {
        slot.closedAt = std::chrono::steady_clock::now();
    }
    ActiveAddSeedDialogSlot = AddSeedDialogSlotCount;
    TrimClosedAddSeedDialogPanels(context);
}

void __cdecl AddSeedActionOnUi(const D2RL::PluginContext* context,
        void*) noexcept {
    const int action = PendingAddSeedAction.exchange(-1,
        std::memory_order_acq_rel);
    if (context == nullptr || context != Context
            || !Operational.load(std::memory_order_acquire)) {
        AddSeedActionPending.store(false, std::memory_order_release);
        return;
    }
    if (action == 0) {
        if (!DesiredPanelOpen.load(std::memory_order_acquire)) {
            AddSeedActionPending.store(false, std::memory_order_release);
            return;
        }
        {
            std::lock_guard lock(AddSeedInputMutex);
            AddSeedNameWide.clear();
            AddSeedValueWide.clear();
        }
        AddSeedNameText.clear();
        AddSeedValueText.clear();
        AddSeedStatus.clear();
        AddSeedFocusedField.store(0, std::memory_order_release);
        AddSeedDialogOpen.store(true, std::memory_order_release);
        AddSeedTypingEnabled.store(true, std::memory_order_release);
        PostAddSeedFieldLoad();
        if (!RefreshAddSeedDialog(context)) {
            AddSeedDialogOpen.store(false, std::memory_order_release);
            AddSeedTypingEnabled.store(false, std::memory_order_release);
        }
    } else if (action == 1) {
        CloseAddSeedDialog(context);
    } else if (action == 3 || action == 4) {
        AddSeedFocusedField.store(action == 3 ? 0 : 1,
            std::memory_order_release);
        PostAddSeedFieldLoad();
        AddSeedStatus.clear();
        (void)RefreshAddSeedDialog(context);
    } else if (action == 5) {
        if (AddSeedDialogOpen.load(std::memory_order_acquire)) {
            if (CurrentSeedPanelOpen.load(std::memory_order_acquire)) {
                CloseCurrentSeedPanel(context);
            } else {
                OpenCurrentSeedPanel(context);
            }
        }
    } else if (action == 6) {
        std::uint32_t seed{};
        if (!MapSeedSelector::GetLastObservedMapSeed(seed)) {
            context->LogWarn("MapSeedSelector: no gameplay map seed has been captured yet.");
        } else if (CopySeedToClipboard(seed)) {
            CloseCurrentSeedPanel(context);
        } else {
            D2RL::LogWarn(context,
                "MapSeedSelector: selected seed could not be copied to the clipboard.");
        }
    } else if (action == 7) {
        CloseCurrentSeedPanel(context);
    } else if (action == 2) {
        std::wstring wideName;
        std::wstring wideSeed;
        {
            std::lock_guard lock(AddSeedInputMutex);
            wideName = AddSeedNameWide;
            wideSeed = AddSeedValueWide;
        }
        const std::string name = WideToUtf8(wideName);
        const std::string seed = WideToUtf8(wideSeed);
        std::string message;
        const bool added = MapSeedSelector::AddSeedEntry(name, seed, message);
        if (added) {
            if (!RefreshSelectorLayouts(context)) {
                message = "Seed saved, but the dropdown could not refresh. Restart D2RLoader to see it.";
                context->LogWarn("MapSeedSelector: seed was saved to TOML, but the selector layout refresh failed.");
            } else {
                message = "Seed added and selected.";
                D2RL::LogInfo(context,
                    "MapSeedSelector: added a seed and refreshed the dropdown immediately.");
            }
            {
                std::lock_guard lock(AddSeedInputMutex);
                AddSeedNameWide.clear();
                AddSeedValueWide.clear();
            }
            AddSeedNameText.clear();
            AddSeedValueText.clear();
            AddSeedFocusedField.store(0, std::memory_order_release);
            CloseAddSeedDialog(context);
        } else {
            context->LogWarn(message.c_str());
            AddSeedStatus = message;
            (void)RefreshAddSeedDialog(context);
        }
    }
    AddSeedActionPending.store(false, std::memory_order_release);
    const int pending = PendingAddSeedAction.load(std::memory_order_acquire);
    if (pending >= 0) ScheduleAddSeedAction(pending);
}

void __cdecl ApplyPanelState(const D2RL::PluginContext* context,
        void*) noexcept {
    PanelSyncPending.store(false, std::memory_order_release);
    if (context == nullptr || context != Context
            || !Operational.load(std::memory_order_acquire)
            || PanelService == nullptr
            || PanelHandle == D2RL::Panels::InvalidHandle) {
        return;
    }

    const bool shouldOpen = DesiredPanelOpen.load(std::memory_order_acquire);
    if (shouldOpen && !NativeDropdownAvailable
            && !FallbackArtworkReady.load(std::memory_order_acquire)) {
        return;
    }
    D2RL::Panels::PanelInfo info{
        .structSize = D2RL::Panels::PanelInfoSize,
    };
    const auto infoResult = PanelService->getPanelInfo(context, PanelHandle,
        &info);
    if (infoResult != D2RL::Panels::Result::Success) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: getPanelInfo failed (result=%u).",
            static_cast<unsigned>(infoResult));
        return;
    }
    const bool isOpen = info.presentationState
        == D2RL::Panels::PresentationState::Open;
    if (shouldOpen && !isOpen) {
        const auto result = PanelService->openPanel(context, PanelHandle);
        if (result != D2RL::Panels::Result::Success) {
            D2RL::LogWarnF(context,
                "MapSeedSelector: character-select panel could not be opened (result=%u).",
                static_cast<unsigned>(result));
            return;
        }
        MarkSelectorPanelOpened(ActivePanelSlot, false);
        context->LogInfo("MapSeedSelector: character-select panel opened by the panel service.");
    } else if (shouldOpen && isOpen) {
        MarkSelectorPanelOpened(ActivePanelSlot, false);
    } else if (!shouldOpen && isOpen) {
        if (PanelService->closePanel(context, PanelHandle)
                == D2RL::Panels::Result::Success) {
            MarkSelectorPanelClosed(ActivePanelSlot, false);
        }
    }

    const auto syncAuxiliaryPanel = [context, shouldOpen](
            D2RL::Panels::RegistrationHandle handle) {
        if (handle == D2RL::Panels::InvalidHandle) return;
        D2RL::Panels::PanelInfo auxiliaryInfo{
            .structSize = D2RL::Panels::PanelInfoSize,
        };
        if (PanelService->getPanelInfo(context, handle, &auxiliaryInfo)
                != D2RL::Panels::Result::Success) {
            return;
        }
        const bool open = auxiliaryInfo.presentationState
            == D2RL::Panels::PresentationState::Open;
        if (shouldOpen && !open) {
            (void)PanelService->openPanel(context, handle);
        } else if (!shouldOpen && open) {
            (void)PanelService->closePanel(context, handle);
        }
    };
    syncAuxiliaryPanel(AddSeedButtonPanelHandle);
    if (!shouldOpen) CloseCurrentSeedPanel(context);
    if (OptionsPanelHandle != D2RL::Panels::InvalidHandle) {
        D2RL::Panels::PanelInfo optionsInfo{
            .structSize = D2RL::Panels::PanelInfoSize,
        };
        if (PanelService->getPanelInfo(context, OptionsPanelHandle,
                &optionsInfo) == D2RL::Panels::Result::Success) {
            const bool optionsOpen = optionsInfo.presentationState
                == D2RL::Panels::PresentationState::Open;
            // The options panel is the expanded list. Keep it collapsed when
            // the character-select flow is initialized or restored; only the
            // user's dropdown action should open it.
            if (optionsOpen
                    && PanelService->closePanel(context, OptionsPanelHandle)
                        == D2RL::Panels::Result::Success) {
                MarkSelectorPanelClosed(ActivePanelSlot, true);
            }
        }
    }
    if (!shouldOpen) CloseAddSeedDialog(context);

    TrimClosedSelectorPanels(context);
    if (shouldOpen) {
        MapSeedSelector::RefreshUi();
        ScheduleUiRefresh();
    }
}

void SchedulePanelSync() noexcept {
    if (Context == nullptr || ThreadService == nullptr
            || ThreadService->runOnUiThread == nullptr
            || PanelHandle == D2RL::Panels::InvalidHandle
            || !Operational.load(std::memory_order_acquire)) {
        return;
    }
    bool expected = false;
    if (!PanelSyncPending.compare_exchange_strong(expected, true,
            std::memory_order_acq_rel)) {
        return;
    }
    const auto result = ThreadService->runOnUiThread(
        Context, ApplyPanelState, nullptr);
    if (result != D2RL::Threads::Result::Success) {
        PanelSyncPending.store(false, std::memory_order_release);
        D2RL::LogWarnF(Context,
            "MapSeedSelector: UI-thread scheduling failed (result=%u).",
            static_cast<unsigned>(result));
    }
}

auto ShouldEndOfflineFlow(std::string_view command) noexcept -> bool {
    return command == "ToJoinCharacterSelect"
        || command == "ToCinematics"
        || command == "ToExit"
        || command == "ToFrontEnd"
        || command == "ToMainMenu"
        || command == "ToMultiplayer"
        || command == "ToOnline"
        || command == "ToLobby"
        || command == "ToCharacterCreate";
}

auto IsOnlineNavigation(std::string_view command) noexcept -> bool {
    return command == "ToJoinCharacterSelect"
        || command == "ToMultiplayer"
        || command == "ToOnline"
        || command == "ToLobby";
}

auto __cdecl OnUiMessage(const D2RL::PluginContext* context,
        const D2RL::SharedEvents::UiMessageEvent* event,
        void*) noexcept -> D2RL::SharedEvents::UiMessageAction {
    if (context == nullptr || context != Context || event == nullptr
            || event->structSize < D2RL::SharedEvents::UiMessageEventRequiredSize
            || event->target == nullptr || event->command == nullptr
            || !Operational.load(std::memory_order_acquire)) {
        return D2RL::SharedEvents::UiMessageAction::Continue;
    }

    const std::string_view target(event->target);
    const std::string_view command(event->command);
    const std::string_view message = event->text != nullptr
        ? std::string_view(event->text) : std::string_view{};

    if (target == "PanelManager" && command == "ClosePanel") {
        if (message == QualifiedCurrentSeedPanelName) {
            const bool wasOpen = CurrentSeedPanelOpen.exchange(false,
                std::memory_order_acq_rel);
            if (wasOpen
                    && AddSeedDialogOpen.load(std::memory_order_acquire)) {
                AddSeedTypingEnabled.store(true, std::memory_order_release);
                PostAddSeedFieldLoad();
            }
            return D2RL::SharedEvents::UiMessageAction::Continue;
        }
        if (message == OpenAddSeedMessage) {
            ScheduleAddSeedAction(0);
            return D2RL::SharedEvents::UiMessageAction::Consume;
        }
        if (message == CommitAddSeedMessage) {
            ScheduleAddSeedAction(2);
            return D2RL::SharedEvents::UiMessageAction::Consume;
        }
        if (message == CancelAddSeedMessage) {
            ScheduleAddSeedAction(1);
            return D2RL::SharedEvents::UiMessageAction::Consume;
        }
        if (message == ShowAddSeedHelpMessage) {
            ScheduleAddSeedAction(5);
            return D2RL::SharedEvents::UiMessageAction::Consume;
        }
        if (message == CopyCurrentSeedMessage) {
            ScheduleAddSeedAction(6);
            return D2RL::SharedEvents::UiMessageAction::Consume;
        }
        if (message == CloseCurrentSeedMessage) {
            ScheduleAddSeedAction(7);
            return D2RL::SharedEvents::UiMessageAction::Consume;
        }
        if (message == FocusSeedNameMessage) {
            ScheduleAddSeedAction(3);
            return D2RL::SharedEvents::UiMessageAction::Consume;
        }
        if (message == FocusSeedValueMessage) {
            ScheduleAddSeedAction(4);
            return D2RL::SharedEvents::UiMessageAction::Consume;
        }
    }

    if (target == "PanelManager" && command == "ClosePanel"
            && message == DropdownToggleMessage) {
        SchedulePickerAction(0);
        return D2RL::SharedEvents::UiMessageAction::Consume;
    }

    if (target == "PanelManager" && command == "ClosePanel"
            && message.starts_with(ChoiceMessagePrefix)) {
        const auto indexText = message.substr(sizeof(ChoiceMessagePrefix) - 1);
        std::size_t choice{};
        const auto parsed = std::from_chars(indexText.data(),
            indexText.data() + indexText.size(), choice);
        if (parsed.ec == std::errc{}
                && parsed.ptr == indexText.data() + indexText.size()) {
            if (choice < static_cast<std::size_t>(std::numeric_limits<int>::max()))
                SchedulePickerAction(static_cast<int>(choice) + 1);
            return D2RL::SharedEvents::UiMessageAction::Consume;
        }
    }

    if (target == "FrontEndNavigation"
            || (target == "PanelManager"
                && (command == "OpenPanel" || command == "ClosePanel"))) {
        D2RL::LogInfoF(context,
            "MapSeedSelector: UI event target=%s command=%s text=%s",
            event->target, event->command,
            event->text != nullptr ? event->text : "");
    }

    if (target == "MapSeedSelector" && command == "SeedChanged") {
        // Dropdown selection payloads are binary; read the selected state from
        // WidgetService on the UI thread instead of parsing event->text.
        ScheduleDropdownChanged();
        return D2RL::SharedEvents::UiMessageAction::Consume;
    }

    if (target == "FrontEndNavigation") {
        if (command == "ToFrontEnd" || command == "ToMainMenu") {
            OnlineFlowActive.store(false, std::memory_order_release);
        }
        if (IsOnlineNavigation(command)) {
            OnlineFlowActive.store(true, std::memory_order_release);
        }
        if (command == "ToCharacterSelect"
                && !OnlineFlowActive.load(std::memory_order_acquire)) {
            MapSeedSelector::SetOfflineFlowActive(true);
            DesiredPanelOpen.store(true, std::memory_order_release);
            SchedulePanelSync();
        } else if (ShouldEndOfflineFlow(command)) {
            DesiredPanelOpen.store(false, std::memory_order_release);
            AddSeedDialogOpen.store(false, std::memory_order_release);
            AddSeedTypingEnabled.store(false, std::memory_order_release);
            MapSeedSelector::SetOfflineFlowActive(false);
            SchedulePanelSync();
        }
    } else if (target == "PanelManager" && command == "OpenPanel"
            && message == "CharacterSelectPanel"
            && !OnlineFlowActive.load(std::memory_order_acquire)) {
        MapSeedSelector::SetOfflineFlowActive(true);
        DesiredPanelOpen.store(true, std::memory_order_release);
        context->LogInfo("MapSeedSelector: character-select panel detected; showing seed choices before character selection.");
        SchedulePanelSync();
    } else if (target == "PanelManager" && command == "ClosePanel"
            && (message == "CharacterSelectPanel"
                || message == std::string_view(QualifiedPanelName))) {
        DesiredPanelOpen.store(false, std::memory_order_release);
        AddSeedDialogOpen.store(false, std::memory_order_release);
        AddSeedTypingEnabled.store(false, std::memory_order_release);
        MapSeedSelector::SetOfflineFlowActive(false);
        SchedulePanelSync();
    }

    return D2RL::SharedEvents::UiMessageAction::Continue;
}

void __cdecl OnGameplayEvent(const D2RL::PluginContext* context,
        const D2RL::Lifecycle::GameplayEvent* event,
        void*) noexcept {
    if (context == nullptr || context != Context || event == nullptr
            || event->structSize < D2RL::Lifecycle::GameplayEventRequiredSize
            || !Operational.load(std::memory_order_acquire)) {
        return;
    }
    if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameJoined
            || event->kind == D2RL::Lifecycle::GameplayEventKind::GameLeft) {
        if (event->kind == D2RL::Lifecycle::GameplayEventKind::GameJoined) {
            MapSeedSelector::LogSeedInitializationSummary();
        }
        DesiredPanelOpen.store(false, std::memory_order_release);
        AddSeedDialogOpen.store(false, std::memory_order_release);
        AddSeedTypingEnabled.store(false, std::memory_order_release);
        MapSeedSelector::SetOfflineFlowActive(false);
        SchedulePanelSync();
    }
}

auto QueryServices(const D2RL::PluginContext* context) noexcept -> bool {
    if (context->QueryService(&ThreadService)
            != D2RL::ServiceQueryResult::Success
            || !D2RL::HasThreadServiceField(ThreadService,
                D2RL::ThreadServiceRequiredSize)
            || ThreadService->runOnUiThread == nullptr) {
        return false;
    }
    if (context->QueryService(&ResourceService)
            != D2RL::ServiceQueryResult::Success
            || !D2RL::HasResourceServiceField(ResourceService,
                D2RL::ResourceServiceRequiredSize)
            || ResourceService->registerResource == nullptr
            || ResourceService->unregisterResource == nullptr) {
        return false;
    }
    if (context->QueryService(&PanelService)
            != D2RL::ServiceQueryResult::Success
            || !D2RL::HasPanelServiceField(PanelService,
                D2RL::PanelServiceRequiredSize)
            || PanelService->registerPanel == nullptr
            || PanelService->unregisterPanel == nullptr
            || PanelService->getPanelInfo == nullptr
            || PanelService->openPanel == nullptr
            || PanelService->closePanel == nullptr) {
        return false;
    }
    if (context->QueryService(&SharedEventService)
            != D2RL::ServiceQueryResult::Success
            || !D2RL::HasSharedEventServiceField(SharedEventService,
                D2RL::SharedEventServiceRequiredSize)
            || SharedEventService->registerUiMessageListener == nullptr
            || SharedEventService->unregisterUiMessageListener == nullptr) {
        return false;
    }
    if (context->QueryService(&WidgetService)
            != D2RL::ServiceQueryResult::Success) {
        context->LogWarn("MapSeedSelector: the native widget service is unavailable.");
        return false;
    }
    if (!D2RL::HasWidgetServiceField(WidgetService,
                D2RL::WidgetServiceRequiredSize)
            || WidgetService->findPanel == nullptr
            || WidgetService->findWidget == nullptr
            || WidgetService->setWidgetVisible == nullptr
            || WidgetService->dispatchUiAction == nullptr) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: required widget services are unavailable (service size=%u, version=%u).",
            WidgetService != nullptr ? WidgetService->serviceSize : 0,
            WidgetService != nullptr ? WidgetService->serviceVersion : 0);
        return false;
    }
    NativeDropdownAvailable = D2RL::HasWidgetServiceField(WidgetService,
            D2RL::WidgetServiceSetDropdownSelectionFieldEnd)
        && WidgetService->getDropdownSelection != nullptr
        && WidgetService->setDropdownUpdateMessage != nullptr
        && WidgetService->setDropdownSelection != nullptr;
    if (!NativeDropdownAvailable) {
        D2RL::LogInfoF(context,
            "MapSeedSelector: native dropdown callbacks are unavailable (widget service size=%u, version=%u); composing D2R button and row artwork for the fallback panel.",
            WidgetService->serviceSize, WidgetService->serviceVersion);
    }
    if (context->QueryService(&LifecycleService)
            != D2RL::ServiceQueryResult::Success
            || !D2RL::HasLifecycleServiceField(LifecycleService,
                D2RL::LifecycleServiceRequiredSize)
            || LifecycleService->registerDataTablesLoadedListener == nullptr
            || LifecycleService->unregisterDataTablesLoadedListener == nullptr
            || LifecycleService->registerGameplayEventListener == nullptr
            || LifecycleService->unregisterGameplayEventListener == nullptr) {
        return false;
    }
    return true;
}

void UnregisterUi() noexcept {
    CloseCurrentSeedPanel(Context);
    (void)UnloadCurrentSeedPanelLayout(Context);
    if (ActiveAddSeedDialogSlot < AddSeedDialogSlotCount) {
        CloseAddSeedDialog(Context);
    }
    StopAddSeedKeyboard();
    Operational.store(false, std::memory_order_release);
    DesiredPanelOpen.store(false, std::memory_order_release);
    AddSeedDialogOpen.store(false, std::memory_order_release);
    AddSeedTypingEnabled.store(false, std::memory_order_release);
    CurrentSeedPanelOpen.store(false, std::memory_order_release);
    AddSeedTextRefreshPending.store(false, std::memory_order_release);
    AddSeedTextChanged.store(false, std::memory_order_release);
    PanelSyncPending.store(false, std::memory_order_release);
    DropdownSyncPending.store(false, std::memory_order_release);
    PickerActionPending.store(false, std::memory_order_release);
    PendingPickerAction.store(-1, std::memory_order_release);
    AddSeedActionPending.store(false, std::memory_order_release);
    PendingAddSeedAction.store(-1, std::memory_order_release);
    UiRefreshPending.store(false, std::memory_order_release);
    if (Context != nullptr && LifecycleService != nullptr) {
        if (DataTablesLoadedHandle != D2RL::Lifecycle::InvalidHandle) {
            (void)LifecycleService->unregisterDataTablesLoadedListener(
                Context, DataTablesLoadedHandle);
        }
        if (GameJoinedHandle != D2RL::Lifecycle::InvalidHandle) {
            (void)LifecycleService->unregisterGameplayEventListener(
                Context, GameJoinedHandle);
        }
        if (GameLeftHandle != D2RL::Lifecycle::InvalidHandle) {
            (void)LifecycleService->unregisterGameplayEventListener(
                Context, GameLeftHandle);
        }
    }
    if (Context != nullptr && SharedEventService != nullptr
            && UiMessageHandle != D2RL::SharedEvents::InvalidHandle) {
        (void)SharedEventService->unregisterUiMessageListener(
            Context, UiMessageHandle);
    }
    if (Context != nullptr && PanelService != nullptr) {
        for (std::size_t slot = 0; slot < SelectorPanelSlotCount; ++slot) {
            if (SelectorOptionsPanelHandles[slot]
                    != D2RL::Panels::InvalidHandle) {
                (void)PanelService->unregisterPanel(Context,
                    SelectorOptionsPanelHandles[slot]);
            }
            if (SelectorPanelHandles[slot]
                    != D2RL::Panels::InvalidHandle) {
                (void)PanelService->unregisterPanel(Context,
                    SelectorPanelHandles[slot]);
            }
        }
    }
    if (Context != nullptr && PanelService != nullptr) {
        for (auto& slot : AddSeedDialogSlots) {
            if (slot.panel != D2RL::Panels::InvalidHandle) {
                (void)PanelService->unregisterPanel(Context, slot.panel);
            }
        }
    }
    if (Context != nullptr && PanelService != nullptr
            && CurrentSeedPanelHandle != D2RL::Panels::InvalidHandle) {
        (void)PanelService->unregisterPanel(Context, CurrentSeedPanelHandle);
    }
    if (Context != nullptr && PanelService != nullptr
            && AddSeedButtonPanelHandle != D2RL::Panels::InvalidHandle) {
        (void)PanelService->unregisterPanel(Context,
            AddSeedButtonPanelHandle);
    }
    if (Context != nullptr && ResourceService != nullptr) {
        for (std::size_t slot = 0; slot < SelectorPanelSlotCount; ++slot) {
            if (SelectorOptionsLayoutHandles[slot]
                    != D2RL::Resources::InvalidHandle) {
                (void)ResourceService->unregisterResource(Context,
                    SelectorOptionsLayoutHandles[slot]);
            }
            if (SelectorLayoutHandles[slot]
                    != D2RL::Resources::InvalidHandle) {
                (void)ResourceService->unregisterResource(Context,
                    SelectorLayoutHandles[slot]);
            }
        }
    }
    if (Context != nullptr && ResourceService != nullptr) {
        for (auto& slot : AddSeedDialogSlots) {
            if (slot.layout != D2RL::Resources::InvalidHandle) {
                (void)ResourceService->unregisterResource(Context,
                    slot.layout);
            }
        }
    }
    if (Context != nullptr && ResourceService != nullptr
            && CurrentSeedPanelLayoutHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            CurrentSeedPanelLayoutHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && AddSeedButtonLayoutHandle != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            AddSeedButtonLayoutHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && AddSeedButtonLowEndSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            AddSeedButtonLowEndSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && AddSeedButtonSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            AddSeedButtonSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && QuestionButtonLowEndSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            QuestionButtonLowEndSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && QuestionButtonSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            QuestionButtonSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && ModalButtonLowEndSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            ModalButtonLowEndSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && ModalButtonSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            ModalButtonSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && CurrentSeedPanelLowEndSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            CurrentSeedPanelLowEndSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && CurrentSeedPanelSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            CurrentSeedPanelSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && CurrentSeedPanelButtonLowEndSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            CurrentSeedPanelButtonLowEndSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && CurrentSeedPanelButtonSpriteHandle
                != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            CurrentSeedPanelButtonSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && BlankSpriteLowEndHandle != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context,
            BlankSpriteLowEndHandle);
    }
    if (Context != nullptr && ResourceService != nullptr
            && BlankSpriteHandle != D2RL::Resources::InvalidHandle) {
        (void)ResourceService->unregisterResource(Context, BlankSpriteHandle);
    }
    if (Context != nullptr && ResourceService != nullptr) {
        for (auto& handle : CompositeSpriteHandles) {
            if (handle != D2RL::Resources::InvalidHandle) {
                (void)ResourceService->unregisterResource(Context, handle);
                handle = D2RL::Resources::InvalidHandle;
            }
        }
    }
    GameJoinedHandle = D2RL::Lifecycle::InvalidHandle;
    GameLeftHandle = D2RL::Lifecycle::InvalidHandle;
    DataTablesLoadedHandle = D2RL::Lifecycle::InvalidHandle;
    UiMessageHandle = D2RL::SharedEvents::InvalidHandle;
    PanelHandle = D2RL::Panels::InvalidHandle;
    OptionsPanelHandle = D2RL::Panels::InvalidHandle;
    SelectorPanelHandles.fill(D2RL::Panels::InvalidHandle);
    SelectorOptionsPanelHandles.fill(D2RL::Panels::InvalidHandle);
    SelectorLayoutHandles.fill(D2RL::Resources::InvalidHandle);
    SelectorOptionsLayoutHandles.fill(D2RL::Resources::InvalidHandle);
    ActivePanelSlot = 0;
    SetActivePanelSlot(0);
    AddSeedButtonPanelHandle = D2RL::Panels::InvalidHandle;
    CurrentSeedPanelHandle = D2RL::Panels::InvalidHandle;
    CurrentSeedPanelLayoutLoaded.store(false, std::memory_order_release);
    for (auto& slot : AddSeedDialogSlots) {
        slot.layout = D2RL::Resources::InvalidHandle;
        slot.panel = D2RL::Panels::InvalidHandle;
        slot.localId.clear();
        slot.qualifiedName.clear();
        slot.resourcePath.clear();
        slot.layoutJson.clear();
        slot.loaded = false;
        slot.lastUsed = 0;
        slot.closedAt = {};
    }
    AddSeedDialogCursor = AddSeedDialogSlotCount - 1;
    ActiveAddSeedDialogSlot = AddSeedDialogSlotCount;
    AddSeedDialogUseSequence = 0;
    LayoutHandle = D2RL::Resources::InvalidHandle;
    OptionsLayoutHandle = D2RL::Resources::InvalidHandle;
    AddSeedButtonLayoutHandle = D2RL::Resources::InvalidHandle;
    CurrentSeedPanelLayoutHandle = D2RL::Resources::InvalidHandle;
    AddSeedButtonSpriteHandle = D2RL::Resources::InvalidHandle;
    AddSeedButtonLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
    QuestionButtonSpriteHandle = D2RL::Resources::InvalidHandle;
    QuestionButtonLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
    ModalButtonSpriteHandle = D2RL::Resources::InvalidHandle;
    ModalButtonLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
    CurrentSeedPanelSpriteHandle = D2RL::Resources::InvalidHandle;
    CurrentSeedPanelLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
    CurrentSeedPanelButtonSpriteHandle = D2RL::Resources::InvalidHandle;
    CurrentSeedPanelButtonLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
    BlankSpriteHandle = D2RL::Resources::InvalidHandle;
    BlankSpriteLowEndHandle = D2RL::Resources::InvalidHandle;
    CompositeSpriteHandles.fill(D2RL::Resources::InvalidHandle);
    OptionsLayoutJson.clear();
    AddSeedButtonLayoutJson.clear();
    CurrentSeedPanelLayoutJson.clear();
    BlankSpriteBytes.fill(0);
    CompositeSprites = {};
    UiRegistrationPending.store(false, std::memory_order_release);
    FallbackArtworkReady.store(false, std::memory_order_release);
    NativeDropdownAvailable = false;
    MapSeedSelector::SetWidgetService(nullptr, false);
}

auto BuildLayout() -> bool {
    try {
        LayoutJson = LayoutTemplate;
        std::int32_t panelX{};
        std::int32_t panelY{};
        MapSeedSelector::GetPanelPosition(panelX, panelY);
        const auto replaceToken = [](std::string_view token,
                const std::string& replacement) {
            const auto position = LayoutJson.find(token);
            if (position == std::string::npos) return false;
            LayoutJson.replace(position, token.size(), replacement);
            return true;
        };
        if (!replaceToken("@panel_name@", QualifiedPanelName)) {
            return false;
        }
        const auto panelHeight = MapSeedSelector::GetPanelHeight();
        const auto panelWidth = MapSeedSelector::GetPanelWidth();
        if (!replaceToken("@panel_x@", std::to_string(panelX))
                || !replaceToken("@panel_y@", std::to_string(panelY))
                || !replaceToken("@panel_width@", std::to_string(panelWidth))
                || !replaceToken("@panel_height@", std::to_string(panelHeight))) {
            return false;
        }
        const auto token = std::string_view("@seed_children@");
        const auto position = LayoutJson.find(token);
        if (position == std::string::npos) return false;
        const auto children = MapSeedSelector::BuildLayoutChildren();
        LayoutJson.replace(position, token.size(), children);
        return LayoutJson.find("@panel_x@") == std::string::npos
            && LayoutJson.find("@panel_y@") == std::string::npos
            && LayoutJson.find("@panel_width@") == std::string::npos
            && LayoutJson.find("@panel_height@") == std::string::npos
            && LayoutJson.find("@seed_children@") == std::string::npos;
    } catch (...) {
        return false;
    }
}

auto BuildOptionsLayout() -> bool {
    try {
        OptionsLayoutJson = R"json({
  "type": "Panel",
  "name": "@panel_name@",
  "fields": {
    "priority": 9006,
    "anchor": { "x": 0.0, "y": 0.0 },
    "rect": { "x": 0, "y": 0, "width": )json";
        OptionsLayoutJson += std::to_string(PopupCanvasWidth);
        OptionsLayoutJson += R"json(, "height": )json";
        OptionsLayoutJson += std::to_string(
            MapSeedSelector::GetOptionsPanelHeight());
        OptionsLayoutJson += R"json( }
  },
  "children": [@seed_children@]
})json";
        const auto panelNameToken = std::string_view("@panel_name@");
        const auto panelNamePosition = OptionsLayoutJson.find(panelNameToken);
        if (panelNamePosition == std::string::npos) return false;
        OptionsLayoutJson.replace(panelNamePosition, panelNameToken.size(),
            QualifiedOptionsPanelName);
        const auto token = std::string_view("@seed_children@");
        const auto position = OptionsLayoutJson.find(token);
        if (position == std::string::npos) return false;
        const auto children = MapSeedSelector::BuildOptionsLayoutChildren();
        OptionsLayoutJson.replace(position, token.size(), children);
        return OptionsLayoutJson.find("@seed_children@") == std::string::npos;
    } catch (...) {
        return false;
    }
}

auto BuildAddSeedButtonLayout() -> bool {
    try {
        AddSeedButtonLayoutJson = AddSeedButtonLayoutTemplate;
        std::int32_t buttonX{};
        std::int32_t buttonY{};
        MapSeedSelector::GetAddSeedButtonPosition(buttonX, buttonY);
        const auto replaceToken = [](std::string_view token,
                const std::string& replacement) {
            const auto position = AddSeedButtonLayoutJson.find(token);
            if (position == std::string::npos) return false;
            AddSeedButtonLayoutJson.replace(position, token.size(), replacement);
            return true;
        };
        return replaceToken("@button_x@", std::to_string(buttonX))
            && replaceToken("@button_y@", std::to_string(buttonY))
            && AddSeedButtonLayoutJson.find("@button_x@") == std::string::npos
            && AddSeedButtonLayoutJson.find("@button_y@") == std::string::npos;
    } catch (...) {
        return false;
    }
}

void SetActivePanelSlot(std::size_t slot) noexcept {
    if (slot >= SelectorPanelSlotCount) return;
    try {
        ActivePanelSlot = slot;
        PanelLocalId = "MapSeeds" + std::to_string(slot);
        QualifiedPanelName = "map-seed-selector/" + PanelLocalId;
        OptionsPanelLocalId = "SeedOptions" + std::to_string(slot);
        QualifiedOptionsPanelName = "map-seed-selector/" + OptionsPanelLocalId;
        LayoutResourcePath = "data/global/ui/layouts/map-seed-selector/"
            + PanelLocalId + "hd.json";
        OptionsLayoutResourcePath =
            "data/global/ui/layouts/map-seed-selector/"
            + OptionsPanelLocalId + "hd.json";
        PanelHandle = SelectorPanelHandles[slot];
        OptionsPanelHandle = SelectorOptionsPanelHandles[slot];
        LayoutHandle = SelectorLayoutHandles[slot];
        OptionsLayoutHandle = SelectorOptionsLayoutHandles[slot];
        MapSeedSelector::SetPanelGeneration(static_cast<std::uint32_t>(slot));
    } catch (...) {
        // Keep the previously selected slot if allocation fails.
    }
}

// D2R caches parsed panel layouts and can crash if a recently closed panel is
// immediately reused. Keep a pool of panels like Grail On: each added seed
// moves the selector to a panel that has not been opened yet.
auto RefreshSelectorLayouts(const D2RL::PluginContext* context) noexcept
        -> bool {
    if (context == nullptr || context != Context || PanelService == nullptr
            || ResourceService == nullptr || WidgetService == nullptr
            || PanelHandle == D2RL::Panels::InvalidHandle
            || LayoutHandle == D2RL::Resources::InvalidHandle) {
        return false;
    }
    const auto previousSlot = ActivePanelSlot;
    const auto nextSlot = previousSlot + 1;
    if (nextSlot >= SelectorPanelSlotCount) {
        context->LogWarn("MapSeedSelector: no unused selector panel remains for an immediate layout refresh.");
        return false;
    }
    SetActivePanelSlot(nextSlot);
    if (!BuildLayout() || (!NativeDropdownAvailable
            && !BuildOptionsLayout())) {
        context->LogWarn("MapSeedSelector: refreshed selector layouts could not be built.");
        SetActivePanelSlot(previousSlot);
        return false;
    }

    const auto closeIfOpen = [context](D2RL::Panels::RegistrationHandle panel,
            std::size_t slot, bool options) -> bool {
        if (panel == D2RL::Panels::InvalidHandle) return true;
        D2RL::Panels::PanelInfo info{
            .structSize = D2RL::Panels::PanelInfoSize,
        };
        if (PanelService->getPanelInfo(context, panel, &info)
                != D2RL::Panels::Result::Success) {
            return false;
        }
        if (info.presentationState != D2RL::Panels::PresentationState::Open) {
            MarkSelectorPanelClosed(slot, options);
            return true;
        }
        if (PanelService->closePanel(context, panel)
                != D2RL::Panels::Result::Success) {
            return false;
        }
        MarkSelectorPanelClosed(slot, options);
        return true;
    };
    const auto reopenMain = [context]() {
        if (!DesiredPanelOpen.load(std::memory_order_acquire)
                || PanelHandle == D2RL::Panels::InvalidHandle) {
            return;
        }
        if (PanelService->openPanel(context, PanelHandle)
                == D2RL::Panels::Result::Success) {
            MarkSelectorPanelOpened(ActivePanelSlot, false);
            MapSeedSelector::RefreshUi();
            ScheduleUiRefresh();
        }
    };
    if (!closeIfOpen(SelectorPanelHandles[previousSlot], previousSlot, false)
            || !closeIfOpen(SelectorOptionsPanelHandles[previousSlot],
                previousSlot, true)) {
        context->LogWarn("MapSeedSelector: the selector panels could not be closed for refresh.");
        SetActivePanelSlot(previousSlot);
        reopenMain();
        return false;
    }

    const auto unregisterLayout = [context](
            D2RL::Resources::RegistrationHandle& handle) -> bool {
        if (handle == D2RL::Resources::InvalidHandle) return true;
        const auto result = ResourceService->unregisterResource(context,
            handle);
        if (result != D2RL::Resources::Result::Success) return false;
        handle = D2RL::Resources::InvalidHandle;
        return true;
    };
    const auto registerLayout = [context](const std::string& path,
            const std::string& json,
            D2RL::Resources::RegistrationHandle& handle) -> bool {
        handle = D2RL::Resources::InvalidHandle;
        const D2RL::Resources::ResourceRegistration registration{
            .structSize = D2RL::Resources::ResourceRegistrationSize,
            .flags = 0,
            .path = path.c_str(),
            .bytes = json.data(),
            .byteCount = json.size(),
        };
        return ResourceService->registerResource(context, &registration,
                &handle) == D2RL::Resources::Result::Success
            && handle != D2RL::Resources::InvalidHandle;
    };
    const bool mainReleased = unregisterLayout(LayoutHandle);
    const bool optionsReleased = NativeDropdownAvailable
        || unregisterLayout(OptionsLayoutHandle);
    if (!mainReleased || !optionsReleased) {
        context->LogWarn("MapSeedSelector: the old selector layout resource could not be released.");
        if (LayoutHandle == D2RL::Resources::InvalidHandle) {
            (void)registerLayout(LayoutResourcePath, LayoutJson, LayoutHandle);
        }
        if (!NativeDropdownAvailable
                && OptionsLayoutHandle == D2RL::Resources::InvalidHandle) {
            (void)registerLayout(OptionsLayoutResourcePath,
                OptionsLayoutJson, OptionsLayoutHandle);
        }
        SelectorLayoutHandles[nextSlot] = LayoutHandle;
        SelectorOptionsLayoutHandles[nextSlot] = OptionsLayoutHandle;
        SetActivePanelSlot(previousSlot);
        reopenMain();
        return false;
    }

    if (!registerLayout(LayoutResourcePath, LayoutJson, LayoutHandle)) {
        context->LogWarn("MapSeedSelector: the updated selector layout could not be registered.");
        SelectorLayoutHandles[nextSlot] = LayoutHandle;
        SelectorOptionsLayoutHandles[nextSlot] = OptionsLayoutHandle;
        SetActivePanelSlot(previousSlot);
        reopenMain();
        return false;
    }
    if (!NativeDropdownAvailable
            && !registerLayout(OptionsLayoutResourcePath,
                OptionsLayoutJson, OptionsLayoutHandle)) {
        context->LogWarn("MapSeedSelector: the updated seed-list layout could not be registered.");
        SelectorLayoutHandles[nextSlot] = LayoutHandle;
        SelectorOptionsLayoutHandles[nextSlot] = OptionsLayoutHandle;
        SetActivePanelSlot(previousSlot);
        reopenMain();
        return false;
    }

    SelectorLayoutHandles[nextSlot] = LayoutHandle;
    SelectorOptionsLayoutHandles[nextSlot] = OptionsLayoutHandle;
    SetActivePanelSlot(nextSlot);
    if (DesiredPanelOpen.load(std::memory_order_acquire)) {
        const auto openResult = PanelService->openPanel(context, PanelHandle);
        if (openResult != D2RL::Panels::Result::Success) {
            D2RL::LogWarnF(context,
                "MapSeedSelector: refreshed selector could not be reopened (result=%u).",
                static_cast<unsigned>(openResult));
            SetActivePanelSlot(previousSlot);
            reopenMain();
            return false;
        }
        MarkSelectorPanelOpened(nextSlot, false);
        MapSeedSelector::RefreshUi();
        ScheduleUiRefresh();
    }
    TrimClosedSelectorPanels(context);
    return true;
}

auto RegisterUi(const D2RL::PluginContext* context) noexcept -> bool {
    MapSeedSelector::SetWidgetService(WidgetService,
        NativeDropdownAvailable);
    for (auto& slot : AddSeedDialogSlots) {
        slot.layout = D2RL::Resources::InvalidHandle;
        slot.panel = D2RL::Panels::InvalidHandle;
        slot.loaded = false;
        slot.lastUsed = 0;
        slot.closedAt = {};
    }
    AddSeedDialogCursor = AddSeedDialogSlotCount - 1;
    ActiveAddSeedDialogSlot = AddSeedDialogSlotCount;
    AddSeedDialogUseSequence = 0;
    if (!RegisterBlankSprite(context)) {
        context->LogWarn("MapSeedSelector: fallback resources or panel layout could not be prepared.");
        return false;
    }

    for (std::size_t slot = 0; slot < SelectorPanelSlotCount; ++slot) {
        SetActivePanelSlot(slot);
        if (!BuildLayout()) {
            context->LogWarn("MapSeedSelector: selector panel layout could not be prepared.");
            return false;
        }
        const D2RL::Resources::ResourceRegistration layout{
            .structSize = D2RL::Resources::ResourceRegistrationSize,
            .flags = 0,
            .path = LayoutResourcePath.c_str(),
            .bytes = LayoutJson.data(),
            .byteCount = LayoutJson.size(),
        };
        const auto layoutResult = ResourceService->registerResource(context,
            &layout, &SelectorLayoutHandles[slot]);
        if (layoutResult != D2RL::Resources::Result::Success
                || SelectorLayoutHandles[slot]
                    == D2RL::Resources::InvalidHandle) {
            D2RL::LogWarnF(context,
                "MapSeedSelector: selector panel layout registration failed for slot %u (result=%u).",
                static_cast<unsigned>(slot), static_cast<unsigned>(layoutResult));
            return false;
        }
        const D2RL::Panels::PanelRegistration panel{
            .structSize = D2RL::Panels::PanelRegistrationSize,
            .flags = D2RL::Panels::PanelFlags::CloseOnEscape,
            .localId = PanelLocalId.c_str(),
        };
        const auto panelResult = PanelService->registerPanel(context, &panel,
            &SelectorPanelHandles[slot]);
        if (panelResult != D2RL::Panels::Result::Success
                || SelectorPanelHandles[slot]
                    == D2RL::Panels::InvalidHandle) {
            D2RL::LogWarnF(context,
                "MapSeedSelector: character-select panel registration failed for slot %u (result=%u).",
                static_cast<unsigned>(slot), static_cast<unsigned>(panelResult));
            return false;
        }

        if (!NativeDropdownAvailable) {
            if (!BuildOptionsLayout()) {
                context->LogWarn("MapSeedSelector: popup panel layout could not be prepared.");
                return false;
            }
            const D2RL::Resources::ResourceRegistration optionsLayout{
                .structSize = D2RL::Resources::ResourceRegistrationSize,
                .flags = 0,
                .path = OptionsLayoutResourcePath.c_str(),
                .bytes = OptionsLayoutJson.data(),
                .byteCount = OptionsLayoutJson.size(),
            };
            const auto optionsLayoutResult = ResourceService->registerResource(
                context, &optionsLayout,
                &SelectorOptionsLayoutHandles[slot]);
            if (optionsLayoutResult != D2RL::Resources::Result::Success
                    || SelectorOptionsLayoutHandles[slot]
                        == D2RL::Resources::InvalidHandle) {
                D2RL::LogWarnF(context,
                    "MapSeedSelector: popup layout registration failed for slot %u (result=%u).",
                    static_cast<unsigned>(slot),
                    static_cast<unsigned>(optionsLayoutResult));
                return false;
            }
            const D2RL::Panels::PanelRegistration optionsPanel{
                .structSize = D2RL::Panels::PanelRegistrationSize,
                .flags = D2RL::Panels::PanelFlags::CloseOnEscape,
                .localId = OptionsPanelLocalId.c_str(),
            };
            const auto optionsPanelResult = PanelService->registerPanel(context,
                &optionsPanel, &SelectorOptionsPanelHandles[slot]);
            if (optionsPanelResult != D2RL::Panels::Result::Success
                    || SelectorOptionsPanelHandles[slot]
                        == D2RL::Panels::InvalidHandle) {
                D2RL::LogWarnF(context,
                    "MapSeedSelector: popup panel registration failed for slot %u (result=%u).",
                    static_cast<unsigned>(slot),
                    static_cast<unsigned>(optionsPanelResult));
                return false;
            }
        }
    }
    SetActivePanelSlot(0);

    const auto registerAddSeedUi = [context]() -> bool {
    const auto registerButtonSprite = [context](const char* path,
            const auto& bytes,
            D2RL::Resources::RegistrationHandle& handle) -> bool {
        const D2RL::Resources::ResourceRegistration sprite{
            .structSize = D2RL::Resources::ResourceRegistrationSize,
            .flags = 0,
            .path = path,
            .bytes = bytes.data(),
            .byteCount = bytes.size(),
        };
        const auto result = ResourceService->registerResource(context, &sprite,
            &handle);
        if (result == D2RL::Resources::Result::Success
                && handle != D2RL::Resources::InvalidHandle) {
            return true;
        }
        D2RL::LogWarnF(context,
            "MapSeedSelector: Add Seed UI sprite registration failed for %s (result=%u).",
            path, static_cast<unsigned>(result));
        return false;
    };
    if (!registerButtonSprite(AddSeedButtonSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::AddSeedButtonSprite,
                AddSeedButtonSpriteHandle)
            || !registerButtonSprite(AddSeedButtonLowEndSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::AddSeedButtonLowEndSprite,
                AddSeedButtonLowEndSpriteHandle)
            || !registerButtonSprite(QuestionButtonSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::QuestionButtonSprite,
                QuestionButtonSpriteHandle)
            || !registerButtonSprite(QuestionButtonLowEndSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::QuestionButtonLowEndSprite,
                QuestionButtonLowEndSpriteHandle)
            || !registerButtonSprite(ModalButtonSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::ModalButtonSprite,
                ModalButtonSpriteHandle)
            || !registerButtonSprite(ModalButtonLowEndSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::ModalButtonLowEndSprite,
                ModalButtonLowEndSpriteHandle)
            || !registerButtonSprite(CurrentSeedPanelSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::CurrentSeedPanelSprite,
                CurrentSeedPanelSpriteHandle)
            || !registerButtonSprite(CurrentSeedPanelLowEndSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::CurrentSeedPanelLowEndSprite,
                CurrentSeedPanelLowEndSpriteHandle)
            || !registerButtonSprite(CurrentSeedPanelButtonSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::CurrentSeedPanelButtonSprite,
                CurrentSeedPanelButtonSpriteHandle)
            || !registerButtonSprite(CurrentSeedPanelButtonLowEndSpriteResourcePath,
                MapSeedSelector::EmbeddedAssets::CurrentSeedPanelButtonLowEndSprite,
                CurrentSeedPanelButtonLowEndSpriteHandle)) {
        return false;
    }
    if (!BuildAddSeedButtonLayout()) {
        context->LogWarn("MapSeedSelector: Add Seed button layout could not be prepared.");
        return false;
    }
    const D2RL::Resources::ResourceRegistration addButtonLayout{
        .structSize = D2RL::Resources::ResourceRegistrationSize,
        .flags = 0,
        .path = AddSeedButtonLayoutResourcePath,
        .bytes = AddSeedButtonLayoutJson.data(),
        .byteCount = AddSeedButtonLayoutJson.size(),
    };
    const auto addButtonLayoutResult = ResourceService->registerResource(
        context, &addButtonLayout, &AddSeedButtonLayoutHandle);
    if (addButtonLayoutResult != D2RL::Resources::Result::Success
            || AddSeedButtonLayoutHandle == D2RL::Resources::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: Add Seed button layout registration failed (result=%u, handle=%llu).",
            static_cast<unsigned>(addButtonLayoutResult),
            static_cast<unsigned long long>(AddSeedButtonLayoutHandle));
        return false;
    }
    const D2RL::Panels::PanelRegistration addButtonPanel{
        .structSize = D2RL::Panels::PanelRegistrationSize,
        .flags = D2RL::Panels::PanelFlags::None,
        .localId = AddSeedButtonPanelLocalId,
    };
    const auto addButtonPanelResult = PanelService->registerPanel(context,
        &addButtonPanel, &AddSeedButtonPanelHandle);
    if (addButtonPanelResult != D2RL::Panels::Result::Success
            || AddSeedButtonPanelHandle == D2RL::Panels::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: Add Seed button panel registration failed (result=%u, handle=%llu).",
            static_cast<unsigned>(addButtonPanelResult),
            static_cast<unsigned long long>(AddSeedButtonPanelHandle));
        return false;
    }

    CurrentSeedPanelLayoutJson = BuildCurrentSeedPanelLayout(false, 0);
    const D2RL::Resources::ResourceRegistration currentSeedLayout{
        .structSize = D2RL::Resources::ResourceRegistrationSize,
        .flags = 0,
        .path = CurrentSeedPanelLayoutResourcePath,
        .bytes = CurrentSeedPanelLayoutJson.data(),
        .byteCount = CurrentSeedPanelLayoutJson.size(),
    };
    const auto currentSeedLayoutResult = ResourceService->registerResource(
        context, &currentSeedLayout, &CurrentSeedPanelLayoutHandle);
    if (currentSeedLayoutResult != D2RL::Resources::Result::Success
            || CurrentSeedPanelLayoutHandle
                == D2RL::Resources::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: current-seed panel layout registration failed (result=%u).",
            static_cast<unsigned>(currentSeedLayoutResult));
        return false;
    }
    const D2RL::Panels::PanelRegistration currentSeedPanel{
        .structSize = D2RL::Panels::PanelRegistrationSize,
        .flags = D2RL::Panels::PanelFlags::CloseOnEscape,
        .localId = CurrentSeedPanelLocalId,
    };
    const auto currentSeedPanelResult = PanelService->registerPanel(context,
        &currentSeedPanel, &CurrentSeedPanelHandle);
    if (currentSeedPanelResult != D2RL::Panels::Result::Success
            || CurrentSeedPanelHandle == D2RL::Panels::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: current-seed panel registration failed (result=%u).",
            static_cast<unsigned>(currentSeedPanelResult));
        return false;
    }

    for (std::size_t slotIndex = 0;
            slotIndex < AddSeedDialogSlotCount; ++slotIndex) {
        auto& slot = AddSeedDialogSlots[slotIndex];
        slot.localId = DialogLocalId(slotIndex);
        slot.qualifiedName = "map-seed-selector/" + slot.localId;
        slot.resourcePath = "data/global/ui/layouts/map-seed-selector/"
            + slot.localId + "hd.json";
        slot.layoutJson = BuildAddSeedDialogLayout(slotIndex, {}, {}, {},
            slotIndex == 0);
        const D2RL::Resources::ResourceRegistration dialogLayout{
            .structSize = D2RL::Resources::ResourceRegistrationSize,
            .flags = 0,
            .path = slot.resourcePath.c_str(),
            .bytes = slot.layoutJson.data(),
            .byteCount = slot.layoutJson.size(),
        };
        const auto layoutResult = ResourceService->registerResource(context,
            &dialogLayout, &slot.layout);
        if (layoutResult != D2RL::Resources::Result::Success
                || slot.layout == D2RL::Resources::InvalidHandle) {
            D2RL::LogWarnF(context,
                "MapSeedSelector: Add Seed dialog layout registration failed for slot %u (result=%u).",
                static_cast<unsigned>(slotIndex),
                static_cast<unsigned>(layoutResult));
            return false;
        }
        const D2RL::Panels::PanelRegistration dialogPanel{
            .structSize = D2RL::Panels::PanelRegistrationSize,
            .flags = D2RL::Panels::PanelFlags::CloseOnEscape,
            .localId = slot.localId.c_str(),
        };
        const auto panelResult = PanelService->registerPanel(context,
            &dialogPanel, &slot.panel);
        if (panelResult != D2RL::Panels::Result::Success
                || slot.panel == D2RL::Panels::InvalidHandle) {
            D2RL::LogWarnF(context,
                "MapSeedSelector: Add Seed dialog panel registration failed for slot %u (result=%u).",
                static_cast<unsigned>(slotIndex),
                static_cast<unsigned>(panelResult));
            return false;
        }
    }
    return true;
    };
    if (!registerAddSeedUi()) {
        if (PanelService != nullptr) {
            for (auto& slot : AddSeedDialogSlots) {
                if (slot.panel != D2RL::Panels::InvalidHandle) {
                    (void)PanelService->unregisterPanel(context, slot.panel);
                    slot.panel = D2RL::Panels::InvalidHandle;
                }
            }
        }
        if (PanelService != nullptr
                && CurrentSeedPanelHandle
                    != D2RL::Panels::InvalidHandle) {
            (void)PanelService->unregisterPanel(context,
                CurrentSeedPanelHandle);
            CurrentSeedPanelHandle = D2RL::Panels::InvalidHandle;
        }
        if (PanelService != nullptr
                && AddSeedButtonPanelHandle
                    != D2RL::Panels::InvalidHandle) {
            (void)PanelService->unregisterPanel(context,
                AddSeedButtonPanelHandle);
        }
        AddSeedButtonPanelHandle = D2RL::Panels::InvalidHandle;
        if (ResourceService != nullptr) {
            for (auto& slot : AddSeedDialogSlots) {
                if (slot.layout != D2RL::Resources::InvalidHandle) {
                    (void)ResourceService->unregisterResource(context,
                        slot.layout);
                    slot.layout = D2RL::Resources::InvalidHandle;
                }
            }
        }
        if (ResourceService != nullptr
                && AddSeedButtonLayoutHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                AddSeedButtonLayoutHandle);
        }
        AddSeedButtonLayoutHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && CurrentSeedPanelLayoutHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                CurrentSeedPanelLayoutHandle);
        }
        CurrentSeedPanelLayoutHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && AddSeedButtonLowEndSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                AddSeedButtonLowEndSpriteHandle);
        }
        AddSeedButtonLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && AddSeedButtonSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                AddSeedButtonSpriteHandle);
        }
        AddSeedButtonSpriteHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && QuestionButtonLowEndSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                QuestionButtonLowEndSpriteHandle);
        }
        QuestionButtonLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && QuestionButtonSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                QuestionButtonSpriteHandle);
        }
        QuestionButtonSpriteHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && ModalButtonLowEndSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                ModalButtonLowEndSpriteHandle);
        }
        ModalButtonLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && ModalButtonSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                ModalButtonSpriteHandle);
        }
        ModalButtonSpriteHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && CurrentSeedPanelLowEndSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                CurrentSeedPanelLowEndSpriteHandle);
        }
        CurrentSeedPanelLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && CurrentSeedPanelSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                CurrentSeedPanelSpriteHandle);
        }
        CurrentSeedPanelSpriteHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && CurrentSeedPanelButtonLowEndSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                CurrentSeedPanelButtonLowEndSpriteHandle);
        }
        CurrentSeedPanelButtonLowEndSpriteHandle = D2RL::Resources::InvalidHandle;
        if (ResourceService != nullptr
                && CurrentSeedPanelButtonSpriteHandle
                    != D2RL::Resources::InvalidHandle) {
            (void)ResourceService->unregisterResource(context,
                CurrentSeedPanelButtonSpriteHandle);
        }
        CurrentSeedPanelButtonSpriteHandle = D2RL::Resources::InvalidHandle;
        context->LogWarn("MapSeedSelector: Add Seed UI could not be registered; the seed picker remains available.");
    }

    const D2RL::SharedEvents::UiMessageListener uiListener{
        .structSize = D2RL::SharedEvents::UiMessageListenerSize,
        .flags = 0,
        .priority = 10000,
        .reserved = 0,
        .callback = OnUiMessage,
        .userData = nullptr,
    };
    const auto uiListenerResult = SharedEventService->registerUiMessageListener(
        context, &uiListener, &UiMessageHandle);
    if (uiListenerResult != D2RL::SharedEvents::Result::Success
            || UiMessageHandle == D2RL::SharedEvents::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: UI listener registration failed (result=%u, handle=%llu).",
            static_cast<unsigned>(uiListenerResult),
            static_cast<unsigned long long>(UiMessageHandle));
        return false;
    }

    const D2RL::Lifecycle::GameplayEventListener joinedListener{
        .structSize = D2RL::Lifecycle::GameplayEventListenerSize,
        .flags = 0,
        .kind = D2RL::Lifecycle::GameplayEventKind::GameJoined,
        .reserved = 0,
        .callback = OnGameplayEvent,
        .userData = nullptr,
    };
    const auto joinedResult = LifecycleService->registerGameplayEventListener(
        context, &joinedListener, &GameJoinedHandle);
    if (joinedResult != D2RL::Lifecycle::Result::Success
            || GameJoinedHandle == D2RL::Lifecycle::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: GameJoined listener registration failed (result=%u, handle=%llu).",
            static_cast<unsigned>(joinedResult),
            static_cast<unsigned long long>(GameJoinedHandle));
        return false;
    }

    const D2RL::Lifecycle::GameplayEventListener leftListener{
        .structSize = D2RL::Lifecycle::GameplayEventListenerSize,
        .flags = 0,
        .kind = D2RL::Lifecycle::GameplayEventKind::GameLeft,
        .reserved = 0,
        .callback = OnGameplayEvent,
        .userData = nullptr,
    };
    const auto leftResult = LifecycleService->registerGameplayEventListener(
        context, &leftListener, &GameLeftHandle);
    if (leftResult != D2RL::Lifecycle::Result::Success
            || GameLeftHandle == D2RL::Lifecycle::InvalidHandle) {
        D2RL::LogWarnF(context,
            "MapSeedSelector: GameLeft listener registration failed (result=%u, handle=%llu).",
            static_cast<unsigned>(leftResult),
            static_cast<unsigned long long>(GameLeftHandle));
        return false;
    }

    Operational.store(true, std::memory_order_release);
    if (!StartAddSeedKeyboard()) {
        context->LogWarn("MapSeedSelector: the Add Seed keyboard hook could not be started; text input and key handling will be unavailable.");
    }
    return true;
}

void __cdecl RegisterDeferredSprites(const D2RL::PluginContext* context,
        void*) noexcept {
    UiRegistrationPending.store(false, std::memory_order_release);
    if (context == nullptr || context != Context
            || !MapSeedSelector::IsAvailable()
            || !Operational.load(std::memory_order_acquire)
            || NativeDropdownAvailable) {
        return;
    }
    if (!RegisterCompositeSprites(context)) {
        MapSeedSelector::SetOfflineFlowActive(false);
        DesiredPanelOpen.store(false, std::memory_order_release);
        Operational.store(false, std::memory_order_release);
        context->LogWarn("MapSeedSelector: composed dropdown artwork could not be registered; configured seeds remain inactive.");
        return;
    }
    FallbackArtworkReady.store(true, std::memory_order_release);
    context->LogInfo("MapSeedSelector: composed D2R dropdown artwork registered; panel is ready.");
    if (DesiredPanelOpen.load(std::memory_order_acquire)) {
        SchedulePanelSync();
    }
}

void __cdecl OnDataTablesLoaded(const D2RL::PluginContext* context,
        const D2RL::Lifecycle::DataTablesLoadedEvent* event,
        void*) noexcept {
    if (context == nullptr || context != Context || event == nullptr
            || !D2RL::Lifecycle::HasDataTablesLoadedEventField(event,
                D2RL::Lifecycle::DataTablesLoadedEventRequiredSize)
            || NativeDropdownAvailable
            || FallbackArtworkReady.load(std::memory_order_acquire)
            || UiRegistrationPending.exchange(true,
                std::memory_order_acq_rel)) {
        return;
    }

    std::string error;
    if (!MapSeedSelector::NativeDropdownArt::Load(context,
            CompositeSprites, error)) {
        UiRegistrationPending.store(false, std::memory_order_release);
        MapSeedSelector::SetOfflineFlowActive(false);
        DesiredPanelOpen.store(false, std::memory_order_release);
        Operational.store(false, std::memory_order_release);
        D2RL::LogWarnF(context,
            "MapSeedSelector: could not assemble native dropdown artwork; picker disabled: %s",
            error.c_str());
        return;
    }

    const auto scheduleResult = ThreadService != nullptr
            && ThreadService->runOnUiThread != nullptr
        ? ThreadService->runOnUiThread(context, RegisterDeferredSprites,
            nullptr)
        : D2RL::Threads::Result::Unavailable;
    if (scheduleResult != D2RL::Threads::Result::Success) {
        CompositeSprites = {};
        UiRegistrationPending.store(false, std::memory_order_release);
        MapSeedSelector::SetOfflineFlowActive(false);
        DesiredPanelOpen.store(false, std::memory_order_release);
        Operational.store(false, std::memory_order_release);
        D2RL::LogWarnF(context,
            "MapSeedSelector: could not queue dropdown artwork registration on the UI thread (result=%u).",
            static_cast<unsigned>(scheduleResult));
    }
}

} // namespace

D2RL_PLUGIN_EXPORT auto D2RLoaderGetPluginInfo() noexcept
        -> const D2RL::PluginInfo* {
    return &Info;
}

D2RL_PLUGIN_EXPORT auto D2RLoaderLoadPlugin(
        const D2RL::PluginContext* context) noexcept -> bool {
    if (!D2RL::HasContext(context)
            || context->abiVersion != D2RL_PLUGIN_ABI_VERSION) {
        return false;
    }
    Context = context;
    NativeDropdownAvailable = false;
    if (!context->EnsureConfig(DefaultConfig)) {
        context->LogError("MapSeedSelector: configuration could not be initialized.");
        return false;
    }
    if (!QueryServices(context)) {
        context->LogWarn("MapSeedSelector: required D2RLoader services are unavailable; the picker is disabled.");
        return true;
    }
    if (!MapSeedSelector::Initialize(context)) {
        context->LogError("MapSeedSelector: seed selector initialization failed.");
        return false;
    }
    if (!MapSeedSelector::IsAvailable()) {
        context->LogInfo("MapSeedSelector: add at least one [[seeds]] entry to the plugin TOML to enable the picker.");
        return true;
    }
    if (NativeDropdownAvailable) {
        if (!RegisterUi(context)) {
            UnregisterUi();
            MapSeedSelector::SetOfflineFlowActive(false);
            context->LogWarn("MapSeedSelector: the seed picker panel could not be registered; configured seeds remain inactive.");
            return true;
        }
        context->LogInfo("MapSeedSelector: native seed dropdown registered for the Offline character-select flow.");
        return true;
    }

    const D2RL::Lifecycle::DataTablesLoadedListener dataTablesListener{
        .structSize = D2RL::Lifecycle::DataTablesLoadedListenerSize,
        .flags = 0,
        .callback = OnDataTablesLoaded,
        .userData = nullptr,
    };
    if (LifecycleService->registerDataTablesLoadedListener(context,
            &dataTablesListener, &DataTablesLoadedHandle)
            != D2RL::Lifecycle::Result::Success
            || DataTablesLoadedHandle == D2RL::Lifecycle::InvalidHandle) {
        MapSeedSelector::SetOfflineFlowActive(false);
        context->LogWarn("MapSeedSelector: the game-art loading callback could not be registered; configured seeds remain inactive.");
        return true;
    }

    if (!RegisterUi(context)) {
        UnregisterUi();
        MapSeedSelector::SetOfflineFlowActive(false);
        context->LogWarn("MapSeedSelector: the fallback picker panel could not be registered during plugin load; configured seeds remain inactive.");
        return true;
    }

    context->LogInfo("MapSeedSelector: waiting for game data before composing the fallback dropdown artwork.");
    return true;
}

D2RL_PLUGIN_EXPORT void D2RLoaderUnloadPlugin() noexcept {
    MapSeedSelector::SetOfflineFlowActive(false);
    UnregisterUi();
    MapSeedSelector::Shutdown();
    LayoutJson.clear();
    ThreadService = nullptr;
    ResourceService = nullptr;
    PanelService = nullptr;
    SharedEventService = nullptr;
    WidgetService = nullptr;
    LifecycleService = nullptr;
    Context = nullptr;
}

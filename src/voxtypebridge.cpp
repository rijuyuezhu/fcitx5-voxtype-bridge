#include "voxtypebridge.h"
#include "clipboard_public.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/keysym.h"
#include "fcitx-utils/log.h"
#include "fcitx/addonfactory.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/candidatelist.h"
#include "fcitx/event.h"
#include "fcitx/inputcontext.h"
#include "fcitx/inputcontextmanager.h"
#include "fcitx/inputpanel.h"
#include "fcitx/instance.h"
#include "fcitx/text.h"
#include "fcitx/userinterface.h"
#include <fcntl.h>
#include <fstream>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>
extern char **environ;

namespace fcitx::voxtypebridge {

FCITX_DEFINE_LOG_CATEGORY(voxtypebridge, "voxtypebridge");

namespace {

// using spawn in multithreaded program
bool executeCommand(const std::string &command) {
    VOXTYPE_DEBUG() << "Executing command: " << command;

    pid_t pid;
    int status = 0;

    const char *argv[] = {"sh", "-c", command.c_str(), nullptr};

    posix_spawn_file_actions_t file_actions;
    posix_spawn_file_actions_init(&file_actions);

    int devNullFd = open("/dev/null", O_WRONLY);
    if (devNullFd >= 0) {
        posix_spawn_file_actions_adddup2(&file_actions, devNullFd,
                                         STDOUT_FILENO);
        posix_spawn_file_actions_addclose(&file_actions, devNullFd);
    }

    int ret = posix_spawnp(&pid, "sh", &file_actions, nullptr,
                           const_cast<char *const *>(argv), environ);

    posix_spawn_file_actions_destroy(&file_actions);

    if (ret != 0) {
        VOXTYPE_ERROR() << "posix_spawnp failed: " << ret;
        if (devNullFd >= 0)
            close(devNullFd);
        return false;
    }

    if (devNullFd >= 0)
        close(devNullFd);

    if (waitpid(pid, &status, 0) == -1) {
        VOXTYPE_ERROR() << "waitpid failed";
        return false;
    }

    VOXTYPE_DEBUG() << "Command executed, status: " << status;

    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
std::string readFromFile(const std::string &resultPath) {
    std::ifstream file(resultPath);
    if (!file.is_open()) {
        VOXTYPE_ERROR() << "Failed to open file: " << resultPath;
        return "";
    }

    std::string result((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();

    std::ofstream clearFile(resultPath, std::ios::trunc);
    clearFile.close();

    while (!result.empty() &&
           (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }

    return result;
}
KeyState modifierKeyToState(VoiceModifierKey key) {
    switch (key) {
    case VoiceModifierKey::Shift:
        return KeyState::Shift;
    case VoiceModifierKey::Ctrl:
        return KeyState::Ctrl;
    case VoiceModifierKey::Alt:
        return KeyState::Alt;
    case VoiceModifierKey::Super:
        return KeyState::Super;
    default:
        return KeyState::NoState;
    }
}
std::string modifierToString(VoiceModifierKey key) {
    switch (key) {
    case VoiceModifierKey::Shift:
        return "Shift";
    case VoiceModifierKey::Ctrl:
        return "Ctrl";
    case VoiceModifierKey::Alt:
        return "Alt";
    case VoiceModifierKey::Super:
        return "Super";
    default:
        return "";
    }
}

bool detectHasModifier(const Key &key, VoiceModifierKey modifierKey) {
    auto state = modifierKeyToState(modifierKey);
    if (state == KeyState::NoState) {
        return false;
    }
    return key.states().test(state);
}
} // namespace

void VoxtypebridgeState::reset(InputContext *ic) {
    if (!isIdle()) {
        // cancel the recording process.
        std::thread([cancelCommand = cancelCommand_] {
            executeCommand(std::move(cancelCommand));
        }).detach();
    }
    recordingStage_ = RecordingStage::Idle;
    recordingStageId_ += 1;
    ic->inputPanel().reset();
    ic->updatePreedit();
    ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}
void VoxtypebridgeState::chooseCommand(bool M1Pressed, bool M2Pressed,
                                       bool isEdit) {
    M1Pressed_ = M1Pressed;
    M2Pressed_ = M2Pressed;
    isEdit_ = isEdit;
    auto &config = q_->config();
    int sel = static_cast<int>(M1Pressed) | static_cast<int>(M2Pressed) << 1 |
              static_cast<int>(isEdit) << 2;
    switch (sel) {
    case 0b000:
        startCommand_ = config.voiceCommandStart.value();
        stopCommand_ = config.voiceCommandStop.value();
        cancelCommand_ = config.voiceCommandCancel.value();
        break;
    case 0b001:
        startCommand_ = config.voiceCommandM1Start.value();
        stopCommand_ = config.voiceCommandM1Stop.value();
        cancelCommand_ = config.voiceCommandM1Cancel.value();
        break;
    case 0b010:
        startCommand_ = config.voiceCommandM2Start.value();
        stopCommand_ = config.voiceCommandM2Stop.value();
        cancelCommand_ = config.voiceCommandM2Cancel.value();
        break;
    case 0b011:
        startCommand_ = config.voiceCommandM1M2Start.value();
        stopCommand_ = config.voiceCommandM1M2Stop.value();
        cancelCommand_ = config.voiceCommandM1M2Cancel.value();
        break;
    case 0b100:
        startCommand_ = config.voiceEditCommandStart.value();
        stopCommand_ = config.voiceEditCommandStop.value();
        cancelCommand_ = config.voiceEditCommandCancel.value();
        break;
    case 0b101:
        startCommand_ = config.voiceEditCommandM1Start.value();
        stopCommand_ = config.voiceEditCommandM1Stop.value();
        cancelCommand_ = config.voiceEditCommandM1Cancel.value();
        break;
    case 0b110:
        startCommand_ = config.voiceEditCommandM2Start.value();
        stopCommand_ = config.voiceEditCommandM2Stop.value();
        cancelCommand_ = config.voiceEditCommandM2Cancel.value();
        break;
    case 0b111:
        startCommand_ = config.voiceEditCommandM1M2Start.value();
        stopCommand_ = config.voiceEditCommandM1M2Stop.value();
        cancelCommand_ = config.voiceEditCommandM1M2Cancel.value();
        break;
    default:
        throw std::logic_error("Invalid command selection");
    }
}

Voxtypebridge::Voxtypebridge(Instance *instance)
    : instance_(instance), factory_([this](InputContext &) {
          return new VoxtypebridgeState(this);
      }) {
    VOXTYPE_DEBUG() << "Initializing voxtypebridge addon";
    instance_->inputContextManager().registerProperty("voxtypebridgeState",
                                                      &factory_);

    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::Default,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            if (keyEvent.isRelease()) {
                return;
            }
            auto [is_hotkey_matched, is_edit_mode] = check_hotkey(keyEvent);
            if (is_hotkey_matched) {
                VOXTYPE_DEBUG()
                    << "Hotkey matched, start recording. isEditMode: "
                    << is_edit_mode;
                start_recording(keyEvent.inputContext(), keyEvent,
                                is_edit_mode);
                keyEvent.filterAndAccept();
                return;
            }
        }));

    auto reset = [this](Event &event) {
        auto &icEvent = static_cast<InputContextEvent &>(event);
        auto *state = icEvent.inputContext()->propertyFor(&factory_);
        state->reset(icEvent.inputContext());
    };
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextFocusOut, EventWatcherPhase::Default, reset));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextReset, EventWatcherPhase::Default, reset));
    eventHandlers_.emplace_back(
        instance_->watchEvent(EventType::InputContextSwitchInputMethod,
                              EventWatcherPhase::Default, reset));
    eventHandlers_.emplace_back(instance_->watchEvent(
        EventType::InputContextKeyEvent, EventWatcherPhase::PreInputMethod,
        [this](Event &event) {
            auto &keyEvent = static_cast<KeyEvent &>(event);
            auto *ic = keyEvent.inputContext();
            auto *state = ic->propertyFor(&factory_);
            if (state->isIdle()) {
                return;
            }
            VOXTYPE_DEBUG()
                << "In non-idle stage, checking key event for recording "
                   "control. Key: "
                << keyEvent.key() << ", isRelease: " << keyEvent.isRelease();

            // make sure no one else will handle it
            keyEvent.filter();

            bool shallStop =
                std::get<0>(check_hotkey(keyEvent)) &&
                ((keyEvent.isRelease() && config_.voiceInputMode.value() ==
                                              VoiceInputMode::PushToTalk) ||
                 (!keyEvent.isRelease() &&
                  config_.voiceInputMode.value() == VoiceInputMode::Trigger));

            if (shallStop) {
                stop_recording(ic);
                keyEvent.accept();
                return;
            }

            // for functionality keys: only detect press, and by pass all
            // modifier

            if (keyEvent.isRelease() || keyEvent.key().isModifier() ||
                keyEvent.key().hasModifier()) {
                return;
            }
            if (keyEvent.key().check(FcitxKey_Escape)) {
                keyEvent.accept();
                state->reset(ic);
                return;
            }
            keyEvent.accept();

            updateUI(ic);
        }));
    reloadConfig();
}

Voxtypebridge::~Voxtypebridge() {}

void Voxtypebridge::updateUI(InputContext *ic) {
    ic->inputPanel().reset();
    auto *state = ic->propertyFor(&factory_);

    Text auxUp(_(state->displayText_));
    ic->inputPanel().setAuxUp(auxUp);
    ic->updatePreedit();
    ic->updateUserInterface(UserInterfaceComponent::InputPanel);
}

void Voxtypebridge::start_recording(InputContext *ic, KeyEvent &keyEvent,
                                    bool isEdit) {
    auto *state = ic->propertyFor(&factory_);
    if (!state->isIdle()) {
        VOXTYPE_DEBUG() << "Not in idle stage, ignore start recording";
        return;
    }
    Key key = keyEvent.key();
    bool M1Pressed = detectHasModifier(key, config_.voiceModifier_M1.value());
    bool M2Pressed = detectHasModifier(key, config_.voiceModifier_M2.value());
    state->chooseCommand(M1Pressed, M2Pressed, isEdit);
    VOXTYPE_DEBUG() << "Modifier M1: " << M1Pressed
                    << ", Modifier M2: " << M2Pressed << ", isEdit: " << isEdit;
    VOXTYPE_DEBUG() << "Selected command to start: " << state->startCommand_;

    state->recordingStage_ = RecordingStage::Recording;
    state->recordingStageId_ += 1;
    state->displayText_ = getDisplayText(isEdit, M1Pressed, M2Pressed);
    updateUI(ic);
    if (isEdit) {
        storeEditText(ic);
    }
    std::thread([startCommand = state->startCommand_] {
        executeCommand(std::move(startCommand));
    }).detach();
}

void Voxtypebridge::stop_recording(InputContext *ic) {
    auto *state = ic->propertyFor(&factory_);
    if (!state->isRecording()) {
        VOXTYPE_DEBUG() << "Not in recording stage, ignore stop recording";
        return;
    }
    VOXTYPE_DEBUG() << "Selected command to stop: " << state->stopCommand_;
    state->recordingStage_ = RecordingStage::Processing;
    std::uint64_t currentRecordingStageId = state->recordingStageId_;
    state->displayText_ = _(config_.voiceProcessingText.value());
    updateUI(ic);

    auto &dispatcher = instance()->eventDispatcher();
    auto icRef = ic->watch();

    std::thread([this, stopCommand = state->stopCommand_, icRef, &dispatcher,
                 currentRecordingStageId] {
        executeCommand(std::move(stopCommand));

        dispatcher.scheduleWithContext(icRef, [this, icRef,
                                               currentRecordingStageId] {
            if (auto *ic = icRef.get()) {
                auto *state = ic->propertyFor(&factory_);
                if (state->isProcessing() &&
                    state->recordingStageId_ == currentRecordingStageId) {
                    state->recordingStage_ = RecordingStage::Idle;
                    std::string result =
                        readFromFile(config().voiceResultPath.value());
                    VOXTYPE_DEBUG() << "Read result: " << result;
                    ic->commitString(result);
                    state->reset(ic);
                } else {
                    VOXTYPE_DEBUG()
                        << "Input context is not in expected state, ignore "
                           "result"
                        << ", isProcessing: " << state->isProcessing()
                        << ", recordingStageId: " << state->recordingStageId_
                        << ", expected recordingStageId: "
                        << currentRecordingStageId;
                }
            }
        });
    }).detach();
}

auto Voxtypebridge::check_hotkey(const KeyEvent &keyEvent)
    -> std::pair<bool, bool> {
    Key key_without_modifier{keyEvent.key().sym(), KeyState()};
    if (key_without_modifier.checkKeyList(config_.voiceInputHotkey.value())) {
        return {true, false};
    } else if (key_without_modifier.checkKeyList(
                   config_.voiceInputEditkey.value())) {
        return {true, true};
    } else {
        return {false, false};
    }
}
void Voxtypebridge::storeEditText(InputContext *ic) {
    std::string editText = getEditText(ic);
    std::string editTextStorePath = config_.voiceEditTextStorePath.value();
    VOXTYPE_DEBUG() << "Edit text: " << editText;
    std::ofstream storeFile(editTextStorePath);
    if (storeFile.is_open()) {
        storeFile << editText;
        storeFile.close();
    } else {
        VOXTYPE_DEBUG() << "Failed to open edit text store file: "
                        << editTextStorePath;
    }
}

std::string Voxtypebridge::getEditText(fcitx::InputContext *ic) {
    auto *clipboard = this->clipboard();
    if (!clipboard) {
        return {};
    }

    std::string text = clipboard->call<fcitx::IClipboard::primary>(ic);

    if (text.empty()) {
        text = clipboard->call<fcitx::IClipboard::clipboard>(ic);
    }

    return text;
}

std::string Voxtypebridge::getDisplayText(bool isEdit, bool M1Pressed,
                                          bool M2Pressed) {

    std::string display = isEdit ? config_.voiceEditRecordingText.value()
                                 : config_.voiceRecordingText.value();
    std::vector<std::string> modifiers;
    if (M1Pressed) {
        modifiers.push_back(modifierToString(config_.voiceModifier_M1.value()));
    }
    if (M2Pressed) {
        modifiers.push_back(modifierToString(config_.voiceModifier_M2.value()));
    }
    // delete all empty strings
    modifiers.erase(
        std::remove_if(modifiers.begin(), modifiers.end(),
                       [](const std::string &s) { return s.empty(); }),
        modifiers.end());
    if (!modifiers.empty()) {
        std::sort(modifiers.begin(), modifiers.end());
        display += " (";
        for (const auto &mod : modifiers) {
            display += mod + "+";
        }
        display.back() = ')';
    }

    return display;
}

class VoxtypebridgeModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Voxtypebridge(manager->instance());
    }
};
} // namespace fcitx::voxtypebridge

FCITX_ADDON_FACTORY_V2(voxtypebridge,
                       fcitx::voxtypebridge::VoxtypebridgeModuleFactory);

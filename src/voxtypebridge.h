#ifndef _FCITX_VOXTYPEBRIDGE_H_
#define _FCITX_VOXTYPEBRIDGE_H_

#include "fcitx-config/configuration.h"
#include "fcitx-config/iniparser.h"
#include "fcitx-config/option.h"
#include "fcitx-config/rawconfig.h"
#include "fcitx-utils/handlertable.h"
#include "fcitx-utils/i18n.h"
#include "fcitx-utils/key.h"
#include "fcitx-utils/log.h"
#include "fcitx/addoninstance.h"
#include "fcitx/addonmanager.h"
#include "fcitx/inputcontextproperty.h"
#include "fcitx/instance.h"
#include <memory>
#include <string>
#include <vector>

namespace fcitx::voxtypebridge {

enum class VoiceModifierKey { None, Shift, Ctrl, Alt, Super };
FCITX_CONFIG_ENUM_NAME_WITH_I18N(VoiceModifierKey, N_("None"), N_("Shift"),
                                 N_("Ctrl"), N_("Alt"), N_("Super"));

enum class VoiceInputMode { PushToTalk, Trigger };

FCITX_CONFIG_ENUM_NAME_WITH_I18N(VoiceInputMode, N_("Push to talk"),
                                 N_("Trigger"));

FCITX_CONFIGURATION(
    VoxtypebridgeConfig,

    Option<KeyList> voiceInputHotkey{this, "VoiceInputHotkey",
                                     _("Voice Input Hotkey"),
                                     fcitx::KeyList{fcitx::Key("F9")}};

    Option<KeyList> voiceInputEditkey{this, "VoiceInputEditkey",
                                      _("Voice Input Edit Key"),
                                      fcitx::KeyList{fcitx::Key("F10")}};

    OptionWithAnnotation<VoiceInputMode, VoiceInputModeI18NAnnotation>
        voiceInputMode{this, "VoiceInputMode", _("Voice Input Mode"),
                       VoiceInputMode::Trigger};

    OptionWithAnnotation<VoiceModifierKey, VoiceModifierKeyI18NAnnotation>
        voiceModifier_M1{this, "VoiceModifier_M1", _("Voice Modifier M1"),
                         VoiceModifierKey::Shift};
    OptionWithAnnotation<VoiceModifierKey, VoiceModifierKeyI18NAnnotation>
        voiceModifier_M2{this, "VoiceModifier_M2", _("Voice Modifier M2"),
                         VoiceModifierKey::Ctrl};

    Option<std::string> voiceCommandStart{
        this, "voiceCommandStart", _("Voice Command Start"),
        "voxtype record start --file /tmp/voxtype-result --wait-till-idle"};
    Option<std::string> voiceCommandStop{
        this, "voiceCommandStop", _("Voice Command Stop"),
        "voxtype record stop --wait-till-idle"};
    Option<std::string> voiceCommandCancel{this, "voiceCommandCancel",
                                           _("Voice Command Cancel"),
                                           "voxtype record cancel"};

    Option<std::string> voiceCommandM1Start{
        this, "voiceCommandM1Start", _("Voice Command M1 Start"),
        "voxtype record start --file /tmp/voxtype-result "
        "--complex-post-process --wait-till-idle"};
    Option<std::string> voiceCommandM1Stop{
        this, "voiceCommandM1Stop", _("Voice Command M1 Stop"),
        "voxtype record stop --wait-till-idle"};
    Option<std::string> voiceCommandM1Cancel{this, "voiceCommandM1Cancel",
                                             _("Voice Command M1 Cancel"),
                                             "voxtype record cancel"};

    Option<std::string> voiceCommandM2Start{
        this, "voiceCommandM2Start", _("Voice Command M2 Start"),
        "voxtype record start --file /tmp/voxtype-result --model base.en "
        "--wait-till-idle"};
    Option<std::string> voiceCommandM2Stop{
        this, "voiceCommandM2Stop", _("Voice Command M2 Stop"),
        "voxtype record stop --wait-till-idle"};
    Option<std::string> voiceCommandM2Cancel{this, "voiceCommandM2Cancel",
                                             _("Voice Command M2 Cancel"),
                                             "voxtype record cancel"};

    Option<std::string> voiceCommandM1M2Start{
        this, "voiceCommandM1M2Start", _("Voice Command M1M2 Start"),
        "voxtype record start --file /tmp/voxtype-result --model base.en "
        "--complex-post-process --wait-till-idle"};
    Option<std::string> voiceCommandM1M2Stop{
        this, "voiceCommandM1M2Stop", _("Voice Command M1M2 Stop"),
        "voxtype record stop --wait-till-idle"};
    Option<std::string> voiceCommandM1M2Cancel{this, "voiceCommandM1M2Cancel",
                                               _("Voice Command M1M2 Cancel"),
                                               "voxtype record cancel"};

    Option<std::string> voiceEditCommandStart{
        this, "voiceEditCommandStart", _("Voice Edit Command Start"),
        "voxtype record start --file /tmp/voxtype-result --edit "
        "--edit-input-file /tmp/voxtype-edit-text --wait-till-idle"};
    Option<std::string> voiceEditCommandStop{
        this, "voiceEditCommandStop", _("Voice Edit Command Stop"),
        "voxtype record stop --wait-till-idle"};
    Option<std::string> voiceEditCommandCancel{this, "voiceEditCommandCancel",
                                               _("Voice Edit Command Cancel"),
                                               "voxtype record cancel"};

    Option<std::string> voiceEditCommandM1Start{
        this, "voiceEditCommandM1Start", _("Voice Edit Command M1 Start"),
        "voxtype record start --file /tmp/voxtype-result "
        "--complex-post-process --edit --edit-input-file "
        "/tmp/voxtype-edit-text --wait-till-idle"};
    Option<std::string> voiceEditCommandM1Stop{
        this, "voiceEditCommandM1Stop", _("Voice Edit Command M1 Stop"),
        "voxtype record stop --wait-till-idle"};
    Option<std::string> voiceEditCommandM1Cancel{
        this, "voiceEditCommandM1Cancel", _("Voice Edit Command M1 Cancel"),
        "voxtype record cancel"};

    Option<std::string> voiceEditCommandM2Start{
        this, "voiceEditCommandM2Start", _("Voice Edit Command M2 Start"),
        "voxtype record start --file /tmp/voxtype-result --model base.en "
        "--edit --edit-input-file /tmp/voxtype-edit-text --wait-till-idle"};
    Option<std::string> voiceEditCommandM2Stop{
        this, "voiceEditCommandM2Stop", _("Voice Edit Command M2 Stop"),
        "voxtype record stop --wait-till-idle"};
    Option<std::string> voiceEditCommandM2Cancel{
        this, "voiceEditCommandM2Cancel", _("Voice Edit Command M2 Cancel"),
        "voxtype record cancel"};

    Option<std::string> voiceEditCommandM1M2Start{
        this, "voiceEditCommandM1M2Start", _("Voice Edit Command M1M2 Start"),
        "voxtype record start --file /tmp/voxtype-result --model base.en "
        "--complex-post-process --edit --edit-input-file "
        "/tmp/voxtype-edit-text --wait-till-idle"};
    Option<std::string> voiceEditCommandM1M2Stop{
        this, "voiceEditCommandM1M2Stop", _("Voice Edit Command M1M2 Stop"),
        "voxtype record stop --wait-till-idle"};
    Option<std::string> voiceEditCommandM1M2Cancel{
        this, "voiceEditCommandM1M2Cancel", _("Voice Edit Command M1M2 Cancel"),
        "voxtype record cancel"};

    Option<std::string> voiceResultPath{
        this, "voiceResultPath", _("Voice Result Path"), "/tmp/voxtype-result"};
    Option<std::string> voiceEditTextStorePath{this, "voiceEditTextStorePath",
                                               _("Voice Edit Text Store Path"),
                                               "/tmp/voxtype-edit-text"};
    Option<std::string> voiceRecordingText{
        this, "voiceRecordingText", _("Voice Recording Text"), "🎤 录音中..."};
    Option<std::string> voiceEditRecordingText{this, "voiceEditRecordingText",
                                               _("Voice Edit Recording Text"),
                                               "✍🏻 语音编辑中..."};
    Option<std::string> voiceProcessingText{this, "voiceProcessingText",
                                            _("Voice Processing Text"),
                                            "⏳ 处理中..."};);
class VoxtypebridgeState;
class Voxtypebridge final : public AddonInstance {
    static constexpr char configFile[] = "conf/voxtypebridge.conf";

  public:
    Voxtypebridge(Instance *instance);
    ~Voxtypebridge();

    Instance *instance() { return instance_; }

    void updateUI(InputContext *ic);
    auto &factory() { return factory_; }

    void reloadConfig() override { readAsIni(config_, configFile); }

    const Configuration *getConfig() const override { return &config_; }
    void setConfig(const RawConfig &config) override {
        config_.load(config, true);
        safeSaveAsIni(config_, configFile);
    }

    const auto &config() const { return config_; }
    void sendNotification(const std::string &summary, const std::string &message);

  private:
    void start_recording(InputContext *ic, KeyEvent &keyEvent, bool isEdit);
    void stop_recording(InputContext *ic);
    void cancel_recording(InputContext *ic);
    // return value: <is_hotkey_matched, is_edit_mode>
    auto check_hotkey(const KeyEvent &keyEvent) -> std::pair<bool, bool>;
    void storeEditText(InputContext *ic);
    std::string getEditText(InputContext *ic);
    std::string getDisplayText(bool isEdit, bool M1Pressed, bool M2Pressed);

    Instance *instance_;
    std::vector<std::unique_ptr<fcitx::HandlerTableEntry<fcitx::EventHandler>>>
        eventHandlers_;
    VoxtypebridgeConfig config_;
    FactoryFor<VoxtypebridgeState> factory_;
    FCITX_ADDON_DEPENDENCY_LOADER(clipboard, instance_->addonManager());
    FCITX_ADDON_DEPENDENCY_LOADER(notifications, instance_->addonManager());
};

enum class RecordingStage {
    Idle,
    Recording,
    Processing,
};

class VoxtypebridgeState : public InputContextProperty {
  public:
    VoxtypebridgeState(Voxtypebridge *q) : q_(q) {}
    void reset(InputContext *ic);
    void chooseCommand(bool M1Pressed, bool M2Pressed, bool isEdit);
    bool isIdle() const { return recordingStage_ == RecordingStage::Idle; }
    bool isRecording() const {
        return recordingStage_ == RecordingStage::Recording;
    }
    bool isProcessing() const {
        return recordingStage_ == RecordingStage::Processing;
    }

    RecordingStage recordingStage_{RecordingStage::Idle};
    std::uint64_t recordingStageId_{0};

    Voxtypebridge *q_;

    bool M1Pressed_{false};
    bool M2Pressed_{false};
    bool isEdit_{false};
    std::string startCommand_;
    std::string stopCommand_;
    std::string cancelCommand_;
    std::string displayText_;
};
FCITX_DECLARE_LOG_CATEGORY(voxtypebridge);

#define VOXTYPE_DEBUG() FCITX_LOGC(::fcitx::voxtypebridge::voxtypebridge, Debug)
#define VOXTYPE_INFO() FCITX_LOGC(::fcitx::voxtypebridge::voxtypebridge, Info)
#define VOXTYPE_WARN()                                                         \
    FCITX_LOGC(::fcitx::voxtypebridge::voxtypebridge, Warning)
#define VOXTYPE_ERROR() FCITX_LOGC(::fcitx::voxtypebridge::voxtypebridge, Error)
} // namespace fcitx::voxtypebridge

#endif // _FCITX_VOXTYPEBRIDGE_H_

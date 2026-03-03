#include "voxtypebridge.h"
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
#include <string>

namespace fcitx::voxtypebridge {

FCITX_DEFINE_LOG_CATEGORY(voxtypebridge, "voxtypebridge");

class VoxtypebridgeState : public InputContextProperty {
  public:
    VoxtypebridgeState(Voxtypebridge *q) : q_(q) {}

    bool enabled_ = false;
    Voxtypebridge *q_;

    void reset(InputContext *ic) {
        enabled_ = false;
        ic->inputPanel().reset();
        ic->updatePreedit();
        ic->updateUserInterface(UserInterfaceComponent::InputPanel);
    }
};

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
            if (keyEvent.key().checkKeyList(config_.voiceInputHotkey.value())) {
                trigger(keyEvent.inputContext());
                keyEvent.filterAndAccept();
                return;
            }
        }));

    auto reset = [this](Event &event) {
        auto &icEvent = static_cast<InputContextEvent &>(event);
        auto *state = icEvent.inputContext()->propertyFor(&factory_);
        if (state->enabled_) {
            state->reset(icEvent.inputContext());
        }
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
            auto *inputContext = keyEvent.inputContext();
            auto *state = inputContext->propertyFor(&factory_);
            if (!state->enabled_) {
                return;
            }

            // make sure no one else will handle it
            keyEvent.filter();
            if (keyEvent.isRelease()) {
                return;
            }

            auto candidateList = inputContext->inputPanel().candidateList();
            if (candidateList) {
                int idx = keyEvent.key().digitSelection();
                if (idx >= 0) {
                    keyEvent.accept();
                    if (idx < candidateList->size()) {
                        candidateList->candidate(idx).select(inputContext);
                    }
                    return;
                }
                if (keyEvent.key().check(FcitxKey_space) ||
                    keyEvent.key().check(FcitxKey_Return) ||
                    keyEvent.key().check(FcitxKey_KP_Enter)) {
                    keyEvent.accept();
                    if (!candidateList->empty() &&
                        candidateList->cursorIndex() >= 0) {
                        candidateList->candidate(candidateList->cursorIndex())
                            .select(inputContext);
                    }
                    return;
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevPage())) {
                    auto *pageable = candidateList->toPageable();
                    if (!pageable->hasPrev()) {
                        if (pageable->usedNextBefore()) {
                            event.accept();
                            return;
                        }
                    } else {
                        event.accept();
                        pageable->prev();
                        inputContext->updateUserInterface(
                            UserInterfaceComponent::InputPanel);
                        return;
                    }
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultNextPage())) {
                    keyEvent.filterAndAccept();
                    candidateList->toPageable()->next();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultPrevCandidate())) {
                    keyEvent.filterAndAccept();
                    candidateList->toCursorMovable()->prevCandidate();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }

                if (keyEvent.key().checkKeyList(
                        instance_->globalConfig().defaultNextCandidate())) {
                    keyEvent.filterAndAccept();
                    candidateList->toCursorMovable()->nextCandidate();
                    inputContext->updateUserInterface(
                        UserInterfaceComponent::InputPanel);
                    return;
                }
            }

            // and by pass all modifier
            if (keyEvent.key().isModifier() || keyEvent.key().hasModifier()) {
                return;
            }
            if (keyEvent.key().check(FcitxKey_Escape)) {
                keyEvent.accept();
                state->reset(inputContext);
                return;
            }
            if (keyEvent.key().check(FcitxKey_Delete) ||
                keyEvent.key().check(FcitxKey_BackSpace)) {
                keyEvent.accept();
                state->reset(inputContext);
                return;
            }
            event.accept();

            updateUI(inputContext);
        }));
    reloadConfig();
}

Voxtypebridge::~Voxtypebridge() {}

void Voxtypebridge::trigger(InputContext *inputContext) {
    auto *state = inputContext->propertyFor(&factory_);
    state->enabled_ = true;
    updateUI(inputContext);
}
void Voxtypebridge::updateUI(InputContext *inputContext) {
    inputContext->inputPanel().reset();

    Text auxUp(_("Voxtypebridge (Press BackSpace/Delete to clear history):"));
    inputContext->inputPanel().setAuxUp(auxUp);
    inputContext->updatePreedit();
    inputContext->updateUserInterface(UserInterfaceComponent::InputPanel);
}

class VoxtypebridgeModuleFactory : public AddonFactory {
    AddonInstance *create(AddonManager *manager) override {
        return new Voxtypebridge(manager->instance());
    }
};
} // namespace fcitx::voxtypebridge

FCITX_ADDON_FACTORY_V2(voxtypebridge,
                       fcitx::voxtypebridge::VoxtypebridgeModuleFactory);

#include <cw.h>

#include <ranges>

#include <rhatt/prelude.h>

#include <rhatt/base/Singleton.hpp>

#include <horrible/OptionalAPI.hpp>

#include <Geode/Geode.hpp>

#include <Geode/modify/PlayLayer.hpp>

using namespace geode::prelude;
using namespace cw::notifs;

static std::vector<std::weak_ptr<Hook>> NOTIF_HOOKS;  // Array of registered notifications hooks
static bool HORRIBLE_OPT_ENABLED = true;              // cuz geode event apis are acting gay...

namespace cw {
    static void showNotif(const Notif* notif, CCNode* to, Function<void(NotifNode*)> finished) {
        auto const size = CCDirector::sharedDirector()->getWinSize();

        auto x = size.width / 2.0f;

        auto notifNode = NotifNode::create(notif, false);
        notifNode->setScale(0.875f);

        auto y = size.height + notifNode->getScaledContentHeight();
        auto yDown = size.height - (0.625f * notifNode->getScaledContentHeight());

        notifNode->setPosition({ x, y });

        to->addChild(notifNode, CW_HIGHEST_Z);

        notifNode->runAction(
            CCSequence::create(
                CCEaseExponentialOut::create(CCMoveTo::create(0.375f, { x, yDown })),
                CCDelayTime::create(cw::rng::get(3.75f, 2.5f)),
                CCEaseExponentialOut::create(CCMoveTo::create(0.375f, { x, y })),
                CallFuncExt::create([notifNode, finished = std::move(finished)]() mutable {
                    notifNode->removeFromParent();
                    finished(notifNode);
                }),
                nullptr
            )
        );

        cw::sfx::play("chest07.ogg");
    }
};

class $modify(NotifPlayLayer, PlayLayer) {
    static void onModify(auto& self) {
        if (Loader::get()->isModInstalled("cubicstudios.horriblemenu")) {
            utils::StringMap<std::shared_ptr<Hook>> const& hooks = self.m_hooks;  // intellisense my beloved
            auto enable = true;
            hm::api::OptionManagerV2::isEnabled(HORRIBLE_MENU_OPT_ID);

            for (auto const& hook : hooks | std::views::values) {
                hook->setAutoEnable(enable);
                (void)hook->toggle(enable);

                NOTIF_HOOKS.push_back(hook);
            };
        };
    }

    struct Fields final {
        NotifFeed* feed = nullptr;
    };

    void toggleAll(bool on) {
        auto f = m_fields.self();

        if (!on) {
            unschedule(schedule_selector(NotifPlayLayer::showNewNotif));
            cue::resetNode(f->feed);

            return;
        };

        if (Mod::get()->getSettingValue<bool>("challenge-mode")) {
            f->feed = NotifFeed::create(m_uiLayer->getScaledContentHeight());
            f->feed->setPosition({ m_uiLayer->getScaledContentWidth(), m_uiLayer->getScaledContentHeight() / 2.0f });

            m_uiLayer->addChild(f->feed, CW_HIGHEST_Z);
        };

        PlatformToolbox::showCursor();

        setupSchedules(!Mod::get()->getSettingValue<bool>("always-on"));
    }

    void setupSchedules(bool on) {
        if (on) scheduleOnce(schedule_selector(NotifPlayLayer::showNewNotif), cw::rng::get(6.25f, 0.125f));
        if (!on) unschedule(schedule_selector(NotifPlayLayer::showNewNotif));
    }

    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();

        toggleAll(true);
    }

    void levelComplete() {
        PlayLayer::levelComplete();

        auto f = m_fields.self();

        if (f->feed) f->feed->clearNotifs();
        setupSchedules(false);
    }

    void showNewNotif(float) {
        auto notifRes = NotifManager::get()->getRandom();
        if (notifRes.isErr()) return;

        auto notif = std::move(notifRes).unwrap();

        cw::showNotif(notif, m_uiLayer, [this](NotifNode* sender) { cleanNotif(sender); });

        auto f = m_fields.self();

        if (f->feed) f->feed->addNotif(notif, [this](bool correct) {
            cw::sfx::play("chestClick.ogg");

            if (!correct) {
                resetLevelFromStart();

                Notification::create("Bad response!", NotificationIcon::Error, 0.875f)->show();
                cw::sfx::play("explode_11.ogg");
            };
        });
    }

    void cleanNotif(CCNode* sender) {
        cue::resetNode(sender);
        scheduleOnce(schedule_selector(NotifPlayLayer::showNewNotif), cw::rng::get(6.25f, 0.125f));
    }
};

// this is all probably a lot messier than it needs to be xdd
#define NH_RUN_GLOBAL_ONCE CCDirector::sharedDirector()->getScheduler()->scheduleSelector(schedule_selector(GlobalNotifDirector::showNewNotif), this, 0.0f, 0, cw::rng::get(6.25f, 0.125f), false)

class GlobalNotifDirector final : public cw::base::Singleton<GlobalNotifDirector>, public CCObject {
protected:
    void showNewNotif(float) {
        auto notifRes = NotifManager::get()->getRandom();
        if (notifRes.isErr()) return;

        auto notif = std::move(notifRes).unwrap();

        cw::showNotif(notif, OverlayManager::get(), [this](NotifNode* sender) { cleanNotif(sender); });
    }

    void cleanNotif(CCNode* sender) {
        cue::resetNode(sender);
        if (HORRIBLE_OPT_ENABLED && Mod::get()->getSettingValue<bool>("always-on")) NH_RUN_GLOBAL_ONCE;
    }

public:
    void start() {
        log::info("Starting up global scheduler");  // extra horrible menu check :p
        if (HORRIBLE_OPT_ENABLED) NH_RUN_GLOBAL_ONCE;
    }

    void stop() {
        log::info("Stopping global scheduler");
        CCDirector::sharedDirector()->getScheduler()->unscheduleAllForTarget(this);
    }
};

namespace cw {  // decl namespace again cuz we need to wait for gnd class decl apparently.....
    static void toggleGlobalNotifs(bool on) {
        auto gnd = GlobalNotifDirector::get();

        on ? gnd->start() : gnd->stop();

#ifdef CW_TESTING
        log::debug("Persistent notifications {}", on ? "ENABLED" : "DISBALED");
#endif
    }
};

$on_game(Loaded) {
    listenForSettingChanges<bool>(
        "always-on",
        [](bool on) {
#ifdef CW_TESTING
            log::trace("{} persistent notifications", on ? "ENABLING" : "DISBALING");
#endif

            if (auto pl = PlayLayer::get()) modify_cast<NotifPlayLayer*>(pl)->setupSchedules(!on);
            cw::toggleGlobalNotifs(on);
        }
    );

    cw::toggleGlobalNotifs(Mod::get()->getSettingValue<bool>("always-on"));

    if (Loader::get()->isModLoaded("cubicstudios.horriblemenu")) {
        auto const opt = hm::api::OptionV2{
            NH_HORRIBLE_MENU_OPT_ID,
            "Spam Notifications",
            "Enable the annoying notification spam during gameplay.",
            "Notification Hell",
            hm::api::SillyTier::High,
            true,
        };

        hm::api::listenForHorribleOptionChanges(
            HORRIBLE_MENU_OPT_ID,
            [](hm::api::OptionSave data) {
                HORRIBLE_OPT_ENABLED = data.enabled;

                for (auto const& hook : NOTIF_HOOKS) {
                    if (auto h = hook.lock()) (void)h->toggle(data.enabled);
                };

                if (auto pl = PlayLayer::get()) modify_cast<NotifPlayLayer*>(pl)->toggleAll(data.enabled);
                cw::toggleGlobalNotifs(data.enabled && Mod::get()->getSettingValue<bool>("always-on"));
            }
        );

        (void)hm::api::OptionManagerV2::registerOption(opt);
    };
}
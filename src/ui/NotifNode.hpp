#pragma once

#include <API.h>

#include <Geode/Geode.hpp>

namespace cw::notifs::ui {
    class NotifNode final : public cocos2d::CCNode {
        using Callback = geode::Function<void(bool)>;

    private:
        Callback m_callback = nullptr;

        geode::ProgressBar* m_countdown = nullptr;

        float m_maxTime = 0.0f;
        float m_time = 0.0f;

    protected:
        bool init(const cw::notifs::Notif* notif, bool withBtns);

        void update(float dt);

    public:
        static NotifNode* create(const cw::notifs::Notif* notif, bool withBtns = true);

        void setCallback(Callback&& callback);
    };
};
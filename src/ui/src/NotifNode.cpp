#include "../NotifNode.hpp"

#include <cw.h>

#include <rhatt/prelude.h>

#include <cue/Util.hpp>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::notifs;

bool NotifNode::init(Notif const* notif, bool withBtns) {
    if (!CCNode::init()) return false;

    setAnchorPoint({ 0.5f, 0.5f });
    setContentSize({ 255.0f, withBtns ? 82.5f : 60.0f });

    auto notifMsg = Label::create(notif->getMessage(), "chatFont.fnt");
    notifMsg->setID("notif-msg");
    notifMsg->setScale(0.875f);
    notifMsg->setMaxWidth(getScaledContentWidth() - 75.0f);
    notifMsg->setAnchorPoint({ 0.0f, 1.0f });
    notifMsg->setZOrder(9);

    setContentHeight(std::max(getContentHeight() + notifMsg->getScaledContentHeight(), withBtns ? 90.0f : 75.0f));

    if (withBtns) {
        m_maxTime = cw::rng::get(25.0f, 12.5f);
        m_time = m_maxTime;

        auto btnContainerLayout = RowLayout::create()
                                      ->setGap(2.5f)
                                      ->setAutoScale(false)
                                      ->setGrowCrossAxis(true);

        auto btnContainer = CCNode::create();
        btnContainer->setID("btn-container");
        btnContainer->setAnchorPoint({ 0.5f, 0.0f });
        btnContainer->setContentSize({ getScaledContentWidth() * 0.925f, 50.0f });
        btnContainer->setLayout(btnContainerLayout);

        std::vector<Notif::Button> btns;
        btns.reserve(notif->getButtons().size());
        for (auto const& btn : notif->getButtons()) btns.push_back(btn);

        cw::rng::internal::shuffle(btns);

        for (auto const& btn : btns) {
            auto btnNode = Button::createWithNode(
                ButtonSprite::create(btn.text.c_str(), "mdFontB.fnt"_gode, "GJ_button_05.png"),
                [this, btn](auto) {
                    unscheduleUpdate();
                    if (m_callback) m_callback(btn.isOk);
                }
            );
            btnNode->setScale(0.875f);

            btnContainer->addChild(btnNode);
        };

        btnContainer->updateLayout();

        setContentHeight(getScaledContentHeight() + btnContainer->getScaledContentHeight());

        addChildAtPosition(btnContainer, Anchor::Bottom, { 0.0f, 20.0f });

        m_countdown = ProgressBar::create();
        m_countdown->setID("countdown");
        m_countdown->setScale(0.75f);
        m_countdown->setAnchorPoint({ 0.5f, 0.5f });
        m_countdown->setZOrder(1);

        m_countdown->updateProgress(100.0f);

        addChildAtPosition(m_countdown, Anchor::Bottom, { 0.0f, 12.5f });

        m_countdown->updateProgress(0.0f, m_maxTime, EasingType::SineInOut, [this](auto, bool complete) {
            if (complete) {
                if (m_callback) m_callback(false);
                return unscheduleUpdate();
            };
        });

        scheduleUpdate();
    };

    cue::attachBackground(
        this,
        {
            .opacity = 255,
            .sidePadding = 1.25f,
            .verticalPadding = 0.0f,
            .texture = "GJ_square02.png",
            .zOrder = -1,
        }
    );

    auto notifTitle = Label::create(notif->getTitle().c_str(), "goldFont.fnt");
    notifTitle->setID("notif-title");
    notifTitle->setScale(0.5f);
    notifTitle->setZOrder(1);

    addChildAtPosition(notifTitle, Anchor::Top, { 0.0f, -(notifTitle->getScaledContentHeight() * 0.925f) });

    auto senderIcon = CCSprite::createWithSpriteFrameName(notif->getSender().getIcon().c_str());
    senderIcon->setID("sender-icon");
    senderIcon->setZOrder(9);

    cue::rescaleToMatch(senderIcon, 42.5f);

    addChildAtPosition(senderIcon, Anchor::TopLeft, { 32.5f, -50.0f });

    auto senderName = Label::create(notif->getSender().getName().c_str(), "bigFont.fnt");
    senderName->setID("sender-name");
    senderName->setScale(0.5f);
    senderName->setAnchorPoint({ 0.0f, 0.5f });
    senderName->setZOrder(1);

    addChildAtPosition(senderName, Anchor::TopLeft, { senderIcon->getPositionX() + (senderIcon->getScaledContentWidth() * 0.75f), -32.5f });

    addChildAtPosition(notifMsg, Anchor::TopLeft, { senderName->getPositionX(), -40.0f });

    return true;
}

void NotifNode::update(float dt) {
    if (m_countdown) m_countdown->setFillColor(cw::colors::fadeColor(m_countdown->getProgress()));
}

void NotifNode::setCallback(Callback&& callback) {
    m_callback = std::move(callback);
}

NotifNode* NotifNode::create(Notif const* notif, bool withBtns) {
    auto ret = new NotifNode();
    if (ret->init(notif, withBtns)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
}
#include "../NotifFeed.hpp"

#include <cue/Util.hpp>

#include <cw.h>

#include <rhatt/prelude.h>

#include <ui/NotifNode.hpp>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::notifs;

bool NotifFeed::init(float height) {
    if (!CCNode::init()) return false;

    setID("notif-feed"_spr);
    setAnchorPoint({ 1.0f, 0.5f });
    setContentSize({ 200.0f, height });

    // yea ik whatever man
    m_mainLayer = CCLayer::create();
    m_mainLayer->ignoreAnchorPointForPosition(false);
    m_mainLayer->setContentSize(getScaledContentSize());

    addChildAtPosition(m_mainLayer, Anchor::Center);

    cue::attachBackground(
        m_mainLayer,
        {
            .opacity = 255,
            .sidePadding = -2.5f,
            .verticalPadding = -2.5f,
            .texture = "GJ_square04.png",
            .zOrder = -1,
        }
    );

    auto label = Label::create("Notifications", "goldFont.fnt");
    label->setID("feed-label");
    label->setScale(0.5f);
    label->setZOrder(1);

    m_mainLayer->addChildAtPosition(label, Anchor::Top, { 0.0f, -15.0f });

    m_list = ScrollLayer::create({ m_mainLayer->getScaledContentWidth() - 25.0f, height - 37.5f });
    m_list->setID("notif-list");
    m_list->ignoreAnchorPointForPosition(false);
    m_list->setAnchorPoint({ 0.5f, 0.0f });
    m_list->setZOrder(9);

    m_list->m_contentLayer->setLayout(ScrollLayer::createDefaultListLayout(5.0f));

    m_mainLayer->addChildAtPosition(m_list, Anchor::Bottom, { 0.0f, 12.5f });

    m_list->m_contentLayer->updateLayout();

    auto listBg = cue::createBackground(
        m_list->getScaledContentSize(),
        {
            .id = "",
        }
    );
    listBg->setAnchorPoint(m_list->getAnchorPoint());

    m_mainLayer->addChildAtPosition(listBg, Anchor::Bottom, { 0.0f, 12.5f });

    auto toggleBtn = Button::createWithSpriteFrameName(
        "accountBtn_messages_001.png",
        [this](Button* sender) {
            m_showing = !m_showing;

            if (m_showing) m_mainLayer->setVisible(true);

            runAction(
                CCSequence::createWithTwoActions(
                    CCEaseSineOut::create(
                        CCMoveBy::create(0.5f, { getScaledContentWidth() * (m_showing ? -1.0f : 1.0f), 0.0f })
                    ),
                    CallFuncExt::create([this]() {
                        if (!m_showing) m_mainLayer->setVisible(false);
                    })
                )
            );

            if (auto spr = typeinfo_cast<CCSprite*>(sender->getDisplayNode())) spr->runAction(CCFadeTo::create(0.5f, m_showing ? 255 : 125));
        }
    );
    toggleBtn->setID("toggle-feed");
    toggleBtn->setScale(0.875f);

    addChildAtPosition(toggleBtn, Anchor::Left, { -(toggleBtn->getScaledContentWidth() * 0.625f), 0.0f });

    m_countLabelBg = CCSprite::createWithSpriteFrameName("updates-multiple.png"_gode);
    m_countLabelBg->setScale(0.575f);
    m_countLabelBg->setVisible(false);

    toggleBtn->getDisplayNode()->addChildAtPosition(m_countLabelBg, Anchor::TopRight, { -2.5f, -2.5f });

    m_countLabel = Label::create(cw::fmtNum(m_list->m_contentLayer->getChildrenCount()), "mdFontB.fnt"_gode);
    m_countLabel->setID("notif-counter");
    m_countLabel->setVisible(false);

    cue::rescaleToMatch(m_countLabel, m_countLabelBg->getScaledContentHeight() * 0.875f);

    m_countLabelBg->addChildAtPosition(m_countLabel, Anchor::Center);

    return true;
}

void NotifFeed::updateCountLabel() {
    auto count = m_list->m_contentLayer->getChildrenCount();

    if (m_countLabel) {
        m_countLabel->setText(cw::fmtNum(count));
        m_countLabel->setVisible(count > 0);
    };

    if (m_countLabelBg) m_countLabelBg->setVisible(count > 0);
}

void NotifFeed::addNotif(const Notif* notif, Callback cb) {
    auto node = NotifNode::create(notif);
    node->setScale(0.65f);

    node->setCallback([this, node, cb = std::move(cb)](bool correct) mutable {
        cb(correct);

        node->removeFromParent();

        m_list->m_contentLayer->updateLayout();
        m_list->scrollToTop();

        updateCountLabel();
    });

    m_list->m_contentLayer->addChild(node);
    m_list->m_contentLayer->updateLayout();

    m_list->scrollToTop();

    updateCountLabel();
}

void NotifFeed::clearNotifs() {
    m_list->m_contentLayer->removeAllChildren();
    updateCountLabel();
}

NotifFeed* NotifFeed::create(float height) {
    auto ret = new NotifFeed();
    if (ret->init(height)) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
}
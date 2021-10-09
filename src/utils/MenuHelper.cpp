/*
 * Kilpatrick Audio Menu Helper
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#include "../plugin.hpp"
#include "MenuHelper.h"
#include <thread>

// website browser launcher
MenuHelperUrlItem::MenuHelperUrlItem(std::string text, std::string url) {
    this->text = text;
    this->url = url;
    this->rightText = "";
}

// the menu item was selected
void MenuHelperUrlItem::onAction(const event::Action& e) {
	std::thread t(system::openBrowser, url);
	t.detach();
}

// add a spacer to a menu
void menuHelperAddSpacer(Menu *menu) {
    menu->addChild(new MenuLabel());
}

// add text to a menu
void menuHelperAddText(Menu *menu, std::string text) {
    menu->addChild(createMenuItem(text, ""));
}

// add a label to a menu
void menuHelperAddLabel(Menu *menu, std::string text) {
    MenuLabel *label = new MenuLabel();
    label->text = text;
    menu->addChild(label);
}

// add an item to the menu - just a front end for addChild()
void menuHelperAddItem(Menu *menu, MenuItem *item) {
    menu->addChild(item);
}

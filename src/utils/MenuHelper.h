/*
 * Kilpatrick Audio Menu Helper
 *
 * Written by: Andrew Kilpatrick
 * Copyright 2020: Andrew Kilpatrick
 *
 * Please see the license file included with this repo for license details.
 *
 */
#ifndef MENU_HELPER
#define MENU_HELPER

#include "../plugin.hpp"

// website browser launcher
struct MenuHelperUrlItem : MenuItem {
    std::string url;

    // create a driver chooser menu item
    MenuHelperUrlItem(std::string text, std::string url);

    // the menu item was selected
	void onAction(const event::Action& e) override;
};

// add a spacer to a menu
void menuHelperAddSpacer(Menu *menu);

// add a label to a menu
void menuHelperAddLabel(Menu *menu, std::string text);

// add text to a menu
void menuHelperAddText(Menu *menu, std::string text);

// add an item to the menu - just a front end for addChild()
void menuHelperAddItem(Menu *menu, MenuItem *item);

#endif

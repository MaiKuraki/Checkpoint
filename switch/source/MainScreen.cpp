/*
 *   This file is part of Checkpoint
 *   Copyright (C) 2017-2025 Bernardo Giordano, FlagBrew
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *   Additional Terms 7.b and 7.c of GPLv3 apply to this file:
 *       * Requiring preservation of specified reasonable legal notices or
 *         author attributions in that material or in the Appropriate Legal
 *         Notices displayed by works containing it.
 *       * Prohibiting misrepresentation of the origin of that material,
 *         or requiring that modified versions of such material be marked in
 *         reasonable ways as different from the original version.
 */

#include "MainScreen.hpp"

static constexpr size_t rowlen = 5, collen = 4, rows = 10, SIDEBAR_w = 96, TOPBAR_h = 48;

MainScreen::MainScreen(const InputState& input) : hid(rowlen * collen, collen, input)
{
    pksmBridge       = false;
    wantInstructions = false;
    selectionTimer   = 0;
    sprintf(ver, "v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);
    backupList    = std::make_unique<Scrollable>(536, 316, 416, 408, rows);
    buttonBackup  = std::make_unique<Clickable>(956, 316, 224, 80, COLOR_BLACK_DARKER, COLOR_GREY_LIGHT, "Backup \ue004", true);
    buttonRestore = std::make_unique<Clickable>(956, 400, 224, 80, COLOR_BLACK_DARKER, COLOR_GREY_LIGHT, "Restore \ue005", true);
    buttonCheats  = std::make_unique<Clickable>(956, 484, 224, 80, COLOR_BLACK_DARKER, COLOR_GREY_LIGHT, "Cheats \ue0c5", true);
    buttonBackup->canChangeColorWhenSelected(true);
    buttonRestore->canChangeColorWhenSelected(true);
    buttonCheats->canChangeColorWhenSelected(true);
}

int MainScreen::selectorX(size_t i) const
{
    return 128 * ((i % (rowlen * collen)) % collen) + 4 * (((i % (rowlen * collen)) % collen) + 1);
}

int MainScreen::selectorY(size_t i) const
{
    const int row = (i % (rowlen * collen)) / collen;
    return 4 + 128 * row + 4 * (row + 1) + TOPBAR_h;
}

void MainScreen::draw() const
{
    auto selEnt          = MS::selectedEntries();
    const size_t entries = hid.maxVisibleEntries();
    const size_t max     = hid.maxEntries(getTitleCount(g_currentUId)) + 1;

    const bool isPKSMBridgeEnabled = getPKSMBridgeFlag();
    SDLH_ClearScreen(COLOR_BLACK_DARKERR);
    SDL_Color colorBar = isPKSMBridgeEnabled ? COLOR_PURPLE_LIGHT : COLOR_BLACK_DARK;
    SDLH_DrawRect(0, TOPBAR_h + 4, 532, 664, COLOR_BLACK_DARKER);
    SDLH_DrawRect(1280 - SIDEBAR_w, 0, SIDEBAR_w, 720, colorBar);
    SDLH_DrawRect(0, 0, 1280, TOPBAR_h, COLOR_BLACK);

    drawPulsingOutline(
        1280 - SIDEBAR_w + (SIDEBAR_w - USER_ICON_SIZE) / 2, 720 - USER_ICON_SIZE - 30, USER_ICON_SIZE, USER_ICON_SIZE, 2, COLOR_GREEN);
    SDLH_DrawImageScale(
        Account::icon(g_currentUId), 1280 - SIDEBAR_w + (SIDEBAR_w - USER_ICON_SIZE) / 2, 720 - USER_ICON_SIZE - 30, USER_ICON_SIZE, USER_ICON_SIZE);

    u32 username_w, username_h;
    std::string username = Account::shortName(g_currentUId);
    SDLH_GetTextDimensions(13, username.c_str(), &username_w, &username_h);
    SDLH_DrawTextBox(13, 1280 - SIDEBAR_w + (SIDEBAR_w - username_w) / 2, 720 - 28 + (28 - username_h) / 2, COLOR_WHITE, SIDEBAR_w, username.c_str());

    // title icons
    for (size_t k = hid.page() * entries; k < hid.page() * entries + max; k++) {
        int selectorx = selectorX(k);
        int selectory = selectorY(k);
        if (smallIcon(g_currentUId, k) != NULL) {
            SDLH_DrawImageScale(smallIcon(g_currentUId, k), selectorx, selectory, 128, 128);
        }
        else {
            SDLH_DrawRect(selectorx, selectory, 128, 128, COLOR_BLACK);
        }

        if (!selEnt.empty() && std::find(selEnt.begin(), selEnt.end(), k) != selEnt.end()) {
            SDLH_DrawIcon("checkbox", selectorx + 86, selectory + 86);
        }

        if (favorite(g_currentUId, k)) {
            SDLH_DrawRect(selectorx + 94, selectory + 8, 24, 24, COLOR_GOLD);
            SDLH_DrawIcon("star", selectorx + 86, selectory);
        }
    }

    // title selector
    if (getTitleCount(g_currentUId) > 0) {
        const int x = selectorX(hid.index()) + 4 / 2;
        const int y = selectorY(hid.index()) + 4 / 2;
        drawPulsingOutline(x, y, 124, 124, 4, COLOR_PURPLE_DARK);
        SDLH_DrawRect(x, y, 124, 124, COLOR_WHITEMASK);
    }

    u32 ver_w, ver_h, checkpoint_h, checkpoint_w;
    SDLH_GetTextDimensions(20, ver, &ver_w, &ver_h);
    SDLH_GetTextDimensions(26, "checkpoint", &checkpoint_w, &checkpoint_h);

    SDLH_DrawText(26, 16, (TOPBAR_h - checkpoint_h) / 2 + 4, COLOR_WHITE, "checkpoint");
    SDLH_DrawText(20, 16 + checkpoint_w + 8, (TOPBAR_h - checkpoint_h) / 2 + checkpoint_h - ver_h + 2, COLOR_GREY_LIGHT, ver);
    SDLH_DrawText(
        20, 16 * 3 + checkpoint_w + 8 + ver_w, (TOPBAR_h - checkpoint_h) / 2 + checkpoint_h - ver_h + 2, COLOR_GREY_LIGHT, "\ue046 Instructions");

    if (getTitleCount(g_currentUId) > 0) {
        Title title;
        getTitle(title, g_currentUId, hid.fullIndex());

        backupList->flush();
        std::vector<std::string> dirs = title.saves();

        for (size_t i = 0; i < dirs.size(); i++) {
            backupList->push_back(COLOR_BLACK_DARKER, COLOR_WHITE, dirs.at(i), i == backupList->index());
        }

        if (title.icon() != NULL) {
            drawOutline(1020, 52, 256, 256, 4, COLOR_BLACK_DARK);
            SDLH_DrawImage(title.icon(), 1020, 52);
        }

        u32 h = 29, offset = 56, i = 0, title_w;
        auto gameName = title.displayName();
        SDLH_GetTextDimensions(26, gameName.c_str(), &title_w, NULL);

        if (title_w >= 720) {
            gameName = gameName.substr(0, 40) + "...";
            SDLH_GetTextDimensions(26, gameName.c_str(), &title_w, NULL);
        }

        SDLH_DrawText(26, 1280 - 8 - title_w, (TOPBAR_h - checkpoint_h) / 2 + 4, COLOR_WHITE, gameName.c_str());
        SDLH_DrawText(23, 538, offset + h * (i++), COLOR_GREY_LIGHT, StringUtils::format("Title ID: %016llX", title.id()).c_str());
        SDLH_DrawText(23, 538, offset + h * (i++), COLOR_GREY_LIGHT, ("Author: " + title.author()).c_str());
        SDLH_DrawText(23, 538, offset + h * (i++), COLOR_GREY_LIGHT, ("User: " + title.userName()).c_str());
        if (!title.playTime().empty()) {
            SDLH_DrawText(23, 538, offset + h * i, COLOR_GREY_LIGHT, ("Play Time: " + title.playTime()).c_str());
        }

        backupList->draw(g_backupScrollEnabled);
        buttonBackup->draw(30, COLOR_PURPLE_LIGHT);
        buttonRestore->draw(30, COLOR_PURPLE_LIGHT);
        buttonCheats->draw(30, COLOR_PURPLE_LIGHT);
    }

    if (wantInstructions && currentOverlay == nullptr) {
        SDLH_DrawRect(0, 0, 1280, 720, COLOR_OVERLAY);
        SDLH_DrawText(27, 1205, 646, COLOR_WHITE, "\ue085\ue086");
        SDLH_DrawText(24, 58, 69, COLOR_WHITE, "\ue058 Tap to select title");
        SDLH_DrawText(24, 58, 109, COLOR_WHITE, ("\ue026 Sort: " + sortMode()).c_str());
        SDLH_DrawText(24, 100, 270, COLOR_WHITE, "\ue006 \ue080 to scroll between titles");
        SDLH_DrawText(24, 100, 300, COLOR_WHITE, "\ue004 \ue005 to scroll between pages");
        SDLH_DrawText(24, 100, 330, COLOR_WHITE, "\ue000 to enter the selected title");
        SDLH_DrawText(24, 100, 360, COLOR_WHITE, "\ue001 to exit the selected title");
        SDLH_DrawText(24, 100, 390, COLOR_WHITE, "\ue002 to change sort mode");
        SDLH_DrawText(24, 100, 420, COLOR_WHITE, "\ue003 to select multiple titles");
        SDLH_DrawText(24, 100, 450, COLOR_WHITE, "Hold \ue003 to select all titles");
        SDLH_DrawText(24, 616, 480, COLOR_WHITE, "\ue002 to delete a backup");
        if (Configuration::getInstance().isPKSMBridgeEnabled()) {
            SDLH_DrawText(24, 100, 480, COLOR_WHITE, "\ue004 + \ue005 to enable PKSM bridge");
        }
        if (gethostid() != INADDR_LOOPBACK) {
            if (g_ftpAvailable && Configuration::getInstance().isFTPEnabled()) {
                SDLH_DrawText(24, 500, 642, COLOR_GOLD, StringUtils::format("FTP server running on %s:50000", getConsoleIP()).c_str());
            }
            SDLH_DrawText(24, 500, 672, COLOR_GOLD, StringUtils::format("Configuration server running on %s:8000", getConsoleIP()).c_str());
        }
    }

    if (g_isTransferringFile) {
        SDLH_DrawRect(0, 0, 1280, 720, COLOR_OVERLAY);

        u32 w, h;
        SDLH_GetTextDimensions(28, g_currentFile.c_str(), &w, &h);
        SDLH_DrawText(28, (1280 - w) / 2, (720 - h) / 2, COLOR_WHITE, g_currentFile.c_str());
    }
}

void MainScreen::update(const InputState& input)
{
    updateSelector(input);
    handleEvents(input);
}

void MainScreen::updateSelector(const InputState& input)
{
    if (!g_backupScrollEnabled) {
        size_t count    = getTitleCount(g_currentUId);
        size_t oldindex = hid.index();
        hid.update(count);

        // loop through every rendered title
        for (u8 row = 0; row < rowlen; row++) {
            for (u8 col = 0; col < collen; col++) {
                u8 index = row * collen + col;
                if (index > hid.maxEntries(count))
                    break;

                u32 x = selectorX(index);
                u32 y = selectorY(index);
                if (input.touch.count > 0 && input.touch.touches[0].x >= x && input.touch.touches[0].x <= x + 128 && input.touch.touches[0].y >= y &&
                    input.touch.touches[0].y <= y + 128) {
                    hid.index(index);
                }
            }
        }

        backupList->resetIndex();
        if (hid.index() != oldindex) {
            setPKSMBridgeFlag(false);
        }
    }
    else {
        backupList->updateSelection();
    }
}

void MainScreen::handleEvents(const InputState& input)
{
    const u64 kheld = input.kHeld;
    const u64 kdown = input.kDown;

    wantInstructions = (kheld & HidNpadButton_Minus);

    if (kdown & HidNpadButton_ZL || kdown & HidNpadButton_ZR) {
        while ((g_currentUId = Account::selectAccount()) == 0)
            ;
        this->index(TITLES, 0);
        this->index(CELLS, 0);
        setPKSMBridgeFlag(false);
    }
    // handle PKSM bridge
    if (Configuration::getInstance().isPKSMBridgeEnabled()) {
        Title title;
        getTitle(title, g_currentUId, this->index(TITLES));
        if (!getPKSMBridgeFlag()) {
            if ((kheld & HidNpadButton_L) && (kheld & HidNpadButton_R) && isPKSMBridgeTitle(title.id())) {
                setPKSMBridgeFlag(true);
                updateButtons();
            }
        }
    }

    // handle touchscreen
    if (!g_backupScrollEnabled && input.touch.count > 0 && input.touch.touches[0].x >= 1200 && input.touch.touches[0].x <= 1200 + USER_ICON_SIZE &&
        input.touch.touches[0].y >= 626 && input.touch.touches[0].y <= 626 + USER_ICON_SIZE) {
        while ((g_currentUId = Account::selectAccount()) == 0)
            ;
        this->index(TITLES, 0);
        this->index(CELLS, 0);
        setPKSMBridgeFlag(false);
    }

    // Handle touching the backup list
    if (input.touch.count > 0 && input.touch.touches[0].x > 538 && input.touch.touches[0].x < 952 && input.touch.touches[0].y > 316 &&
        input.touch.touches[0].y < 720) {
        // Activate backup list only if multiple selections are not enabled
        if (!MS::multipleSelectionEnabled()) {
            g_backupScrollEnabled = true;
            updateButtons();
            entryType(CELLS);
        }
    }

    // Handle pressing A
    // Backup list active:   Backup/Restore
    // Backup list inactive: Activate backup list only if multiple
    //                       selections are enabled
    if (kdown & HidNpadButton_A) {
        // If backup list is active...
        if (g_backupScrollEnabled) {
            // If the "New..." entry is selected...
            if (0 == this->index(CELLS)) {
                if (!getPKSMBridgeFlag()) {
                    auto result = io::backup(this->index(TITLES), g_currentUId, this->index(CELLS));
                    if (std::get<0>(result)) {
                        currentOverlay = std::make_shared<InfoOverlay>(*this, std::get<2>(result));
                    }
                    else {
                        currentOverlay = std::make_shared<ErrorOverlay>(*this, std::get<1>(result), std::get<2>(result));
                    }
                }
            }
            else {
                if (getPKSMBridgeFlag()) {
                    auto result = recvFromPKSMBridge(this->index(TITLES), g_currentUId, this->index(CELLS));
                    if (std::get<0>(result)) {
                        currentOverlay = std::make_shared<InfoOverlay>(*this, std::get<2>(result));
                    }
                    else {
                        currentOverlay = std::make_shared<ErrorOverlay>(*this, std::get<1>(result), std::get<2>(result));
                    }
                }
                else {
                    currentOverlay = std::make_shared<YesNoOverlay>(
                        *this, "Restore selected save?",
                        [this]() {
                            auto result = io::restore(this->index(TITLES), g_currentUId, this->index(CELLS), nameFromCell(this->index(CELLS)));
                            if (std::get<0>(result)) {
                                currentOverlay = std::make_shared<InfoOverlay>(*this, std::get<2>(result));
                            }
                            else {
                                currentOverlay = std::make_shared<ErrorOverlay>(*this, std::get<1>(result), std::get<2>(result));
                            }
                        },
                        [this]() { this->removeOverlay(); });
                }
            }
        }
        else {
            // Activate backup list only if multiple selections are not enabled
            if (!MS::multipleSelectionEnabled()) {
                g_backupScrollEnabled = true;
                updateButtons();
                entryType(CELLS);
            }
        }
    }

    // Handle pressing B
    if ((kdown & HidNpadButton_B) || (input.touch.count > 0 && input.touch.touches[0].x <= 532 && input.touch.touches[0].y <= 664)) {
        this->index(CELLS, 0);
        g_backupScrollEnabled = false;
        entryType(TITLES);
        MS::clearSelectedEntries();
        setPKSMBridgeFlag(false);
        updateButtons(); // Do this last
    }

    // Handle pressing X
    if (kdown & HidNpadButton_X) {
        if (g_backupScrollEnabled) {
            size_t index = this->index(CELLS);
            if (index > 0) {
                currentOverlay = std::make_shared<YesNoOverlay>(
                    *this, "Delete selected backup?",
                    [this, index]() {
                        Title title;
                        getTitle(title, g_currentUId, this->index(TITLES));
                        std::string path = title.fullPath(index);
                        io::deleteFolderRecursively((path + "/").c_str());
                        refreshDirectories(title.id());
                        this->index(CELLS, index - 1);
                        this->removeOverlay();
                    },
                    [this]() { this->removeOverlay(); });
            }
        }
        else {
            rotateSortMode();
        }
    }

    // Handle pressing Y
    // Backup list active:   Deactivate backup list, select title, and
    //                       enable backup button
    // Backup list inactive: Select title and enable backup button
    if (kdown & HidNpadButton_Y) {
        if (g_backupScrollEnabled) {
            this->index(CELLS, 0);
            g_backupScrollEnabled = false;
        }
        entryType(TITLES);
        MS::addSelectedEntry(this->index(TITLES));
        setPKSMBridgeFlag(false);
        updateButtons(); // Do this last
    }

    // Handle holding Y
    if (kheld & HidNpadButton_Y && !(g_backupScrollEnabled)) {
        selectionTimer++;
    }
    else {
        selectionTimer = 0;
    }

    if (selectionTimer > 45) {
        MS::clearSelectedEntries();
        for (size_t i = 0, sz = getTitleCount(g_currentUId); i < sz; i++) {
            MS::addSelectedEntry(i);
        }
        selectionTimer = 0;
    }

    // Handle pressing/touching L
    if (buttonBackup->released() || (kdown & HidNpadButton_L)) {
        if (MS::multipleSelectionEnabled()) {
            resetIndex(CELLS);
            std::vector<size_t> list = MS::selectedEntries();
            for (size_t i = 0, sz = list.size(); i < sz; i++) {
                // check if multiple selection is enabled and don't ask for confirmation if that's the case
                auto result = io::backup(list.at(i), g_currentUId, this->index(CELLS));
                if (std::get<0>(result)) {
                    currentOverlay = std::make_shared<InfoOverlay>(*this, std::get<2>(result));
                }
                else {
                    currentOverlay = std::make_shared<ErrorOverlay>(*this, std::get<1>(result), std::get<2>(result));
                }
            }
            MS::clearSelectedEntries();
            updateButtons();
            blinkLed(4);
            currentOverlay = std::make_shared<InfoOverlay>(*this, "Progress correctly saved to disk.");
        }
        else if (g_backupScrollEnabled) {
            if (getPKSMBridgeFlag()) {
                if (this->index(CELLS) != 0) {
                    currentOverlay = std::make_shared<YesNoOverlay>(
                        *this, "Send save to PKSM?",
                        [this]() {
                            auto result = sendToPKSMBrigde(this->index(TITLES), g_currentUId, this->index(CELLS));
                            if (std::get<0>(result)) {
                                currentOverlay = std::make_shared<InfoOverlay>(*this, std::get<2>(result));
                            }
                            else {
                                currentOverlay = std::make_shared<ErrorOverlay>(*this, std::get<1>(result), std::get<2>(result));
                            }
                        },
                        [this]() { this->removeOverlay(); });
                }
            }
            else {
                currentOverlay = std::make_shared<YesNoOverlay>(
                    *this, "Backup selected save?",
                    [this]() {
                        auto result = io::backup(this->index(TITLES), g_currentUId, this->index(CELLS));
                        if (std::get<0>(result)) {
                            currentOverlay = std::make_shared<InfoOverlay>(*this, std::get<2>(result));
                        }
                        else {
                            currentOverlay = std::make_shared<ErrorOverlay>(*this, std::get<1>(result), std::get<2>(result));
                        }
                    },
                    [this]() { this->removeOverlay(); });
            }
        }
    }

    // Handle pressing/touching R
    if (buttonRestore->released() || (kdown & HidNpadButton_R)) {
        if (g_backupScrollEnabled) {
            if (getPKSMBridgeFlag() && this->index(CELLS) != 0) {
                currentOverlay = std::make_shared<YesNoOverlay>(
                    *this, "Receive save from PKSM?",
                    [this]() {
                        auto result = recvFromPKSMBridge(this->index(TITLES), g_currentUId, this->index(CELLS));
                        if (std::get<0>(result)) {
                            currentOverlay = std::make_shared<InfoOverlay>(*this, std::get<2>(result));
                        }
                        else {
                            currentOverlay = std::make_shared<ErrorOverlay>(*this, std::get<1>(result), std::get<2>(result));
                        }
                    },
                    [this]() { this->removeOverlay(); });
            }
            else {
                if (this->index(CELLS) != 0) {
                    currentOverlay = std::make_shared<YesNoOverlay>(
                        *this, "Restore selected save?",
                        [this]() {
                            auto result = io::restore(this->index(TITLES), g_currentUId, this->index(CELLS), nameFromCell(this->index(CELLS)));
                            if (std::get<0>(result)) {
                                currentOverlay = std::make_shared<InfoOverlay>(*this, std::get<2>(result));
                            }
                            else {
                                currentOverlay = std::make_shared<ErrorOverlay>(*this, std::get<1>(result), std::get<2>(result));
                            }
                        },
                        [this]() { this->removeOverlay(); });
                }
            }
        }
    }

    if ((buttonCheats->released() || (kdown & HidNpadButton_StickR)) && CheatManager::getInstance().cheats() != nullptr) {
        if (MS::multipleSelectionEnabled()) {
            MS::clearSelectedEntries();
            updateButtons();
        }
        else {
            Title title;
            getTitle(title, g_currentUId, this->index(TITLES));
            std::string key = StringUtils::format("%016llX", title.id());
            if (CheatManager::getInstance().areCheatsAvailable(key)) {
                currentOverlay = std::make_shared<CheatManagerOverlay>(*this, key);
            }
            else {
                currentOverlay = std::make_shared<InfoOverlay>(*this, "No available cheat codes for this title.");
            }
        }
    }
}

std::string MainScreen::nameFromCell(size_t index) const
{
    return backupList->cellName(index);
}

void MainScreen::entryType(entryType_t type_)
{
    type = type_;
}

void MainScreen::resetIndex(entryType_t type)
{
    if (type == TITLES) {
        hid.reset();
    }
    else {
        backupList->resetIndex();
    }
}

size_t MainScreen::index(entryType_t type) const
{
    return type == TITLES ? hid.fullIndex() : backupList->index();
}

void MainScreen::index(entryType_t type, size_t i)
{
    if (type == TITLES) {
        hid.page(i / hid.maxVisibleEntries());
        hid.index(i - hid.page() * hid.maxVisibleEntries());
    }
    else {
        backupList->setIndex(i);
    }
}

bool MainScreen::getPKSMBridgeFlag(void) const
{
    return Configuration::getInstance().isPKSMBridgeEnabled() ? pksmBridge : false;
}

void MainScreen::setPKSMBridgeFlag(bool f)
{
    pksmBridge = f;
    updateButtons();
}

void MainScreen::updateButtons(void)
{
    if (MS::multipleSelectionEnabled()) {
        buttonRestore->canChangeColorWhenSelected(true);
        buttonRestore->canChangeColorWhenSelected(false);
        buttonCheats->canChangeColorWhenSelected(false);
        buttonBackup->setColors(COLOR_BLACK_DARKER, COLOR_WHITE);
        buttonRestore->setColors(COLOR_BLACK_DARKER, COLOR_GREY_LIGHT);
        buttonCheats->setColors(COLOR_BLACK_DARKER, COLOR_GREY_LIGHT);
    }
    else if (g_backupScrollEnabled) {
        buttonBackup->canChangeColorWhenSelected(true);
        buttonRestore->canChangeColorWhenSelected(true);
        buttonCheats->canChangeColorWhenSelected(true);
        buttonBackup->setColors(COLOR_BLACK_DARKER, COLOR_WHITE);
        buttonRestore->setColors(COLOR_BLACK_DARKER, COLOR_WHITE);
        buttonCheats->setColors(COLOR_BLACK_DARKER, COLOR_WHITE);
    }
    else {
        buttonBackup->setColors(COLOR_BLACK_DARKER, COLOR_GREY_LIGHT);
        buttonRestore->setColors(COLOR_BLACK_DARKER, COLOR_GREY_LIGHT);
        buttonCheats->setColors(COLOR_BLACK_DARKER, COLOR_GREY_LIGHT);
    }

    if (getPKSMBridgeFlag()) {
        buttonBackup->text("Send \ue004");
        buttonRestore->text("Receive \ue005");
    }
    else {
        buttonBackup->text("Backup \ue004");
        buttonRestore->text("Restore \ue005");
    }
}

std::string MainScreen::sortMode() const
{
    switch (g_sortMode) {
        case SORT_LAST_PLAYED:
            return "Last played";
        case SORT_PLAY_TIME:
            return "Play time";
        case SORT_ALPHA:
            return "Alphabetical";
        default:
            break;
    }
    return "";
}

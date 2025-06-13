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
#include "loader.hpp"
#include "server.hpp"

static constexpr size_t rowlen = 4, collen = 8;

MainScreen::MainScreen(void) : hid(rowlen * collen, collen)
{
    selectionTimer = 0;
    refreshTimer   = 0;

    staticBuf  = C2D_TextBufNew(261);
    dynamicBuf = C2D_TextBufNew(256);

    buttonBackup    = std::make_unique<Clickable>(204, 102, 110, 35, COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT, "Backup \uE004", true);
    buttonRestore   = std::make_unique<Clickable>(204, 139, 110, 35, COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT, "Restore \uE005", true);
    buttonCheats    = std::make_unique<Clickable>(204, 176, 110, 36, COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT, "Cheats", true);
    buttonPlayCoins = std::make_unique<Clickable>(204, 176, 110, 36, COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT, "\uE075 Coins", true);
    directoryList   = std::make_unique<Scrollable>(6, 102, 196, 110, 5);
    buttonBackup->canChangeColorWhenSelected(true);
    buttonRestore->canChangeColorWhenSelected(true);
    buttonCheats->canChangeColorWhenSelected(true);
    buttonPlayCoins->canChangeColorWhenSelected(true);

    sprintf(ver, "v%d.%d.%d", VERSION_MAJOR, VERSION_MINOR, VERSION_MICRO);

    C2D_TextParse(&ins1, staticBuf, "Hold SELECT to see commands. Press \uE002 for ");
    C2D_TextParse(&ins2, staticBuf, "extdata");
    C2D_TextParse(&ins3, staticBuf, ".");
    C2D_TextParse(&ins4, staticBuf, "Press \uE073 or START to exit.");
    C2D_TextParse(&version, staticBuf, ver);
    C2D_TextParse(&checkpoint, staticBuf, "checkpoint");
    C2D_TextParse(&c2dId, staticBuf, "ID:");
    C2D_TextParse(&c2dMediatype, staticBuf, "Mediatype:");

    C2D_TextParse(&top_move, staticBuf, "\uE006 to move between titles");
    C2D_TextParse(&top_a, staticBuf, "\uE000 to enter target");
    C2D_TextParse(&top_y, staticBuf, "\uE003 to select multiple titles");
    C2D_TextParse(&top_my, staticBuf, "\uE003 hold to select all titles");
    C2D_TextParse(&top_b, staticBuf, "\uE001 to exit target or deselect all titles");
    C2D_TextParse(&top_hb, staticBuf, "\uE001 hold to refresh titles");
    C2D_TextParse(&bot_ts, staticBuf, "\uE01D \uE006 to move between backups");
    C2D_TextParse(&bot_x, staticBuf, "\uE002 to delete backups");
    C2D_TextParse(&coins, staticBuf, "\uE075");

    C2D_TextOptimize(&ins1);
    C2D_TextOptimize(&ins2);
    C2D_TextOptimize(&ins3);
    C2D_TextOptimize(&ins4);
    C2D_TextOptimize(&version);
    C2D_TextOptimize(&checkpoint);
    C2D_TextOptimize(&c2dId);
    C2D_TextOptimize(&c2dMediatype);

    C2D_TextOptimize(&top_move);
    C2D_TextOptimize(&top_a);
    C2D_TextOptimize(&top_y);
    C2D_TextOptimize(&top_my);
    C2D_TextOptimize(&top_b);
    C2D_TextOptimize(&top_hb);
    C2D_TextOptimize(&bot_ts);
    C2D_TextOptimize(&bot_x);
    C2D_TextOptimize(&coins);

    C2D_PlainImageTint(&checkboxTint, COLOR_BLACK_DARKERR, 1.0f);
    C2D_PlainImageTint(&flagTint, COLOR_PURPLE_LIGHT, 1.0f);
}

MainScreen::~MainScreen(void)
{
    C2D_TextBufDelete(dynamicBuf);
    C2D_TextBufDelete(staticBuf);
}

void MainScreen::drawTop(void) const
{
    auto selEnt          = MS::selectedEntries();
    const size_t entries = hid.maxVisibleEntries();
    const size_t max     = hid.maxEntries(TitleLoader::getTitleCount()) + 1;

    C2D_TargetClear(g_top, COLOR_BLACK_DARKERR);
    C2D_TargetClear(g_bottom, COLOR_BLACK_DARKERR);

    C2D_SceneBegin(g_top);
    C2D_DrawRectSolid(0, 0, 0.5f, 400, 19, COLOR_BLACK_DARKER);
    C2D_DrawRectSolid(0, 221, 0.5f, 400, 19, COLOR_BLACK_DARKER);

    C2D_Text timeText;
    C2D_TextParse(&timeText, dynamicBuf, DateTime::timeStr().c_str());
    C2D_TextOptimize(&timeText);
    C2D_DrawText(&timeText, C2D_WithColor, 4.0f, 3.0f, 0.5f, 0.45f, 0.45f, COLOR_GREY_LIGHT);

    C2D_DrawText(&version, C2D_WithColor, 400 - 4 - ceilf(0.45f * version.width), 3.0f, 0.5f, 0.45f, 0.45f, COLOR_GREY_LIGHT);
    C2D_DrawImageAt(flag, 400 - 24 - ceilf(version.width * 0.45f), 0.0f, 0.5f, &flagTint, 1.0f, 1.0f);
    C2D_DrawText(&checkpoint, C2D_WithColor, 400 - 6 - 0.45f * version.width - 0.5f * checkpoint.width - 19, 2.0f, 0.5f, 0.5f, 0.5f, COLOR_WHITE);

    if (g_isLoadingTitles) {
        // Show a loading message
        int percentage = g_loadingTitlesLimit == 0 ? 0 : (g_loadingTitlesCounter * 100) / g_loadingTitlesLimit;
        if (percentage >= 100) {
            percentage = 99;
        }

        char loadingMessage[32] = {0};
        snprintf(loadingMessage, sizeof(loadingMessage), "Loading titles... %d%%", percentage);

        C2D_Text loadingText;
        C2D_TextParse(&loadingText, dynamicBuf, loadingMessage);
        C2D_TextOptimize(&loadingText);
        C2D_DrawText(&loadingText, C2D_WithColor, ceilf((400 - StringUtils::textWidth(loadingText, 0.6f)) / 2),
            ceilf((240 - 0.6f * fontGetInfo(NULL)->lineFeed) / 2), 0.9f, 0.6f, 0.6f, COLOR_WHITE);
    }
    else {
        for (size_t k = hid.page() * entries; k < hid.page() * entries + max; k++) {
            C2D_Image titleIcon = TitleLoader::icon(k);
            if (titleIcon.subtex->width == 48) {
                C2D_DrawImageAt(titleIcon, selectorX(k) + 1, selectorY(k) + 1, 0.5f, NULL, 1.0f, 1.0f);
            }
            else {
                C2D_DrawImageAt(titleIcon, selectorX(k) + 9, selectorY(k) + 9, 0.5f, NULL, 1.0f, 1.0f);
            }
        }

        if (TitleLoader::getTitleCount() > 0) {
            drawSelector();
        }

        for (size_t k = hid.page() * entries; k < hid.page() * entries + max; k++) {
            if (!selEnt.empty() && std::find(selEnt.begin(), selEnt.end(), k) != selEnt.end()) {
                C2D_DrawRectSolid(selectorX(k) + 31, selectorY(k) + 31, 0.5f, 16, 16, COLOR_WHITE);
                C2D_SpriteSetPos(&checkbox, selectorX(k) + 27, selectorY(k) + 27);
                C2D_DrawSpriteTinted(&checkbox, &checkboxTint);
            }

            if (TitleLoader::favorite(k)) {
                C2D_DrawRectSolid(selectorX(k) + 31, selectorY(k) + 3, 0.5f, 16, 16, COLOR_GOLD);
                C2D_SpriteSetPos(&star, selectorX(k) + 27, selectorY(k) - 1);
                C2D_DrawSpriteTinted(&star, &checkboxTint);
            }
        }

        if (hidKeysHeld() & KEY_SELECT) {
            const u32 inst_lh      = scaleInst * fontGetInfo(NULL)->lineFeed;
            const u32 total_height = inst_lh * 6;
            const u32 inst_h       = (240 - total_height) / 2;

            C2D_DrawRectSolid(0, 0, 0.5f, 400, 240, COLOR_OVERLAY);
            C2D_DrawText(&top_move, C2D_WithColor, ceilf((400 - StringUtils::textWidth(top_move, scaleInst)) / 2), inst_h, 0.9f, scaleInst, scaleInst,
                COLOR_WHITE);
            C2D_DrawText(&top_a, C2D_WithColor, ceilf((400 - StringUtils::textWidth(top_a, scaleInst)) / 2), inst_h + inst_lh * 1, 0.9f, scaleInst,
                scaleInst, COLOR_WHITE);
            C2D_DrawText(&top_b, C2D_WithColor, ceilf((400 - StringUtils::textWidth(top_b, scaleInst)) / 2), inst_h + inst_lh * 2, 0.9f, scaleInst,
                scaleInst, COLOR_WHITE);
            C2D_DrawText(&top_y, C2D_WithColor, ceilf((400 - StringUtils::textWidth(top_y, scaleInst)) / 2), inst_h + inst_lh * 3, 0.9f, scaleInst,
                scaleInst, COLOR_WHITE);
            C2D_DrawText(&top_my, C2D_WithColor, ceilf((400 - StringUtils::textWidth(top_my, scaleInst)) / 2), inst_h + inst_lh * 4, 0.9f, scaleInst,
                scaleInst, COLOR_WHITE);
            C2D_DrawText(&top_hb, C2D_WithColor, ceilf((400 - StringUtils::textWidth(top_hb, scaleInst)) / 2), inst_h + inst_lh * 5, 0.9f, scaleInst,
                scaleInst, COLOR_WHITE);
        }

        if (hidKeysHeld() & KEY_SELECT && Server::isRunning() && Server::getAddress().length() > 0) {
            C2D_Text logsText;
            C2D_TextParse(&logsText, dynamicBuf, ("Logs available at " + Server::getAddress() + "/logs/memory").c_str());
            C2D_TextOptimize(&logsText);
            C2D_DrawText(&logsText, C2D_WithColor, ceilf((400 - logsText.width * 0.47f) / 2), 223, 0.5f, 0.47f, 0.47f, COLOR_GREY_LIGHT);
        }
        else {
            static const float border = ceilf((400 - (ins1.width + ins2.width + ins3.width) * 0.47f) / 2);
            C2D_DrawText(&ins1, C2D_WithColor, border, 223, 0.5f, 0.47f, 0.47f, COLOR_GREY_LIGHT);
            C2D_DrawText(&ins2, C2D_WithColor, border + ceilf(ins1.width * 0.47f), 223, 0.5f, 0.47f, 0.47f,
                Archive::mode() == MODE_SAVE ? COLOR_WHITE : COLOR_RED);
            C2D_DrawText(&ins3, C2D_WithColor, border + ceilf((ins1.width + ins2.width) * 0.47f), 223, 0.5f, 0.47f, 0.47f, COLOR_GREY_LIGHT);
        }

        if (g_isTransferringFile) {
            C2D_DrawRectSolid(0, 0, 0.5f, 400, 240, COLOR_OVERLAY);

            float size = 0.7f;
            C2D_Text text;
            C2D_TextParse(&text, dynamicBuf, StringUtils::UTF16toUTF8(g_currentFile).c_str());
            C2D_TextOptimize(&text);
            C2D_DrawText(&text, C2D_WithColor, ceilf((400 - StringUtils::textWidth(text, size)) / 2),
                ceilf((240 - size * fontGetInfo(NULL)->lineFeed) / 2), 0.9f, size, size, COLOR_WHITE);
        }
    }
}

void MainScreen::drawBottom(void) const
{
    C2D_TextBufClear(dynamicBuf);

    const Mode_t mode = Archive::mode();

    C2D_DrawRectSolid(0, 0, 0.5f, 320, 19, COLOR_BLACK_DARKER);
    C2D_DrawRectSolid(0, 221, 0.5f, 320, 19, COLOR_BLACK_DARKER);

    if (g_isLoadingTitles) {}

    else if (TitleLoader::getTitleCount() > 0) {
        Title title;
        TitleLoader::getTitle(title, hid.fullIndex());

        directoryList->flush();
        std::vector<std::u16string> dirs = mode == MODE_SAVE ? title.saves() : title.extdata();
        static std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t> convert;

        for (size_t i = 0; i < dirs.size(); i++) {
            directoryList->push_back(COLOR_BLACK_DARKERR, COLOR_WHITE, convert.to_bytes(dirs.at(i)), i == directoryList->index());
        }

        C2D_Text longDesc, c2dTitleInfo;
        std::string desc = title.longDescription();
        std::replace(desc.begin(), desc.end(), '\n', ' ');

        std::string titleInfo =
            StringUtils::format("ID: %08X (%s)\nMedia type: %s", (int)title.lowId(), title.productCode, title.mediaTypeString().c_str());

        C2D_TextParse(&longDesc, dynamicBuf, desc.c_str());
        C2D_TextParse(&c2dTitleInfo, dynamicBuf, titleInfo.c_str());

        C2D_TextOptimize(&longDesc);
        C2D_TextOptimize(&c2dTitleInfo);

        C2D_DrawText(&longDesc, C2D_WithColor, 4, 1, 0.5f, 0.55f, 0.55f, COLOR_WHITE);
        C2D_DrawText(&c2dTitleInfo, C2D_WithColor, 4, 29, 0.5f, 0.5f, 0.5f, COLOR_GREY_LIGHT);

        C2D_DrawRectSolid(260, 27, 0.5f, 52, 52, COLOR_PURPLE_DARK);
        if (title.icon().subtex->width == 48) {
            C2D_DrawImageAt(title.icon(), 262, 29, 0.5f, NULL, 1.0f, 1.0f);
        }
        else {
            C2D_DrawImageAt(title.icon(), 262 + 8, 29 + 8, 0.5f, NULL, 1.0f, 1.0f);
        }

        C2D_DrawRectSolid(4, 100, 0.5f, 312, 114, COLOR_BLACK_DARK);

        directoryList->draw(g_bottomScrollEnabled);
        buttonBackup->draw(0.7, COLOR_PURPLE_LIGHT);
        buttonRestore->draw(0.7, COLOR_PURPLE_LIGHT);
        if (title.isActivityLog()) {
            buttonPlayCoins->draw(0.7, COLOR_PURPLE_LIGHT);
        }
        else {
            buttonCheats->draw(0.7, COLOR_PURPLE_LIGHT);
        }
    }

    C2D_DrawText(&ins4, C2D_WithColor, ceilf((320 - ins4.width * 0.47f) / 2), 223, 0.5f, 0.47f, 0.47f, COLOR_GREY_LIGHT);

    if (hidKeysHeld() & KEY_SELECT) {
        C2D_DrawRectSolid(0, 0, 0.5f, 320, 240, COLOR_OVERLAY);
        C2D_DrawText(&bot_ts, C2D_WithColor, (320 - bot_ts.width * scaleInst) / 2, 102, 0.5f, scaleInst, scaleInst, COLOR_WHITE);
        C2D_DrawText(&bot_x, C2D_WithColor, (320 - bot_x.width * scaleInst) / 2, 132, 0.5f, scaleInst, scaleInst, COLOR_WHITE);
        // play coins
        C2D_DrawText(&coins, C2D_WithColor, ceilf(318 - StringUtils::textWidth(coins, scaleInst)), -1, 0.5f, scaleInst, scaleInst, COLOR_GOLD);
    }

    if (g_isTransferringFile) {
        C2D_DrawRectSolid(0, 0, 0.5f, 400, 240, COLOR_OVERLAY);
    }
}

void MainScreen::update(const InputState& input)
{
    updateSelector();
    handleEvents(input);
}

void MainScreen::updateSelector(void)
{
    if (g_isLoadingTitles) {
        // Don't update selection while loading
        return;
    }

    if (!g_bottomScrollEnabled) {
        size_t count = TitleLoader::getTitleCount();
        if (count > 0) {
            hid.update(count);
            directoryList->resetIndex();
        }
    }
    else {
        directoryList->updateSelection();
    }
}

void MainScreen::handleEvents(const InputState& input)
{
    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();
    // Handle pressing A
    // Backup list active:   Backup/Restore
    // Backup list inactive: Activate backup list only if multiple
    //                       selections are enabled
    if (kDown & KEY_A) {
        // If backup list is active...
        if (g_bottomScrollEnabled) {
            // If the "New..." entry is selected...
            if (0 == directoryList->index()) {
                currentOverlay = std::make_shared<YesNoOverlay>(
                    *this, "Backup selected title?",
                    [this]() {
                        auto result = io::backup(hid.fullIndex(), 0);
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
                currentOverlay = std::make_shared<YesNoOverlay>(
                    *this, "Restore selected title?",
                    [this]() {
                        size_t cellIndex = directoryList->index();
                        auto result      = io::restore(hid.fullIndex(), cellIndex, nameFromCell(cellIndex));
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
            // Activate backup list only if multiple selections are not enabled
            if (!MS::multipleSelectionEnabled()) {
                g_bottomScrollEnabled = true;
                updateButtons();
            }
        }
    }

    if (kDown & KEY_B) {
        g_bottomScrollEnabled = false;
        MS::clearSelectedEntries();
        directoryList->resetIndex();
        updateButtons();
    }

    if (kDown & KEY_X) {
        if (g_bottomScrollEnabled) {
            bool isSaveMode = Archive::mode() == MODE_SAVE;
            size_t index    = directoryList->index();
            // avoid actions if X is pressed on "New..."
            if (index > 0) {
                currentOverlay = std::make_shared<YesNoOverlay>(
                    *this, "Delete selected backup?",
                    [this, isSaveMode, index]() {
                        Title title;
                        TitleLoader::getTitle(title, hid.fullIndex());
                        std::u16string path = isSaveMode ? title.fullSavePath(index) : title.fullExtdataPath(index);
                        io::deleteBackupFolder(path);
                        TitleLoader::refreshDirectories(title.id());
                        directoryList->setIndex(index - 1);
                        this->removeOverlay();
                    },
                    [this]() { this->removeOverlay(); });
            }
        }
        else {
            hid.reset();
            Archive::mode(Archive::mode() == MODE_SAVE ? MODE_EXTDATA : MODE_SAVE);
            MS::clearSelectedEntries();
            directoryList->resetIndex();
        }
    }

    if (kDown & KEY_Y) {
        if (g_bottomScrollEnabled) {
            directoryList->resetIndex();
            g_bottomScrollEnabled = false;
        }
        MS::addSelectedEntry(hid.fullIndex());
        updateButtons(); // Do this last
    }

    if (kHeld & KEY_Y) {
        selectionTimer++;
    }
    else {
        selectionTimer = 0;
    }

    if (selectionTimer > 90) {
        MS::clearSelectedEntries();
        for (size_t i = 0, sz = TitleLoader::getTitleCount(); i < sz; i++) {
            MS::addSelectedEntry(i);
        }
        selectionTimer = 0;
    }

    if (kHeld & KEY_B) {
        refreshTimer++;
    }
    else {
        refreshTimer = 0;
    }

    if (refreshTimer > 90) {
        hid.reset();
        MS::clearSelectedEntries();
        directoryList->resetIndex();
        Threads::executeTask(TitleLoader::loadTitlesThread);
        refreshTimer = 0;
    }

    if (buttonBackup->released() || (kDown & KEY_L)) {
        if (MS::multipleSelectionEnabled()) {
            directoryList->resetIndex();
            std::vector<size_t> list = MS::selectedEntries();
            for (size_t i = 0, sz = list.size(); i < sz; i++) {
                auto result = io::backup(list.at(i), directoryList->index());
                if (std::get<0>(result)) {
                    currentOverlay = std::make_shared<InfoOverlay>(*this, std::get<2>(result));
                }
                else {
                    currentOverlay = std::make_shared<ErrorOverlay>(*this, std::get<1>(result), std::get<2>(result));
                }
            }
            MS::clearSelectedEntries();
            updateButtons();
        }
        else if (g_bottomScrollEnabled) {
            currentOverlay = std::make_shared<YesNoOverlay>(
                *this, "Backup selected save?",
                [this]() {
                    auto result = io::backup(hid.fullIndex(), directoryList->index());
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

    if (buttonRestore->released() || (kDown & KEY_R)) {
        size_t cellIndex = directoryList->index();
        if (MS::multipleSelectionEnabled()) {
            MS::clearSelectedEntries();
            updateButtons();
        }
        else if (g_bottomScrollEnabled && cellIndex > 0) {
            currentOverlay = std::make_shared<YesNoOverlay>(
                *this, "Restore selected save?",
                [this, cellIndex]() {
                    auto result = io::restore(hid.fullIndex(), cellIndex, nameFromCell(cellIndex));
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

    if (TitleLoader::getTitleCount() > 0) {
        Title title;
        TitleLoader::getTitle(title, hid.fullIndex());
        if ((title.isActivityLog() && buttonPlayCoins->released()) || ((hidKeysDown() & KEY_TOUCH) && input.py < 20 && input.px > 294)) {
            if (!Archive::setPlayCoins()) {
                currentOverlay = std::make_shared<ErrorOverlay>(*this, -1, "Failed to set play coins.");
            }
        }
        else {
            if (buttonCheats->released() && CheatManager::getInstance().cheats() != nullptr) {
                if (MS::multipleSelectionEnabled()) {
                    MS::clearSelectedEntries();
                    updateButtons();
                }
                else {
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
    }
}

int MainScreen::selectorX(size_t i) const
{
    return 50 * ((i % (rowlen * collen)) % collen);
}

int MainScreen::selectorY(size_t i) const
{
    return 20 + 50 * ((i % (rowlen * collen)) / collen);
}

void MainScreen::drawSelector(void) const
{
    static const int w         = 2;
    const int x                = selectorX(hid.index());
    const int y                = selectorY(hid.index());
    float highlight_multiplier = fmax(0.0, fabs(fmod(g_timer, 1.0) - 0.5) / 0.5);
    u8 r                       = COLOR_PURPLE_LIGHT & 0xFF;
    u8 g                       = (COLOR_PURPLE_LIGHT >> 8) & 0xFF;
    u8 b                       = (COLOR_PURPLE_LIGHT >> 16) & 0xFF;
    u32 color = C2D_Color32(r + (255 - r) * highlight_multiplier, g + (255 - g) * highlight_multiplier, b + (255 - b) * highlight_multiplier, 255);

    C2D_DrawRectSolid(x, y, 0.5f, 50, 50, COLOR_WHITEMASK);
    C2D_DrawRectSolid(x, y, 0.5f, 50, w, color);                      // top
    C2D_DrawRectSolid(x, y + w, 0.5f, w, 50 - 2 * w, color);          // left
    C2D_DrawRectSolid(x + 50 - w, y + w, 0.5f, w, 50 - 2 * w, color); // right
    C2D_DrawRectSolid(x, y + 50 - w, 0.5f, 50, w, color);             // bottom
}

void MainScreen::updateButtons(void)
{
    if (MS::multipleSelectionEnabled()) {
        buttonRestore->canChangeColorWhenSelected(true);
        buttonRestore->canChangeColorWhenSelected(false);
        buttonCheats->canChangeColorWhenSelected(false);
        buttonPlayCoins->canChangeColorWhenSelected(false);
        buttonBackup->setColors(COLOR_BLACK_DARKERR, COLOR_WHITE);
        buttonRestore->setColors(COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT);
        buttonCheats->setColors(COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT);
        buttonPlayCoins->setColors(COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT);
    }
    else if (g_bottomScrollEnabled) {
        buttonBackup->canChangeColorWhenSelected(true);
        buttonRestore->canChangeColorWhenSelected(true);
        buttonCheats->canChangeColorWhenSelected(true);
        buttonPlayCoins->canChangeColorWhenSelected(true);
        buttonBackup->setColors(COLOR_BLACK_DARKERR, COLOR_WHITE);
        buttonRestore->setColors(COLOR_BLACK_DARKERR, COLOR_WHITE);
        buttonCheats->setColors(COLOR_BLACK_DARKERR, COLOR_WHITE);
        buttonPlayCoins->setColors(COLOR_BLACK_DARKERR, COLOR_WHITE);
    }
    else {
        buttonBackup->setColors(COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT);
        buttonRestore->setColors(COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT);
        buttonCheats->setColors(COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT);
        buttonPlayCoins->setColors(COLOR_BLACK_DARKERR, COLOR_GREY_LIGHT);
    }
}

std::string MainScreen::nameFromCell(size_t index) const
{
    return directoryList->cellName(index);
}

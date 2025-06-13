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

#include "clickable.hpp"

bool Clickable::held()
{
    return g_input->touch.count > 0 && g_input->touch.touches[0].y > (unsigned)my && g_input->touch.touches[0].y < (unsigned)(my + mh) &&
           g_input->touch.touches[0].x > (unsigned)mx && g_input->touch.touches[0].x < (unsigned)(mx + mw);
}

bool Clickable::released()
{
    const auto [on, currentlyTouching] = [this]() {
        return std::make_pair(g_input->touch.count > 0 && g_input->touch.touches[0].y > (unsigned)my &&
                                  g_input->touch.touches[0].y < (unsigned)(my + mh) && g_input->touch.touches[0].x > (unsigned)mx &&
                                  g_input->touch.touches[0].x < (unsigned)(mx + mw),
            g_input->touch.count > 0);
    }();

    if (on) {
        mOldPressed = true;
    }
    else {
        if (mOldPressed && !currentlyTouching) {
            mOldPressed = false;
            return true;
        }
        mOldPressed = false;
    }

    return false;
}

void Clickable::draw(float font, SDL_Color overlay)
{
    u32 textw, texth;
    SDLH_GetTextDimensions(font, mText.c_str(), &textw, &texth);
    const u32 messageWidth = mCentered ? textw : mw - 20;

    SDLH_DrawRect(mx, my, mw, mh, mColorBg);
    if (mCanChangeColorWhenSelected && held()) {
        SDLH_DrawRect(mx, my, mw, mh, FC_MakeColor(overlay.r, overlay.g, overlay.b, 100));
    }
    if (!mCentered && mSelected) {
        SDLH_DrawRect(mx + 4, my + 6, 4, mh - 12, COLOR_WHITE);
    }
    if (mSelected) {
        SDLH_DrawRect(mx, my, mw, mh, FC_MakeColor(overlay.r, overlay.g, overlay.b, 100));
    }
    u32 offset = mx + (mw - messageWidth) / 2 + (!mCentered ? 8 : 0);
    SDLH_DrawTextBox(font, offset, my + (mh - texth) / 2 + 2, mColorText, mw - 4 * 2, mText.c_str());
}

void Clickable::drawOutline(SDL_Color color)
{
    drawPulsingOutline(mx, my, mw, mh, 4, color);
}
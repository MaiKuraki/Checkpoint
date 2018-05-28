/*
*   This file is part of Checkpoint
*   Copyright (C) 2017-2018 Bernardo Giordano
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

#include "confighandler.hpp"

Configuration::Configuration(void)
{
    // check for existing config.json files on the sd card, BASEPATH
    if (!io::fileExists(Archive::sdmc(), StringUtils::UTF8toUTF16(BASEPATH.c_str())))
    {
        std::ifstream src("romfs:/config.json");
        std::ofstream dst(BASEPATH);
        dst << src.rdbuf();
        src.close();
        dst.close();
    }

    // load json config file
    std::ifstream i(BASEPATH);
    i >> mJson;
    i.close();

    // TODO: check for json version to match with the app

    // parse filters
    std::vector<std::string> filter = mJson["filter"];
    for (auto& id : filter)
    {
        mFilterIds.emplace(strtoull(id.c_str(), NULL, 16));
    }

    // parse nand saves
    mNandSaves = mJson["nandsaves"];
}

bool Configuration::filter(u64 id)
{
    return mFilterIds.find(id) != mFilterIds.end();
}

bool Configuration::nandSaves(void)
{
    return mNandSaves;
}
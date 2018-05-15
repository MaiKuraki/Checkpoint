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

#include "title.hpp"

static bool checkHigh(u64 id);
static C2D_Image loadTextureIcon(smdh_s *smdh);

static std::vector<Title> titleSaves;
static std::vector<Title> titleExtdatas;

static void exportTitleListCache(std::vector<Title> list, const std::u16string path);
static void importTitleListCache(void);

void Title::load(void)
{
    mId = 0xFFFFFFFFFFFFFFFF;
    mMedia = MEDIATYPE_SD;
    mCard = CARD_CTR;
    memset(productCode, 0, 16);
    mShortDescription = StringUtils::UTF8toUTF16(" ");
    mLongDescription = StringUtils::UTF8toUTF16(" ");
    mSavePath = StringUtils::UTF8toUTF16(" ");
    mExtdataPath = StringUtils::UTF8toUTF16(" ");
    mIcon = Gui::noIcon();
    mAccessibleSave = false;
    mAccessibleExtdata = false;
    mSaves.clear();
    mExtdata.clear();
}

bool Title::load(u64 _id, FS_MediaType _media, FS_CardType _card)
{
    bool loadTitle = false;
    mId = _id;
    mMedia = _media;
    mCard = _card;

    if (mCard == CARD_CTR)
    {
        smdh_s *smdh = loadSMDH(lowId(), highId(), mMedia);
        if (smdh == NULL)
        {
            return false;
        }
        
        char unique[12] = {0};
        sprintf(unique, "0x%05X ", (unsigned int)uniqueId());
        
        mShortDescription = StringUtils::removeForbiddenCharacters((char16_t*)smdh->applicationTitles[1].shortDescription);
        mLongDescription = (char16_t*)smdh->applicationTitles[1].longDescription;
        mSavePath = StringUtils::UTF8toUTF16("/3ds/Checkpoint/saves/") + StringUtils::UTF8toUTF16(unique) + mShortDescription;
        mExtdataPath = StringUtils::UTF8toUTF16("/3ds/Checkpoint/extdata/") + StringUtils::UTF8toUTF16(unique) + mShortDescription;
        AM_GetTitleProductCode(mMedia, mId, productCode);
        
        mAccessibleSave = Archive::accessible(mediaType(), lowId(), highId());
        mAccessibleExtdata = Archive::accessible(extdataId());
        
        if (mAccessibleSave)
        {
            loadTitle = true;
            if (!io::directoryExists(Archive::sdmc(), mSavePath))
            {
                Result res = io::createDirectory(Archive::sdmc(), mSavePath);
                if (R_FAILED(res))
                {
                    Gui::createError(res, "Failed to create backup directory.");
                }
            }		
        }

        if (mAccessibleExtdata)
        {
            loadTitle = true;
            if (!io::directoryExists(Archive::sdmc(), mExtdataPath))
            {
                Result res = io::createDirectory(Archive::sdmc(), mExtdataPath);
                if (R_FAILED(res))
                {
                    Gui::createError(res, "Failed to create backup directory.");
                }
            }
        }
        
        if (loadTitle)
        {
            mIcon = loadTextureIcon(smdh);
        }
        
        delete smdh;		
    }
    else
    {
        u8* headerData = new u8[0x3B4];
        Result res = FSUSER_GetLegacyRomHeader(mMedia, 0LL, headerData);
        if (R_FAILED(res))
        {
            delete[] headerData;
            return false;
        }
        
        char _cardTitle[14] = {0};
        char _gameCode[6] = {0};
        
        std::copy(headerData, headerData + 12, _cardTitle);
        std::copy(headerData + 12, headerData + 16, _gameCode);
        _cardTitle[13] = '\0';
        _gameCode[5] = '\0';
        
        res = SPIGetCardType(&mCardType, (_gameCode[0] == 'I') ? 1 : 0);
        if (R_FAILED(res))
        {
            delete[] headerData;
            return false;
        }
        
        delete[] headerData;
        
        mShortDescription = StringUtils::removeForbiddenCharacters(StringUtils::UTF8toUTF16(_cardTitle));
        mLongDescription = mShortDescription;
        mSavePath = StringUtils::UTF8toUTF16("/3ds/Checkpoint/saves/") + StringUtils::UTF8toUTF16(_gameCode) + StringUtils::UTF8toUTF16(" ") + mShortDescription;
        mExtdataPath = mSavePath;
        memset(productCode, 0, 16);
        
        mAccessibleSave = true;
        mAccessibleExtdata = false;
        
        loadTitle = true;
        if (!io::directoryExists(Archive::sdmc(), mSavePath))
        {
            Result res = io::createDirectory(Archive::sdmc(), mSavePath);
            if (R_FAILED(res))
            {
                Gui::createError(res, "Failed to create backup directory.");
            }
        }
        
        mIcon = Gui::TWLIcon();
    }
    
    refreshDirectories();
    return loadTitle;
}

bool Title::accessibleSave(void)
{
    return mAccessibleSave;
}

bool Title::accessibleExtdata(void)
{
    return mAccessibleExtdata;
}

std::string Title::mediaTypeString(void)
{
    switch(mMedia)
    {
        case MEDIATYPE_SD: return "SD Card";
        case MEDIATYPE_GAME_CARD: return "Cartridge";
        case MEDIATYPE_NAND: return "NAND";
        default: return " ";
    }
    
    return " ";
}

std::string Title::shortDescription(void)
{
    return StringUtils::UTF16toUTF8(mShortDescription);
}

std::string Title::longDescription(void)
{
    return StringUtils::UTF16toUTF8(mLongDescription);
}

std::u16string Title::savePath(void)
{
    return mSavePath;
}

std::u16string Title::extdataPath(void)
{
    return mExtdataPath;
}

std::vector<std::u16string> Title::saves(void)
{
    return mSaves;
}

std::vector<std::u16string> Title::extdata(void)
{
    return mExtdata;
}

void Title::refreshDirectories(void)
{
    mSaves.clear();
    mExtdata.clear();
    
    if (accessibleSave())
    {
        // save backups
        Directory savelist(Archive::sdmc(), mSavePath);
        if (savelist.good())
        {
            for (size_t i = 0, sz = savelist.size(); i < sz; i++)
            {
                if (savelist.folder(i))
                {
                    mSaves.push_back(savelist.entry(i));
                }
            }
            
            std::sort(mSaves.rbegin(), mSaves.rend());
            mSaves.insert(mSaves.begin(), StringUtils::UTF8toUTF16("New..."));
        }
        else
        {
            Gui::createError(savelist.error(), "Couldn't retrieve the directory list for the title " + shortDescription());
        }
    }
    
    if (accessibleExtdata())
    {
        // extdata backups
        Directory extlist(Archive::sdmc(), mExtdataPath);
        if (extlist.good())
        {
            for (size_t i = 0, sz = extlist.size(); i < sz; i++)
            {
                if (extlist.folder(i))
                {
                    mExtdata.push_back(extlist.entry(i));
                }
            }
            
            std::sort(mExtdata.begin(), mExtdata.end());
            mExtdata.insert(mExtdata.begin(), StringUtils::UTF8toUTF16("New..."));
        }
        else
        {
            Gui::createError(extlist.error(), "Couldn't retrieve the extdata list for the title " + shortDescription());
        }
    }
}

u32 Title::highId(void)
{
    return (u32)(mId >> 32);
}

u32 Title::lowId(void)
{
    return (u32)mId;
}

u32 Title::uniqueId(void)
{
    return (lowId() >> 8);
}

u64 Title::id(void)
{
    return mId;
}

u32 Title::extdataId(void)
{
    u32 low = lowId();
    switch(low)
    {
        case 0x00055E00: return 0x055D; // Pokémon Y
        case 0x0011C400: return 0x11C5; // Pokémon Omega Ruby
        case 0x00175E00: return 0x1648; // Pokémon Moon
        case 0x00179600:
        case 0x00179800: return 0x1794; // Fire Emblem Conquest SE NA
        case 0x00179700:
        case 0x0017A800: return 0x1795; // Fire Emblem Conquest SE EU
        case 0x0012DD00:
        case 0x0012DE00: return 0x12DC; // Fire Emblem If JP
        case 0x001B5100: return 0x1B50; // Pokémon Ultramoon
    }
    
    return low >> 8;
}

FS_MediaType Title::mediaType(void)
{
    return mMedia;
}

FS_CardType Title::cardType(void)
{
    return mCard;
}

CardType Title::SPICardType(void)
{
    return mCardType;
}

C2D_Image Title::icon(void)
{
    return mIcon;
}

static bool checkHigh(u64 id)
{
    u32 high = id >> 32;
    return (high == 0x00040000 || high == 0x00040002);
}

void loadTitles(bool forceRefresh)
{
    std::u16string savecachePath = StringUtils::UTF8toUTF16("/3ds/Checkpoint/savecache");
    std::u16string extdatacachePath = StringUtils::UTF8toUTF16("/3ds/Checkpoint/extdatacache");
    
    // on refreshing
    titleSaves.clear();
    titleExtdatas.clear();

    bool optimizedLoad = false;
    
    u8 hash[SHA256_BLOCK_SIZE];
    calculateTitleDBHash(hash);

    std::u16string titlesHashPath = StringUtils::UTF8toUTF16("/3ds/Checkpoint/titles.sha");
    if (!io::fileExists(Archive::sdmc(), titlesHashPath) || !io::fileExists(Archive::sdmc(), savecachePath) || !io::fileExists(Archive::sdmc(), extdatacachePath))
    {
        // create title list sha256 hash file if it doesn't exist in the working directory
        FSStream output(Archive::sdmc(), titlesHashPath, FS_OPEN_WRITE, SHA256_BLOCK_SIZE);
        output.write(hash, SHA256_BLOCK_SIZE);
        output.close();
    }
    else
    {
        // compare current hash with the previous hash
        FSStream input(Archive::sdmc(), titlesHashPath, FS_OPEN_READ);
        if (input.good() && input.size() == SHA256_BLOCK_SIZE)
        {
            u8* buf = new u8[input.size()];
            input.read(buf, input.size());
            input.close();
            
            if (memcmp(hash, buf, SHA256_BLOCK_SIZE) == 0)
            {
                // hash matches
                optimizedLoad = true;
            }
            else
            {
                FSUSER_DeleteFile(Archive::sdmc(), fsMakePath(PATH_UTF16, titlesHashPath.data()));
                FSStream output(Archive::sdmc(), titlesHashPath, FS_OPEN_WRITE, SHA256_BLOCK_SIZE);
                output.write(hash, SHA256_BLOCK_SIZE);
                output.close();
            }
            
            delete[] buf;
        }
    }
    
    if (optimizedLoad && !forceRefresh)
    {
        // deserialize data
        importTitleListCache();
    }
    else
    {
        u32 count = 0;
        AM_GetTitleCount(MEDIATYPE_SD, &count);
        titleSaves.reserve(count + 1);
        titleExtdatas.reserve(count + 1);

        u64 ids[count];
        AM_GetTitleList(NULL, MEDIATYPE_SD, count, ids);

        for (u32 i = 0; i < count; i++)
        {
            if (checkHigh(ids[i]))
            {
                Title title;
                if (title.load(ids[i], MEDIATYPE_SD, CARD_CTR))
                {
                    if (title.accessibleSave())
                    {
                        titleSaves.push_back(title);
                    }
                    
                    if (title.accessibleExtdata())
                    {
                        titleExtdatas.push_back(title);
                    }
                }
            }
        }
        
        std::sort(titleSaves.begin(), titleSaves.end(), [](Title& l, Title& r) {
            return l.shortDescription() < r.shortDescription();
        });
        
        std::sort(titleExtdatas.begin(), titleExtdatas.end(), [](Title& l, Title& r) {
            return l.shortDescription() < r.shortDescription();
        });
        
        FS_CardType cardType;
        Result res = FSUSER_GetCardType(&cardType);
        if (R_SUCCEEDED(res))
        {
            if (cardType == CARD_CTR)
            {
                AM_GetTitleCount(MEDIATYPE_GAME_CARD, &count);
                if (count > 0)
                {
                    AM_GetTitleList(NULL, MEDIATYPE_GAME_CARD, count, ids);	
                    if (checkHigh(ids[0]))
                    {
                        Title title;
                        if (title.load(ids[0], MEDIATYPE_GAME_CARD, cardType))
                        {
                            if (title.accessibleSave())
                            {
                                titleSaves.insert(titleSaves.begin(), title);
                            }
                            
                            if (title.accessibleExtdata())
                            {
                                titleExtdatas.insert(titleExtdatas.begin(), title);
                            }
                        }
                    }
                }
            }
            else
            {
                Title title;
                if (title.load(0, MEDIATYPE_GAME_CARD, cardType))
                {
                    titleSaves.insert(titleSaves.begin(), title);
                }
            }
        }
    }
    
    // serialize data
    exportTitleListCache(titleSaves, savecachePath);
    exportTitleListCache(titleExtdatas, extdatacachePath);
}

void getTitle(Title &dst, int i)
{
    const Mode_t mode = Archive::mode();
    if (i < getTitleCount())
    {
        dst = mode == MODE_SAVE ? titleSaves.at(i) : titleExtdatas.at(i);
    }	
}

int getTitleCount(void)
{
    const Mode_t mode = Archive::mode();
    return mode == MODE_SAVE ? titleSaves.size() : titleExtdatas.size();
}

C2D_Image icon(int i)
{
    const Mode_t mode = Archive::mode();
    return mode == MODE_SAVE ? titleSaves.at(i).icon() : titleExtdatas.at(i).icon();
}

static C2D_Image loadTextureIcon(smdh_s *smdh)
{
    C3D_Tex* tex = (C3D_Tex*)malloc(sizeof(C3D_Tex));
    static const Tex3DS_SubTexture subt3x = { 48, 48, 0.0f, 48/64.0f, 48/64.0f, 0.0f };
    C2D_Image image = (C2D_Image){ tex, &subt3x };
    C3D_TexInit(image.tex, 64, 64, GPU_RGB565);
    
    u16* dest = (u16*)image.tex->data + (64-48)*64;
    u16* src = (u16*)smdh->bigIconData;
    for (int j = 0; j < 48; j += 8)
    {
        memcpy(dest, src, 48*8*sizeof(u16));
        src += 48*8;
        dest += 64*8;
    }
    
    return image;
}

void refreshDirectories(u64 id)
{
    const Mode_t mode = Archive::mode();
    if (mode == MODE_SAVE)
    {
        for (size_t i = 0; i < titleSaves.size(); i++)
        {
            if (titleSaves.at(i).id() == id)
            {
                titleSaves.at(i).refreshDirectories();
            }
        }
    }
    else
    {
        for (size_t i = 0; i < titleExtdatas.size(); i++)
        {
            if (titleExtdatas.at(i).id() == id)
            {
                titleExtdatas.at(i).refreshDirectories();
            }
        }
    }
}

static void exportTitleListCache(std::vector<Title> list, const std::u16string path)
{
    u8* cache = new u8[list.size() * 10];
    for (size_t i = 0; i < list.size(); i++)
    {
        u64 id = list.at(i).id();
        FS_MediaType media = list.at(i).mediaType();
        FS_CardType card = list.at(i).cardType();
        memcpy(cache + i*10 + 0, &id, sizeof(u64));
        memcpy(cache + i*10 + 8, &media, sizeof(u8));
        memcpy(cache + i*10 + 9, &card, sizeof(u8));
    }
    FSUSER_DeleteFile(Archive::sdmc(), fsMakePath(PATH_UTF16, path.data()));
    FSStream output(Archive::sdmc(), path, FS_OPEN_WRITE, list.size() * 10);
    output.write(cache, list.size() * 10);
    output.close();
    delete[] cache;
}

static void importTitleListCache(void)
{
    FSStream inputsaves(Archive::sdmc(), StringUtils::UTF8toUTF16("/3ds/Checkpoint/savecache"), FS_OPEN_READ);
    u32 sizesaves = inputsaves.size() / 10;
    u8* cachesaves = new u8[inputsaves.size()];
    inputsaves.read(cachesaves, inputsaves.size());
    inputsaves.close();
    
    FSStream inputextdatas(Archive::sdmc(), StringUtils::UTF8toUTF16("/3ds/Checkpoint/extdatacache"), FS_OPEN_READ);
    u32 sizeextdatas = inputextdatas.size() / 10;
    u8* cacheextdatas = new u8[inputextdatas.size()];
    inputextdatas.read(cacheextdatas, inputextdatas.size());
    inputextdatas.close();
    
    // fill the lists with blank titles firsts
    for (size_t i = 0, sz = std::max(sizesaves, sizeextdatas); i < sz; i++)
    {
        Title title;
        title.load();
        if (i < sizesaves)
        {
            titleSaves.push_back(title);
        }
        if (i < sizeextdatas)
        {
            titleExtdatas.push_back(title);
        }
    }
    
    // store already loaded ids
    std::vector<u64> alreadystored;
    
    for (size_t i = 0; i < sizesaves; i++)
    {
        u64 id;
        FS_MediaType media;
        FS_CardType card;
        memcpy(&id, cachesaves + i*10, sizeof(u64));
        memcpy(&media, cachesaves + i*10 + 8, sizeof(u8));
        memcpy(&card, cachesaves + i*10 + 9, sizeof(u8));
        Title title;
        title.load(id, media, card);
        titleSaves.at(i) = title;
        alreadystored.push_back(id);
    }
    
    for (size_t i = 0; i < sizeextdatas; i++)
    {
        u64 id;
        memcpy(&id, cacheextdatas + i*10, sizeof(u64));
        std::vector<u64>::iterator it = find(alreadystored.begin(), alreadystored.end(), id);
        if (it == alreadystored.end())
        {
            FS_MediaType media;
            FS_CardType card;
            memcpy(&media, cacheextdatas + i*10 + 8, sizeof(u8));
            memcpy(&card, cacheextdatas + i*10 + 9, sizeof(u8));
            Title title;
            title.load(id, media, card);
            titleExtdatas.at(i) = title;			
        }
        else
        {
            auto pos = it - alreadystored.begin();
            
            // avoid to copy a cartridge title into the extdata list twice
            if (i != 0 && pos == 0)
            {
                auto newpos = find(alreadystored.rbegin(), alreadystored.rend(), id);
                titleExtdatas.at(i) = titleSaves.at(alreadystored.rend() - newpos);
            }
            else
            {
                titleExtdatas.at(i) = titleSaves.at(pos);
            }
        }
    }
    
    delete[] cachesaves;
    delete[] cacheextdatas;
}
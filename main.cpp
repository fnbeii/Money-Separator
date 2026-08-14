// A port from PC to Android (https://github.com/The-Musaigen/money-separator) | thanks for RusJJ
#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>
#include <string>
#include <stdint.h>

MYMODCFG(net.KillerSA.mnfy.moneyseparator, Money Separator, 2.3, KillerSA)

std::string separator = ".";
std::string centSeparator = ".";
int displayMode = 1;
bool useCustomRGB = false;
int moneyR = 50, moneyG = 255, moneyB = 50;

struct CRGBA {
    uint8_t r, g, b, a;
};

void* g_pPlayerInfo = nullptr;

static std::string AddSeparators(std::string aValue) 
{
    if (displayMode == 0 || aValue.empty()) return aValue;

    bool isNegative = false;
    bool hasDollar = false;

    // 1. Sedot simbol $ dan - dari depan
    while (!aValue.empty()) {
        if (aValue[0] == '$') {
            hasDollar = true;
            aValue.erase(0, 1);
        } else if (aValue[0] == '-') {
            isNegative = true;
            aValue.erase(0, 1);
        } else {
            break;
        }
    }

    // 2. HAPUS ANGKA NOL DI DEPAN (Leading Zeros)
    // Selama teks lebih dari 1 karakter dan karakter depannya '0', hapus!
    // (Batas > 1 agar jika uangnya benar-benar "0", tidak terhapus semua)
    while (aValue.length() > 1 && aValue[0] == '0') {
        aValue.erase(0, 1);
    }

    std::string result = "";

    // MODE 1: Format Standar (Tanpa Sen)
    if (displayMode == 1) 
    {
        int len = aValue.length();
        int size = 3;
        while (len > size)
        {
            aValue.insert(len - size, separator);
            size += 3 + separator.length(); 
            len += separator.length();
        }
        result = aValue;
    } 
    // MODE 2: Format dengan Sen
    else if (displayMode == 2) 
    {
        // Pastikan minimal ada 3 digit (misal: "0" jadi "000" agar sen aman)
        while (aValue.length() < 3) {
            aValue.insert(0, "0"); 
        }

        std::string cents = aValue.substr(aValue.length() - 2);
        std::string dollars = aValue.substr(0, aValue.length() - 2);

        int len = dollars.length();
        int size = 3;
        while (len > size)
        {
            dollars.insert(len - size, separator);
            size += 3 + separator.length();
            len += separator.length();
        }
        result = dollars + centSeparator + cents;
    }
    
    // 3. Kembalikan simbol ke posisi semula
    if (hasDollar) result = "$" + result;
    if (isNegative) result = "-" + result; 
    
    return result;
}

DECL_HOOKv(Money_AsciiToGxtChar, const char* aSource, unsigned short* aTarget)
{
    if (displayMode == 0) {
        Money_AsciiToGxtChar(aSource, aTarget);
    } else {
        std::string sep = AddSeparators(std::string(aSource));
        Money_AsciiToGxtChar(sep.c_str(), aTarget);
    }

    // Bug 10 Juta Bypass
    if (g_pPlayerInfo) {
        int* m_nDisplayMoney = (int*)((uintptr_t)g_pPlayerInfo + 0xBC);
        *m_nDisplayMoney = 0; 
    }
}

DECL_HOOKv(CHudColours_GetRGB, CRGBA* out, void* self, int colorIndex, uint8_t alpha)
{
    CHudColours_GetRGB(out, self, colorIndex, alpha);
    if (useCustomRGB && colorIndex == 1) {        
        out->r = moneyR;
        out->g = moneyG;
        out->b = moneyB;
    }
}

DECL_HOOKv(CPlayerInfo_Process, void* self, int playerIndex)
{
    CPlayerInfo_Process(self, playerIndex);
    g_pPlayerInfo = self;

    int* m_nMoney = (int*)((uintptr_t)self + 0xB8);
    int* m_nDisplayMoney = (int*)((uintptr_t)self + 0xBC);

    *m_nDisplayMoney = *m_nMoney;
}

extern "C" void OnModLoad()
{
    logger->SetTag("Money Separator");
    cfg->Bind("Author", "", "About")->SetString("KillerSA"); cfg->ClearLast();
    cfg->Bind("GitHub", "", "About")->SetString("https://github.com/KillerSAA/Money-Separator/tree/main"); cfg->ClearLast();
    
    uintptr_t pGame = aml->GetLib("libGTASA.so");
    if(pGame)
    {
        HOOKBLX(Money_AsciiToGxtChar, pGame + BYBIT(0x2BD26E + 0x1, 0x37D4C4));
        HOOK(CHudColours_GetRGB, pGame + 0x43AB0C + 0x1);
        HOOK(CPlayerInfo_Process, pGame + 0x40908C + 0x1);
    }
    else
    {
        pGame = aml->GetLib("libGTAVC.so");
        if(pGame)
        {
            HOOKBL(Money_AsciiToGxtChar, pGame + BYBIT(0x1E9F74 + 0x1, 0x2C3AC8));
        }
        else
        {
            logger->Error("This game is unsupported");
            return;
        }
    }

    displayMode = cfg->Bind("Mode", 1, "Configs")->GetInt();
    
    useCustomRGB = cfg->Bind("UseCustomRGB", false, "Colors")->GetBool();
    moneyR = cfg->Bind("moneyR", 50, "Colors")->GetInt();
    moneyG = cfg->Bind("moneyG", 255, "Colors")->GetInt();
    moneyB = cfg->Bind("moneyB", 50, "Colors")->GetInt();
}
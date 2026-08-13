#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>
#include <string>

MYMODCFG(net.KillerSA.moneyseparator, Money Separator, 1.7, KillerSA)

std::string separator = ".";
std::string centSeparator = ",";
int displayMode = 1;

bool useCustomRGB = false;
int rPlus = 50, gPlus = 255, bPlus = 50;
int rMinus = 255, gMinus = 50, bMinus = 50;

struct CRGBA {
    uint8_t r, g, b, a;
};

static std::string AddSeparators(std::string aValue) 
{
    if (displayMode == 0 || aValue.empty()) return aValue;

    bool isNegative = false;
    if (aValue[0] == '-') {
        isNegative = true;
        aValue.erase(0, 1);
    }

    bool hasDollar = false;
    if (!aValue.empty() && aValue[0] == '$') {
        hasDollar = true;
        aValue.erase(0, 1);
    }

    std::string result = "";

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
    else if (displayMode >= 2 && displayMode <= 5) 
    {
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

    if (hasDollar) result = "$" + result;
    if (isNegative) result = "-" + result; 
    
    return result;
}

DECL_HOOKv(Money_AsciiToGxtChar, const char* aSource, unsigned short* aTarget)
{
    if (displayMode == 0) {
        Money_AsciiToGxtChar(aSource, aTarget);
        return;
    }
    std::string sep = AddSeparators(std::string(aSource));
    Money_AsciiToGxtChar(sep.c_str(), aTarget);
}

DECL_HOOKv(CHudColours_GetRGB, CRGBA* out, void* self, int colorIndex, uint8_t alpha)
{
    CHudColours_GetRGB(out, self, colorIndex, alpha);
    if (useCustomRGB) {
        if (colorIndex == 1) {
            out->r = rPlus;
            out->g = gPlus;
            out->b = bPlus;
        }
        else if (colorIndex == 0) {
            out->r = rMinus;
            out->g = gMinus;
            out->b = bMinus;
        }
    }
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
        void* pGetRGB = aml->GetSym(pGame, "_ZN11CHudColours6GetRGBEih");
        if (pGetRGB) {
            HOOKv(CHudColours_GetRGB, pGetRGB);
        } else {
            void* pGetRGBA = aml->GetSym(pGame, "_ZN11CHudColours7GetRGBAEih");
            if (pGetRGBA) HOOKv(CHudColours_GetRGB, pGetRGBA);
        }
    }
    else
    {
        logger->Error("This game is unsupported");
        return;
    }

    displayMode = cfg->Bind("Mode", 1, "Configs")->GetInt();
    
    useCustomRGB = cfg->Bind("UseCustomRGB", false, "Colors")->GetBool();
    
    rPlus = cfg->Bind("R_Plus", 50, "Colors")->GetInt();
    gPlus = cfg->Bind("G_Plus", 255, "Colors")->GetInt();
    bPlus = cfg->Bind("B_Plus", 50, "Colors")->GetInt();
    
    rMinus = cfg->Bind("R_Minus", 255, "Colors")->GetInt();
    gMinus = cfg->Bind("G_Minus", 50, "Colors")->GetInt();
    bMinus = cfg->Bind("B_Minus", 50, "Colors")->GetInt();
    
    switch (displayMode) {
        case 1: separator = "."; break;
        case 2: separator = ","; centSeparator = "."; break;
        case 3: separator = "."; centSeparator = ","; break;
        case 4: separator = "."; centSeparator = "."; break;
        case 5: separator = ","; centSeparator = ","; break;
        default: displayMode = 1; separator = "."; break;
    }
}

// A port from PC to Android (https://github.com/The-Musaigen/money-separator) | thanks for RusJJ
#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>
#include <string>
#include <stdint.h> // Tambahan untuk mengatasi error uintptr_t / uint8_t

MYMODCFG(net.KillerSA.moneyseparator, Money Separator, 1.8, KillerSA)

std::string separator = ".";
std::string centSeparator = ",";
int displayMode = 1;

// === VARIABEL RGB CUSTOM ===
bool useCustomRGB = false;
int rPlus = 50, gPlus = 255, bPlus = 50;    // Default: Hijau Uang
int rMinus = 255, gMinus = 50, bMinus = 50; // Default: Merah Minus

// Struktur memori warna bawaan game
struct CRGBA {
    uint8_t r, g, b, a;
};

// ==========================================
// MESIN 1: PEMISAH ANGKA (Teks Murni)
// ==========================================
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

// ==========================================
// MESIN 2: PEMBAJAK RGB LANGSUNG KE MEMORI
// ==========================================
DECL_HOOKv(CHudColours_GetRGB, CRGBA* out, void* self, int colorIndex, uint8_t alpha)
{
    // Biarkan game mengambil warna aslinya terlebih dahulu
    CHudColours_GetRGB(out, self, colorIndex, alpha);
    
    // Cegat dan timpa dengan warna kita!
    if (useCustomRGB) {
        if (colorIndex == 1) {        // Index 1 = Uang Positif
            out->r = rPlus;
            out->g = gPlus;
            out->b = bPlus;
        }
        else if (colorIndex == 0) {   // Index 0 = Uang Minus/Merah (Juga dipakai untuk hal lain)
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
        // 1. Hook Pemisah Uang
        HOOKBLX(Money_AsciiToGxtChar, pGame + BYBIT(0x2BD26E + 0x1, 0x37D4C4));
        
        // 2. Hook Mesin Warna RGB Dinamis (Hardcoded Offset v2.00 dari Python)
        HOOK(CHudColours_GetRGB, pGame + 0x43AB0C + 0x1);
    }
    else
    {
        pGame = aml->GetLib("libGTAVC.so");
        if(pGame)
        {
            // Fallback untuk GTA VC jika ada
            HOOKBL(Money_AsciiToGxtChar, pGame + BYBIT(0x1E9F74 + 0x1, 0x2C3AC8));
        }
        else
        {
            logger->Error("This game is unsupported");
            return;
        }
    }

    // === MEMBACA CONFIGS ===
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

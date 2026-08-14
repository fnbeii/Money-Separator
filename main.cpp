#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>
#include <string>
#include <stdint.h>

MYMODCFG(net.KillerSA.mnfy.moneyseparator, Money Separator, 3.3, KillerSA)

std::string separator = ".";
std::string centSeparator = ".";
int displayMode = 1;
bool useCustomRGB = false;
int moneyR = 50, moneyG = 255, moneyB = 50;

struct CRGBA {
    uint8_t r, g, b, a;
};

void* g_pPlayerInfo = nullptr;

// ==========================================
// MESIN 1: LOGIKA PEMISAH ANGKA
// ==========================================
static std::string AddSeparators(std::string aValue) 
{
    if (displayMode == 0 || aValue.empty()) return aValue;

    bool isNegative = false;
    bool isPositive = false;
    bool hasDollar = false;

    // Sedot simbol dari depan angka agar tidak bentrok dengan separator
    while (!aValue.empty()) {
        if (aValue[0] == '$') {
            hasDollar = true;
            aValue.erase(0, 1);
        } else if (aValue[0] == '-') {
            isNegative = true;
            aValue.erase(0, 1);
        } else if (aValue[0] == '+') {
            isPositive = true;
            aValue.erase(0, 1);
        } else {
            break;
        }
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
    else if (displayMode == 2) 
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
    
    // Kembalikan simbol ke tempat semula
    if (hasDollar) result = "$" + result;
    if (isNegative) result = "-" + result; 
    if (isPositive) result = "+" + result; 
    
    return result;
}

// ==========================================
// MESIN 2: PEMBAJAK TEKS GLOBAL (Menangkap Semua Teks Uang)
// ==========================================
DECL_HOOKv(Global_AsciiToGxtChar, const char* aSource, unsigned short* aTarget)
{
    std::string s(aSource);
    
    // Deteksi jika ini adalah teks HUD Uang Utama atau teks Uang Melayang (Money Changer)
    if ((s.length() > 0 && s[0] == '$') || (s.length() > 1 && (s[0] == '+' || s[0] == '-') && s[1] == '$')) {
        s = AddSeparators(s);
        Global_AsciiToGxtChar(s.c_str(), aTarget);
    } 
    else {
        Global_AsciiToGxtChar(aSource, aTarget);
    }
}

// ==========================================
// MESIN 3: PEMBLOKIR BUG 10 JUTA
// ==========================================
DECL_HOOKv(CFont_SetScale, float w, float h)
{
    // Jika game mencoba mengecilkan font sampai tidak masuk akal, kita abaikan instruksinya
    if (h > 0.0f && h < 0.001f) {
        return; 
    }
    CFont_SetScale(w, h);
}

// ==========================================
// MESIN 4: PEMBAJAK RGB WARNA PLUS
// ==========================================
DECL_HOOKv(CHudColours_GetRGB, CRGBA* out, void* self, int colorIndex, uint8_t alpha)
{
    CHudColours_GetRGB(out, self, colorIndex, alpha);
    if (useCustomRGB && colorIndex == 1) {        
        out->r = moneyR;
        out->g = moneyG;
        out->b = moneyB;
    }
}

// ==========================================
// MESIN 5: TRIK ILUSI HUD (Uang Instan, Animasi Melayang Tetap Jalan)
// ==========================================
DECL_HOOKv(CPlayerInfo_Process, void* self, int playerIndex)
{
    CPlayerInfo_Process(self, playerIndex);
    g_pPlayerInfo = self;
}

DECL_HOOK(void*, CWidgetPlayerInfo_Draw, void* self)
{
    int tempDisplay = 0;
    int* m_nDisplayMoney = nullptr;
    
    // Tepat sebelum HUD uang digambar, kita ubah nilai DisplayMoney jadi instan!
    if (g_pPlayerInfo) {
        int* m_nMoney = (int*)((uintptr_t)g_pPlayerInfo + 0xB8);
        m_nDisplayMoney = (int*)((uintptr_t)g_pPlayerInfo + 0xBC);
        
        tempDisplay = *m_nDisplayMoney;
        *m_nDisplayMoney = *m_nMoney; 
    }
    
    void* result = CWidgetPlayerInfo_Draw(self);
    
    // Setelah selesai digambar, kembalikan nilai animasi agar teks Money Changer tetap muncul
    if (m_nDisplayMoney) {
        *m_nDisplayMoney = tempDisplay;
    }
    
    return result;
}

// ==========================================
// INISIALISASI MOD
// ==========================================
extern "C" void OnModLoad()
{
    logger->SetTag("Money Separator");
    cfg->Bind("Author", "", "About")->SetString("KillerSA"); cfg->ClearLast();
    cfg->Bind("GitHub", "", "About")->SetString("https://github.com/KillerSAA/Money-Separator/tree/main"); cfg->ClearLast();
    
    uintptr_t pGame = aml->GetLib("libGTASA.so");
    if(pGame)
    {
        uintptr_t sym_AsciiToGxtChar = aml->GetSym(pGame, "_Z14AsciiToGxtCharPKcPt");
        uintptr_t sym_SetScale       = aml->GetSym(pGame, "_ZN5CFont8SetScaleEff");
        
        if (sym_AsciiToGxtChar) {
            HOOK(Global_AsciiToGxtChar, sym_AsciiToGxtChar);
        }
        
        // FIX FATAL: CFont::SetScale dikompilasi sebagai ARM murni (32-bit), TANPA + 0x1
        if (sym_SetScale) {
            HOOK(CFont_SetScale, sym_SetScale);
        } else {
            HOOK(CFont_SetScale, pGame + 0x190E6C); 
        }
        
        HOOK(CHudColours_GetRGB, pGame + 0x43AB0C + 0x1);
        HOOK(CPlayerInfo_Process, pGame + 0x40908C + 0x1);
        HOOK(CWidgetPlayerInfo_Draw, pGame + 0x2BCC88 + 0x1);
    }
    else
    {
        pGame = aml->GetLib("libGTAVC.so");
        if(pGame)
        {
            uintptr_t sym_AsciiToGxtChar = aml->GetSym(pGame, "_Z14AsciiToGxtCharPKcPt");
            if (sym_AsciiToGxtChar) HOOK(Global_AsciiToGxtChar, sym_AsciiToGxtChar);
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

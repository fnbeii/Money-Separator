#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>
#include <string>
#include <stdint.h>

MYMODCFG(net.KillerSA.moneyseparator, Money Separator, 3.1, KillerSA)

std::string separator = ".";
std::string centSeparator = ".";
int displayMode = 1;

bool useCustomRGB = false;
int moneyR = 50, moneyG = 255, moneyB = 50; 

struct CRGBA {
    uint8_t r, g, b, a;
};

void (*CFont_SetProportional)(unsigned char);
void* g_pPlayerInfo = nullptr;

static std::string AddSeparators(std::string aValue) 
{
    if (displayMode == 0 || aValue.empty()) return aValue;

    bool isNegative = false;
    bool isPositive = false;
    bool hasDollar = false;

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

    if (hasDollar) result = "$" + result;
    if (isNegative) result = "-" + result; 
    if (isPositive) result = "+" + result; 
    
    return result;
}

DECL_HOOKv(Global_AsciiToGxtChar, const char* aSource, unsigned short* aTarget)
{
    std::string s(aSource);
    
    // Tangkap Uang Utama ("$1000") DAN Uang Melayang ("+$50" / "-$50")
    if ((s.length() > 0 && s[0] == '$') || (s.length() > 1 && (s[0] == '+' || s[0] == '-') && s[1] == '$')) {
        s = AddSeparators(s);
        Global_AsciiToGxtChar(s.c_str(), aTarget);
        if (CFont_SetProportional) CFont_SetProportional(1); // Paksa Proporsional
    } 
    else {
        Global_AsciiToGxtChar(aSource, aTarget);
    }
}

DECL_HOOKv(CFont_SetScale, float w, float h)
{
    if (h > 0.0f && h < 0.001f) {
        return;
    }
    CFont_SetScale(w, h);
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
}

DECL_HOOK(void*, CWidgetPlayerInfo_Draw, void* self)
{
    int tempDisplay = 0;
    int* m_nDisplayMoney = nullptr;
    
    if (g_pPlayerInfo) {
        int* m_nMoney = (int*)((uintptr_t)g_pPlayerInfo + 0xB8);
        m_nDisplayMoney = (int*)((uintptr_t)g_pPlayerInfo + 0xBC);
        tempDisplay = *m_nDisplayMoney;
        *m_nDisplayMoney = *m_nMoney; 
    }
    
    void* result = CWidgetPlayerInfo_Draw(self);
    
    if (m_nDisplayMoney) {
        *m_nDisplayMoney = tempDisplay;
    }
    
    return result;
}

extern "C" void OnModLoad()
{
    logger->SetTag("Money Separator");
    cfg->Bind("Author", "", "About")->SetString("KillerSA"); cfg->ClearLast();
    
    uintptr_t pGame = aml->GetLib("libGTASA.so");
    if(pGame)
    {
        void* sym_AsciiToGxtChar = aml->GetSym(pGame, "_Z14AsciiToGxtCharPKcPt");
        void* sym_SetScale       = aml->GetSym(pGame, "_ZN5CFont8SetScaleEff");
        
        if (sym_AsciiToGxtChar) HOOK(Global_AsciiToGxtChar, sym_AsciiToGxtChar);
        
        if (sym_SetScale) HOOK(CFont_SetScale, sym_SetScale);
        else HOOK(CFont_SetScale, pGame + 0x190E6C + 0x1); 
        
        HOOK(CHudColours_GetRGB, pGame + 0x43AB0C + 0x1);
        HOOK(CPlayerInfo_Process, pGame + 0x40908C + 0x1);
        HOOK(CWidgetPlayerInfo_Draw, pGame + 0x2BCC88 + 0x1);
        
        CFont_SetProportional = (void (*)(unsigned char))(pGame + 0x194F7C + 0x1);
    }
    else
    {
        logger->Error("This game is unsupported");
        return;
    }

    displayMode = cfg->Bind("Mode", 1, "Configs")->GetInt();
    useCustomRGB = cfg->Bind("UseCustomRGB", false, "Colors")->GetBool();
    moneyR = cfg->Bind("moneyR", 50, "Colors")->GetInt();
    moneyG = cfg->Bind("moneyG", 255, "Colors")->GetInt();
    moneyB = cfg->Bind("moneyB", 50, "Colors")->GetInt();
}

// A port from PC to Android (https://github.com/The-Musaigen/money-separator) | thanks for RusJJ
#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>
#include <string>

MYMODCFG(net.KillerSA.moneyseparator, Money Separator, 1.4, KillerSA)

std::string separator = ". ";
std::string centSeparator = ", ";
int displayMode = 1;

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
        
        if (hasDollar) aValue.insert(0, "$");
        if (isNegative) aValue.insert(0, "-");
        
        return aValue;
    } 
    else if (displayMode == 2 || displayMode == 3) 
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

        std::string result = dollars + centSeparator + cents;
        
        if (hasDollar) result = "$" + result;
        if (isNegative) result = "-" + result; 
        
        return result;
    }

    return aValue;
}

DECL_HOOKv(Money_AsciiToGxtChar, const char* aSource, unsigned short* aTarget)
{
    std::string source = std::string { aSource };
    std::string sep = AddSeparators(source);
    Money_AsciiToGxtChar(sep.c_str(), aTarget);
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
    displayMode = cfg->GetInt("Mode", 1, "Configs");
    
    if (displayMode == 1) {
        separator = ". ";
    } else if (displayMode == 2) {
        separator = ", ";
        centSeparator = ". ";
    } else if (displayMode == 3) {
        separator = ". ";
        centSeparator = ", ";
    }
}

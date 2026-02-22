// A port from PC to Android (https://github.com/The-Musaigen/money-separator) | thanks for RusJJ
#include <mod/amlmod.h>
#include <mod/config.h>
#include <mod/logger.h>
#include <cctype>
#include <string>

MYMODCFG(net.KillerSA.moneyseparator, Money Separator, 1.2, KillerSA)

char separator = '.';
char centSeparator = ',';
int displayMode = 1;
static std::string AddSeparators(std::string aValue) 
{
    if (aValue.empty()) return aValue;

    bool isNegative = (aValue[0] == '-');
    if (isNegative) aValue.erase(0, 1);

    if (displayMode == 1) 
    {
        int len = aValue.length();
        int size = 3;
        while (len > size)
        {
            aValue.insert(len - size, 1, separator);
            size += 4;
            len += 1;
        }
        
        if (isNegative) aValue.insert(0, "-");
        return aValue;
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
            dollars.insert(len - size, 1, separator);
            size += 4;
            len += 1;
        }

        std::string result = "$" + dollars + centSeparator + cents;
        
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
    
    const char* sep = cfg->GetString("Separator", ".", "Configs");
    if(strcasecmp(sep, "space") == 0) separator = ' ';
    else if(std::isprint(sep[0])) separator = sep[0];

    const char* cSep = cfg->GetString("CentSeparator", ",", "Configs");
    if(std::isprint(cSep[0])) centSeparator = cSep[0];
}

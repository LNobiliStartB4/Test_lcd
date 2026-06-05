#include <gui/screen12_screen/Screen12View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <stdint.h>
#include <touchgfx/Application.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Texts.hpp>

namespace
{
const touchgfx::colortype kActiveBorderColor = touchgfx::Color::getColorFromRGB(216, 178, 71);
const touchgfx::colortype kInactiveBorderColor = touchgfx::Color::getColorFromRGB(86, 104, 116);
const uint8_t kActiveBorderSize = 3;
const uint8_t kInactiveBorderSize = 2;
}

Screen12View::Screen12View()
{
}

void Screen12View::setupScreen()
{
    Screen12ViewBase::setupScreen();

    const UiLanguage language = (presenter != 0) ? presenter->getUiLanguage() : UiLanguageEnglish;
    applyLanguage(language);
}

void Screen12View::tearDownScreen()
{
    Screen12ViewBase::tearDownScreen();
}

void Screen12View::backClicked()
{
    application().gotoSettingsScreenNoTransition();
}

void Screen12View::englishClicked()
{
    applyLanguage(UiLanguageEnglish);
}

void Screen12View::italianClicked()
{
    applyLanguage(UiLanguageItalian);
}

void Screen12View::applyLanguage(UiLanguage language)
{
    if (presenter != 0)
    {
        presenter->setUiLanguage(language);
    }

    touchgfx::Texts::setLanguage(language == UiLanguageItalian ? IT : GB);
    refreshLanguageSelection(language);
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen12View::refreshLanguageSelection(UiLanguage language)
{
    const bool italianSelected = language == UiLanguageItalian;

    englishFrame.setBorderColor(italianSelected ? kInactiveBorderColor : kActiveBorderColor);
    englishFrame.setBorderSize(italianSelected ? kInactiveBorderSize : kActiveBorderSize);
    englishFrame.invalidate();

    italianFrame.setBorderColor(italianSelected ? kActiveBorderColor : kInactiveBorderColor);
    italianFrame.setBorderSize(italianSelected ? kActiveBorderSize : kInactiveBorderSize);
    italianFrame.invalidate();
}

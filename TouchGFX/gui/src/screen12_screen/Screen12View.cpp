#include <gui/screen12_screen/Screen12View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <stdint.h>
#include <touchgfx/Application.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Texts.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/widgets/TextArea.hpp>
#include <touchgfx/widgets/BoxWithBorder.hpp>
#include <touchgfx/containers/buttons/Buttons.hpp>

namespace
{
// Single source of truth for the language list (the only hand-maintained glue).
// Order MUST match UiLanguage / touchgfx::LANGUAGES (GB, IT, FR, DE, ES): the
// slot index is the UiLanguage value and the mapping to LANGUAGES is a cast.
// To add a language: add it in the Designer (texts.xml) + its autonym text,
// extend UiLanguage, and add one entry here. The paginated layout adapts itself.
const touchgfx::TypedTextId kLanguageName[] =
{
    T_TEXT_ENGLISH,
    T_TEXT_ITALIAN,
    T_TEXT_FRENCH,
    T_TEXT_GERMAN,
    T_TEXT_SPANISH
};
const uint8_t kLanguageCount = sizeof(kLanguageName) / sizeof(kLanguageName[0]);
const uint8_t kRowsPerPage = 4;

// Only the selected language box turns THD yellow; the others stay dim.
const touchgfx::colortype kActiveBorderColor = touchgfx::Color::getColorFromRGB(240, 170, 0);
const touchgfx::colortype kInactiveBorderColor = touchgfx::Color::getColorFromRGB(86, 104, 116);
const uint8_t kActiveBorderSize = 3;
const uint8_t kInactiveBorderSize = 2;

uint8_t pageCount()
{
    return static_cast<uint8_t>((kLanguageCount + kRowsPerPage - 1) / kRowsPerPage);
}
}

Screen12View::Screen12View()
    : currentPage(0),
      currentLanguage(UiLanguageEnglish)
{
}

void Screen12View::setupScreen()
{
    Screen12ViewBase::setupScreen();

    currentLanguage = (presenter != 0) ? presenter->getUiLanguage() : UiLanguageEnglish;
    // Open on the page that contains the currently selected language.
    showPage(static_cast<uint8_t>(currentLanguage) / kRowsPerPage);
}

void Screen12View::tearDownScreen()
{
    Screen12ViewBase::tearDownScreen();
}

void Screen12View::backClicked()
{
    application().gotoSettingsScreenNoTransition();
}

void Screen12View::showPage(uint8_t pageIndex)
{
    if (pageIndex >= pageCount())
    {
        pageIndex = static_cast<uint8_t>(pageCount() - 1);
    }
    currentPage = pageIndex;

    touchgfx::BoxWithBorderButtonStyle<touchgfx::ClickButtonTrigger>* buttons[kRowsPerPage] =
        { &langRowButton0, &langRowButton1, &langRowButton2, &langRowButton3 };
    touchgfx::BoxWithBorder* frames[kRowsPerPage] =
        { &langRowFrame0, &langRowFrame1, &langRowFrame2, &langRowFrame3 };
    touchgfx::TextArea* labels[kRowsPerPage] =
        { &langRowLabel0, &langRowLabel1, &langRowLabel2, &langRowLabel3 };

    for (uint8_t slot = 0; slot < kRowsPerPage; ++slot)
    {
        const uint8_t index = static_cast<uint8_t>(currentPage * kRowsPerPage + slot);
        const bool available = index < kLanguageCount;

        buttons[slot]->setVisible(available);
        buttons[slot]->setTouchable(available);
        frames[slot]->setVisible(available);
        labels[slot]->setVisible(available);

        if (available)
        {
            labels[slot]->setTypedText(touchgfx::TypedText(kLanguageName[index]));
            const bool selected = (index == static_cast<uint8_t>(currentLanguage));
            frames[slot]->setBorderColor(selected ? kActiveBorderColor : kInactiveBorderColor);
            frames[slot]->setBorderSize(selected ? kActiveBorderSize : kInactiveBorderSize);
        }

        buttons[slot]->invalidate();
        frames[slot]->invalidate();
        labels[slot]->invalidate();
    }

    const bool paginated = pageCount() > 1;
    langPrevButton.setVisible(paginated);
    langPrevLabel.setVisible(paginated);
    langPageIndicator.setVisible(paginated);
    langNextButton.setVisible(paginated);
    langNextLabel.setVisible(paginated);

    if (paginated)
    {
        langPrevButton.setTouchable(currentPage > 0);
        langNextButton.setTouchable((currentPage + 1) < pageCount());
        touchgfx::Unicode::snprintf(langPageIndicatorBuffer,
                                    LANGPAGEINDICATOR_SIZE,
                                    "%u/%u",
                                    currentPage + 1,
                                    pageCount());
        langPrevButton.invalidate();
        langNextButton.invalidate();
        langPageIndicator.invalidate();
    }
}

void Screen12View::selectSlot(uint8_t slot)
{
    const uint8_t index = static_cast<uint8_t>(currentPage * kRowsPerPage + slot);
    if (index < kLanguageCount)
    {
        applyLanguage(static_cast<UiLanguage>(index));
    }
}

void Screen12View::row0Clicked() { selectSlot(0); }
void Screen12View::row1Clicked() { selectSlot(1); }
void Screen12View::row2Clicked() { selectSlot(2); }
void Screen12View::row3Clicked() { selectSlot(3); }

void Screen12View::previousPageClicked()
{
    if (currentPage > 0)
    {
        showPage(static_cast<uint8_t>(currentPage - 1));
    }
}

void Screen12View::nextPageClicked()
{
    if ((currentPage + 1) < pageCount())
    {
        showPage(static_cast<uint8_t>(currentPage + 1));
    }
}

void Screen12View::applyLanguage(UiLanguage language)
{
    currentLanguage = language;
    if (presenter != 0)
    {
        presenter->setUiLanguage(language);
    }

    // UiLanguage is aligned numerically with the (global) LANGUAGES enum -> cast.
    touchgfx::Texts::setLanguage(static_cast<LANGUAGES>(language));
    showPage(currentPage); // move the gold highlight to the new selection
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

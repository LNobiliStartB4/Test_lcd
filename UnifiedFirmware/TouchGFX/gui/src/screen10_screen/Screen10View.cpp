#include <gui/screen10_screen/Screen10View.hpp>
#include <images/SVGDatabase.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Unicode.hpp>

namespace
{
enum SettingsAction
{
    SettingsActionLanguage,
    SettingsActionBrightness,
    SettingsActionAdmin
};

struct SettingsEntry
{
    uint16_t iconId;
    touchgfx::TypedTextId titleId;
    touchgfx::TypedTextId descriptionId;
    SettingsAction action;
};

const SettingsEntry kSettingsEntries[] =
{
    { SVG_SETTINGS_LANGUAGE_GOLD_ID, T_TEXT_SETTINGSLANGUAGEITEM, T_TEXT_SETTINGSLANGUAGEDESCRIPTION, SettingsActionLanguage },
    { SVG_SETTINGS_BRIGHTNESS_GOLD_ID, T_TEXT_SETTINGSBRIGHTNESSITEM, T_TEXT_SETTINGSBRIGHTNESSDESCRIPTION, SettingsActionBrightness },
    { SVG_SETTINGS_ADMIN_GOLD_ID, T_TEXT_SETTINGSADMINITEM, T_TEXT_SETTINGSADMINDESCRIPTION, SettingsActionAdmin }
};

const uint8_t kSettingsEntryCount = sizeof(kSettingsEntries) / sizeof(kSettingsEntries[0]);
}

Screen10View::Screen10View()
    : currentPage(0),
      row0Callback(this, &Screen10View::row0Clicked),
      row1Callback(this, &Screen10View::row1Clicked),
      row2Callback(this, &Screen10View::row2Clicked)
{
}

void Screen10View::setupScreen()
{
    Screen10ViewBase::setupScreen();
    settingsRow0.setAction(row0Callback);
    settingsRow1.setAction(row1Callback);
    settingsRow2.setAction(row2Callback);
    showPage(0);
}

void Screen10View::tearDownScreen()
{
    Screen10ViewBase::tearDownScreen();
}

void Screen10View::backClicked()
{
    application().gotoProductSelectScreenNoTransition();
}

void Screen10View::previousPageClicked()
{
    if (currentPage > 0)
    {
        showPage(currentPage - 1);
    }
}

void Screen10View::nextPageClicked()
{
    const uint8_t pageCount = (kSettingsEntryCount + kRowsPerPage - 1) / kRowsPerPage;
    if ((currentPage + 1) < pageCount)
    {
        showPage(currentPage + 1);
    }
}

void Screen10View::showPage(uint8_t pageIndex)
{
    SettingsMenuRow* rows[kRowsPerPage] = { &settingsRow0, &settingsRow1, &settingsRow2 };
    const uint8_t pageCount = (kSettingsEntryCount + kRowsPerPage - 1) / kRowsPerPage;

    if (pageIndex >= pageCount)
    {
        pageIndex = pageCount - 1;
    }
    currentPage = pageIndex;

    for (uint8_t slot = 0; slot < kRowsPerPage; ++slot)
    {
        const uint8_t itemIndex = currentPage * kRowsPerPage + slot;
        const bool itemAvailable = itemIndex < kSettingsEntryCount;
        rows[slot]->setVisible(itemAvailable);
        rows[slot]->setTouchable(itemAvailable);

        if (itemAvailable)
        {
            const SettingsEntry& entry = kSettingsEntries[itemIndex];
            rows[slot]->setContent(entry.iconId, entry.titleId, entry.descriptionId);
        }
        rows[slot]->invalidate();
    }

    const bool paginationVisible = pageCount > 1;
    settingsPreviousButton.setVisible(paginationVisible);
    settingsPreviousLabel.setVisible(paginationVisible);
    settingsPageIndicator.setVisible(paginationVisible);
    settingsNextButton.setVisible(paginationVisible);
    settingsNextLabel.setVisible(paginationVisible);

    if (paginationVisible)
    {
        settingsPreviousButton.setTouchable(currentPage > 0);
        settingsNextButton.setTouchable((currentPage + 1) < pageCount);
        touchgfx::Unicode::snprintf(settingsPageIndicatorBuffer,
                                   SETTINGSPAGEINDICATOR_SIZE,
                                   "%u/%u",
                                   currentPage + 1,
                                   pageCount);
        settingsPageIndicator.invalidate();
    }
}

void Screen10View::activateSlot(uint8_t slotIndex)
{
    const uint8_t itemIndex = currentPage * kRowsPerPage + slotIndex;
    if (itemIndex >= kSettingsEntryCount)
    {
        return;
    }

    switch (kSettingsEntries[itemIndex].action)
    {
    case SettingsActionLanguage:
        application().gotoLanguageScreenNoTransition();
        break;
    case SettingsActionBrightness:
        application().gotoBrightnessScreenNoTransition();
        break;
    case SettingsActionAdmin:
        if (presenter != 0)
        {
            presenter->prepareAdminAccess();
        }
        application().gotoAdminPinScreenNoTransition();
        break;
    }
}

void Screen10View::row0Clicked()
{
    activateSlot(0);
}

void Screen10View::row1Clicked()
{
    activateSlot(1);
}

void Screen10View::row2Clicked()
{
    activateSlot(2);
}

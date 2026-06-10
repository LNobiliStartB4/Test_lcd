#include <gui/screen13_screen/Screen13View.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>

namespace
{
const touchgfx::colortype kIvory = touchgfx::Color::getColorFromRGB(245, 242, 232);
const touchgfx::colortype kError = touchgfx::Color::getColorFromRGB(225, 92, 92);
}

Screen13View::Screen13View()
    : pinLength(0U), invalidPin(false), tickDivider(0U)
{
    pinDigits[0] = pinDigits[1] = pinDigits[2] = pinDigits[3] = 0U;
}

void Screen13View::setupScreen()
{
    Screen13ViewBase::setupScreen();
    if ((presenter != 0) && presenter->isAuthenticated())
    {
        application().gotoAdminMenuScreenNoTransition();
        return;
    }
    pinLength = 0U;
    invalidPin = false;
    tickDivider = 0U;
    updatePinDisplay();
    updateLockoutState();
}

void Screen13View::tearDownScreen()
{
    Screen13ViewBase::tearDownScreen();
}

void Screen13View::handleTickEvent()
{
    if (++tickDivider >= 6U)
    {
        tickDivider = 0U;
        updateLockoutState();
    }
}

void Screen13View::backClicked()
{
    if (presenter != 0)
    {
        presenter->logout();
    }
    application().gotoSettingsScreenNoTransition();
}

void Screen13View::appendDigit(uint8_t digit)
{
    if ((presenter == 0) || (presenter->getLockoutSeconds() != 0U) || (pinLength >= 4U))
    {
        return;
    }
    pinDigits[pinLength++] = digit;
    invalidPin = false;
    updatePinDisplay();
    updateLockoutState();

    if (pinLength == 4U)
    {
        attemptAuthenticate();
    }
}

void Screen13View::deleteClicked()
{
    if ((presenter == 0) || (presenter->getLockoutSeconds() != 0U) || (pinLength == 0U))
    {
        return;
    }
    --pinLength;
    invalidPin = false;
    updatePinDisplay();
    updateLockoutState();
}

void Screen13View::attemptAuthenticate()
{
    if (presenter == 0)
    {
        return;
    }

    const uint16_t pin = static_cast<uint16_t>((pinDigits[0] * 1000U) +
                                               (pinDigits[1] * 100U) +
                                               (pinDigits[2] * 10U) +
                                               pinDigits[3]);
    const AdminAuthResult result = presenter->authenticate(pin);
    pinLength = 0U;
    updatePinDisplay();

    if (result == AdminAuthSuccess)
    {
        application().gotoAdminMenuScreenNoTransition();
        return;
    }

    invalidPin = (result == AdminAuthInvalidPin);
    updateLockoutState();
}

void Screen13View::updatePinDisplay()
{
    uint8_t bufferIndex = 0U;
    for (uint8_t i = 0U; i < 4U; ++i)
    {
        adminPinValueBuffer[bufferIndex++] = (i < pinLength) ? '*' : '-';
        if (i != 3U)
        {
            adminPinValueBuffer[bufferIndex++] = ' ';
        }
    }
    adminPinValueBuffer[bufferIndex] = 0;
    adminPinValue.setColor(invalidPin ? kError : kIvory);
    adminPinValue.invalidate();
}

void Screen13View::updateLockoutState()
{
    const uint8_t seconds = (presenter != 0) ? presenter->getLockoutSeconds() : 0U;
    const bool enabled = (seconds == 0U);

    digit0Button.setTouchable(enabled);
    digit1Button.setTouchable(enabled);
    digit2Button.setTouchable(enabled);
    digit3Button.setTouchable(enabled);
    digit4Button.setTouchable(enabled);
    digit5Button.setTouchable(enabled);
    digit6Button.setTouchable(enabled);
    digit7Button.setTouchable(enabled);
    digit8Button.setTouchable(enabled);
    digit9Button.setTouchable(enabled);
    deleteButton.setTouchable(enabled);
}

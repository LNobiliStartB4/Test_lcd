#include <gui/screen18_screen/Screen18View.hpp>
#include <gui/model/BandyCompletion.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>

namespace
{
const int32_t kMinimumSetpointMbar = 290;
const int32_t kMaximumSetpointMbar = 490;
const touchgfx::colortype kIvory = touchgfx::Color::getColorFromRGB(245, 242, 232);
const touchgfx::colortype kDim = touchgfx::Color::getColorFromRGB(124, 137, 148);
const touchgfx::colortype kError = touchgfx::Color::getColorFromRGB(216, 82, 72);

}

Screen18View::Screen18View()
    : latestState(),
      enteredValue(0U),
      entryLength(0U),
      replaceOnNextDigit(true),
      invalidEntry(false),
      entryInitialized(false),
      screenTransitionRequested(false)
{
}

void Screen18View::setupScreen()
{
    Screen18ViewBase::setupScreen();
    entryInitialized = false;
    screenTransitionRequested = false;

    if (presenter != 0)
    {
        applyBandyState(presenter->getBandyState());
    }
}

void Screen18View::tearDownScreen()
{
    screenTransitionRequested = false;
    Screen18ViewBase::tearDownScreen();
}

void Screen18View::cancelClicked()
{
    application().gotoSetpointEditScreenNoTransition();
}

void Screen18View::digit0Clicked()
{
    appendDigit(0U);
}

void Screen18View::digit1Clicked()
{
    appendDigit(1U);
}

void Screen18View::digit2Clicked()
{
    appendDigit(2U);
}

void Screen18View::digit3Clicked()
{
    appendDigit(3U);
}

void Screen18View::digit4Clicked()
{
    appendDigit(4U);
}

void Screen18View::digit5Clicked()
{
    appendDigit(5U);
}

void Screen18View::digit6Clicked()
{
    appendDigit(6U);
}

void Screen18View::digit7Clicked()
{
    appendDigit(7U);
}

void Screen18View::digit8Clicked()
{
    appendDigit(8U);
}

void Screen18View::digit9Clicked()
{
    appendDigit(9U);
}

void Screen18View::deleteClicked()
{
    replaceOnNextDigit = false;
    if (entryLength > 0U)
    {
        enteredValue = static_cast<uint16_t>(enteredValue / 10U);
        --entryLength;
    }

    invalidEntry = false;
    updateValueDisplay();
}

void Screen18View::applyClicked()
{
    if ((entryLength == 0U) ||
        (enteredValue < kMinimumSetpointMbar) ||
        (enteredValue > kMaximumSetpointMbar))
    {
        invalidEntry = true;
        updateValueDisplay();
        return;
    }

    if (presenter != 0)
    {
        presenter->setTarget(static_cast<int32_t>(enteredValue));
    }
    application().gotoSetpointEditScreenNoTransition();
}

void Screen18View::applyBandyState(const BandyState& state)
{
    const BandyState previousState = latestState;
    latestState = state;

    if (!screenTransitionRequested && isNaturalBandyCompletion(previousState, state))
    {
        screenTransitionRequested = true;
        application().gotoBandyCompletedScreenNoTransition();
        return;
    }

    if (!entryInitialized)
    {
        initializeEntry(state.targetVacuumMbar);
    }
}

void Screen18View::appendDigit(uint8_t digit)
{
    if (replaceOnNextDigit)
    {
        enteredValue = digit;
        entryLength = 1U;
        replaceOnNextDigit = false;
    }
    else if (entryLength < 3U)
    {
        enteredValue = static_cast<uint16_t>((enteredValue * 10U) + digit);
        ++entryLength;
    }

    invalidEntry = false;
    updateValueDisplay();
}

void Screen18View::updateValueDisplay()
{
    if (entryLength == 0U)
    {
        keypadValueBuffer[0] = '-';
        keypadValueBuffer[1] = '-';
        keypadValueBuffer[2] = '-';
        keypadValueBuffer[3] = 0;
    }
    else
    {
        touchgfx::Unicode::snprintf(keypadValueBuffer,
                                   KEYPADVALUE_SIZE,
                                   "%u",
                                   static_cast<unsigned int>(enteredValue));
    }

    keypadValue.setColor(invalidEntry ? kError : kIvory);
    keypadValue.invalidate();
}

void Screen18View::initializeEntry(int32_t targetMbar)
{
    enteredValue = static_cast<uint16_t>(targetMbar);
    entryLength = 3U;
    replaceOnNextDigit = true;
    invalidEntry = false;
    entryInitialized = true;
    updateValueDisplay();
}

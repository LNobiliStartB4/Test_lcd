#include <gui/screen20_screen/Screen20View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>

Screen20View::Screen20View()
    : transitionRequested(false),
      successDisplayActive(false),
      successReadyToContinue(false),
      successDisplayTicks(0U),
      successfulProduct(ActiveProductNone)
{
}

void Screen20View::setupScreen()
{
    Screen20ViewBase::setupScreen();
    transitionRequested = false;
    successDisplayActive = false;
    successReadyToContinue = false;
    successDisplayTicks = 0U;
    successfulProduct = ActiveProductNone;

    applyStatus(presenter->getStatus());
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen20View::tearDownScreen()
{
    Screen20ViewBase::tearDownScreen();
}

void Screen20View::handleTickEvent()
{
    Screen20ViewBase::handleTickEvent();

    if (successDisplayActive && successDisplayTicks < 60U)
    {
        ++successDisplayTicks;
    }

    if (!transitionRequested && successReadyToContinue && successDisplayTicks >= 60U)
    {
        completeSuccessfulPretest();
    }
}

void Screen20View::backClicked()
{
    if (!transitionRequested)
    {
        presenter->cancelPretest();
    }
}

void Screen20View::updatePretest(const PneumaticPretestStatus& status)
{
    if (transitionRequested)
    {
        return;
    }

    applyStatus(status);

    if (status.state == PneumaticPretestReleasing &&
        status.result != PneumaticPretestResultNone &&
        status.result != PneumaticPretestResultPassed &&
        status.result != PneumaticPretestResultCancelled)
    {
        transitionRequested = true;
        application().gotoPneumaticPretestErrorScreenNoTransition();
    }
    else if (status.result == PneumaticPretestResultPassed &&
             (status.state == PneumaticPretestReleasing ||
              status.state == PneumaticPretestPassed))
    {
        if (!successDisplayActive)
        {
            successDisplayActive = true;
            successDisplayTicks = 0U;
        }

        successfulProduct = status.product;
        if (status.state == PneumaticPretestPassed)
        {
            successReadyToContinue = true;
        }
    }
    else if (status.state == PneumaticPretestCancelled)
    {
        transitionRequested = true;
        presenter->resetPretest();
        application().gotoProductSelectScreenNoTransition();
    }
}

void Screen20View::applyStatus(const PneumaticPretestStatus& status)
{
    touchgfx::Unicode::snprintf(pressureValueBuffer, PRESSUREVALUE_SIZE, "%d", static_cast<int>(status.pressureMbar));
    touchgfx::Unicode::snprintf(timeRemainingBuffer, TIMEREMAINING_SIZE, "%02u", status.remainingSeconds);
    pressureValue.invalidate();
    timeRemaining.invalidate();

    const bool success =
        status.result == PneumaticPretestResultPassed &&
        (status.state == PneumaticPretestReleasing ||
         status.state == PneumaticPretestPassed);

    if (status.state == PneumaticPretestPullDown)
    {
        phaseLabel.setTypedText(touchgfx::TypedText(T_TEXT_PRETESTPULLDOWN));
        countdownCaption.setTypedText(touchgfx::TypedText(T_TEXT_PRETESTTIMEOUT));
    }
    else if (status.state == PneumaticPretestHold)
    {
        phaseLabel.setTypedText(touchgfx::TypedText(T_TEXT_PRETESTHOLD));
        countdownCaption.setTypedText(touchgfx::TypedText(T_TEXT_PRETESTSEALCOUNTDOWN));
    }
    else if (success)
    {
        phaseLabel.setTypedText(touchgfx::TypedText(T_TEXT_PRETESTSUCCESS));
    }
    else
    {
        phaseLabel.setTypedText(touchgfx::TypedText(T_TEXT_PRETESTRELEASING));
    }

    int32_t boundedPressure = status.pressureMbar;
    if (boundedPressure < 0)
    {
        boundedPressure = 0;
    }
    if (status.controlTargetMbar > 0 && boundedPressure > status.controlTargetMbar)
    {
        boundedPressure = status.controlTargetMbar;
    }

    uint16_t fillPixels = 0U;
    if (status.controlTargetMbar > 0)
    {
        fillPixels = static_cast<uint16_t>((boundedPressure * 400L) / status.controlTargetMbar);
    }
    if (fillPixels > 400U)
    {
        fillPixels = 400U;
    }

    const bool showFill = fillPixels > 0U;
    progressFill.setVisible(showFill && fillPixels > 12U);
    progressFillLeftCap.setVisible(showFill);
    progressFillRightCap.setVisible(showFill);
    if (showFill)
    {
        const uint16_t roundedPixels = fillPixels < 12U ? 12U : fillPixels;
        if (roundedPixels > 12U)
        {
            progressFill.setWidth(static_cast<int16_t>(roundedPixels - 12U));
        }
        progressFillRightCap.setX(static_cast<int16_t>(40U + roundedPixels - 12U));
    }


    countdownCaption.setVisible(!success);
    timeRemaining.setVisible(!success);

    progressFill.invalidate();
    progressFillLeftCap.invalidate();
    progressFillRightCap.invalidate();

    phaseLabel.invalidate();
    countdownCaption.invalidate();
    timeRemaining.invalidate();
}

void Screen20View::completeSuccessfulPretest()
{
    transitionRequested = true;
    presenter->resetPretest();
    if (successfulProduct == ActiveProductBandy)
    {
        application().gotoRfidWaitScreenNoTransition();
    }
    else
    {
        application().gotoHemorflowWaitScreenNoTransition();
    }
}

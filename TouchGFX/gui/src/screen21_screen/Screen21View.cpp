#include <gui/screen21_screen/Screen21View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <images/SVGDatabase.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/Unicode.hpp>

namespace
{
// Durata dell'animazione dei pallini "releasing pressure": ~5 s a 60 Hz.
// Per cambiare il tempo modifica questo valore (ticks = secondi * 60).
constexpr uint16_t kReleaseDotsDurationTicks = 300; // ~5 s
// Velocita di avanzamento del pallino acceso: ~333 ms per pallino.
constexpr uint16_t kReleaseDotCycleTicks     = 20;
}

Screen21View::Screen21View()
    : transitionRequested(false),
      exitRequested(false),
      releaseDotsActive(false),
      releaseDotTicks(0)
{
}

void Screen21View::setupScreen()
{
    Screen21ViewBase::setupScreen();
    transitionRequested = false;
    exitRequested = false;
    releaseDotsActive = false;
    releaseDotTicks = 0;
    hideReleaseDots();
    applyStatus(presenter->getStatus());
    touchgfx::Application::getInstance()->invalidateArea(touchgfx::Rect(0, 0, 480, 320));
}

void Screen21View::tearDownScreen()
{
    Screen21ViewBase::tearDownScreen();
}

void Screen21View::handleTickEvent()
{
    Screen21ViewBase::handleTickEvent();

    if (!releaseDotsActive)
    {
        return;
    }

    if (releaseDotTicks >= kReleaseDotsDurationTicks)
    {
        // Trascorsi i ~2 s i pallini spariscono.
        hideReleaseDots();
        releaseDotsActive = false;
        return;
    }

    updateDots(static_cast<uint8_t>((releaseDotTicks / kReleaseDotCycleTicks) % 3U));
    ++releaseDotTicks;
}

void Screen21View::backClicked()
{
    if (transitionRequested)
    {
        return;
    }

    const PneumaticPretestStatus status = presenter->getStatus();
    if (status.state == PneumaticPretestReleasing)
    {
        exitRequested = true;
        return;
    }

    transitionRequested = true;
    presenter->resetPretest();
    application().gotoProductSelectScreenNoTransition();
}

void Screen21View::retryClicked()
{
    if (transitionRequested || !isRetryAvailable(presenter->getStatus()))
    {
        return;
    }

    if (presenter->retryPretest())
    {
        transitionRequested = true;
        application().gotoPneumaticPretestScreenNoTransition();
    }
}

void Screen21View::updatePretest(const PneumaticPretestStatus& status)
{
    if (transitionRequested)
    {
        return;
    }

    applyStatus(status);
    if (exitRequested && status.state != PneumaticPretestReleasing)
    {
        transitionRequested = true;
        presenter->resetPretest();
        application().gotoProductSelectScreenNoTransition();
    }
}

void Screen21View::applyStatus(const PneumaticPretestStatus& status)
{
    const bool technicalFault =
        status.result == PneumaticPretestResultSensorFault ||
        status.result == PneumaticPretestResultActuatorFault ||
        status.state == PneumaticPretestTechnicalFault;

    errorTitle.setTypedText(touchgfx::TypedText(technicalFault ? T_TEXT_PRETESTTECHNICALTITLE : T_TEXT_PRETESTLEAKTITLE));
    errorMessage.setTypedText(touchgfx::TypedText(technicalFault ? T_TEXT_PRETESTTECHNICALMESSAGE : T_TEXT_PRETESTLEAKMESSAGE));

    const bool releasing = status.state == PneumaticPretestReleasing;
    releaseStatus.setVisible(releasing);
    setReleaseDotsActive(releasing);
    const bool retryAvailable = isRetryAvailable(status);
    retryButton.setTouchable(retryAvailable);
    retryButton.setAlpha(retryAvailable ? 255 : 90);
    retryLabel.setAlpha(retryAvailable ? 255 : 90);

    errorTitle.invalidate();
    errorMessage.invalidate();
    releaseStatus.invalidate();
    retryButton.invalidate();
    retryLabel.invalidate();
}

void Screen21View::setReleaseDotsActive(bool active)
{
    if (active)
    {
        if (!releaseDotsActive)
        {
            // Avvia l'animazione la prima volta che entriamo nello stato "releasing".
            releaseDotsActive = true;
            releaseDotTicks = 0;
            updateDots(0);
        }
    }
    else
    {
        releaseDotsActive = false;
        hideReleaseDots();
    }
}

void Screen21View::updateDots(uint8_t activeIndex)
{
    releaseDot1.setSVG(activeIndex == 0U ? SVG_DOT_ACTIVE_ID : SVG_DOT_DIM_ID);
    releaseDot2.setSVG(activeIndex == 1U ? SVG_DOT_ACTIVE_ID : SVG_DOT_DIM_ID);
    releaseDot3.setSVG(activeIndex == 2U ? SVG_DOT_ACTIVE_ID : SVG_DOT_DIM_ID);
    releaseDot1.setVisible(true);
    releaseDot2.setVisible(true);
    releaseDot3.setVisible(true);
    releaseDot1.invalidate();
    releaseDot2.invalidate();
    releaseDot3.invalidate();
}

void Screen21View::hideReleaseDots()
{
    releaseDot1.setVisible(false);
    releaseDot2.setVisible(false);
    releaseDot3.setVisible(false);
    releaseDot1.invalidate();
    releaseDot2.invalidate();
    releaseDot3.invalidate();
}

bool Screen21View::isRetryAvailable(const PneumaticPretestStatus& status) const
{
    return status.state == PneumaticPretestLeakFailed ||
           status.state == PneumaticPretestTechnicalFault;
}

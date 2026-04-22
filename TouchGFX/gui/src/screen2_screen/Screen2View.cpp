#include <gui/screen2_screen/Screen2View.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/Unicode.hpp>

namespace
{
uint16_t rgb(uint8_t red, uint8_t green, uint8_t blue)
{
    return touchgfx::Color::getColorFromRGB(red, green, blue);
}

int clampTrendValue(int value)
{
    if (value < 0)
    {
        return 0;
    }

    if (value > 280)
    {
        return 280;
    }

    return value;
}
}

Screen2View::Screen2View()
    : graphSeeded(false)
{
    touchgfx::Unicode::snprintf(currentValueBuffer, CURRENTVALUE_SIZE, "%d", 0);
    touchgfx::Unicode::snprintf(targetValueBuffer, TARGETVALUE_SIZE, "%d", 200);
    touchgfx::Unicode::snprintf(stabilityValueBuffer, STABILITYVALUE_SIZE, "%d%%", 0);
    touchgfx::Unicode::snprintf(ligaturesValueBuffer, LIGATURESVALUE_SIZE, "%d", 0);
    touchgfx::Unicode::strncpy(statusValueBuffer, "PRONTO", STATUSVALUE_SIZE);
    touchgfx::Unicode::strncpy(warningValueBuffer, "IN ATTESA", WARNINGVALUE_SIZE);
    touchgfx::Unicode::strncpy(suctionToggleLabelBuffer, "AVVIA", SUCTIONTOGGLELABEL_SIZE);
}

void Screen2View::setupScreen()
{
    Screen2ViewBase::setupScreen();

    trendGraph.clear();
    graphSeeded = false;

    if (presenter != 0)
    {
        applyDashboardState(presenter->getDashboardState());
    }
}

void Screen2View::tearDownScreen()
{
    trendGraph.clear();
    graphSeeded = false;
    Screen2ViewBase::tearDownScreen();
}

void Screen2View::decreaseTargetClicked()
{
    if (presenter != 0)
    {
        presenter->decreaseTarget();
    }
}

void Screen2View::increaseTargetClicked()
{
    if (presenter != 0)
    {
        presenter->increaseTarget();
    }
}

void Screen2View::suctionToggleClicked()
{
    if (presenter != 0)
    {
        presenter->toggleSuction();
    }
}

void Screen2View::releaseConfirmedClicked()
{
    if (presenter != 0)
    {
        presenter->confirmRelease();
    }
}

void Screen2View::applyDashboardState(const DashboardState& state)
{
    touchgfx::Unicode::snprintf(currentValueBuffer, CURRENTVALUE_SIZE, "%d", static_cast<int>(state.currentPressureMbar));
    touchgfx::Unicode::snprintf(targetValueBuffer, TARGETVALUE_SIZE, "%d", static_cast<int>(state.targetPressureMbar));
    touchgfx::Unicode::snprintf(stabilityValueBuffer, STABILITYVALUE_SIZE, "%d%%", static_cast<int>(state.stabilityPercent));
    touchgfx::Unicode::snprintf(ligaturesValueBuffer, LIGATURESVALUE_SIZE, "%d", static_cast<int>(state.ligatureCount));
    touchgfx::Unicode::strncpy(statusValueBuffer, getStatusText(state), STATUSVALUE_SIZE);
    touchgfx::Unicode::strncpy(warningValueBuffer, getWarningText(state), WARNINGVALUE_SIZE);
    touchgfx::Unicode::strncpy(suctionToggleLabelBuffer, getSuctionLabel(state), SUCTIONTOGGLELABEL_SIZE);

    updateStatusAppearance(state);
    updateWarningAppearance(state);
    updateProcedureAppearance(state.procedureStep);

    currentValue.invalidate();
    targetValue.invalidate();
    stabilityValue.invalidate();
    ligaturesValue.invalidate();
    statusValue.invalidate();
    warningValue.invalidate();
    suctionToggleLabel.invalidate();

    trendGraph.addDataPoint(clampTrendValue(static_cast<int>(state.currentPressureMbar)));
    graphSeeded = true;
}

void Screen2View::updateStatusAppearance(const DashboardState& state)
{
    uint16_t statusFill = rgb(24, 80, 74);
    uint16_t statusBorder = rgb(78, 146, 136);
    uint16_t heroBorder = rgb(56, 86, 124);
    uint16_t trendColor = rgb(52, 181, 199);

    if (state.vacuumState == VacuumStatePulling)
    {
        statusFill = rgb(17, 71, 102);
        statusBorder = rgb(68, 125, 171);
        heroBorder = rgb(68, 125, 171);
        trendColor = rgb(52, 181, 199);
    }
    else if (state.vacuumState == VacuumStateTissueReady)
    {
        statusFill = rgb(22, 92, 54);
        statusBorder = rgb(96, 170, 118);
        heroBorder = rgb(96, 170, 118);
        trendColor = rgb(103, 210, 137);
    }
    else if (state.vacuumState == VacuumStateWarning)
    {
        statusFill = rgb(111, 48, 24);
        statusBorder = rgb(229, 126, 69);
        heroBorder = rgb(229, 126, 69);
        trendColor = rgb(229, 126, 69);
    }

    statusBadge.setColor(statusFill);
    statusBadge.setBorderColor(statusBorder);
    statusBadge.invalidate();

    heroCard.setBorderColor(heroBorder);
    heroCard.invalidate();

    trendGraphLine1Painter.setColor(trendColor);
    trendGraph.invalidate();
}

void Screen2View::updateWarningAppearance(const DashboardState& state)
{
    uint16_t fillColor = rgb(16, 28, 44);
    uint16_t borderColor = rgb(56, 86, 124);
    uint16_t textColor = rgb(242, 247, 250);

    if (state.warningCode == WarningCodeLeak)
    {
        fillColor = rgb(72, 34, 20);
        borderColor = rgb(229, 126, 69);
        textColor = rgb(255, 227, 205);
    }
    else if (state.warningCode == WarningCodeSensorMissing)
    {
        fillColor = rgb(80, 25, 25);
        borderColor = rgb(214, 92, 92);
        textColor = rgb(255, 220, 220);
    }
    else if (state.vacuumState == VacuumStateTissueReady)
    {
        fillColor = rgb(15, 43, 33);
        borderColor = rgb(90, 160, 110);
        textColor = rgb(225, 246, 231);
    }

    warningCard.setColor(fillColor);
    warningCard.setBorderColor(borderColor);
    warningCard.invalidate();

    warningValue.setColor(textColor);
    warningValue.invalidate();
}

void Screen2View::updateProcedureAppearance(ProcedureStep currentStep)
{
    const uint8_t currentIndex = static_cast<uint8_t>(currentStep);

    applyStepVisual(stepLoadBox, stepLoadText, currentIndex == 0 ? 2 : 1);
    applyStepVisual(stepPositionBox, stepPositionText, currentIndex == 1 ? 2 : (currentIndex > 1 ? 1 : 0));
    applyStepVisual(stepAspirateBox, stepAspirateText, currentIndex == 2 ? 2 : (currentIndex > 2 ? 1 : 0));
    applyStepVisual(stepReleaseBox, stepReleaseText, currentIndex == 3 ? 2 : (currentIndex > 3 ? 1 : 0));
    applyStepVisual(stepVerifyBox, stepVerifyText, currentIndex == 4 ? 2 : 0);
}

void Screen2View::applyStepVisual(touchgfx::BoxWithBorder& box, touchgfx::TextArea& label, uint8_t visualState)
{
    uint16_t fillColor = rgb(15, 27, 43);
    uint16_t borderColor = rgb(40, 64, 92);
    uint16_t textColor = rgb(160, 178, 196);

    if (visualState == 1)
    {
        fillColor = rgb(19, 42, 69);
        borderColor = rgb(82, 129, 176);
        textColor = rgb(215, 230, 244);
    }
    else if (visualState == 2)
    {
        fillColor = rgb(95, 64, 19);
        borderColor = rgb(234, 167, 44);
        textColor = rgb(255, 244, 220);
    }

    box.setColor(fillColor);
    box.setBorderColor(borderColor);
    box.invalidate();

    label.setColor(textColor);
    label.invalidate();
}

const char* Screen2View::getStatusText(const DashboardState& state) const
{
    switch (state.vacuumState)
    {
    case VacuumStatePulling:
        return "ASPIRAZIONE";
    case VacuumStateTissueReady:
        return "TESSUTO OK";
    case VacuumStateWarning:
        return "ATTENZIONE";
    case VacuumStateReady:
    default:
        return "PRONTO";
    }
}

const char* Screen2View::getWarningText(const DashboardState& state) const
{
    switch (state.warningCode)
    {
    case WarningCodeLeak:
        return "PERDITA ASPIRAZ.";
    case WarningCodeSensorMissing:
        return "SENSORE ASSENTE";
    case WarningCodeNone:
    default:
        break;
    }

    if (state.procedureStep == ProcedureStepVerify)
    {
        return "VERIFICA RILASCIO";
    }

    if (state.vacuumState == VacuumStateTissueReady)
    {
        return "RILASCIO CONSENTITO";
    }

    if (state.suctionEnabled)
    {
        return "CONTROLLO TESSUTO";
    }

    return "IN ATTESA";
}

const char* Screen2View::getSuctionLabel(const DashboardState& state) const
{
    return state.suctionEnabled ? "STOP" : "AVVIA";
}

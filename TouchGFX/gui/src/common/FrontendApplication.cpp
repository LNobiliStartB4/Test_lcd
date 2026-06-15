#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/common/AdminScreens.hpp>
#include <gui/screen13_screen/Screen13View.hpp>
#include <gui/screen13_screen/Screen13Presenter.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen4_screen/Screen4Presenter.hpp>
#include <gui/screen4_screen/Screen4View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <gui/screen3_screen/Screen3View.hpp>
#include <gui/screen5_screen/Screen5Presenter.hpp>
#include <gui/screen5_screen/Screen5View.hpp>
#include <gui/screen6_screen/Screen6Presenter.hpp>
#include <gui/screen6_screen/Screen6View.hpp>
#include <gui/screen7_screen/Screen7Presenter.hpp>
#include <gui/screen7_screen/Screen7View.hpp>
#include <gui/screen8_screen/Screen8Presenter.hpp>
#include <gui/screen8_screen/Screen8View.hpp>
#include <gui/screen9_screen/Screen9Presenter.hpp>
#include <gui/screen9_screen/Screen9View.hpp>
#include <gui/screen10_screen/Screen10Presenter.hpp>
#include <gui/screen10_screen/Screen10View.hpp>
#include <gui/screen11_screen/Screen11Presenter.hpp>
#include <gui/screen11_screen/Screen11View.hpp>
#include <gui/screen12_screen/Screen12Presenter.hpp>
#include <gui/screen12_screen/Screen12View.hpp>
#include <gui/screen18_screen/Screen18Presenter.hpp>
#include <gui/screen18_screen/Screen18View.hpp>
#include <gui/screen19_screen/Screen19Presenter.hpp>
#include <gui/screen19_screen/Screen19View.hpp>
#include <gui/screen20_screen/Screen20Presenter.hpp>
#include <gui/screen20_screen/Screen20View.hpp>
#include <gui/screen21_screen/Screen21Presenter.hpp>
#include <gui/screen21_screen/Screen21View.hpp>
#include <touchgfx/transitions/NoTransition.hpp>
#include <touchgfx/events/ClickEvent.hpp>
#include <touchgfx/events/DragEvent.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap),
      transitionDeferred(false)
{
}

void FrontendApplication::handleClickEvent(const touchgfx::ClickEvent& event)
{
#if TOUCH_FEEDBACK_ENABLED
    if (event.getType() == touchgfx::ClickEvent::PRESSED)
    {
        touchfeedback::TouchFeedbackController::instance().recordPress(event.getX(), event.getY());
    }
    else // RELEASED or CANCEL
    {
        touchfeedback::TouchFeedbackController::instance().recordRelease();
    }
#endif
    // Preserve normal event routing to the active screen.
    FrontendApplicationBase::handleClickEvent(event);
}

void FrontendApplication::handleDragEvent(const touchgfx::DragEvent& event)
{
#if TOUCH_FEEDBACK_ENABLED
    touchfeedback::TouchFeedbackController::instance().recordMove(event.getNewX(), event.getNewY());
#endif
    FrontendApplicationBase::handleDragEvent(event);
}

void FrontendApplication::handlePendingScreenTransition()
{
#if TOUCH_FEEDBACK_ENABLED
    // A screen change redraws the whole display (slow over SPI). If the pointer
    // is still visible, clear it this frame and defer the transition by ONE
    // frame, so the dot is wiped from the framebuffer before the heavy redraw
    // instead of lingering, "frozen", until overwritten. The transitionDeferred
    // guard bounds this to a single frame so continuous touch activity can never
    // stall navigation.
    const bool transitionPending =
        (pendingScreenTransitionCallback != 0) && pendingScreenTransitionCallback->isValid();
    if (transitionPending && !transitionDeferred &&
        touchfeedback::TouchFeedbackController::instance().isPointerVisible())
    {
        touchfeedback::TouchFeedbackController::instance().cancel();
        transitionDeferred = true;
        return; // keep the pending transition; it runs next frame (pointer now hidden)
    }
    transitionDeferred = false;
#endif
    FrontendApplicationBase::handlePendingScreenTransition();
}

void FrontendApplication::gotoProductSelectScreenNoTransition()
{
    productSelectTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoProductSelectScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &productSelectTransitionCallback;
}

void FrontendApplication::gotoBandyScreenNoTransition()
{
    bandyTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoBandyScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &bandyTransitionCallback;
}

void FrontendApplication::gotoRfidWaitScreenNoTransition()
{
    rfidWaitTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoRfidWaitScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &rfidWaitTransitionCallback;
}

void FrontendApplication::gotoSetpointEditScreenNoTransition()
{
    setpointEditTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoSetpointEditScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &setpointEditTransitionCallback;
}

void FrontendApplication::gotoSetpointKeypadScreenNoTransition()
{
    setpointKeypadTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoSetpointKeypadScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &setpointKeypadTransitionCallback;
}

void FrontendApplication::gotoPauseScreenNoTransition()
{
    pauseTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoPauseScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &pauseTransitionCallback;
}

void FrontendApplication::gotoEndConfirmScreenNoTransition()
{
    endConfirmTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoEndConfirmScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &endConfirmTransitionCallback;
}

void FrontendApplication::gotoHemorflowWaitScreenNoTransition()
{
    hemorflowWaitTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoHemorflowWaitScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &hemorflowWaitTransitionCallback;
}

void FrontendApplication::gotoHemorflowScreenNoTransition()
{
    hemorflowTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoHemorflowScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &hemorflowTransitionCallback;
}

void FrontendApplication::gotoSettingsScreenNoTransition()
{
    settingsTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoSettingsScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &settingsTransitionCallback;
}

void FrontendApplication::gotoBrightnessScreenNoTransition()
{
    brightnessTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoBrightnessScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &brightnessTransitionCallback;
}

void FrontendApplication::gotoLanguageScreenNoTransition()
{
    languageTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoLanguageScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &languageTransitionCallback;
}

void FrontendApplication::gotoAdminPinScreenNoTransition()
{
    adminPinTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoAdminPinScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &adminPinTransitionCallback;
}

void FrontendApplication::gotoAdminMenuScreenNoTransition()
{
    adminMenuTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoAdminMenuScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &adminMenuTransitionCallback;
}

void FrontendApplication::gotoAdminSystemInfoScreenNoTransition()
{
    adminSystemInfoTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoAdminSystemInfoScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &adminSystemInfoTransitionCallback;
}

void FrontendApplication::gotoAdminPressureScreenNoTransition()
{
    adminPressureTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoAdminPressureScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &adminPressureTransitionCallback;
}

void FrontendApplication::gotoAdminMemoryScreenNoTransition()
{
    adminMemoryTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoAdminMemoryScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &adminMemoryTransitionCallback;
}

void FrontendApplication::gotoBandyCompletedScreenNoTransition()
{
    bandyCompletedTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoBandyCompletedScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &bandyCompletedTransitionCallback;
}

void FrontendApplication::gotoPneumaticPretestScreenNoTransition()
{
    pneumaticPretestTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoPneumaticPretestScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &pneumaticPretestTransitionCallback;
}

void FrontendApplication::gotoPneumaticPretestErrorScreenNoTransition()
{
    pneumaticPretestErrorTransitionCallback = touchgfx::Callback<FrontendApplication>(this, &FrontendApplication::gotoPneumaticPretestErrorScreenNoTransitionImpl);
    pendingScreenTransitionCallback = &pneumaticPretestErrorTransitionCallback;
}

void FrontendApplication::gotoProductSelectScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen2View, Screen2Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoRfidWaitScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen4View, Screen4Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoBandyScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen3View, Screen3Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoSetpointEditScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen5View, Screen5Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoSetpointKeypadScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen18View, Screen18Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoPauseScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen6View, Screen6Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoEndConfirmScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen7View, Screen7Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoHemorflowWaitScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen8View, Screen8Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoHemorflowScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen9View, Screen9Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoSettingsScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen10View, Screen10Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoBrightnessScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen11View, Screen11Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoLanguageScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen12View, Screen12Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoAdminPinScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen13View, Screen13Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoAdminMenuScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen14View, Screen14Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoAdminSystemInfoScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen15View, Screen15Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoAdminPressureScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen16View, Screen16Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoAdminMemoryScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen17View, Screen17Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoBandyCompletedScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen19View, Screen19Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoPneumaticPretestScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen20View, Screen20Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

void FrontendApplication::gotoPneumaticPretestErrorScreenNoTransitionImpl()
{
    touchgfx::makeTransition<Screen21View, Screen21Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, frontendHeap, &currentTransition, &model);
}

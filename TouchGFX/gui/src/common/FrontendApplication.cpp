#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
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
#include <touchgfx/transitions/NoTransition.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap)
{
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

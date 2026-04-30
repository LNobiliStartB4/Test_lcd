#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/screen2_screen/Screen2View.hpp>
#include <gui/screen4_screen/Screen4Presenter.hpp>
#include <gui/screen4_screen/Screen4View.hpp>
#include <gui/screen3_screen/Screen3Presenter.hpp>
#include <gui/screen3_screen/Screen3View.hpp>
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

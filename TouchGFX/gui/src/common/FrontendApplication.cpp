#include <gui/common/FrontendApplication.hpp>
#include <gui/common/FrontendHeap.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>
#include <gui/screen2_screen/Screen2View.hpp>
#include <touchgfx/transitions/NoTransition.hpp>

FrontendApplication::FrontendApplication(Model& m, FrontendHeap& heap)
    : FrontendApplicationBase(m, heap),
      pendingAppScreenId(0)
{

}

void FrontendApplication::gotoScreen2ScreenNoTransition()
{
    pendingAppScreenId = 2;
}

void FrontendApplication::handlePendingScreenTransition()
{
    if (pendingAppScreenId == 2)
    {
        pendingAppScreenId = 0;
        touchgfx::makeTransition<Screen2View, Screen2Presenter, touchgfx::NoTransition, Model>(&currentScreen, &currentPresenter, static_cast<touchgfx::MVPHeap&>(frontendHeap), &currentTransition, &model);
        return;
    }

    FrontendApplicationBase::handlePendingScreenTransition();
}

#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>

class FrontendHeap;

using namespace touchgfx;

class FrontendApplication : public FrontendApplicationBase
{
public:
    FrontendApplication(Model& m, FrontendHeap& heap);
    virtual ~FrontendApplication() { }

    void gotoProductSelectScreenNoTransition();
    void gotoRfidWaitScreenNoTransition();
    void gotoBandyScreenNoTransition();

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

private:
    void gotoProductSelectScreenNoTransitionImpl();
    void gotoRfidWaitScreenNoTransitionImpl();
    void gotoBandyScreenNoTransitionImpl();

    touchgfx::Callback<FrontendApplication> productSelectTransitionCallback;
    touchgfx::Callback<FrontendApplication> rfidWaitTransitionCallback;
    touchgfx::Callback<FrontendApplication> bandyTransitionCallback;
};

#endif // FRONTENDAPPLICATION_HPP

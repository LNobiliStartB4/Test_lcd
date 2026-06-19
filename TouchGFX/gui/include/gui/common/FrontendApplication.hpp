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
    void gotoSetpointEditScreenNoTransition();
    void gotoPauseScreenNoTransition();
    void gotoEndConfirmScreenNoTransition();
    void gotoHemorflowWaitScreenNoTransition();
    void gotoHemorflowScreenNoTransition();

    virtual void handleTickEvent()
    {
        model.tick();
        FrontendApplicationBase::handleTickEvent();
    }

private:
    void gotoProductSelectScreenNoTransitionImpl();
    void gotoRfidWaitScreenNoTransitionImpl();
    void gotoBandyScreenNoTransitionImpl();
    void gotoSetpointEditScreenNoTransitionImpl();
    void gotoPauseScreenNoTransitionImpl();
    void gotoEndConfirmScreenNoTransitionImpl();
    void gotoHemorflowWaitScreenNoTransitionImpl();
    void gotoHemorflowScreenNoTransitionImpl();

    touchgfx::Callback<FrontendApplication> productSelectTransitionCallback;
    touchgfx::Callback<FrontendApplication> rfidWaitTransitionCallback;
    touchgfx::Callback<FrontendApplication> bandyTransitionCallback;
    touchgfx::Callback<FrontendApplication> setpointEditTransitionCallback;
    touchgfx::Callback<FrontendApplication> pauseTransitionCallback;
    touchgfx::Callback<FrontendApplication> endConfirmTransitionCallback;
    touchgfx::Callback<FrontendApplication> hemorflowWaitTransitionCallback;
    touchgfx::Callback<FrontendApplication> hemorflowTransitionCallback;
};

#endif // FRONTENDAPPLICATION_HPP

#ifndef FRONTENDAPPLICATION_HPP
#define FRONTENDAPPLICATION_HPP

#include <gui_generated/common/FrontendApplicationBase.hpp>
#include <gui/common/TouchFeedbackController.hpp>

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
    void gotoSetpointKeypadScreenNoTransition();
    void gotoPauseScreenNoTransition();
    void gotoEndConfirmScreenNoTransition();
    void gotoHemorflowWaitScreenNoTransition();
    void gotoHemorflowScreenNoTransition();
    void gotoSettingsScreenNoTransition();
    void gotoBrightnessScreenNoTransition();
    void gotoLanguageScreenNoTransition();
    void gotoAdminPinScreenNoTransition();
    void gotoAdminMenuScreenNoTransition();
    void gotoAdminSystemInfoScreenNoTransition();
    void gotoAdminPressureScreenNoTransition();
    void gotoAdminMemoryScreenNoTransition();

    virtual void handleTickEvent()
    {
        model.tick();
#if TOUCH_FEEDBACK_ENABLED
        touchfeedback::TouchFeedbackController::instance().tick();
#endif
        FrontendApplicationBase::handleTickEvent();
    }

    virtual void handleClickEvent(const touchgfx::ClickEvent& event);
    virtual void handleDragEvent(const touchgfx::DragEvent& event);
    virtual void handlePendingScreenTransition();

private:
    void gotoProductSelectScreenNoTransitionImpl();
    void gotoRfidWaitScreenNoTransitionImpl();
    void gotoBandyScreenNoTransitionImpl();
    void gotoSetpointEditScreenNoTransitionImpl();
    void gotoSetpointKeypadScreenNoTransitionImpl();
    void gotoPauseScreenNoTransitionImpl();
    void gotoEndConfirmScreenNoTransitionImpl();
    void gotoHemorflowWaitScreenNoTransitionImpl();
    void gotoHemorflowScreenNoTransitionImpl();
    void gotoSettingsScreenNoTransitionImpl();
    void gotoBrightnessScreenNoTransitionImpl();
    void gotoLanguageScreenNoTransitionImpl();
    void gotoAdminPinScreenNoTransitionImpl();
    void gotoAdminMenuScreenNoTransitionImpl();
    void gotoAdminSystemInfoScreenNoTransitionImpl();
    void gotoAdminPressureScreenNoTransitionImpl();
    void gotoAdminMemoryScreenNoTransitionImpl();

    touchgfx::Callback<FrontendApplication> productSelectTransitionCallback;
    touchgfx::Callback<FrontendApplication> rfidWaitTransitionCallback;
    touchgfx::Callback<FrontendApplication> bandyTransitionCallback;
    touchgfx::Callback<FrontendApplication> setpointEditTransitionCallback;
    touchgfx::Callback<FrontendApplication> setpointKeypadTransitionCallback;
    touchgfx::Callback<FrontendApplication> pauseTransitionCallback;
    touchgfx::Callback<FrontendApplication> endConfirmTransitionCallback;
    touchgfx::Callback<FrontendApplication> hemorflowWaitTransitionCallback;
    touchgfx::Callback<FrontendApplication> hemorflowTransitionCallback;
    touchgfx::Callback<FrontendApplication> settingsTransitionCallback;
    touchgfx::Callback<FrontendApplication> brightnessTransitionCallback;
    touchgfx::Callback<FrontendApplication> languageTransitionCallback;
    touchgfx::Callback<FrontendApplication> adminPinTransitionCallback;
    touchgfx::Callback<FrontendApplication> adminMenuTransitionCallback;
    touchgfx::Callback<FrontendApplication> adminSystemInfoTransitionCallback;
    touchgfx::Callback<FrontendApplication> adminPressureTransitionCallback;
    touchgfx::Callback<FrontendApplication> adminMemoryTransitionCallback;

    // Guards handlePendingScreenTransition so the touch pointer can postpone a
    // transition by at most one frame (clear the dot, then transition), never
    // stalling navigation even under continuous touch activity.
    bool transitionDeferred;
};

#endif // FRONTENDAPPLICATION_HPP

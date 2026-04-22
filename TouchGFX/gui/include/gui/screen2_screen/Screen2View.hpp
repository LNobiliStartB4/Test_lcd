#ifndef SCREEN2VIEW_HPP
#define SCREEN2VIEW_HPP

#include <gui/model/DashboardTypes.hpp>
#include <gui_generated/screen2_screen/Screen2ViewBase.hpp>
#include <gui/screen2_screen/Screen2Presenter.hpp>

class Screen2View : public Screen2ViewBase
{
public:
    Screen2View();
    virtual ~Screen2View()
    {
    }

    virtual void setupScreen();
    virtual void tearDownScreen();

    virtual void decreaseTargetClicked();
    virtual void increaseTargetClicked();
    virtual void suctionToggleClicked();
    virtual void releaseConfirmedClicked();

    void applyDashboardState(const DashboardState& state);

private:
    void updateStatusAppearance(const DashboardState& state);
    void updateWarningAppearance(const DashboardState& state);
    void updateProcedureAppearance(ProcedureStep currentStep);
    void applyStepVisual(touchgfx::BoxWithBorder& box, touchgfx::TextArea& label, uint8_t visualState);
    const char* getStatusText(const DashboardState& state) const;
    const char* getWarningText(const DashboardState& state) const;
    const char* getSuctionLabel(const DashboardState& state) const;

    bool graphSeeded;
};

#endif // SCREEN2VIEW_HPP

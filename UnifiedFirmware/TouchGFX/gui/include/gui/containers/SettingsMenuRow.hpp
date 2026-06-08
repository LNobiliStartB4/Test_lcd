#ifndef SETTINGSMENUROW_HPP
#define SETTINGSMENUROW_HPP

#include <gui_generated/containers/SettingsMenuRowBase.hpp>
#include <touchgfx/Callback.hpp>

class SettingsMenuRow : public SettingsMenuRowBase
{
public:
    SettingsMenuRow();
    virtual ~SettingsMenuRow() {}

    virtual void initialize();
    virtual void rowClicked();

    void setContent(uint16_t iconId,
                    touchgfx::TypedTextId titleId,
                    touchgfx::TypedTextId descriptionId);
    void setAction(touchgfx::GenericCallback<>& callback);
protected:
    touchgfx::GenericCallback<>* actionCallback;
};

#endif // SETTINGSMENUROW_HPP

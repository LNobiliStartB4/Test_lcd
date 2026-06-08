#include <gui/containers/SettingsMenuRow.hpp>
#include <images/SVGDatabase.hpp>

SettingsMenuRow::SettingsMenuRow()
    : actionCallback(0)
{

}

void SettingsMenuRow::initialize()
{
    SettingsMenuRowBase::initialize();
    rowIcon.setScale(1.0f, 1.0f);
    rowChevron.setScale(1.0f, 1.0f);
}

void SettingsMenuRow::rowClicked()
{
    if ((actionCallback != 0) && actionCallback->isValid())
    {
        actionCallback->execute();
    }
}

void SettingsMenuRow::setContent(uint16_t iconId,
                                 touchgfx::TypedTextId titleId,
                                 touchgfx::TypedTextId descriptionId)
{
    rowIcon.setSVG(iconId);
    rowIcon.setScale(1.0f, 1.0f);
    rowTitle.setTypedText(touchgfx::TypedText(titleId));
    rowDescription.setTypedText(touchgfx::TypedText(descriptionId));
    rowIcon.invalidate();
    rowTitle.invalidate();
    rowDescription.invalidate();
}

void SettingsMenuRow::setAction(touchgfx::GenericCallback<>& callback)
{
    actionCallback = &callback;
}

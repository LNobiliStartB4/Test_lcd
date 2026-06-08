#include <gui/screen12_screen/Screen12View.hpp>
#include <gui/screen12_screen/Screen12Presenter.hpp>

Screen12Presenter::Screen12Presenter(Screen12View& v)
    : view(v)
{

}

void Screen12Presenter::activate()
{

}

void Screen12Presenter::deactivate()
{

}

UiLanguage Screen12Presenter::getUiLanguage() const
{
    return (model != 0) ? model->getUiLanguage() : UiLanguageEnglish;
}

void Screen12Presenter::setUiLanguage(UiLanguage language)
{
    if (model != 0)
    {
        model->setUiLanguage(language);
    }
}

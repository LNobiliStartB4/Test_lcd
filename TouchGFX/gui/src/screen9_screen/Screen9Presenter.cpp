#include <gui/screen9_screen/Screen9Presenter.hpp>
#include <gui/screen9_screen/Screen9View.hpp>
#include <gui/model/Model.hpp>

Screen9Presenter::Screen9Presenter(Screen9View& v)
    : view(v)
{
}

void Screen9Presenter::activate()
{
}

void Screen9Presenter::deactivate()
{
}

HemorflowState Screen9Presenter::getHemorflowState() const
{
    return model->getHemorflowState();
}

bool Screen9Presenter::shouldReturnToHemorflowWait() const
{
    return model->shouldReturnToHemorflowWait();
}

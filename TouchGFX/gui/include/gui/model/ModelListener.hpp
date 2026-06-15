#ifndef MODELLISTENER_HPP
#define MODELLISTENER_HPP

#include <gui/model/DashboardTypes.hpp>

class Model;

class ModelListener
{
public:
    ModelListener()
        : model(0)
    {
    }

    virtual ~ModelListener()
    {
    }

    void bind(Model* m)
    {
        model = m;
    }

    virtual void bandyStateUpdated(const BandyState& state)
    {
        (void)state;
    }

    virtual void pneumaticPretestUpdated(const PneumaticPretestStatus& status)
    {
        (void)status;
    }

protected:
    Model* model;
};

#endif // MODELLISTENER_HPP

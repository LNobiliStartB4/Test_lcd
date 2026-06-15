#ifndef BANDYCOMPLETION_HPP
#define BANDYCOMPLETION_HPP

#include <gui/model/DashboardTypes.hpp>

inline bool isNaturalBandyCompletion(const BandyState& previousState, const BandyState& state)
{
    return (previousState.sessionState == BandySessionRunning) &&
           (state.sessionState == BandySessionWaitRfid) &&
           (state.remainingSeconds == 0U);
}

#endif // BANDYCOMPLETION_HPP

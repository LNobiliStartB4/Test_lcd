#ifndef APPLICATION_FEEDBACK_H
#define APPLICATION_FEEDBACK_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ApplicationFeedback_Process(void);
void ApplicationFeedback_SetWarningBuzzerActive(bool active);

#ifdef __cplusplus
}
#endif

#endif

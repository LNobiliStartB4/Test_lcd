#ifndef THD_AUTH_H
#define THD_AUTH_H

#include <stdbool.h>
#include <stdint.h>

#define THD_AUTH_SECRET_KEY 0xB782U

typedef struct
{
  uint16_t token;
  bool tokenValid;
} thd_auth_context_t;

void ThdAuth_Init(thd_auth_context_t *context);
uint16_t ThdAuth_CalcResponse(uint16_t token);
void ThdAuth_IssueToken(thd_auth_context_t *context, uint16_t token);
bool ThdAuth_HasToken(const thd_auth_context_t *context);
bool ThdAuth_ConsumeResponse(thd_auth_context_t *context, uint16_t response);

#endif

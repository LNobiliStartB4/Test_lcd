#include "thd_auth.h"

#include <stddef.h>

void ThdAuth_Init(thd_auth_context_t *context)
{
  if (context == NULL)
  {
    return;
  }

  context->token = 0U;
  context->tokenValid = false;
}

uint16_t ThdAuth_CalcResponse(uint16_t token)
{
  uint16_t x = (uint16_t)(token ^ THD_AUTH_SECRET_KEY);

  x ^= (uint16_t)(x << 7);
  x ^= (uint16_t)(x >> 9);
  x ^= (uint16_t)(x << 8);

  return x;
}

void ThdAuth_IssueToken(thd_auth_context_t *context, uint16_t token)
{
  if (context == NULL)
  {
    return;
  }

  context->token = token;
  context->tokenValid = true;
}

bool ThdAuth_HasToken(const thd_auth_context_t *context)
{
  return (context != NULL) && context->tokenValid;
}

bool ThdAuth_ConsumeResponse(thd_auth_context_t *context, uint16_t response)
{
  bool accepted;

  if ((context == NULL) || !context->tokenValid)
  {
    return false;
  }

  accepted = (response == ThdAuth_CalcResponse(context->token));
  context->tokenValid = false;

  return accepted;
}

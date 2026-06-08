#include "thd_protocol_utils.h"

#include <stddef.h>
#include <string.h>

#define THD_PROTOCOL_HEX4_LEN 4U
#define THD_PROTOCOL_START_PREFIX "START"
#define THD_PROTOCOL_START_PREFIX_LEN 5U
#define THD_PROTOCOL_BANDY_DEFAULT_PRESSURE_TARGET_MBAR 490
#define THD_PROTOCOL_BANDY_PRESSURE_CONTROL_MARGIN_MBAR 30
#define THD_PROTOCOL_BANDY_PRESSURE_RESTART_MARGIN_MBAR 50
#define THD_PROTOCOL_HEMORFLOW_DEFAULT_PRESSURE_TARGET_MBAR 150
#define THD_PROTOCOL_HEMORFLOW_PRESSURE_CONTROL_MARGIN_MBAR 20
#define THD_PROTOCOL_HEMORFLOW_PRESSURE_RESTART_MARGIN_MBAR 50

static bool ThdProtocol_HexNibble(char c, uint8_t *nibble)
{
  if (nibble == NULL)
  {
    return false;
  }

  if ((c >= '0') && (c <= '9'))
  {
    *nibble = (uint8_t)(c - '0');
    return true;
  }

  if ((c >= 'A') && (c <= 'F'))
  {
    *nibble = (uint8_t)(c - 'A' + 10);
    return true;
  }

  if ((c >= 'a') && (c <= 'f'))
  {
    *nibble = (uint8_t)(c - 'a' + 10);
    return true;
  }

  return false;
}

bool ThdProtocol_ParseHex4(const char *text, uint16_t *value)
{
  uint16_t parsed = 0U;

  if ((text == NULL) || (value == NULL) || (strlen(text) != THD_PROTOCOL_HEX4_LEN))
  {
    return false;
  }

  for (uint8_t i = 0U; i < THD_PROTOCOL_HEX4_LEN; i++)
  {
    uint8_t nibble;

    if (!ThdProtocol_HexNibble(text[i], &nibble))
    {
      return false;
    }

    parsed = (uint16_t)((parsed << 4) | nibble);
  }

  *value = parsed;
  return true;
}

thd_protocol_command_t ThdProtocol_ParseCommand(const char *command, uint16_t *startResponse)
{
  if (command == NULL)
  {
    return THD_PROTOCOL_COMMAND_UNKNOWN;
  }

  if (strcmp(command, "SET0") == 0)
  {
    return THD_PROTOCOL_COMMAND_SET0;
  }

  if (strcmp(command, "SET1") == 0)
  {
    return THD_PROTOCOL_COMMAND_SET1;
  }

  if (strcmp(command, "TOKEN") == 0)
  {
    return THD_PROTOCOL_COMMAND_TOKEN;
  }

  if (strcmp(command, "STOP") == 0)
  {
    return THD_PROTOCOL_COMMAND_STOP;
  }

  if (strcmp(command, "VLV21") == 0)
  {
    return THD_PROTOCOL_COMMAND_VLV21;
  }

  if (strcmp(command, "VLV20") == 0)
  {
    return THD_PROTOCOL_COMMAND_VLV20;
  }

  if (strcmp(command, "VLV11") == 0)
  {
    return THD_PROTOCOL_COMMAND_VLV11;
  }

  if (strcmp(command, "VLV10") == 0)
  {
    return THD_PROTOCOL_COMMAND_VLV10;
  }

  if ((strncmp(command, THD_PROTOCOL_START_PREFIX, THD_PROTOCOL_START_PREFIX_LEN) == 0) &&
      (strlen(command) == (THD_PROTOCOL_START_PREFIX_LEN + THD_PROTOCOL_HEX4_LEN)) &&
      ThdProtocol_ParseHex4(&command[THD_PROTOCOL_START_PREFIX_LEN], startResponse))
  {
    return THD_PROTOCOL_COMMAND_START;
  }

  return THD_PROTOCOL_COMMAND_UNKNOWN;
}

thd_protocol_valve_action_t ThdProtocol_GetValveAction(thd_protocol_command_t command)
{
  switch (command)
  {
    case THD_PROTOCOL_COMMAND_VLV21:
      return THD_PROTOCOL_VALVE_ACTION_OUTLET_OPEN;

    case THD_PROTOCOL_COMMAND_VLV20:
      return THD_PROTOCOL_VALVE_ACTION_OUTLET_CLOSE;

    case THD_PROTOCOL_COMMAND_VLV11:
    case THD_PROTOCOL_COMMAND_VLV10:
      return THD_PROTOCOL_VALVE_ACTION_UNSUPPORTED;

    default:
      return THD_PROTOCOL_VALVE_ACTION_NONE;
  }
}

bool ThdProtocol_IsStartAllowed(thd_protocol_bank_t selectedBank, bool authAccepted)
{
  if (!authAccepted)
  {
    return false;
  }

  return (selectedBank == THD_PROTOCOL_BANK_NONE) ||
         (selectedBank == THD_PROTOCOL_BANK_HEMORFLOW);
}

thd_protocol_active_product_t ThdProtocol_GetStartProduct(thd_protocol_bank_t selectedBank, bool authAccepted)
{
  if (ThdProtocol_IsStartAllowed(selectedBank, authAccepted))
  {
    return THD_PROTOCOL_ACTIVE_PRODUCT_HEMORFLOW;
  }

  return THD_PROTOCOL_ACTIVE_PRODUCT_NONE;
}

thd_protocol_active_product_t ThdProtocol_GetStopProduct(void)
{
  return THD_PROTOCOL_ACTIVE_PRODUCT_NONE;
}

thd_protocol_bank_t ThdProtocol_GetStopBank(void)
{
  return THD_PROTOCOL_BANK_NONE;
}

int32_t ThdProtocol_GetDefaultPressureTargetMbar(thd_protocol_active_product_t product)
{
  thd_protocol_pressure_profile_t profile;

  if (ThdProtocol_GetPressureProfile(product, &profile))
  {
    return profile.userTargetMbar;
  }

  return 0;
}

bool ThdProtocol_GetPressureProfile(thd_protocol_active_product_t product, thd_protocol_pressure_profile_t *profile)
{
  int32_t userTargetMbar;
  int32_t controlMarginMbar;
  int32_t restartMarginMbar;

  if (profile == NULL)
  {
    return false;
  }

  switch (product)
  {
    case THD_PROTOCOL_ACTIVE_PRODUCT_BANDY:
      userTargetMbar = THD_PROTOCOL_BANDY_DEFAULT_PRESSURE_TARGET_MBAR;
      controlMarginMbar = THD_PROTOCOL_BANDY_PRESSURE_CONTROL_MARGIN_MBAR;
      restartMarginMbar = THD_PROTOCOL_BANDY_PRESSURE_RESTART_MARGIN_MBAR;
      break;

    case THD_PROTOCOL_ACTIVE_PRODUCT_HEMORFLOW:
      userTargetMbar = THD_PROTOCOL_HEMORFLOW_DEFAULT_PRESSURE_TARGET_MBAR;
      controlMarginMbar = THD_PROTOCOL_HEMORFLOW_PRESSURE_CONTROL_MARGIN_MBAR;
      restartMarginMbar = THD_PROTOCOL_HEMORFLOW_PRESSURE_RESTART_MARGIN_MBAR;
      break;

    default:
      return false;
  }

  profile->userTargetMbar = userTargetMbar;
  profile->controlTargetMbar = userTargetMbar - controlMarginMbar;
  profile->restartTargetMbar = userTargetMbar - restartMarginMbar;
  return true;
}

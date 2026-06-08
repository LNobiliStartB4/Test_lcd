#ifndef THD_PROTOCOL_UTILS_H
#define THD_PROTOCOL_UTILS_H

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
  THD_PROTOCOL_BANK_NONE = 0,
  THD_PROTOCOL_BANK_HEMORFLOW,
  THD_PROTOCOL_BANK_BANDY
} thd_protocol_bank_t;

typedef enum
{
  THD_PROTOCOL_COMMAND_UNKNOWN = 0,
  THD_PROTOCOL_COMMAND_SET0,
  THD_PROTOCOL_COMMAND_SET1,
  THD_PROTOCOL_COMMAND_TOKEN,
  THD_PROTOCOL_COMMAND_START,
  THD_PROTOCOL_COMMAND_STOP,
  THD_PROTOCOL_COMMAND_VLV21,
  THD_PROTOCOL_COMMAND_VLV20,
  THD_PROTOCOL_COMMAND_VLV11,
  THD_PROTOCOL_COMMAND_VLV10
} thd_protocol_command_t;

typedef enum
{
  THD_PROTOCOL_VALVE_ACTION_NONE = 0,
  THD_PROTOCOL_VALVE_ACTION_OUTLET_OPEN,
  THD_PROTOCOL_VALVE_ACTION_OUTLET_CLOSE,
  THD_PROTOCOL_VALVE_ACTION_UNSUPPORTED
} thd_protocol_valve_action_t;

typedef enum
{
  THD_PROTOCOL_ACTIVE_PRODUCT_NONE = 0,
  THD_PROTOCOL_ACTIVE_PRODUCT_BANDY = 1,
  THD_PROTOCOL_ACTIVE_PRODUCT_HEMORFLOW = 2
} thd_protocol_active_product_t;

typedef struct
{
  int32_t userTargetMbar;
  int32_t controlTargetMbar;
  int32_t restartTargetMbar;
} thd_protocol_pressure_profile_t;

bool ThdProtocol_ParseHex4(const char *text, uint16_t *value);
thd_protocol_command_t ThdProtocol_ParseCommand(const char *command, uint16_t *startResponse);
thd_protocol_valve_action_t ThdProtocol_GetValveAction(thd_protocol_command_t command);
bool ThdProtocol_IsStartAllowed(thd_protocol_bank_t selectedBank, bool authAccepted);
thd_protocol_active_product_t ThdProtocol_GetStartProduct(thd_protocol_bank_t selectedBank, bool authAccepted);
thd_protocol_active_product_t ThdProtocol_GetStopProduct(void);
thd_protocol_bank_t ThdProtocol_GetStopBank(void);
int32_t ThdProtocol_GetDefaultPressureTargetMbar(thd_protocol_active_product_t product);
bool ThdProtocol_GetPressureProfile(thd_protocol_active_product_t product, thd_protocol_pressure_profile_t *profile);

#endif

#ifndef ADMINACCESSCONTROLLER_HPP
#define ADMINACCESSCONTROLLER_HPP

#include <stdint.h>

enum AdminAuthResult
{
    AdminAuthSuccess = 0,
    AdminAuthInvalidPin,
    AdminAuthLocked
};

class AdminAccessController
{
public:
    AdminAccessController()
        : authenticated(false),
          failedAttempts(0U),
          lockoutTicks100ms(0U)
    {
    }

    AdminAuthResult authenticate(uint16_t pin)
    {
        if (lockoutTicks100ms != 0U)
        {
            return AdminAuthLocked;
        }

        if (pin == kAdminPin)
        {
            authenticated = true;
            failedAttempts = 0U;
            return AdminAuthSuccess;
        }

        authenticated = false;
        ++failedAttempts;
        if (failedAttempts >= kMaxFailedAttempts)
        {
            failedAttempts = 0U;
            lockoutTicks100ms = kLockoutTicks100ms;
            return AdminAuthLocked;
        }

        return AdminAuthInvalidPin;
    }

    void tick100ms()
    {
        if (lockoutTicks100ms != 0U)
        {
            --lockoutTicks100ms;
        }
    }

    void logout()
    {
        authenticated = false;
    }

    bool isAuthenticated() const
    {
        return authenticated;
    }

    uint8_t getLockoutRemainingSeconds() const
    {
        return static_cast<uint8_t>((lockoutTicks100ms + 9U) / 10U);
    }

private:
    static const uint16_t kAdminPin = 1234U;
    static const uint8_t kMaxFailedAttempts = 3U;
    static const uint16_t kLockoutTicks100ms = 300U;

    bool authenticated;
    uint8_t failedAttempts;
    uint16_t lockoutTicks100ms;
};

#endif // ADMINACCESSCONTROLLER_HPP

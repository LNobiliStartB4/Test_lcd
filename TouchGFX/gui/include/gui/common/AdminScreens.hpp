#ifndef ADMINSCREENS_HPP
#define ADMINSCREENS_HPP

#include <gui/common/FrontendApplication.hpp>
#include <gui/model/DashboardTypes.hpp>
#include <gui/model/ModelListener.hpp>
#include <images/BitmapDatabase.hpp>
#include <stdio.h>
#include <mvp/Presenter.hpp>
#include <mvp/View.hpp>
#include <texts/TextKeysAndLanguages.hpp>
#include <touchgfx/Application.hpp>
#include <touchgfx/Callback.hpp>
#include <touchgfx/Color.hpp>
#include <touchgfx/TypedText.hpp>
#include <touchgfx/Unicode.hpp>
#include <touchgfx/containers/buttons/Buttons.hpp>
#include <touchgfx/widgets/Box.hpp>
#include <touchgfx/widgets/Image.hpp>
#include <touchgfx/widgets/TextArea.hpp>
#include <touchgfx/widgets/TextAreaWithWildcard.hpp>

class AdminPinScreenView;
class Screen14View;
class Screen15View;
class Screen16View;
class Screen17View;

class AdminPinScreenPresenter : public touchgfx::Presenter, public ModelListener
{
public:
    explicit AdminPinScreenPresenter(AdminPinScreenView& v) : view(v) {}
    virtual void activate() {}
    virtual void deactivate() {}
    AdminAuthResult authenticate(uint16_t pin)
    {
        return (model != 0) ? model->authenticateAdminPin(pin) : AdminAuthInvalidPin;
    }
    bool isAuthenticated() const
    {
        return (model != 0) && model->isAdminAuthenticated();
    }
    void logout()
    {
        if (model != 0)
        {
            model->logoutAdmin();
        }
    }
    uint8_t getLockoutSeconds() const
    {
        return (model != 0) ? model->getAdminLockoutRemainingSeconds() : 0U;
    }

private:
    AdminPinScreenView& view;
};

class Screen14Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    explicit Screen14Presenter(Screen14View& v) : view(v) {}
    virtual void activate() {}
    virtual void deactivate() {}
    bool isAuthenticated() const
    {
        return (model != 0) && model->isAdminAuthenticated();
    }
    void logout()
    {
        if (model != 0)
        {
            model->logoutAdmin();
        }
    }

private:
    Screen14View& view;
};

class Screen15Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    explicit Screen15Presenter(Screen15View& v) : view(v) {}
    virtual void activate() {}
    virtual void deactivate() {}
    bool isAuthenticated() const
    {
        return (model != 0) && model->isAdminAuthenticated();
    }
    AdminDiagnosticsSnapshot getDiagnostics() const
    {
        return (model != 0) ? model->getAdminDiagnosticsSnapshot() : AdminDiagnosticsSnapshot();
    }

private:
    Screen15View& view;
};

class Screen16Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    explicit Screen16Presenter(Screen16View& v) : view(v) {}
    virtual void activate() {}
    virtual void deactivate() {}
    bool isAuthenticated() const
    {
        return (model != 0) && model->isAdminAuthenticated();
    }
    AdminDiagnosticsSnapshot getDiagnostics() const
    {
        return (model != 0) ? model->getAdminDiagnosticsSnapshot() : AdminDiagnosticsSnapshot();
    }

private:
    Screen16View& view;
};

class Screen17Presenter : public touchgfx::Presenter, public ModelListener
{
public:
    explicit Screen17Presenter(Screen17View& v) : view(v) {}
    virtual void activate() {}
    virtual void deactivate() {}
    bool isAuthenticated() const
    {
        return (model != 0) && model->isAdminAuthenticated();
    }
    void refreshMemoryDiagnostics()
    {
        if (model != 0)
        {
            model->refreshAdminMemoryDiagnostics();
        }
    }
    AdminDiagnosticsSnapshot getDiagnostics() const
    {
        return (model != 0) ? model->getAdminDiagnosticsSnapshot() : AdminDiagnosticsSnapshot();
    }

private:
    Screen17View& view;
};

namespace adminui
{
static const touchgfx::colortype kBlack = touchgfx::Color::getColorFromRGB(0, 0, 0);
static const touchgfx::colortype kPressed = touchgfx::Color::getColorFromRGB(14, 14, 12);
static const touchgfx::colortype kIvory = touchgfx::Color::getColorFromRGB(245, 242, 232);
static const touchgfx::colortype kGold = touchgfx::Color::getColorFromRGB(216, 178, 71);
static const touchgfx::colortype kDim = touchgfx::Color::getColorFromRGB(124, 137, 148);
static const touchgfx::colortype kBorder = touchgfx::Color::getColorFromRGB(86, 104, 116);
static const touchgfx::colortype kError = touchgfx::Color::getColorFromRGB(225, 92, 92);

typedef touchgfx::BoxWithBorderButtonStyle<touchgfx::ClickButtonTrigger> OutlineButton;

inline void configureButton(OutlineButton& button,
                            int16_t x,
                            int16_t y,
                            int16_t width,
                            int16_t height,
                            touchgfx::colortype borderColor)
{
    button.setBoxWithBorderPosition(0, 0, 0, 0);
    button.setBorderSize(1);
    button.setBoxWithBorderColors(kBlack, kPressed, borderColor, kIvory);
    button.setPosition(x, y, width, height);
}

// State strings are now TouchGFX texts (translated automatically by the active
// language via Texts::setLanguage), instead of per-language C++ literals.
inline touchgfx::TypedTextId yesNoText(bool value)
{
    return value ? T_TEXT_STATEYES : T_TEXT_STATENO;
}

inline touchgfx::TypedTextId availableText(bool value)
{
    return value ? T_TEXT_STATEPRESENT : T_TEXT_STATEABSENT;
}

inline touchgfx::TypedTextId validText(bool value)
{
    return value ? T_TEXT_STATEVALID : T_TEXT_STATEINVALID;
}

inline touchgfx::TypedTextId languageNameText(UiLanguage language)
{
    switch (language)
    {
    case UiLanguageItalian: return T_TEXT_ITALIAN;
    case UiLanguageFrench:  return T_TEXT_FRENCH;
    case UiLanguageGerman:  return T_TEXT_GERMAN;
    case UiLanguageSpanish: return T_TEXT_SPANISH;
    case UiLanguageEnglish:
    default:                return T_TEXT_ENGLISH;
    }
}

template <typename PresenterType>
class AdminViewBase : public touchgfx::View<PresenterType>
{
public:
    explicit AdminViewBase(uint16_t titleId)
        : backCallback(this, &AdminViewBase::backCallbackHandler)
    {
        background.setPosition(0, 0, 480, 320);
        background.setColor(kBlack);
        this->add(background);

        configureButton(backButton, 12, 14, 96, 48, kIvory);
        backButton.setAction(backCallback);
        this->add(backButton);

        backLabel.setPosition(22, 28, 76, 20);
        backLabel.setColor(kIvory);
        backLabel.setTypedText(touchgfx::TypedText(T_TEXT_BACK));
        this->add(backLabel);

        title.setPosition(112, 22, 308, 32);
        title.setColor(kIvory);
        title.setTypedText(touchgfx::TypedText(titleId));
        this->add(title);

        logo.setBitmap(touchgfx::Bitmap(BITMAP_THD_CORNER_LOGO_ID));
        logo.setXY(428, 8);
        this->add(logo);
    }

protected:
    virtual void handleBack() = 0;

    FrontendApplication& application()
    {
        return *static_cast<FrontendApplication*>(touchgfx::Application::getInstance());
    }

private:
    void backCallbackHandler(const touchgfx::AbstractButtonContainer&)
    {
        handleBack();
    }

    touchgfx::Box background;
    OutlineButton backButton;
    touchgfx::TextArea backLabel;
    touchgfx::TextArea title;
    touchgfx::Image logo;
    touchgfx::Callback<AdminViewBase, const touchgfx::AbstractButtonContainer&> backCallback;
};

template <typename PresenterType>
class DiagnosticViewBase : public AdminViewBase<PresenterType>
{
public:
    explicit DiagnosticViewBase(uint16_t titleId)
        : AdminViewBase<PresenterType>(titleId),
          lineCount(0U)
    {
        for (uint8_t i = 0U; i < kMaxLines; ++i)
        {
            valueBuffers[i][0] = 0;
            labels[i].setPosition(36, static_cast<int16_t>(78 + (i * 24)), 220, 19);
            labels[i].setColor(kGold);
            values[i].setPosition(252, static_cast<int16_t>(78 + (i * 24)), 192, 19);
            values[i].setColor(kIvory);
            values[i].setTypedText(touchgfx::TypedText(T_TEXT_ADMINVALUE));
            values[i].setWildcard(valueBuffers[i]);
        }
    }

protected:
    static const uint8_t kMaxLines = 9U;
    static const uint16_t kValueBufferSize = 32U;

    void configureLines(const uint16_t* labelIds, uint8_t count)
    {
        lineCount = (count > kMaxLines) ? kMaxLines : count;
        for (uint8_t i = 0U; i < lineCount; ++i)
        {
            labels[i].setTypedText(touchgfx::TypedText(labelIds[i]));
            this->add(labels[i]);
            this->add(values[i]);
        }
    }

    // Show a translated text (state strings) instead of a dynamic buffer.
    void setTypedValue(uint8_t index, touchgfx::TypedTextId textId)
    {
        if (index >= lineCount)
        {
            return;
        }
        values[index].setTypedText(touchgfx::TypedText(textId));
        values[index].invalidate();
    }

    void setTextValue(uint8_t index, const char* value)
    {
        if ((index >= lineCount) || (value == 0))
        {
            return;
        }
        rebindWildcard(index);
        touchgfx::Unicode::fromUTF8(reinterpret_cast<const uint8_t*>(value),
                                   valueBuffers[index],
                                   kValueBufferSize);
        values[index].invalidate();
    }

    void setIntValue(uint8_t index, int32_t value, const char* suffix)
    {
        if (index >= lineCount)
        {
            return;
        }
        rebindWildcard(index);
        touchgfx::Unicode::snprintf(valueBuffers[index],
                                   kValueBufferSize,
                                   "%ld%s",
                                   static_cast<long>(value),
                                   suffix);
        values[index].invalidate();
    }

    void setUnsignedValue(uint8_t index, uint32_t value, const char* suffix)
    {
        if (index >= lineCount)
        {
            return;
        }
        rebindWildcard(index);
        touchgfx::Unicode::snprintf(valueBuffers[index],
                                   kValueBufferSize,
                                   "%lu%s",
                                   static_cast<unsigned long>(value),
                                   suffix);
        values[index].invalidate();
    }

private:
    // Re-bind a value field to its wildcard buffer (undo a previous setTypedValue
    // so the field shows its dynamic buffer again).
    void rebindWildcard(uint8_t index)
    {
        values[index].setTypedText(touchgfx::TypedText(T_TEXT_ADMINVALUE));
        values[index].setWildcard(valueBuffers[index]);
    }

    uint8_t lineCount;
    touchgfx::TextArea labels[kMaxLines];
    touchgfx::TextAreaWithOneWildcard values[kMaxLines];
    touchgfx::Unicode::UnicodeChar valueBuffers[kMaxLines][kValueBufferSize];
};
}

class AdminPinScreenView : public adminui::AdminViewBase<AdminPinScreenPresenter>
{
public:
    AdminPinScreenView()
        : adminui::AdminViewBase<AdminPinScreenPresenter>(T_TEXT_ADMINPINTITLE),
          digitCallback(this, &AdminPinScreenView::digitCallbackHandler),
          commandCallback(this, &AdminPinScreenView::commandCallbackHandler),
          pinLength(0U),
          invalidPin(false),
          tickDivider(0U)
    {
        subtitle.setPosition(0, 64, 480, 20);
        subtitle.setColor(adminui::kDim);
        subtitle.setTypedText(touchgfx::TypedText(T_TEXT_ADMINPINSUBTITLE));
        add(subtitle);

        pinBuffer[0] = 0;
        pinValue.setPosition(120, 86, 240, 40);
        pinValue.setColor(adminui::kIvory);
        pinValue.setTypedText(touchgfx::TypedText(T_TEXT_ADMINPINVALUE));
        pinValue.setWildcard(pinBuffer);
        add(pinValue);

        lockoutBuffer[0] = 0;
        status.setPosition(100, 124, 280, 18);
        status.setColor(adminui::kDim);
        status.setTypedText(touchgfx::TypedText(T_TEXT_ADMINPINPROMPTSTATUS));
        status.setWildcard(lockoutBuffer);
        add(status);

        static const uint16_t digitTextIds[10] =
        {
            T_TEXT_DIGIT0, T_TEXT_DIGIT1, T_TEXT_DIGIT2, T_TEXT_DIGIT3, T_TEXT_DIGIT4,
            T_TEXT_DIGIT5, T_TEXT_DIGIT6, T_TEXT_DIGIT7, T_TEXT_DIGIT8, T_TEXT_DIGIT9
        };

        for (uint8_t digit = 0U; digit < 10U; ++digit)
        {
            int16_t x;
            int16_t y;
            if (digit == 0U)
            {
                x = 188;
                y = 258;
            }
            else
            {
                const uint8_t position = static_cast<uint8_t>(digit - 1U);
                x = static_cast<int16_t>(72 + ((position % 3U) * 116));
                y = static_cast<int16_t>(146 + ((position / 3U) * 38));
            }

            adminui::configureButton(digitButtons[digit], x, y, 104, 34, adminui::kBorder);
            digitButtons[digit].setAction(digitCallback);
            add(digitButtons[digit]);

            digitLabels[digit].setPosition(x, static_cast<int16_t>(y + 7), 104, 20);
            digitLabels[digit].setColor(adminui::kIvory);
            digitLabels[digit].setTypedText(touchgfx::TypedText(digitTextIds[digit]));
            add(digitLabels[digit]);
        }

        adminui::configureButton(deleteButton, 72, 258, 104, 34, adminui::kBorder);
        deleteButton.setAction(commandCallback);
        add(deleteButton);
        deleteLabel.setPosition(72, 265, 104, 20);
        deleteLabel.setColor(adminui::kIvory);
        deleteLabel.setTypedText(touchgfx::TypedText(T_TEXT_ADMINDELETE));
        add(deleteLabel);

        adminui::configureButton(unlockButton, 304, 258, 104, 34, adminui::kGold);
        unlockButton.setAction(commandCallback);
        add(unlockButton);
        unlockLabel.setPosition(304, 265, 104, 20);
        unlockLabel.setColor(adminui::kIvory);
        unlockLabel.setTypedText(touchgfx::TypedText(T_TEXT_ADMINUNLOCK));
        add(unlockLabel);

        updatePinDisplay();
    }

    virtual void setupScreen()
    {
        if ((presenter != 0) && presenter->isAuthenticated())
        {
            application().gotoAdminMenuScreenNoTransition();
            return;
        }
        updateLockoutState();
    }

    virtual void handleTickEvent()
    {
        if (++tickDivider >= 6U)
        {
            tickDivider = 0U;
            updateLockoutState();
        }
    }

protected:
    virtual void handleBack()
    {
        if (presenter != 0)
        {
            presenter->logout();
        }
        application().gotoSettingsScreenNoTransition();
    }

private:
    void digitCallbackHandler(const touchgfx::AbstractButtonContainer& source)
    {
        if ((presenter == 0) || (presenter->getLockoutSeconds() != 0U) || (pinLength >= 4U))
        {
            return;
        }

        for (uint8_t digit = 0U; digit < 10U; ++digit)
        {
            if (&source == &digitButtons[digit])
            {
                pinDigits[pinLength++] = digit;
                invalidPin = false;
                updatePinDisplay();
                updateLockoutState();
                return;
            }
        }
    }

    void commandCallbackHandler(const touchgfx::AbstractButtonContainer& source)
    {
        if (presenter == 0)
        {
            return;
        }

        if (&source == &deleteButton)
        {
            if ((presenter->getLockoutSeconds() == 0U) && (pinLength != 0U))
            {
                --pinLength;
                invalidPin = false;
                updatePinDisplay();
                updateLockoutState();
            }
            return;
        }

        if (&source != &unlockButton)
        {
            return;
        }

        if ((presenter->getLockoutSeconds() != 0U) || (pinLength != 4U))
        {
            invalidPin = presenter->getLockoutSeconds() == 0U;
            updateLockoutState();
            return;
        }

        const uint16_t pin = static_cast<uint16_t>((pinDigits[0] * 1000U) +
                                                   (pinDigits[1] * 100U) +
                                                   (pinDigits[2] * 10U) +
                                                   pinDigits[3]);
        const AdminAuthResult result = presenter->authenticate(pin);
        pinLength = 0U;
        updatePinDisplay();

        if (result == AdminAuthSuccess)
        {
            application().gotoAdminMenuScreenNoTransition();
            return;
        }

        invalidPin = result == AdminAuthInvalidPin;
        updateLockoutState();
    }

    void updatePinDisplay()
    {
        uint8_t bufferIndex = 0U;
        for (uint8_t i = 0U; i < 4U; ++i)
        {
            pinBuffer[bufferIndex++] = (i < pinLength) ? '*' : '-';
            if (i != 3U)
            {
                pinBuffer[bufferIndex++] = ' ';
            }
        }
        pinBuffer[bufferIndex] = 0;
        pinValue.invalidate();
    }

    void updateLockoutState()
    {
        const uint8_t seconds = (presenter != 0) ? presenter->getLockoutSeconds() : 0U;
        const bool enabled = seconds == 0U;

        for (uint8_t i = 0U; i < 10U; ++i)
        {
            digitButtons[i].setTouchable(enabled);
        }
        deleteButton.setTouchable(enabled);
        unlockButton.setTouchable(enabled);

        if (!enabled)
        {
            touchgfx::Unicode::snprintf(lockoutBuffer, 4U, "%u", seconds);
            status.setTypedText(touchgfx::TypedText(T_TEXT_ADMINPINLOCKEDSTATUS));
            status.setColor(adminui::kError);
        }
        else if (invalidPin)
        {
            status.setTypedText(touchgfx::TypedText(T_TEXT_ADMINPININVALIDSTATUS));
            status.setColor(adminui::kError);
        }
        else
        {
            status.setTypedText(touchgfx::TypedText(T_TEXT_ADMINPINPROMPTSTATUS));
            status.setColor(adminui::kDim);
        }
        status.invalidate();
    }

    touchgfx::TextArea subtitle;
    touchgfx::TextAreaWithOneWildcard pinValue;
    touchgfx::Unicode::UnicodeChar pinBuffer[9];
    touchgfx::TextAreaWithOneWildcard status;
    touchgfx::Unicode::UnicodeChar lockoutBuffer[4];
    adminui::OutlineButton digitButtons[10];
    touchgfx::TextArea digitLabels[10];
    adminui::OutlineButton deleteButton;
    touchgfx::TextArea deleteLabel;
    adminui::OutlineButton unlockButton;
    touchgfx::TextArea unlockLabel;
    touchgfx::Callback<AdminPinScreenView, const touchgfx::AbstractButtonContainer&> digitCallback;
    touchgfx::Callback<AdminPinScreenView, const touchgfx::AbstractButtonContainer&> commandCallback;
    uint8_t pinDigits[4];
    uint8_t pinLength;
    bool invalidPin;
    uint8_t tickDivider;
};

class Screen14View : public adminui::AdminViewBase<Screen14Presenter>
{
public:
    Screen14View()
        : adminui::AdminViewBase<Screen14Presenter>(T_TEXT_ADMINMENUTITLE),
          rowCallback(this, &Screen14View::rowCallbackHandler)
    {
        static const uint8_t kRowCount = 2U;
        subtitle.setPosition(0, 62, 480, 20);
        subtitle.setColor(adminui::kDim);
        subtitle.setTypedText(touchgfx::TypedText(T_TEXT_ADMINMENUSUBTITLE));
        add(subtitle);

        static const uint16_t titleIds[kRowCount] =
        {
            T_TEXT_ADMINSYSTEMITEM,
            T_TEXT_ADMINMEMORYITEM
        };
        static const uint16_t descriptionIds[kRowCount] =
        {
            T_TEXT_ADMINSYSTEMDESCRIPTION,
            T_TEXT_ADMINMEMORYDESCRIPTION
        };

        for (uint8_t i = 0U; i < kRowCount; ++i)
        {
            const int16_t y = static_cast<int16_t>(88 + (i * 72));
            adminui::configureButton(rows[i], 40, y, 400, 62, adminui::kBorder);
            rows[i].setBorderSize(0);
            rows[i].setAction(rowCallback);
            add(rows[i]);

            rowTitles[i].setPosition(56, static_cast<int16_t>(y + 8), 320, 20);
            rowTitles[i].setColor(adminui::kIvory);
            rowTitles[i].setTypedText(touchgfx::TypedText(titleIds[i]));
            add(rowTitles[i]);

            rowDescriptions[i].setPosition(56, static_cast<int16_t>(y + 34), 330, 16);
            rowDescriptions[i].setColor(adminui::kDim);
            rowDescriptions[i].setTypedText(touchgfx::TypedText(descriptionIds[i]));
            add(rowDescriptions[i]);

            chevrons[i].setPosition(404, static_cast<int16_t>(y + 19), 20, 20);
            chevrons[i].setColor(adminui::kDim);
            chevrons[i].setTypedText(touchgfx::TypedText(T_TEXT_ADMINCHEVRON));
            add(chevrons[i]);

            dividers[i].setPosition(56, static_cast<int16_t>(y + 66), 368, 1);
            dividers[i].setColor(adminui::kBorder);
            add(dividers[i]);
        }
    }

    virtual void setupScreen()
    {
        if ((presenter == 0) || !presenter->isAuthenticated())
        {
            application().gotoAdminPinScreenNoTransition();
        }
    }

protected:
    virtual void handleBack()
    {
        if (presenter != 0)
        {
            presenter->logout();
        }
        application().gotoSettingsScreenNoTransition();
    }

private:
    void rowCallbackHandler(const touchgfx::AbstractButtonContainer& source)
    {
        if (&source == &rows[0])
        {
            application().gotoAdminSystemInfoScreenNoTransition();
        }
        else if (&source == &rows[1])
        {
            application().gotoAdminPressureScreenNoTransition();
        }
        else if (&source == &rows[2])
        {
            application().gotoAdminMemoryScreenNoTransition();
        }
    }

    touchgfx::TextArea subtitle;
    adminui::OutlineButton rows[3];
    touchgfx::TextArea rowTitles[3];
    touchgfx::TextArea rowDescriptions[3];
    touchgfx::TextArea chevrons[3];
    touchgfx::Box dividers[3];
    touchgfx::Callback<Screen14View, const touchgfx::AbstractButtonContainer&> rowCallback;
};

class Screen15View : public adminui::DiagnosticViewBase<Screen15Presenter>
{
public:
    Screen15View()
        : adminui::DiagnosticViewBase<Screen15Presenter>(T_TEXT_ADMINSYSTEMTITLE),
          tickDivider(0U)
    {
        static const uint16_t labels[] =
        {
            T_TEXT_FIELDDEVICE,
            T_TEXT_FIELDFIRMWARE,
            T_TEXT_FIELDUPTIME,
            T_TEXT_FIELDLANGUAGE,
            T_TEXT_FIELDBRIGHTNESS
        };
        configureLines(labels, 5U);
    }

    virtual void setupScreen()
    {
        if ((presenter == 0) || !presenter->isAuthenticated())
        {
            application().gotoAdminPinScreenNoTransition();
            return;
        }
        refresh();
    }

    virtual void handleTickEvent()
    {
        if (++tickDivider >= 60U)
        {
            tickDivider = 0U;
            refresh();
        }
    }

protected:
    virtual void handleBack()
    {
        application().gotoAdminMenuScreenNoTransition();
    }

private:
    void refresh()
    {
        const AdminDiagnosticsSnapshot snapshot = presenter->getDiagnostics();
        if (snapshot.deviceName[0] != 0) { setTextValue(0U, snapshot.deviceName); } else { setTypedValue(0U, T_TEXT_STATENA); }
        if (snapshot.firmwareVersion[0] != 0) { setTextValue(1U, snapshot.firmwareVersion); } else { setTypedValue(1U, T_TEXT_STATENA); }

        const uint32_t hours = snapshot.uptimeSeconds / 3600U;
        const uint32_t minutes = (snapshot.uptimeSeconds / 60U) % 60U;
        const uint32_t seconds = snapshot.uptimeSeconds % 60U;
        char uptime[16];
        snprintf(uptime, sizeof(uptime), "%02lu:%02lu:%02lu",
                 static_cast<unsigned long>(hours),
                 static_cast<unsigned long>(minutes),
                 static_cast<unsigned long>(seconds));
        setTextValue(2U, uptime);
        setTypedValue(3U, adminui::languageNameText(snapshot.language));
        setUnsignedValue(4U, snapshot.brightnessPercent, " %");
    }

    uint8_t tickDivider;
};

class Screen16View : public adminui::DiagnosticViewBase<Screen16Presenter>
{
public:
    Screen16View()
        : adminui::DiagnosticViewBase<Screen16Presenter>(T_TEXT_ADMINPRESSURETITLE),
          tickDivider(0U)
    {
        static const uint16_t labels[] =
        {
            T_TEXT_FIELDSENSOR,
            T_TEXT_FIELDRELATIVEPRESSURE,
            T_TEXT_FIELDRAWPRESSURE,
            T_TEXT_FIELDZEROOFFSET,
            T_TEXT_FIELDAMBIENTADC,
            T_TEXT_FIELDCHAMBERADC,
            T_TEXT_FIELDTARGET,
            T_TEXT_FIELDDUTY,
            T_TEXT_FIELDFAULT
        };
        configureLines(labels, 9U);
    }

    virtual void setupScreen()
    {
        if ((presenter == 0) || !presenter->isAuthenticated())
        {
            application().gotoAdminPinScreenNoTransition();
            return;
        }
        refresh();
    }

    virtual void handleTickEvent()
    {
        if (++tickDivider >= 12U)
        {
            tickDivider = 0U;
            refresh();
        }
    }

protected:
    virtual void handleBack()
    {
        application().gotoAdminMenuScreenNoTransition();
    }

private:
    void refresh()
    {
        const AdminDiagnosticsSnapshot snapshot = presenter->getDiagnostics();

        if (snapshot.pressureAvailable)
        {
            setTypedValue(0U, adminui::validText(snapshot.pressureValid));
            setIntValue(1U, snapshot.relativePressureMbar, " mbar");
            setIntValue(6U, snapshot.targetMbar, " mbar");
            setUnsignedValue(7U, snapshot.pumpDutyPercent, " %");
            setUnsignedValue(8U, snapshot.pressureFault, "");
        }
        else
        {
            setTypedValue(0U, T_TEXT_STATENA);
            setTypedValue(1U, T_TEXT_STATENA);
            setTypedValue(6U, T_TEXT_STATENA);
            setTypedValue(7U, T_TEXT_STATENA);
            setTypedValue(8U, T_TEXT_STATENA);
        }

        if (snapshot.pressureDetailsAvailable)
        {
            setIntValue(2U, snapshot.rawRelativePressureMbar, " mbar");
            setIntValue(3U, snapshot.zeroOffsetMbar, " mbar");
            setUnsignedValue(4U, snapshot.ambientRawAdc, "");
            setUnsignedValue(5U, snapshot.chamberRawAdc, "");
        }
        else
        {
            setTypedValue(2U, T_TEXT_STATENA);
            setTypedValue(3U, T_TEXT_STATENA);
            setTypedValue(4U, T_TEXT_STATENA);
            setTypedValue(5U, T_TEXT_STATENA);
        }
    }

    uint8_t tickDivider;
};

class Screen17View : public adminui::DiagnosticViewBase<Screen17Presenter>
{
public:
    Screen17View()
        : adminui::DiagnosticViewBase<Screen17Presenter>(T_TEXT_ADMINMEMORYTITLE)
    {
        static const uint16_t labels[] =
        {
            T_TEXT_FIELDFRAM,
            T_TEXT_FIELDFRAMID,
            T_TEXT_FIELDFRAMSIZE,
            T_TEXT_FIELDSESSION,
            T_TEXT_FIELDWINBOND,
            T_TEXT_FIELDWINBONDID,
            T_TEXT_FIELDWINBONDSIZE,
            T_TEXT_FIELDASSETPACKAGE
        };
        configureLines(labels, 8U);
    }

    virtual void setupScreen()
    {
        if ((presenter == 0) || !presenter->isAuthenticated())
        {
            application().gotoAdminPinScreenNoTransition();
            return;
        }

        presenter->refreshMemoryDiagnostics();
        refresh();
    }

protected:
    virtual void handleBack()
    {
        application().gotoAdminMenuScreenNoTransition();
    }

private:
    void refresh()
    {
        const AdminDiagnosticsSnapshot snapshot = presenter->getDiagnostics();

        if (snapshot.framAvailable) { setTypedValue(0U, adminui::availableText(snapshot.framPresent)); }
        else { setTypedValue(0U, T_TEXT_STATENA); }

        if (snapshot.framAvailable && snapshot.framId[0] != 0) { setTextValue(1U, snapshot.framId); }
        else { setTypedValue(1U, T_TEXT_STATENA); }

        if (snapshot.framAvailable)
        {
            setUnsignedValue(2U, snapshot.framSizeBytes / 1024U, " KB");
            setTypedValue(3U, adminui::yesNoText(snapshot.sessionRecordValid));
        }
        else
        {
            setTypedValue(2U, T_TEXT_STATENA);
            setTypedValue(3U, T_TEXT_STATENA);
        }

        if (snapshot.winbondAvailable) { setTypedValue(4U, adminui::availableText(snapshot.winbondPresent)); }
        else { setTypedValue(4U, T_TEXT_STATENA); }

        if (snapshot.winbondAvailable && snapshot.winbondId[0] != 0) { setTextValue(5U, snapshot.winbondId); }
        else { setTypedValue(5U, T_TEXT_STATENA); }

        if (snapshot.winbondAvailable)
        {
            setUnsignedValue(6U, snapshot.winbondSizeBytes / (1024U * 1024U), " MB");
            setTypedValue(7U, adminui::validText(snapshot.assetPackageValid));
        }
        else
        {
            setTypedValue(6U, T_TEXT_STATENA);
            setTypedValue(7U, T_TEXT_STATENA);
        }
    }
};

#endif // ADMINSCREENS_HPP

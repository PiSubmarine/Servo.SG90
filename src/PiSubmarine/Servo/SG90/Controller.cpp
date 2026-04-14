#include "PiSubmarine/Servo/SG90/Controller.h"

#include <chrono>
#include <cmath>

namespace PiSubmarine::Servo::SG90
{
    namespace
    {
        using PwmError = PiSubmarine::PWM::Api::IDriver::Error;

        constexpr auto AllowedTargetAngleSector = AngularSector(Radians(0.0), Degrees(180.0).ToRadians());
        constexpr auto PwmFrequency = Hertz(50.0);
        constexpr auto MinimumPulseWidth = std::chrono::microseconds(500);
        constexpr auto MaximumPulseWidth = std::chrono::microseconds(2400);

        [[nodiscard]] constexpr bool IsAllowedTargetAngle(const Radians angle) noexcept
        {
            return angle.Value >= AllowedTargetAngleSector.Start.Value
                && angle.Value <= AllowedTargetAngleSector.Start.Value + AllowedTargetAngleSector.Sweep.Value;
        }

        [[nodiscard]] NormalizedFraction ConvertAngleToDutyCycle(const Radians angle)
        {
            const auto angleFraction =
                (angle.Value - AllowedTargetAngleSector.Start.Value) / AllowedTargetAngleSector.Sweep.Value;

            constexpr auto minimumPulseWidthSeconds = std::chrono::duration<double>(MinimumPulseWidth).count();
            constexpr auto maximumPulseWidthSeconds = std::chrono::duration<double>(MaximumPulseWidth).count();
            const auto pulseWidthSeconds = std::lerp(minimumPulseWidthSeconds, maximumPulseWidthSeconds, angleFraction);
            const auto dutyCycle = pulseWidthSeconds * PwmFrequency.Value;

            return {dutyCycle};
        }

        [[nodiscard]] std::expected<void, Servo::Error> ConvertPwmResult(const PwmError error)
        {
            switch (error)
            {
            case PwmError::Ok:
            case PwmError::Disabled:
                return {};

            case PwmError::Busy:
                return std::unexpected(Servo::Error::Timeout);

            case PwmError::InvalidArgument:
            case PwmError::UnsupportedParameter:
                return std::unexpected(Servo::Error::HardwareUnavailable);

            case PwmError::ProtocolError:
            case PwmError::IoFailure:
            case PwmError::UnknownError:
                return std::unexpected(Servo::Error::CommunicationFailure);
            }

            return std::unexpected(Servo::Error::CommunicationFailure);
        }
    }

    Controller::Controller(PiSubmarine::PWM::Api::IDriver& pwmDriver)
        : m_PwmDriver(pwmDriver)
    {
    }

    std::expected<void, Error> Controller::SetTargetAngle(const Radians angle)
    {
        if (!IsAllowedTargetAngle(angle))
        {
            return std::unexpected(Error::TargetAngleOutOfRange);
        }

        m_TargetAngle = angle;
        return ApplyTargetSignal();
    }

    Radians Controller::GetTargetAngle() const
    {
        return m_TargetAngle;
    }

    AngularSector Controller::GetAllowedTargetAngleSector() const
    {
        return AllowedTargetAngleSector;
    }

    std::expected<void, Error> Controller::SetEnabled(const bool isEnabled)
    {
        if (isEnabled)
        {
            const auto applyTargetSignalResult = ApplyTargetSignal();
            if (!applyTargetSignalResult.has_value())
            {
                return applyTargetSignalResult;
            }
        }

        return ConvertPwmResult(m_PwmDriver.SetEnabled(isEnabled));
    }

    bool Controller::IsEnabled() const
    {
        return m_PwmDriver.IsEnabled();
    }

    std::expected<void, Error> Controller::ApplyTargetSignal() const
    {
        return ConvertPwmResult(m_PwmDriver.SetFrequencyAndDuty(PwmFrequency, ConvertAngleToDutyCycle(m_TargetAngle)));
    }
}

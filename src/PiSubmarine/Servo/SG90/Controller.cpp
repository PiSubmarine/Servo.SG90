#include "PiSubmarine/Servo/SG90/Controller.h"

#include <chrono>
#include <cmath>

#include "PiSubmarine/Error/Api/MakeError.h"

namespace PiSubmarine::Servo::SG90
{
    namespace
    {
        using ErrorCondition = PiSubmarine::Error::Api::ErrorCondition;

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

        [[nodiscard]] PiSubmarine::Error::Api::Result<void> ConvertPwmResult(
            PiSubmarine::Error::Api::Result<void> pwmResult)
        {
            if (pwmResult.has_value())
            {
                return {};
            }

            const auto& error = pwmResult.error();
            switch (error.Condition)
            {
            case ErrorCondition::ContractError:
                return std::unexpected(PiSubmarine::Error::Api::MakeError(ErrorCondition::DeviceError, error.Cause));

            case ErrorCondition::CommunicationError:
            case ErrorCondition::DeviceError:
            case ErrorCondition::UnknownError:
                return std::unexpected(error);
            }

            return std::unexpected(PiSubmarine::Error::Api::MakeError(ErrorCondition::UnknownError));
        }
    }

    Controller::Controller(PiSubmarine::PWM::Api::IDriver& pwmDriver)
        : m_PwmDriver(pwmDriver)
    {
    }

    PiSubmarine::Error::Api::Result<void> Controller::SetTargetAngle(const Radians angle)
    {
        if (!IsAllowedTargetAngle(angle))
        {
            return std::unexpected(PiSubmarine::Error::Api::MakeError(ErrorCondition::ContractError));
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

    PiSubmarine::Error::Api::Result<void> Controller::SetEnabled(const bool isEnabled)
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

    PiSubmarine::Error::Api::Result<bool> Controller::IsEnabled() const
    {
        return m_PwmDriver.IsEnabled();
    }

    PiSubmarine::Error::Api::Result<void> Controller::ApplyTargetSignal() const
    {
        return ConvertPwmResult(m_PwmDriver.SetFrequencyAndDuty(PwmFrequency, ConvertAngleToDutyCycle(m_TargetAngle)));
    }
}

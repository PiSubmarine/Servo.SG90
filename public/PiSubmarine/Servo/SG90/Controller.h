#pragma once

#include "PiSubmarine/Error/Api/Result.h"
#include "PiSubmarine/PWM/Api/IDriver.h"
#include "PiSubmarine/Servo/IController.h"

namespace PiSubmarine::Servo::SG90
{
    class Controller : public Servo::IController
    {
    public:
        explicit Controller(PiSubmarine::PWM::Api::IDriver& pwmDriver);

        [[nodiscard]] PiSubmarine::Error::Api::Result<void> SetTargetAngle(Radians angle) override;
        [[nodiscard]] Radians GetTargetAngle() const override;
        [[nodiscard]] AngularSector GetAllowedTargetAngleSector() const override;
        [[nodiscard]] PiSubmarine::Error::Api::Result<void> SetEnabled(bool isEnabled) override;
        [[nodiscard]] PiSubmarine::Error::Api::Result<bool> IsEnabled() const override;

    private:
        PiSubmarine::PWM::Api::IDriver& m_PwmDriver;
        Radians m_TargetAngle = Degrees(90.0).ToRadians();

        [[nodiscard]] PiSubmarine::Error::Api::Result<void> ApplyTargetSignal() const;
    };
}

#pragma once

#include "PiSubmarine/PWM/Api/IDriver.h"
#include "PiSubmarine/Servo/IController.h"

namespace PiSubmarine::Servo::SG90
{
    class Controller : public Servo::IController
    {
    public:
        explicit Controller(PiSubmarine::PWM::Api::IDriver& pwmDriver);

        [[nodiscard]] std::expected<void, Error> SetTargetAngle(Radians angle) override;
        [[nodiscard]] Radians GetTargetAngle() const override;
        [[nodiscard]] AngularSector GetAllowedTargetAngleSector() const override;
        [[nodiscard]] std::expected<void, Error> SetEnabled(bool isEnabled) override;
        [[nodiscard]] bool IsEnabled() const override;

    private:
        PiSubmarine::PWM::Api::IDriver& m_PwmDriver;
        Radians m_TargetAngle = Degrees(90.0).ToRadians();

        [[nodiscard]] std::expected<void, Error> ApplyTargetSignal() const;
    };
}

#include <cmath>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "PiSubmarine/Error/Api/MakeError.h"
#include "PiSubmarine/PWM/Api/IDriverMock.h"
#include "PiSubmarine/Servo/SG90/Controller.h"

namespace PiSubmarine::Servo::SG90
{
    namespace
    {
        using ErrorCondition = PiSubmarine::Error::Api::ErrorCondition;

        [[nodiscard]] bool IsClose(const double actual, const double expected)
        {
            return std::abs(actual - expected) < 1e-9;
        }
    }

    TEST(ControllerTest, ReturnsSg90AllowedAngleSector)
    {
        testing::StrictMock<PiSubmarine::PWM::Api::IDriverMock> pwmDriver;
        Controller controller(pwmDriver);

        const auto sector = controller.GetAllowedTargetAngleSector();

        EXPECT_EQ(sector.Start, Radians(0.0));
        EXPECT_EQ(sector.Sweep, Degrees(180.0).ToRadians());
    }

    TEST(ControllerTest, RejectsAngleBelowAllowedSector)
    {
        testing::StrictMock<PiSubmarine::PWM::Api::IDriverMock> pwmDriver;
        Controller controller(pwmDriver);

        const auto result = controller.SetTargetAngle(Radians(-0.001));

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().Condition, ErrorCondition::ContractError);
    }

    TEST(ControllerTest, RejectsAngleAboveAllowedSectorWithoutWraparound)
    {
        testing::StrictMock<PiSubmarine::PWM::Api::IDriverMock> pwmDriver;
        Controller controller(pwmDriver);

        const auto result = controller.SetTargetAngle(Degrees(181.0).ToRadians());

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().Condition, ErrorCondition::ContractError);
    }

    TEST(ControllerTest, MapsMinimumAngleToSg90MinimumDutyCycle)
    {
        testing::StrictMock<PiSubmarine::PWM::Api::IDriverMock> pwmDriver;
        Controller controller(pwmDriver);

        EXPECT_CALL(
            pwmDriver,
            SetFrequencyAndDuty(
                testing::Truly([](const Hertz frequency) { return IsClose(frequency.Value, 50.0); }),
                testing::Truly([](const NormalizedFraction dutyCycle) { return IsClose(static_cast<double>(dutyCycle), 0.025); })))
            .WillOnce(testing::Return(PiSubmarine::Error::Api::Result<void>{}));

        const auto result = controller.SetTargetAngle(Radians(0.0));

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(controller.GetTargetAngle(), Radians(0.0));
    }

    TEST(ControllerTest, TreatsDisabledPwmDriverAsSuccessfulSignalStaging)
    {
        testing::StrictMock<PiSubmarine::PWM::Api::IDriverMock> pwmDriver;
        Controller controller(pwmDriver);

        EXPECT_CALL(pwmDriver, SetFrequencyAndDuty(testing::_, testing::_))
            .WillOnce(testing::Return(PiSubmarine::Error::Api::Result<void>{}));

        const auto result = controller.SetTargetAngle(Degrees(45.0).ToRadians());

        ASSERT_TRUE(result.has_value());
        EXPECT_EQ(controller.GetTargetAngle(), Degrees(45.0).ToRadians());
    }

    TEST(ControllerTest, EnablingStagesCurrentSignalBeforeEnablingPwmOutput)
    {
        testing::StrictMock<PiSubmarine::PWM::Api::IDriverMock> pwmDriver;
        Controller controller(pwmDriver);

        {
            testing::InSequence sequence;

            EXPECT_CALL(pwmDriver, SetFrequencyAndDuty(testing::_, testing::_))
                .WillOnce(testing::Return(PiSubmarine::Error::Api::Result<void>{}));
            EXPECT_CALL(pwmDriver, SetEnabled(true))
                .WillOnce(testing::Return(PiSubmarine::Error::Api::Result<void>{}));
        }

        const auto result = controller.SetEnabled(true);

        ASSERT_TRUE(result.has_value());
    }

    TEST(ControllerTest, MapsPwmContractFailureToDeviceError)
    {
        testing::StrictMock<PiSubmarine::PWM::Api::IDriverMock> pwmDriver;
        Controller controller(pwmDriver);

        {
            testing::InSequence sequence;

            EXPECT_CALL(pwmDriver, SetFrequencyAndDuty(testing::_, testing::_))
                .WillOnce(testing::Return(std::unexpected(
                    PiSubmarine::Error::Api::MakeError(ErrorCondition::ContractError))));
        }

        const auto result = controller.SetEnabled(true);

        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(result.error().Condition, ErrorCondition::DeviceError);
    }

    TEST(ControllerTest, DelegatesEnabledStateToPwmDriver)
    {
        testing::StrictMock<PiSubmarine::PWM::Api::IDriverMock> pwmDriver;
        Controller controller(pwmDriver);

        EXPECT_CALL(pwmDriver, IsEnabled())
            .WillOnce(testing::Return(PiSubmarine::Error::Api::Result<bool>(true)));

        const auto result = controller.IsEnabled();
        ASSERT_TRUE(result.has_value());
        EXPECT_TRUE(result.value());
    }
}

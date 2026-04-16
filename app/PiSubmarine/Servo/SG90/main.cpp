#include <cstdlib>
#include <exception>
#include <filesystem>
#include <memory>
#include <sstream>
#include <string>

#include <boost/program_options.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include "PiSubmarine/Error/Api/Error.h"
#include "PiSubmarine/PWM/Linux/Driver.h"
#include "PiSubmarine/Servo/SG90/Controller.h"

namespace PiSubmarine::Servo::SG90
{
    namespace
    {
        [[nodiscard]] std::shared_ptr<spdlog::logger> CreateLogger()
        {
            auto sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
            auto logger = std::make_shared<spdlog::logger>("PiSubmarine.Servo.SG90.App", std::move(sink));
            logger->set_pattern("[%n] %^[%l]%$ %v");
            return logger;
        }

        [[nodiscard]] std::string ToString(const PiSubmarine::Error::Api::Error& error)
        {
            std::string message;
            switch (error.Condition)
            {
            case PiSubmarine::Error::Api::ErrorCondition::ContractError:
                message = "Requested servo operation violates the API contract.";
                break;

            case PiSubmarine::Error::Api::ErrorCondition::CommunicationError:
                message = "Failed to communicate with the PWM controller.";
                break;

            case PiSubmarine::Error::Api::ErrorCondition::DeviceError:
                message = "PWM hardware rejected or could not apply the SG90 signal.";
                break;

            case PiSubmarine::Error::Api::ErrorCondition::UnknownError:
                message = "Unknown servo error.";
                break;
            }

            if (error.HasCause())
            {
                message += " Cause: " + error.Cause.message();
            }

            return message;
        }
    }
}

int main(const int argc, const char* const argv[])
{
    namespace po = boost::program_options;
    auto logger = PiSubmarine::Servo::SG90::CreateLogger();

    try
    {
        std::filesystem::path pwmChannelPath = "/sys/class/pwm/pwmchip0/pwm0";
        double angleDegrees = 0.0;
        bool disable = false;

        po::options_description options("Options");
        options.add_options()
            ("help,h", "Show help")
            ("pwm-channel,p", po::value<std::filesystem::path>(&pwmChannelPath)->default_value(pwmChannelPath),
             "Linux PWM sysfs channel path, e.g. /sys/class/pwm/pwmchip0/pwm0")
            ("angle,a", po::value<double>(&angleDegrees), "Set servo angle in degrees within [0, 180]")
            ("disable,d", po::bool_switch(&disable), "Disable the PWM output for the servo");

        po::variables_map variables;
        po::store(po::parse_command_line(argc, argv, options), variables);
        po::notify(variables);

        if (variables.contains("help"))
        {
            std::ostringstream output;
            output << "Control a single SG90 servo through Linux PWM.\n\n" << options;
            logger->info("{}", output.str());
            return EXIT_SUCCESS;
        }

        const bool hasAngle = variables.contains("angle");
        if (hasAngle == disable)
        {
            std::ostringstream output;
            output << "Specify exactly one action: either --angle or --disable.\n\n" << options;
            logger->error("{}", output.str());
            return EXIT_FAILURE;
        }

        PiSubmarine::PWM::Linux::Driver pwmDriver(pwmChannelPath, std::chrono::milliseconds(10), 100);
        PiSubmarine::Servo::SG90::Controller controller(pwmDriver);

        if (disable)
        {
            const auto result = controller.SetEnabled(false);
            if (!result.has_value())
            {
                logger->error("{}", PiSubmarine::Servo::SG90::ToString(result.error()));
                return EXIT_FAILURE;
            }

            logger->info("SG90 output disabled.");
            return EXIT_SUCCESS;
        }

        const auto targetAngle = PiSubmarine::Degrees(angleDegrees).ToRadians();
        const auto setTargetResult = controller.SetTargetAngle(targetAngle);
        if (!setTargetResult.has_value())
        {
            logger->error("{}", PiSubmarine::Servo::SG90::ToString(setTargetResult.error()));
            return EXIT_FAILURE;
        }

        const auto enableResult = controller.SetEnabled(true);
        if (!enableResult.has_value())
        {
            logger->error("{}", PiSubmarine::Servo::SG90::ToString(enableResult.error()));
            return EXIT_FAILURE;
        }

        logger->info("SG90 target angle set to {} degrees.", angleDegrees);
        return EXIT_SUCCESS;
    }
    catch (const std::exception& exception)
    {
        logger->error("{}", exception.what());
        return EXIT_FAILURE;
    }
}

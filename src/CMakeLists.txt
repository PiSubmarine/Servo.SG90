include("${PISUBMARINE_BUILD_SYSTEM}")

message("Adding library: ${PISUBMARINE_MODULE_NAME}")
add_library(${PISUBMARINE_MODULE_NAME}
        PiSubmarine/Servo/SG90/Controller.cpp)
target_include_directories(${PISUBMARINE_MODULE_NAME} PUBLIC "../public")
PiSubmarineInitTarget(${PISUBMARINE_MODULE_NAME})

PiSubmarineAddDependency("https://github.com/PiSubmarine/Servo.Api.git" "")
PiSubmarineAddDependency("https://github.com/PiSubmarine/PWM.Api.git" "")
target_link_libraries(${PISUBMARINE_MODULE_NAME} PUBLIC PiSubmarine.Servo.Api PiSubmarine.PWM.Api)

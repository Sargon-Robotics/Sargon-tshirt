// Copyright (c) FIRST and other WPILib contributors.
// Open Source Software; you can modify and/or share it under the terms of
// the WPILib BSD license file in the root directory of this project.

#include <utility>

#include <frc/Joystick.h>
#include <frc/XboxController.h>
#include <frc/DoubleSolenoid.h>
#include <frc/Solenoid.h>
#include <frc/TimedRobot.h>
#include <frc/drive/DifferentialDrive.h>
#include <frc/motorcontrol/PWMSparkMax.h>
#include <frc/motorcontrol/Spark.h>
#include <frc/AnalogPotentiometer.h>
#include <frc/DriverStation.h>
#include <wpi/DataLog.h>
#include <frc/DataLogManager.h>
#include <string>
#include <frc/ADXRS450_Gyro.h>
#include <frc/motorcontrol/VictorSP.h>
#include <frc/motorcontrol/MotorControllerGroup.h>
#include <ctre/Phoenix.h>
#include <ctre/phoenix/motorcontrol/can/VictorSPX.h>
#include <frc/PowerDistribution.h>
#include <frc/smartdashboard/SmartDashboard.h>
#include <frc/shuffleboard/Shuffleboard.h>

#include "Distance.h"
#include "LedStrip.h"

//#define CONTROL_2FLIGHTSTICK
#define CONTROL_1XBOX
//#define CONTROL_1WHEEL

int64_t nowMs() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

struct Controls
{
#ifdef CONTROL_2FLIGHTSTICK
  frc::Joystick leftStick{0};
  frc::Joystick rightStick{1};

  template <typename DriveT>
  void drive(DriveT &motors)
  {
    motors.TankDrive(leftStick.GetY(), rightStick.GetY());
  }
#elif defined(CONTROL_1XBOX)
  frc::XboxController gamepad1{0};

  template <typename DriveT>
  void drive(DriveT &motors, bool speed)
  {
    motors.ArcadeDrive(-gamepad1.GetLeftY() * (speed ? 1.0 : 0.4), gamepad1.GetLeftX() * (speed ? 1.0 : 0.5));
  }
#elif defined(CONTROL_1WHEEL)
  frc::Joystick wheel{0};

  template <typename DriveT>
  void drive(DriveT &motors)
  {
    motors.ArcadeDrive(wheel.GetRawAxis(1), wheel.GetRawAxis(0));
  }

  void pressed(int button)
  {
    wheel.GetRawButtonPressed(button);
  }
#endif
};

/**
 * This is a demo program showing the use of the DifferentialDrive class.
 * Runs the motors with tank steering.
 */
class Robot : public frc::TimedRobot
{
  // frc::PWMSparkMax leftMotor{0};
  // frc::PWMSparkMax rightMotor{1};
  ctre::phoenix::motorcontrol::can::WPI_VictorSPX leftFront{2};
  ctre::phoenix::motorcontrol::can::WPI_VictorSPX leftBack{3};
  ctre::phoenix::motorcontrol::can::WPI_VictorSPX rightFront{1};
  ctre::phoenix::motorcontrol::can::WPI_VictorSPX rightBack{4};

  frc::DifferentialDrive* robotDrive;

  frc::ADXRS450_Gyro gyro;
  double gyroPk = 1.0;

  bool useTank = false;
  bool fast = false;

  frc::PWMSparkMax extender{0};
  frc::AnalogPotentiometer extenderPot{0};

  frc::Solenoid shooter{frc::PneumaticsModuleType::CTREPCM, 0};
  bool shoot = false;

  Controls controls;

  LedStrip leds{1, 2, 3, 0, defaultLedPatterns[0]};

  int64_t prevUpdate = 0;

public:
  void RobotInit() override
  {
    // We need to invert one side of the drivetrain so that positive voltages
    // result in both sides moving forward. Depending on how your robot's
    // gearbox is constructed, you might have to invert the left side instead.
    rightFront.SetInverted(true);
    rightBack.Follow(rightFront);
    rightBack.SetInverted(InvertType::FollowMaster);
    leftBack.Follow(leftFront);
    leftBack.SetInverted(InvertType::FollowMaster);

    rightFront.ConfigNeutralDeadband(0.01);
    leftFront.ConfigNeutralDeadband(0.01);

    rightFront.ConfigOpenloopRamp(0.5);
    leftFront.ConfigOpenloopRamp(0.5);

    robotDrive = new frc::DifferentialDrive(leftFront, rightFront);
    robotDrive->SetDeadband(0.0);

    gyro.Calibrate();

    frc::DataLogManager::Start();

    Distance::init();

    prevUpdate = nowMs();
  }

  void TeleopInit() override {
    leds.power.Set(frc::Relay::Value::kOn);
  }

  void TeleopExit() override {
    leds.power.Set(frc::Relay::Value::kOff);
  }

  void TeleopPeriodic() override
  {
    int64_t now = nowMs();
    int64_t delta = now - prevUpdate;

    controls.drive(*robotDrive, fast);
    robotDrive->Feed();
    robotDrive->Check();
    Distance::tick();
    
    /*char logBuf[100] = {};
    size_t written = snprintf(logBuf, 100, "Distance: %d", Distance::reading);
    frc::DataLogManager::Log(std::string_view(logBuf, written));*/
    frc::SmartDashboard::PutNumber("Ultrasonic Sensor", Distance::reading);
    

    if (controls.gamepad1.GetAButtonPressed())
    {
      fast = !fast;
    }
    if (controls.gamepad1.GetBButtonPressed())
    {
      useTank = !useTank;
    }
    // robotDrive.ArcadeDriveIK

    extender.Set(-controls.gamepad1.GetRightY());

    if (controls.gamepad1.GetXButtonPressed() && Distance::safeToShoot())
    {
      shooter.Set(true);
    }
    else
    {
      shooter.Set(false);
    }

    


    /*if (controls.gamepad1.GetLeftBumperPressed() || controls.gamepad1.GetRightBumperPressed())
    {
      static std::string out;
      out.clear();
      out += "Pot: ";
      //extenderPot.InitSendable();
      out += extenderPot.Get();
      frc::DataLogManager::Log(out);

    }*/


    leds.intensityMultiplier = frc::SmartDashboard::GetNumber("intensityMultiplier", leds.intensityMultiplier);
    defaultColor.r = frc::SmartDashboard::GetNumber("defaultColorRed", defaultColor.r);
    defaultColor.g = frc::SmartDashboard::GetNumber("defaultColorGreen", defaultColor.g);
    defaultColor.b = frc::SmartDashboard::GetNumber("defaultColorBlue", defaultColor.b);

    if (controls.gamepad1.GetYButtonPressed()) {
      currentLedPattern = currentLedPattern + 1 >= (sizeof(defaultLedPatterns) / sizeof(ColorUpdater)) ? 0 : currentLedPattern + 1;
      leds.setUpdater(defaultLedPatterns[currentLedPattern]);
    }

    leds.tick(now, delta);

    prevUpdate = now;
  }
};

#ifndef RUNNING_FRC_TESTS
int main()
{
  return frc::StartRobot<Robot>();
}
#endif

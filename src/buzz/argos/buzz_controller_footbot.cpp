#include "buzz_controller_footbot.h"
#include <argos3/core/utility/logging/argos_log.h>

/****************************************/
/****************************************/

CBuzzControllerFootBot::SWheelTurningParams::SWheelTurningParams() :
   TurningMechanism(NO_TURN),
   HardTurnOnAngleThreshold(ToRadians(CDegrees(90.0))),
   SoftTurnOnAngleThreshold(ToRadians(CDegrees(70.0))),
   NoTurnAngleThreshold(ToRadians(CDegrees(10.0))),
   MaxSpeed(10.0)
{
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::SWheelTurningParams::Init(TConfigurationNode& t_node) {
   try {
      TurningMechanism = NO_TURN;
      CDegrees cAngle;
      GetNodeAttribute(t_node, "hard_turn_angle_threshold", cAngle);
      HardTurnOnAngleThreshold = ToRadians(cAngle);
      GetNodeAttribute(t_node, "soft_turn_angle_threshold", cAngle);
      SoftTurnOnAngleThreshold = ToRadians(cAngle);
      GetNodeAttribute(t_node, "no_turn_angle_threshold", cAngle);
      NoTurnAngleThreshold = ToRadians(cAngle);
      GetNodeAttribute(t_node, "max_speed", MaxSpeed);
   }
   catch(CARGoSException& ex) {
      THROW_ARGOSEXCEPTION_NESTED("Error initializing controller wheel turning parameters.", ex);
   }
}

/****************************************/
/****************************************/

static int BuzzGoToC(buzzvm_t vm) {
   /* Push the vector components */
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   /* Create a new vector with that */
   buzzobj_t tX = buzzvm_stack_at(vm, 2);
   buzzobj_t tY = buzzvm_stack_at(vm, 1);
   CVector2 cDir;
   if(tX->o.type == BUZZTYPE_INT) cDir.SetX(tX->i.value);
   else if(tX->o.type == BUZZTYPE_FLOAT) cDir.SetX(tX->f.value);
   else {
      buzzvm_seterror(vm,
                      BUZZVM_ERROR_TYPE,
                      "gotoc(x,y): expected %s, got %s in first argument",
                      buzztype_desc[BUZZTYPE_FLOAT],
                      buzztype_desc[tX->o.type]
         );
      return vm->state;
   }      
   if(tY->o.type == BUZZTYPE_INT) cDir.SetY(tY->i.value);
   else if(tY->o.type == BUZZTYPE_FLOAT) cDir.SetY(tY->f.value);
   else {
      buzzvm_seterror(vm,
                      BUZZVM_ERROR_TYPE,
                      "gotoc(x,y): expected %s, got %s in second argument",
                      buzztype_desc[BUZZTYPE_FLOAT],
                      buzztype_desc[tY->o.type]
         );
      return vm->state;
   }
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(buzzvm_stack_at(vm, 1)->u.value)->SetWheelSpeedsFromVector(cDir);
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

static int BuzzGoToP(buzzvm_t vm) {
   /* Push the vector components */
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   /* Create a new vector with that */
   buzzobj_t tLinSpeed = buzzvm_stack_at(vm, 2);
   buzzobj_t tAngSpeed = buzzvm_stack_at(vm, 1);
   Real fLinSpeed = 0.0, fAngSpeed = 0.0;
   if(tLinSpeed->o.type == BUZZTYPE_INT) fLinSpeed = tLinSpeed->i.value;
   else if(tLinSpeed->o.type == BUZZTYPE_FLOAT) fLinSpeed = tLinSpeed->f.value;
   else {
      buzzvm_seterror(vm,
                      BUZZVM_ERROR_TYPE,
                      "gotop(linspeed,angspeed): expected %s, got %s in first argument",
                      buzztype_desc[BUZZTYPE_FLOAT],
                      buzztype_desc[tLinSpeed->o.type]
         );
      return vm->state;
   }      
   if(tAngSpeed->o.type == BUZZTYPE_INT) fAngSpeed = tAngSpeed->i.value;
   else if(tAngSpeed->o.type == BUZZTYPE_FLOAT) fAngSpeed = tAngSpeed->f.value;
   else {
      buzzvm_seterror(vm,
                      BUZZVM_ERROR_TYPE,
                      "gotop(linspeed,angspeed): expected %s, got %s in second argument",
                      buzztype_desc[BUZZTYPE_FLOAT],
                      buzztype_desc[tAngSpeed->o.type]
         );
      return vm->state;
   }
   CVector2 cDir(fLinSpeed, CRadians(fAngSpeed));
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(buzzvm_stack_at(vm, 1)->u.value)->SetWheelSpeedsFromVector(cDir);
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

static int BuzzSetWheels(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 2);
   /* Push speeds */
   buzzvm_lload(vm, 1); /* Left speed */
   buzzvm_lload(vm, 2); /* Right speed */
   buzzvm_type_assert(vm, 2, BUZZTYPE_FLOAT);
   buzzvm_type_assert(vm, 1, BUZZTYPE_FLOAT);
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(
      buzzvm_stack_at(vm, 1)->u.value)->
      SetWheels(buzzvm_stack_at(vm, 3)->f.value, /* Left speed */
                buzzvm_stack_at(vm, 2)->f.value  /* Right speed */
         );
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

static int BuzzSetLEDs(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 3);
   /* Push the color components */
   buzzvm_lload(vm, 1);
   buzzvm_lload(vm, 2);
   buzzvm_lload(vm, 3);
   buzzvm_type_assert_number(vm, 3);
   buzzvm_type_assert_number(vm, 2);
   buzzvm_type_assert_number(vm, 1);
   /* Create a new color with that */
   CColor cColor(buzzvm_stack_number_to_int(vm, 3),
                 buzzvm_stack_number_to_int(vm, 2),
                 buzzvm_stack_number_to_int(vm, 1));
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(buzzvm_stack_at(vm, 1)->u.value)->SetLEDs(cColor);
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

static int BuzzSetLED(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 4);
   /* Push the color components */
   buzzvm_lload(vm, 1); // idx
   buzzvm_lload(vm, 2); // red
   buzzvm_lload(vm, 3); // green
   buzzvm_lload(vm, 4); // blue
   buzzvm_type_assert_number(vm, 4);
   buzzvm_type_assert_number(vm, 3);
   buzzvm_type_assert_number(vm, 2);
   buzzvm_type_assert_number(vm, 1);
   /* Create a new color with that */
   UInt32 unIdx = buzzvm_stack_number_to_int(vm, 4);
   CColor cColor(buzzvm_stack_number_to_int(vm, 3),
                 buzzvm_stack_number_to_int(vm, 2),
                 buzzvm_stack_number_to_int(vm, 1));
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(buzzvm_stack_at(vm, 1)->u.value)->
      SetLED(unIdx, cColor);
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

static int BuzzCameraEnable(buzzvm_t vm) {
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(buzzvm_stack_at(vm, 1)->u.value)->CameraEnable();
   return buzzvm_ret0(vm);
}

static int BuzzCameraDisable(buzzvm_t vm) {
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(buzzvm_stack_at(vm, 1)->u.value)->CameraDisable();
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

static int BuzzGripperLock(buzzvm_t vm) {
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(
      buzzvm_stack_at(vm, 1)->u.value)->GripperLock();
   return buzzvm_ret0(vm);
}

static int BuzzGripperUnlock(buzzvm_t vm) {
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(
      buzzvm_stack_at(vm, 1)->u.value)->GripperUnlock();
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

static int BuzzTurretEnable(buzzvm_t vm) {
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(
      buzzvm_stack_at(vm, 1)->u.value)->TurretEnable();
   return buzzvm_ret0(vm);
}

static int BuzzTurretDisable(buzzvm_t vm) {
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(
      buzzvm_stack_at(vm, 1)->u.value)->TurretDisable();
   return buzzvm_ret0(vm);
}

static int BuzzTurretSet(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   /* Push rotation */
   buzzvm_lload(vm, 1);
   buzzvm_type_assert(vm, 1, BUZZTYPE_FLOAT);
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(
      buzzvm_stack_at(vm, 1)->u.value)->TurretSet(
         buzzvm_stack_at(vm, 2)->f.value);
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

static int BuzzDistanceScannerEnable(buzzvm_t vm) {
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(
      buzzvm_stack_at(vm, 1)->u.value)->DistanceScannerEnable();
   return buzzvm_ret0(vm);
}

static int BuzzDistanceScannerDisable(buzzvm_t vm) {
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(
      buzzvm_stack_at(vm, 1)->u.value)->DistanceScannerDisable();
   return buzzvm_ret0(vm);
}

static int BuzzDistanceScannerSetRPM(buzzvm_t vm) {
   buzzvm_lnum_assert(vm, 1);
   /* Push rpm */
   buzzvm_lload(vm, 1);
   buzzvm_type_assert(vm, 1, BUZZTYPE_FLOAT);
   /* Get pointer to the controller */
   buzzvm_pushs(vm, buzzvm_string_register(vm, "controller", 1));
   buzzvm_gload(vm);
   /* Call function */
   reinterpret_cast<CBuzzControllerFootBot*>(
      buzzvm_stack_at(vm, 1)->u.value)->DistanceScannerSetRPM(
         buzzvm_stack_at(vm, 2)->f.value);
   return buzzvm_ret0(vm);
}

/****************************************/
/****************************************/

CBuzzControllerFootBot::CBuzzControllerFootBot() :
   m_pcWheelsA(NULL),
   m_pcLEDs(NULL),
   m_pcGripper(NULL),
   m_pcProximity(NULL),
   m_pcDistanceScannerA(NULL),
   m_pcDistanceScannerS(NULL),
   m_pcLight(NULL),
   m_pcCamera(NULL),
   m_pcWheelsS(NULL) {
}

/****************************************/
/****************************************/

CBuzzControllerFootBot::~CBuzzControllerFootBot() {
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::Init(TConfigurationNode& t_node) {
   try {
      /* Get pointers to devices */
      try {
         m_pcWheelsA = GetActuator<CCI_DifferentialSteeringActuator>("differential_steering");
         m_sWheelTurningParams.Init(GetNode(t_node, "wheel_turning"));
      }
      catch(CARGoSException& ex) {}
      try { m_pcLEDs = GetActuator<CCI_LEDsActuator>("leds"); }
      catch(CARGoSException& ex) {}
      try { m_pcGripper = GetActuator<CCI_FootBotGripperActuator>("footbot_gripper"); }
      catch(CARGoSException& ex) {}
      try { m_pcTurretA = GetActuator<CCI_FootBotTurretActuator>("footbot_turret"); }
      catch(CARGoSException& ex) {}
      try { m_pcProximity = GetSensor<CCI_FootBotProximitySensor>("footbot_proximity"); }
      catch(CARGoSException& ex) {}
      try { m_pcDistanceScannerA = GetActuator<CCI_FootBotDistanceScannerActuator>("footbot_distance_scanner"); }
      catch(CARGoSException& ex) {}
      try { m_pcDistanceScannerS = GetSensor<CCI_FootBotDistanceScannerSensor>("footbot_distance_scanner"); }
      catch(CARGoSException& ex) {}
      try { m_pcLight = GetSensor<CCI_FootBotLightSensor>("footbot_light"); }
      catch(CARGoSException& ex) {}
      try { m_pcCamera = GetSensor<CCI_ColoredBlobOmnidirectionalCameraSensor>("colored_blob_omnidirectional_camera"); }
      catch(CARGoSException& ex) {}
      try { m_pcWheelsS = GetSensor<CCI_DifferentialSteeringSensor>("differential_steering"); }
      catch(CARGoSException& ex) {}
      /* Initialize the rest */
      CBuzzController::Init(t_node);
   }
   catch(CARGoSException& ex) {
      THROW_ARGOSEXCEPTION_NESTED("Error initializing the Buzz controller for the foot-bot", ex);
   }
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::UpdateSensors() {
   /*
    * Update generic sensors
    */
   CBuzzController::UpdateSensors();
   /*
    * Update proximity sensor table
    */
   if(m_pcProximity != NULL) {
      /* Create empty proximity table */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "proximity", 1));
      buzzvm_pusht(m_tBuzzVM);
      buzzobj_t tProxTable = buzzvm_stack_at(m_tBuzzVM, 1);
      buzzvm_gstore(m_tBuzzVM);
      /* Get proximity readings */
      const CCI_FootBotProximitySensor::TReadings& tProxReads = m_pcProximity->GetReadings();
      /* Fill into the proximity table */
      buzzobj_t tProxRead;
      for(size_t i = 0; i < tProxReads.size(); ++i) {
         /* Create table for i-th read */
         buzzvm_pusht(m_tBuzzVM);
         tProxRead = buzzvm_stack_at(m_tBuzzVM, 1);
         buzzvm_pop(m_tBuzzVM);
         /* Fill in the read */
         TablePut(tProxRead, "value", tProxReads[i].Value);
         TablePut(tProxRead, "angle", tProxReads[i].Angle);
         /* Store read table in the proximity table */
         TablePut(tProxTable, i, tProxRead);
      }
   }
   /*
    * Update proximity sensor table
    */
   if(m_pcLight) {
      /* Create empty light table */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "light", 1));
      buzzvm_pusht(m_tBuzzVM);
      buzzobj_t tLightTable = buzzvm_stack_at(m_tBuzzVM, 1);
      buzzvm_gstore(m_tBuzzVM);
      /* Get light readings */
      const CCI_FootBotLightSensor::TReadings& tLightReads = m_pcLight->GetReadings();
      /* Fill into the light table */
      buzzobj_t tLightRead;
      for(size_t i = 0; i < tLightReads.size(); ++i) {
         /* Create table for i-th read */
         buzzvm_pusht(m_tBuzzVM);
         tLightRead = buzzvm_stack_at(m_tBuzzVM, 1);
         buzzvm_pop(m_tBuzzVM);
         /* Fill in the read */
         TablePut(tLightRead, "value", tLightReads[i].Value);
         TablePut(tLightRead, "angle", tLightReads[i].Angle);
         /* Store read table in the light table */
         TablePut(tLightTable, i, tLightRead);
      }
   }
   
   /*
   * Update distance scanner sensor table
   */
   if(m_pcDistanceScannerS) {
      /* Create empty distance scanner table */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "distance_scanner", 1));
      buzzvm_pusht(m_tBuzzVM);
      buzzobj_t tDistanceScannerTable = buzzvm_stack_at(m_tBuzzVM, 1);
      buzzvm_gstore(m_tBuzzVM);
      /* Get distance readings */
      const CCI_FootBotDistanceScannerSensor::TReadingsMap& tDistanceScannerReads = m_pcDistanceScannerS->GetReadingsMap();
      /* Fill into the distance table */
      buzzobj_t tDistanceScannerRead;
      int i = 0;
      for(auto const& [key, val] : tDistanceScannerReads) {
         /* Create table for i-th read */
         buzzvm_pusht(m_tBuzzVM);
         tDistanceScannerRead = buzzvm_stack_at(m_tBuzzVM, 1);
         buzzvm_pop(m_tBuzzVM);
         /* Fill in the read */
         TablePut(tDistanceScannerRead, "value", val);
         TablePut(tDistanceScannerRead, "angle", key);
         /* Store read table in the distance scanner table */
         TablePut(tDistanceScannerTable, i, tDistanceScannerRead);
         i++;
      }
   }
   /*
    * Camera
    */
   if(m_pcCamera) {
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "blobs", 1));
      buzzvm_pusht(m_tBuzzVM);
      buzzobj_t tBlobs = buzzvm_stack_at(m_tBuzzVM, 1);
      buzzvm_gstore(m_tBuzzVM);
      const CCI_ColoredBlobOmnidirectionalCameraSensor::SReadings& sBlobs = m_pcCamera->GetReadings();
      for(size_t i = 0; i < sBlobs.BlobList.size(); ++i) {
         buzzvm_pusht(m_tBuzzVM);
         buzzobj_t tEntry = buzzvm_stack_at(m_tBuzzVM, 1);
         buzzvm_pop(m_tBuzzVM);
         TablePut(tBlobs, i, tEntry);
         TablePut(tEntry, "distance", sBlobs.BlobList[i]->Distance);
         TablePut(tEntry, "angle",    sBlobs.BlobList[i]->Angle);
         TablePut(tEntry, "color",    sBlobs.BlobList[i]->Color);
      }
   }
   /*
    * Differential steering sensor
    */
   if(m_pcWheelsS) {
      /* Make "wheels" table */
      const CCI_DifferentialSteeringSensor::SReading& sWheels = m_pcWheelsS->GetReading();
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "wheels", 1));
      buzzvm_pusht(m_tBuzzVM);
      buzzobj_t tWheels = buzzvm_stack_at(m_tBuzzVM, 1);
      buzzvm_gstore(m_tBuzzVM);
      /* Make "velocity" table */
      buzzvm_pusht(m_tBuzzVM);
      buzzobj_t tVelocity = buzzvm_stack_at(m_tBuzzVM, 1);
      buzzvm_pop(m_tBuzzVM);
      TablePut(tWheels, "velocity", tVelocity);
      TablePut(tVelocity, "left", sWheels.VelocityLeftWheel);
      TablePut(tVelocity, "right", sWheels.VelocityRightWheel);
      /* Make "covered_distance" table */
      buzzvm_pusht(m_tBuzzVM);
      buzzobj_t tCoveredDistance = buzzvm_stack_at(m_tBuzzVM, 1);
      buzzvm_pop(m_tBuzzVM);
      TablePut(tWheels, "covered_distance", tCoveredDistance);
      TablePut(tCoveredDistance, "left", sWheels.CoveredDistanceLeftWheel);
      TablePut(tCoveredDistance, "right", sWheels.CoveredDistanceRightWheel);
      /* Axis length */
      TablePut(tWheels, "axis_length", sWheels.WheelAxisLength);
   }
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::SetWheelSpeedsFromVector(const CVector2& c_heading) {
   /* Get the heading angle */
   CRadians cHeadingAngle = c_heading.Angle().SignedNormalize();
   /* Get the length of the heading vector */
   Real fHeadingLength = c_heading.Length();
   /* Clamp the speed so that it's not greater than MaxSpeed */
   Real fBaseAngularWheelSpeed = Min<Real>(fHeadingLength, m_sWheelTurningParams.MaxSpeed);
   /* State transition logic */
   if(m_sWheelTurningParams.TurningMechanism == SWheelTurningParams::HARD_TURN) {
      if(Abs(cHeadingAngle) <= m_sWheelTurningParams.SoftTurnOnAngleThreshold) {
         m_sWheelTurningParams.TurningMechanism = SWheelTurningParams::SOFT_TURN;
      }
   }
   if(m_sWheelTurningParams.TurningMechanism == SWheelTurningParams::SOFT_TURN) {
      if(Abs(cHeadingAngle) > m_sWheelTurningParams.HardTurnOnAngleThreshold) {
         m_sWheelTurningParams.TurningMechanism = SWheelTurningParams::HARD_TURN;
      }
      else if(Abs(cHeadingAngle) <= m_sWheelTurningParams.NoTurnAngleThreshold) {
         m_sWheelTurningParams.TurningMechanism = SWheelTurningParams::NO_TURN;
      }
   }
   if(m_sWheelTurningParams.TurningMechanism == SWheelTurningParams::NO_TURN) {
      if(Abs(cHeadingAngle) > m_sWheelTurningParams.HardTurnOnAngleThreshold) {
         m_sWheelTurningParams.TurningMechanism = SWheelTurningParams::HARD_TURN;
      }
      else if(Abs(cHeadingAngle) > m_sWheelTurningParams.NoTurnAngleThreshold) {
         m_sWheelTurningParams.TurningMechanism = SWheelTurningParams::SOFT_TURN;
      }
   }
   /* Wheel speeds based on current turning state */
   Real fSpeed1, fSpeed2;
   switch(m_sWheelTurningParams.TurningMechanism) {
      case SWheelTurningParams::NO_TURN: {
         /* Just go straight */
         fSpeed1 = fBaseAngularWheelSpeed;
         fSpeed2 = fBaseAngularWheelSpeed;
         break;
      }
      case SWheelTurningParams::SOFT_TURN: {
         /* Both wheels go straight, but one is faster than the other */
         Real fSpeedFactor = (m_sWheelTurningParams.HardTurnOnAngleThreshold - Abs(cHeadingAngle)) / m_sWheelTurningParams.HardTurnOnAngleThreshold;
         fSpeed1 = fBaseAngularWheelSpeed - fBaseAngularWheelSpeed * (1.0 - fSpeedFactor);
         fSpeed2 = fBaseAngularWheelSpeed + fBaseAngularWheelSpeed * (1.0 - fSpeedFactor);
         break;
      }
      case SWheelTurningParams::HARD_TURN: {
         /* Opposite wheel speeds */
         fSpeed1 = -m_sWheelTurningParams.MaxSpeed;
         fSpeed2 =  m_sWheelTurningParams.MaxSpeed;
         break;
      }
   }
   /* Apply the calculated speeds to the appropriate wheels */
   Real fLeftWheelSpeed, fRightWheelSpeed;
   if(cHeadingAngle > CRadians::ZERO) {
      /* Turn Left */
      fLeftWheelSpeed  = fSpeed1;
      fRightWheelSpeed = fSpeed2;
   }
   else {
      /* Turn Right */
      fLeftWheelSpeed  = fSpeed2;
      fRightWheelSpeed = fSpeed1;
   }
   /* Finally, set the wheel speeds */
   m_pcWheelsA->SetLinearVelocity(fLeftWheelSpeed, fRightWheelSpeed);
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::SetWheels(Real f_left_speed,
                                       Real f_right_speed) {
   m_pcWheelsA->SetLinearVelocity(f_left_speed,
                                  f_right_speed);
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::SetLEDs(const CColor& c_color) {
   m_pcLEDs->SetAllColors(c_color);
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::SetLED(UInt32 un_idx,
                                    const CColor& c_color) {
   m_pcLEDs->SetSingleColor(un_idx, c_color);
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::CameraEnable() {
   m_pcCamera->Enable();
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::CameraDisable() {
   m_pcCamera->Disable();
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::GripperLock() {
   m_pcGripper->LockPositive();
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::GripperUnlock() {
   m_pcGripper->Unlock();
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::TurretEnable() {
   m_pcTurretA->SetMode(CCI_FootBotTurretActuator::MODE_POSITION_CONTROL);
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::TurretDisable() {
   m_pcTurretA->SetMode(CCI_FootBotTurretActuator::MODE_OFF);
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::TurretSet(Real f_rotation) {
   m_pcTurretA->SetRotation(CRadians(f_rotation));
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::DistanceScannerEnable() {
   m_pcDistanceScannerA->Enable();
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::DistanceScannerDisable() {
  m_pcDistanceScannerA->Disable();
}

/****************************************/
/****************************************/

void CBuzzControllerFootBot::DistanceScannerSetRPM(Real f_rpm) {
   m_pcDistanceScannerA->SetRPM(f_rpm);
}

/****************************************/
/****************************************/

buzzvm_state CBuzzControllerFootBot::RegisterFunctions() {
   /* Register base functions */
   CBuzzController::RegisterFunctions();
   if(m_pcWheelsA) {
      /* BuzzSetWheels */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "set_wheels", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzSetWheels));
      buzzvm_gstore(m_tBuzzVM);
      /* BuzzGoTo with Cartesian coordinates */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "goto", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzGoToC));
      buzzvm_gstore(m_tBuzzVM);
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "gotoc", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzGoToC));
      buzzvm_gstore(m_tBuzzVM);
      /* BuzzGoTo with Polar coordinates */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "gotop", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzGoToP));
      buzzvm_gstore(m_tBuzzVM);
   }
   if(m_pcLEDs) {
      /* BuzzSetLEDs */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "set_leds", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzSetLEDs));
      buzzvm_gstore(m_tBuzzVM);
      /* BuzzSetLED */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "set_led", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzSetLED));
      buzzvm_gstore(m_tBuzzVM);
   }
   if(m_pcCamera) {
      /* BuzzCameraEnable */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "camera_enable", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzCameraEnable));
      buzzvm_gstore(m_tBuzzVM);
      /* BuzzCameraDisable */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "camera_disable", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzCameraDisable));
      buzzvm_gstore(m_tBuzzVM);
   }
   if(m_pcGripper) {
      /* BuzzGripperLock */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "gripper_lock", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzGripperLock));
      buzzvm_gstore(m_tBuzzVM);
      /* BuzzGripperUnlock */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "gripper_unlock", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzGripperUnlock));
      buzzvm_gstore(m_tBuzzVM);
   }
   if(m_pcTurretA) {
      /* BuzzTurretEnable */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "turret_enable", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzTurretEnable));
      buzzvm_gstore(m_tBuzzVM);
      /* BuzzTurretDisable */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "turret_disable", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzTurretDisable));
      buzzvm_gstore(m_tBuzzVM);
      /* BuzzTurretSet */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "turret_set", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzTurretSet));
      buzzvm_gstore(m_tBuzzVM);
   }
   if(m_pcDistanceScannerA) {
      /* BuzzDistanceScannerEnable */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "distance_scanner_enable", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzDistanceScannerEnable));
      buzzvm_gstore(m_tBuzzVM);
      /* BuzzDistanceScannerDisable */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "distance_scanner_disable", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzTurretDisable));
      buzzvm_gstore(m_tBuzzVM);
      /* BuzzDistanceScannerSetRPM */
      buzzvm_pushs(m_tBuzzVM, buzzvm_string_register(m_tBuzzVM, "distance_scanner_set_rpm", 1));
      buzzvm_pushcc(m_tBuzzVM, buzzvm_function_register(m_tBuzzVM, BuzzDistanceScannerSetRPM));
      buzzvm_gstore(m_tBuzzVM);
   }
   return m_tBuzzVM->state;
}

/****************************************/
/****************************************/

REGISTER_CONTROLLER(CBuzzControllerFootBot, "buzz_controller_footbot");

#include <argos3/plugins/robots/foot-bot/simulator/footbot_entity.h>
REGISTER_BUZZ_ROBOT(CFootBotEntity);


#pragma once

#include <string>

#define INIT_DELAY     600
#define NUM_BUTTONS    14
#define NUM_AXES       8
#define INPUT_DEADZONE ( 0.1f * float(0x7fff) ) // Default to 10% of the +/- 32767 range

class CGamepad
{
public:

   enum TJoystickType
   {
      J_NONE,
      J_LOGITECH_EXTREME_3D,
      J_LOGITECH_GAMEPAD_F310,
      J_XBOX_GAMEPAD
   };

   enum TJoystickState
   {
      J_INIT,
      J_RUN,
      J_SHUTDOWN
   };

   CGamepad();
   ~CGamepad();

   TJoystickState GetState() const { return mState; };
   TJoystickType GetType() const { return mType; };

   void Update();

   void Shutdown();

private:

   int Initialize();
   int ReadData();
   void ConvertData();

   std::string       mName;
   std::string       mDevice;
   int               mFd;
   int               mInitDelay;
   TJoystickType     mType;
   TJoystickState    mState;
   int               mAxis[NUM_AXES];
   int               mButtons[NUM_BUTTONS];
};

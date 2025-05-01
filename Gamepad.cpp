#include <errno.h>
#include <sys/file.h>
#include <linux/joystick.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include "Gamepad.h"

CGamepad::CGamepad()
   : mName(""),
     mDevice(""),
     mFd(-1),
     mInitDelay(0),
     mType(J_NONE),
     mState(J_INIT),
     mAxis{},
     mButtons{}
{
}

CGamepad::~CGamepad()
{
   Shutdown();
}

void CGamepad::ConvertData()
{
   int index; // active weapon

#if 0
   switch (mType)
   {
      // handle conversion for Xbox Controller
      case J_XBOX_GAMEPAD:

         switch (mMode)
         {
            case J_FLIGHT:
               // triggers
               ios_server_outputs->turn_left = ((float)mAxis[5] + 32767.0) / 65535.0;
               ios_server_outputs->turn_right = ((float)mAxis[4] + 32767.0) / 65535.0;

               // left analog stick - left/right
               if (mAxis[0] < INPUT_DEADZONE && mAxis[0] > -INPUT_DEADZONE)
                  ios_server_outputs->l_stick_left_right = 0.0;
               else
                  ios_server_outputs->l_stick_left_right = -((float)mAxis[0]) / 32768.0;

               // left analog stick - up/down
               if (mAxis[1] < INPUT_DEADZONE && mAxis[1] > -INPUT_DEADZONE)
                  ios_server_outputs->altitude_up_down = 0.0;
               else
                  ios_server_outputs->altitude_up_down = ((float)mAxis[1]) / 32768.0;
               if (ios_server_outputs->invert_collective_axis)
                  ios_server_outputs->altitude_up_down = -ios_server_outputs->altitude_up_down;

               // right analog stick - left/right
               if (mAxis[2] < INPUT_DEADZONE && mAxis[2] > -INPUT_DEADZONE)
                  ios_server_outputs->flight_left_right_motion = 0.0;
               else
                  ios_server_outputs->flight_left_right_motion = ((float)mAxis[2]) / 32768.0;

               // right analog stick - up/down
               if (mAxis[3] < INPUT_DEADZONE && mAxis[3] > -INPUT_DEADZONE)
                  ios_server_outputs->flight_fwd_aft_motion = 0.0;
               else
                  ios_server_outputs->flight_fwd_aft_motion = -((float)mAxis[3]) / 32768.0;
               if (ios_server_outputs->invert_cyclic_axis)
                  ios_server_outputs->flight_fwd_aft_motion = -ios_server_outputs->flight_fwd_aft_motion;

               break;
         }

         break;
   }
#endif
}

int CGamepad::Initialize()
{
   // look for flight controllers in js0 - js10 range
   for (int i = 0; i <= 10; i++)
   {
      char name[128] = {};

      mType = J_NONE;
      mDevice = "/dev/input/js" + std::to_string(i);

      printf("Attempting to open %s\n",  mDevice.c_str());

      mFd = open(mDevice.c_str(), O_RDONLY | O_NONBLOCK);

      if (mFd < 0)
      {
         // failed to open, continue
         continue;
      }

      if (ioctl(mFd, JSIOCGNAME(sizeof(name)), name) < 0)
      {
         printf("Error reading %s: %s\n", mDevice.c_str(), strerror(errno));
         mName = "Unknown";
      }
      else
      {
         mName = name;
         printf("device %s, name %s\n", mDevice.c_str(), mName.c_str());
      }

      if (mName == "Logitech Extreme 3D")
         mType = J_LOGITECH_EXTREME_3D;
      else if (mName == "Logitech Gamepad F310")
         mType = J_LOGITECH_GAMEPAD_F310;
      else if (mName == "Xbox Gamepad" ||
               mName == "Microsoft X-Box 360 pad")
         mType = J_XBOX_GAMEPAD;

      if (mType != J_NONE)
      {
         // try to lock device
         if (flock(mFd, LOCK_EX | LOCK_NB) < 0)
         {
            // failed to lock, close the file and continue
            close(mFd);
            mFd = -1;
            continue;
         }

         printf("Connected to %s: %s\n", mDevice.c_str(), mName.c_str());
      }
      else
      {
         printf("Closing device %s\n", mDevice.c_str());
         // not a supported joystick, close the file and continue
         close(mFd);
         mFd = -1;
         printf("Closed device %s\n", mDevice.c_str());
         continue;
      }

      return 1;
   }

   mFd = -1;
   return 0;
}

int CGamepad::ReadData()
{
   struct js_event joystick_event;
   int bytes_read;
   bytes_read = read(mFd, &joystick_event, sizeof(struct js_event));

   while (bytes_read > 0)
   {
      // handle button presses
      if ((joystick_event.type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON)
      {
         if (joystick_event.number < NUM_BUTTONS)
            mButtons[joystick_event.number] = joystick_event.value;
         else
            printf("Got flight controller button %d data (max %d)\n", joystick_event.number, NUM_BUTTONS);

      }

      // handle axis movement
      if ((joystick_event.type & ~JS_EVENT_INIT) == JS_EVENT_AXIS)
      {
         if (joystick_event.number < NUM_AXES)
            mAxis[joystick_event.number] = joystick_event.value;
         else
            printf("Got flight controller axis %d data (max %d)\n", joystick_event.number, NUM_AXES);
      }

      // print some debug
      char msg_string[256];

      printf("JOYSTICK - %s\n", mDevice.c_str());
      strcpy(msg_string, "Axis     ");
      for (int i = 0; i < NUM_AXES; i++)
      {
         char tmp_string[10];
         sprintf(tmp_string, "%d:%6d ", i+1, mAxis[i]);
         strcat(msg_string, tmp_string);
      }
      strcat(msg_string, "\n");
      printf(msg_string);

      strcpy(msg_string, "Buttons ");
      for (int i = 0; i < NUM_BUTTONS; i++)
      {
         char tmp_string[10];
         sprintf(tmp_string, "% 2d: %d ", i+1, mButtons[i]);
         strcat(msg_string, tmp_string);
      }
      strcat(msg_string, "\n");
      printf(msg_string);

      bytes_read = read(mFd, &joystick_event, sizeof(struct js_event));
   }

   if (errno != EAGAIN)
      return -1;
   else
      return 0;
}

void CGamepad::Update()
{
   switch (mState)
   {
      case J_INIT:

         // try to initialize every 10 seconds
         if (mInitDelay <= 0)
         {
            int status;

            mInitDelay = INIT_DELAY;
            status = Initialize();
            if (status)
            {
               mState = J_RUN;
            }
         }

         mInitDelay--;

         break;

      case J_RUN:
      {
         if (ReadData() < 0)
         {
            // check error returned
            printf("Error reading %s: %s\n", mDevice.c_str(), strerror(errno));
            flock(mFd, LOCK_UN | LOCK_NB);
            close(mFd);
            mFd = -1;
            mType = J_NONE;
            mState = J_INIT;
            memset(mButtons, 0, sizeof(mButtons));
            memset(mAxis, 0, sizeof(mAxis));
            return;
         }

         // convert to globals for flight models
         ConvertData();

         mInitDelay = INIT_DELAY;

         break;
      }

      case J_SHUTDOWN:
         Shutdown();
         break;
   }

#if 0
   // set raw data to IOS globals
   for (int i = 0; i < NUM_BUTTONS; i++)
   {
      ios_server_outputs->fc_button[i] = mButtons[i];
   }

   for (int i = 0; i < NUM_AXES; i++)
   {
      ios_server_outputs->fc_axis[i] = mAxis[i];
   }
#endif
}

void CGamepad::Shutdown()
{
   if (mFd)
   {
      flock(mFd, LOCK_UN | LOCK_NB);
      close(mFd);
      mFd = -1;
   }
}

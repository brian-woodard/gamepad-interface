#include <stdio.h>
#include <unistd.h>
#include "Gamepad.h"

int main()
{
   CGamepad gamepad;

   while (1)
   {
      gamepad.Update();
      usleep(16666);
   }

   gamepad.Shutdown();
}

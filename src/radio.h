#pragma once
#include "typedefs.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>


bool setup_CC1101()
{

  ELECHOUSE_cc1101.setSpiPin(CC1101_sck, CC1101_miso, CC1101_mosi, CC1101_ss);

  if (!ELECHOUSE_cc1101.getCC1101())
  {
    return false;
  }
  ELECHOUSE_cc1101.Init();         // must be set to initialize the cc1101!
  ELECHOUSE_cc1101.setMHZ(433.92); // Here you can set your basic frequency. The lib calculates the frequency automatically (default = 433.92).The cc1101 can: 300-348 MHZ, 387-464MHZ and 779-928MHZ. Read More info from datasheet.
  ELECHOUSE_cc1101.setRxBW(812.50); // Set the Receive Bandwidth in kHz. Value from 58.03 to 812.50. Default is 812.50 kHz.

  ELECHOUSE_cc1101.SetRx(); // set Receive on
  return true;
}

bool initRadio()
{
  if (!setup_CC1101()) {
    return false;
  }
  return true;
}
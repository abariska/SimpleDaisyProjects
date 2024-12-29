#include <stdio.h>
#include <string.h>
#include "daisy_pod.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;

using MyOledDisplay = OledDisplay<SSD130xI2c128x32Driver>;

DaisySeed     hw;
MyOledDisplay display;

int main(void)
{
    uint8_t message_idx;
    hw.Configure();
    hw.Init();

    /** Configure the Display */
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.i2c_address               = 0x3C;
    disp_cfg.driver_config.transport_config.i2c_config.periph         = I2CHandle::Config::Peripheral::I2C_1;
    disp_cfg.driver_config.transport_config.i2c_config.speed          = I2CHandle::Config::Speed::I2C_100KHZ;
    disp_cfg.driver_config.transport_config.i2c_config.mode           = I2CHandle::Config::Mode::I2C_MASTER;
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = {DSY_GPIOB, 8};    disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = {DSY_GPIOB, 9};
    /** And Initialize */
    display.Init(disp_cfg);
    message_idx = 0;
    char strbuff[128];
    while(1)
    {
        System::Delay(500);
        switch(message_idx)
        {
            case 0: sprintf(strbuff, "Testing. . ."); break;
            case 1: sprintf(strbuff, "Daisy. . ."); break;
            case 2: sprintf(strbuff, "1. . ."); break;
            case 3: sprintf(strbuff, "2. . ."); break;
            case 4: sprintf(strbuff, "3. . ."); break;
            default: break;
        }
        message_idx = (message_idx + 1) % 5;
        display.Fill(true);
        display.SetCursor(0, 0);
        //display.DrawCircle(20, 20, 10, true);
        display.WriteString(strbuff, Font_11x18, false);
        display.Update();
    }
}
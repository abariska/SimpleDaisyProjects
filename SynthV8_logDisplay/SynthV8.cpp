#include <stdio.h>
#include <string.h>
#include "daisy_pod.h"
#include "daisysp.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;
using namespace daisysp;
using MyOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;

static DaisyPod hw;
static MyOledDisplay display;
static Oscillator osc;
static AdEnv ad;
static Parameter p_release;
static MidiUsbHandler midi;
static Parameter tuning;
static CpuLoadMeter cpu_meter;

static float volChange;
static float inc;
float adRelease;
float midiFreq;
float totalFreq;
bool adCycle;
bool isCpuEnabled;

void DisplayView();

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    cpu_meter.OnBlockStart();

    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();

    if(hw.button1.RisingEdge() || (adCycle && !ad.IsRunning()))
        ad.Trigger();

    if(hw.button2.RisingEdge())
        adCycle = !adCycle;

    adRelease = p_release.Process();
    
    inc = hw.encoder.Increment();
    if(inc > 0)
        volChange += 0.02;
    else if(inc < 0)
        volChange -= 0.02;

    if (volChange > 1)
        volChange = 1;
    else if(volChange < 0)
        volChange = 0;

    if(hw.encoder.RisingEdge())
        isCpuEnabled = !isCpuEnabled;
    
    for(size_t i = 0; i < size; i += 2)
    {
        float sig;
        float adOut;

        ad.SetTime(ADENV_SEG_DECAY, adRelease);
        adOut = ad.Process();
        totalFreq = 400.0f;
        osc.SetFreq(totalFreq);
        sig = osc.Process();
        sig *= adOut;
        sig *= volChange;
        out[i] = sig;
        out[i + 1] = sig;
    }
    cpu_meter.OnBlockEnd();
}
int main(void)
{
    float samplerate;
    isCpuEnabled = 0;

    hw.seed.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    samplerate = hw.AudioSampleRate();

    hw.seed.StartLog();
    cpu_meter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());

    /** Configure the Display */
    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.i2c_address               = 0x3C;
    disp_cfg.driver_config.transport_config.i2c_config.periph         = I2CHandle::Config::Peripheral::I2C_1;
    disp_cfg.driver_config.transport_config.i2c_config.speed          = I2CHandle::Config::Speed::I2C_100KHZ;
    disp_cfg.driver_config.transport_config.i2c_config.mode           = I2CHandle::Config::Mode::I2C_MASTER;
    // disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = {DSY_GPIOB, 8};    
    // disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = {DSY_GPIOB, 9};
    /** And Initialize */
    display.Init(disp_cfg);

    volChange = 0.5;
    inc = 0;

    osc.Init(samplerate);
    osc.SetWaveform(osc.WAVE_TRI);
    osc.SetAmp(0.1f);

    ad.Init(samplerate);
    ad.SetTime(ADENV_SEG_ATTACK, 0.01f);
    ad.SetTime(ADENV_SEG_DECAY, 1.0f);
    ad.SetMax(1);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    tuning.Init(hw.knob1, -0.1, 0.1, tuning.LINEAR);

    p_release.Init(hw.knob2, 0.01f, 0.7f, Parameter::LINEAR);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    
    while (1) 
    {
        DisplayView();
    }
}
void ProcessDigitalControls()
{
    hw.encoder.Debounce();
    hw.button1.Debounce();
    hw.button2.Debounce();
}
void DisplayView()
{
    display.Fill(false);
    if (isCpuEnabled)
        {
        char bufferMin[128];
        char bufferAv[128];
        char bufferMax[128];

        const float avgLoad = cpu_meter.GetAvgCpuLoad();
        const float maxLoad = cpu_meter.GetMaxCpuLoad();
        const float minLoad = cpu_meter.GetMinCpuLoad();

        sprintf(bufferMin, "CPUmin %.1f%%", minLoad * 100);
        sprintf(bufferAv,  "CPUav  %.1f%%", avgLoad * 100);
        sprintf(bufferMax, "CPUmax %.1f%%", maxLoad * 100);

        display.SetCursor(1, 0);
        display.WriteString(bufferMin, Font_11x18, true);
        display.SetCursor(1, 19);
        display.WriteString(bufferAv, Font_11x18, true);
        display.SetCursor(1, 36);
        display.WriteString(bufferMax, Font_11x18, true);

        hw.seed.PrintLine(bufferMin);
        hw.seed.PrintLine(bufferAv);
        hw.seed.PrintLine(bufferMax);
        }
    display.Update();
}

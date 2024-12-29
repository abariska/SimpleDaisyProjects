#include "daisy_pod.h"
#include "daisysp.h"
#include "dev/oled_ssd130x.h"

using namespace daisy;
using namespace daisysp;

static DaisyPod hw;
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
bool isVolumeOn;

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
        isVolumeOn = !isVolumeOn;
    
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
        sig *= isVolumeOn;
        out[i] = sig;
        out[i + 1] = sig;
    }
    cpu_meter.OnBlockEnd();
}
int main(void)
{
    float samplerate;

    hw.Init();
    hw.SetAudioBlockSize(4);
    samplerate = hw.AudioSampleRate();

    hw.seed.StartLog();
    cpu_meter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());

    volChange = 0.5;
    isVolumeOn = true;
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
        // Виводимо завантаження CPU у серійний монітор кожні 100 мс
        const float avgLoad = cpu_meter.GetAvgCpuLoad();
        const float maxLoad = cpu_meter.GetMaxCpuLoad();
        const float minLoad = cpu_meter.GetMinCpuLoad();

        hw.seed.PrintLine("Processing Load %:");
        hw.seed.PrintLine("Max: " FLT_FMT3, FLT_VAR3(maxLoad * 100.0f));
        hw.seed.PrintLine("Avg: " FLT_FMT3, FLT_VAR3(avgLoad * 100.0f));
        hw.seed.PrintLine("Min: " FLT_FMT3, FLT_VAR3(minLoad * 100.0f));
        // don't spam the serial connection too much
        System::Delay(1000);
        
    }
}
void ProcessDigitalControls()
{
    hw.encoder.Debounce();
    hw.button1.Debounce();
    hw.button2.Debounce();
}

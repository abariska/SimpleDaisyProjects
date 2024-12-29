#include "daisy_pod.h"
#include "daisysp.h"
//#include "adEnv.h"

using namespace daisy;
using namespace daisysp;

DaisyPod hw;
Oscillator osc;
CpuLoadMeter cpu_meter;

float noteFreq = 440.0;
float sig;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    
    cpu_meter.OnBlockStart();
    for(size_t i = 0; i < size; i += 2)
    {
        osc.SetFreq(noteFreq);
        sig = osc.Process();

        // left out
        out[i] = sig;

        // right out
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

    cpu_meter.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
    hw.seed.StartLog();

    osc.Init(samplerate);
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(noteFreq);
    osc.SetAmp(0.1f);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    cpu_meter.Reset();
    uint32_t last_update = System::GetNow();

    while (1) {
        // Виводимо завантаження CPU у серійний монітор кожні 100 мс
        const float avgLoad = cpu_meter.GetAvgCpuLoad();
        const float maxLoad = cpu_meter.GetMaxCpuLoad();
        const float minLoad = cpu_meter.GetMinCpuLoad();

        hw.seed.PrintLine("Processing Load %:");
        hw.seed.PrintLine("Max: " FLT_FMT3, FLT_VAR3(maxLoad * 100.0f));
        hw.seed.PrintLine("Avg: " FLT_FMT3, FLT_VAR3(avgLoad * 100.0f));
        hw.seed.PrintLine("Min: " FLT_FMT3, FLT_VAR3(minLoad * 100.0f));
        // don't spam the serial connection too much
        System::Delay(500);
        
    }
}

    

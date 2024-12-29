#include "daisy_pod.h"
#include "daisysp.h"
//#include "adEnv.h"

using namespace daisy;
using namespace daisysp;

DaisyPod hw;
Oscillator osc;

float noteFreq = 440.0;
float sig;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    for(size_t i = 0; i < size; i += 2)
    {
        osc.SetFreq(noteFreq);
        sig = osc.Process();

        // left out
        out[i] = sig;

        // right out
        out[i + 1] = sig;
    }
}

int main(void)
{
    float samplerate;

    hw.Init();
    hw.SetAudioBlockSize(4);
    samplerate = hw.AudioSampleRate();

    osc.Init(samplerate);
    osc.SetWaveform(osc.WAVE_SIN);
    osc.SetFreq(noteFreq);
    osc.SetAmp(0.1f);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while (1) {}
}

    

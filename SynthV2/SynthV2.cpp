#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

static DaisyPod hw;
static Oscillator osc;
static Parameter  p_freq;

float sig;
float freq;

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    hw.ProcessAnalogControls();
    hw.ProcessDigitalControls();
    freq = p_freq.Process();

    for(size_t i = 0; i < size; i += 2)
    {
        osc.SetFreq(freq);
        sig = osc.Process();
        out[i] = sig;
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
    osc.SetWaveform(osc.WAVE_TRI);
    osc.SetAmp(0.1f);

    p_freq.Init(hw.knob1, 50, 5000, Parameter::LOGARITHMIC);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);

    while (1) {}
}


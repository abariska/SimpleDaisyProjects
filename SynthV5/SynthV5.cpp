#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

static DaisyPod hw;
static Oscillator osc;
static AdEnv ad;
static Parameter p_release;
static MidiUsbHandler midi;
static Parameter tuning;

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
        totalFreq = midiFreq + (midiFreq * tuning.Process());
        osc.SetFreq(totalFreq);
        sig = osc.Process();
        sig *= adOut;
        sig *= volChange;
        sig *= isVolumeOn;
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

    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    midi.Init(midi_cfg);

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
        midi.Listen();
        while(midi.HasEvents())
        {
            auto msg = midi.PopEvent();
            switch(msg.type)
            {
                case NoteOn:
                {
                    ad.Trigger();
                    auto note_msg = msg.AsNoteOn();
                    if(note_msg.velocity != 0)
                        midiFreq = mtof(note_msg.note);
                        // totalFreq = midiFreq *= tuning.Process();
                        // osc.SetFreq(totalFreq);
                }
                break;
                default: break;
            }
        }
    }
}
void ProcessDigitalControls()
{
    hw.encoder.Debounce();
    hw.button1.Debounce();
    hw.button2.Debounce();
}

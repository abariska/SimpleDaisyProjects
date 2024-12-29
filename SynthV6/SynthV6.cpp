#include "daisy_pod.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

static DaisyPod hw;
static MidiUsbHandler midi;
static Oscillator osc;
static AdEnv ad;
static Parameter p_release;

static float volChange;
static float inc;
float adDecay;
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

    adDecay = p_release.Process();

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

        ad.SetTime(ADENV_SEG_DECAY, adDecay);
        adOut = ad.Process();
        sig = osc.Process();
        sig *= adOut;
        sig *= volChange;
        sig *= isVolumeOn;
        out[i] = sig;
        out[i + 1] = sig;
    }
}
void HandleMidiMessage(MidiEvent m)
{
    switch(m.type)
    {
        case NoteOn:
        {
            NoteOnEvent p = m.AsNoteOn();
            char        buff[512];
            sprintf(buff,
                    "Note Received:\t%d\t%d\t%d\r\n",
                    m.channel,
                    m.data[0],
                    m.data[1]);
            if(m.data[1] != 0)
            {
                ad.Trigger();
                p = m.AsNoteOn();
                osc.SetFreq(mtof(p.note));
                osc.SetAmp((p.velocity / 127.0f));
            }
        }
        break;
        default: break;
    }
}
int main(void)
{
    float samplerate;

    hw.Init();
    hw.SetAudioBlockSize(4);
    hw.seed.usb_handle.Init(UsbHandle::FS_INTERNAL);
    System::Delay(250);
    samplerate = hw.AudioSampleRate();

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

    p_release.Init(hw.knob2, 0.01f, 0.7f, Parameter::LINEAR);

    hw.StartAdc();
    hw.StartAudio(AudioCallback);
    hw.midi.StartReceive();

    for(;;)
    {
        midi.Listen();
        while(hw.midi.HasEvents())
        {
            HandleMidiMessage(hw.midi.PopEvent());
        }
    }   
}
void ProcessDigitalControls()
{
    hw.encoder.Debounce();
    hw.button1.Debounce();
    hw.button2.Debounce();
}

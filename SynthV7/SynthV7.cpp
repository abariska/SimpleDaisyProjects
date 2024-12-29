#include "daisy_pod.h"
#include "daisysp.h"
#include "daisysp-lgpl.h"
#include "dev/oled_ssd130x.h"

#define MAX_DELAY static_cast<size_t>(48000 * 1.f)

using namespace daisy;
using namespace daisysp;

using MyOledDisplay = OledDisplay<SSD130xI2c128x64Driver>;

static DaisyPod pod;
MyOledDisplay display;
static MidiUsbHandler midi;
static Oscillator osc;
static AdEnv ad;
static MoogLadder flt;
// static DelayLine delay;
static ReverbSc rev;
static CpuLoadMeter cpu_meter;

static Parameter osc_volume, osc_tuning, filter_cutoff, filter_resonance, ad_attack, ad_decay, rev_feedback, rev_filter;

enum Menu { OSCILLATOR, FILTER, ENVELOPE, DELAY, REVERB };
Menu current_menu = OSCILLATOR; 

u_int8_t voice_num = 4;

int wave_type = 0;
float midiFreq, osc_freq, volume, tune;
float cutoff, resonance, filterEnv;
float attack, decay, drive;
float delay_rate, delay_feedback, delay_level;
float revFeedback, revFilter, revWetLevelL, revWetLevelR, revLevel;
bool isVolumeOn;
float oldk1, oldk2, k1, k2;
bool isCpuEnabled;
int inc;

void MidiUpdate();
void ProcessControls();
void GetReverbSample(float &inl, float &inr);
void DisplayView();

void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update);

void UpdateLeds() {
    // Задаємо кольори залежно від активного пункту меню
    switch (current_menu) {
        case OSCILLATOR:
            pod.led1.Set(0.3 ,0 ,0);
            break;
        case FILTER:
            pod.led1.Set(0 ,0.3 ,0);
            break;
        case ENVELOPE:
            pod.led1.Set(0 ,0 ,0.3);
            break;
        case DELAY:
            pod.led1.Set(0.3 ,0.3 ,0);
            break;
        case REVERB:
            pod.led1.Set(0.3 ,0 ,0.3);
            break;
    }
    pod.UpdateLeds();
}

static void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                          AudioHandle::InterleavingOutputBuffer out,
                          size_t                                size)
{
    cpu_meter.OnBlockStart();

    UpdateLeds();
    ProcessControls();
    MidiUpdate();

    for(size_t i = 0; i < size; i += 2)
    {
        float sig;

        osc.SetWaveform(wave_type);
        osc.SetFreq(midiFreq + (midiFreq * tune));
        sig = osc.Process();
        sig = flt.Process(sig);
        sig *= ad.Process();
        
        float wet_l = sig;
        float wet_r = sig;
        GetReverbSample(wet_l, wet_r);

        out[i] = wet_l * revWetLevelL + sig * (1.0f - revWetLevelL);
        out[i + 1] = wet_r * revWetLevelR + sig * (1.0f - revWetLevelR);  
    }
    cpu_meter.OnBlockEnd();
}
int main(void)
{
    float samplerate;
    volume = 0.5f;
    tune = 0.0f;
    wave_type = 0;
    cutoff = 1000.0f;
    resonance = 0.0f;
    drive = 1.0f;
    attack = 0.01f;
    decay = 0.2f;
    // delay_rate = 0.5f;
    // delay_feedback = 0.5f;
    // delay_level = 0.5f;
    revFeedback = 0.7f;
    revFilter = 1000.0f;
    revWetLevelL = 0.3;
    revWetLevelR = 0.3;
    revLevel = 0.0;
    oldk1 = oldk2 = 0;
    k1 = k2   = 0;
    isCpuEnabled = 0;

    pod.seed.Configure();
    pod.Init();
    pod.SetAudioBlockSize(4);
    samplerate = pod.AudioSampleRate();
    
    pod.seed.StartLog();
    cpu_meter.Init(pod.AudioSampleRate(), pod.AudioBlockSize());

    MidiUsbHandler::Config midi_cfg;
    midi_cfg.transport_config.periph = MidiUsbTransport::Config::INTERNAL;
    midi.Init(midi_cfg);

    osc.Init(samplerate);
    osc.SetWaveform(osc.WAVE_SQUARE);
    osc.SetAmp(0.1f);

    flt.Init(samplerate);
    flt.SetFreq(cutoff);
    flt.SetRes(resonance);

    ad.Init(samplerate);
    ad.SetTime(ADENV_SEG_ATTACK, attack);
    ad.SetTime(ADENV_SEG_DECAY, decay);
    ad.SetMax(1);
    ad.SetMin(0);
    ad.SetCurve(0.5);

    rev.Init(samplerate);
    rev.SetLpFreq(18000.0f);
    rev.SetFeedback(0.85f);

    osc_volume.Init(pod.knob1, 0, 1, osc_volume.LINEAR);
    osc_tuning.Init(pod.knob2, -0.1, 0.1, osc_tuning.LINEAR);
    filter_cutoff.Init(pod.knob1, 50, 10000, filter_cutoff.LOGARITHMIC);
    filter_resonance.Init(pod.knob2, 0, 1, filter_resonance.LINEAR);
    ad_attack.Init(pod.knob1, 0, 1, ad_attack.LINEAR);
    ad_decay.Init(pod.knob2, 0, 1, ad_decay.LINEAR);
    rev_feedback.Init(pod.knob1, 0, 1, rev_feedback.LINEAR);
    rev_filter.Init(pod.knob2, 200, 20000, rev_filter.LOGARITHMIC);

    MyOledDisplay::Config disp_cfg;
    disp_cfg.driver_config.transport_config.i2c_address               = 0x3C;
    disp_cfg.driver_config.transport_config.i2c_config.periph         = I2CHandle::Config::Peripheral::I2C_1;
    disp_cfg.driver_config.transport_config.i2c_config.speed          = I2CHandle::Config::Speed::I2C_100KHZ;
    disp_cfg.driver_config.transport_config.i2c_config.mode           = I2CHandle::Config::Mode::I2C_MASTER;
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.scl = {DSY_GPIOB, 8};    
    disp_cfg.driver_config.transport_config.i2c_config.pin_config.sda = {DSY_GPIOB, 9};
    disp_cfg.driver_config.transport_config.i2c_config.speed = I2CHandle::Config::Speed::I2C_400KHZ;  // Підвищення швидкості до 400 кГц
    display.Init(disp_cfg);

    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while (1)
    {
        DisplayView();
    }
}
void ConditionalParameter(float  oldVal,
                          float  newVal,
                          float &param,
                          float  update)
{
    if(abs(oldVal - newVal) > 0.00005)
    {
        param = update;
    }
}
void ProcessButtons() {
    if (pod.button1.RisingEdge()) {
        if (current_menu == OSCILLATOR)
            current_menu = REVERB;
        else
            current_menu = static_cast<Menu>(static_cast<int>(current_menu) - 1);
    }
    if (pod.button2.RisingEdge()) {
        current_menu = static_cast<Menu>((static_cast<int>(current_menu) + 1) % 5);
    }
}
void ProcessKnobs() {
    k1 = pod.knob1.Process();
    k2 = pod.knob2.Process();

    switch (current_menu)
    {
        case OSCILLATOR:
            ConditionalParameter(oldk1, k1, volume, osc_volume.Process());
            ConditionalParameter(oldk2, k2, tune, osc_tuning.Process());
            osc.SetAmp(volume);

            break;
        case FILTER:
            ConditionalParameter(oldk1, k1, cutoff, filter_cutoff.Process());
            ConditionalParameter(oldk2, k2, resonance, filter_resonance.Process());
            flt.SetFreq(cutoff);
            flt.SetRes(resonance);
            break;
        case ENVELOPE:
            ConditionalParameter(oldk1, k1, attack, ad_attack.Process());
            ConditionalParameter(oldk2, k2, decay, ad_decay.Process());
            ad.SetTime(ADENV_SEG_ATTACK, attack);
            ad.SetTime(ADENV_SEG_DECAY, decay);
            break;
        case DELAY:
            delay_rate = pod.knob1.Process();
            delay_feedback = pod.knob2.Process();
            break;
        case REVERB:
            ConditionalParameter(oldk1, k1, revFeedback, rev_feedback.Process());
            ConditionalParameter(oldk2, k2, revFilter, rev_filter.Process());
            rev.SetFeedback(revFeedback);
            rev.SetLpFreq(revFilter);
            break;
        default:
        break;
    }
    oldk1 = k1;
    oldk2 = k2;
}
void ProcessEncoder() {
    inc = 0;
    float incMultiplier = 1;
    float maxEncoderValue;
    float minEncoderValue;
    
    if(pod.encoder.RisingEdge())
        isCpuEnabled = !isCpuEnabled;

    switch (current_menu)
    {
        case OSCILLATOR:
            incMultiplier = 1;
            minEncoderValue = 0;
            maxEncoderValue = 7;
            inc = pod.encoder.Increment();
            wave_type += inc * incMultiplier;
            if (wave_type > maxEncoderValue)
                wave_type = maxEncoderValue;
            else if(wave_type < minEncoderValue)
                wave_type = minEncoderValue;
            osc.SetWaveform(wave_type);
        case FILTER:
            // drive = pod.encoder.Increment();
        case ENVELOPE:
            // envelope_level = pod.encoder.Increment();
        case DELAY:
            // delay_level = pod.encoder.Increment();
        case REVERB:
            incMultiplier = 0.1;
            minEncoderValue = 0;
            maxEncoderValue = 1;
            inc = pod.encoder.Increment();
            revLevel += inc * incMultiplier;
            if (revLevel > maxEncoderValue)
                revLevel = maxEncoderValue;
            else if(revLevel < minEncoderValue)
                revLevel = minEncoderValue;
        default:
        break;
    }
}

void GetReverbSample(float &inl, float &inr)
{
    float wetL, wetR;
    rev.Process(inl, inr, &wetL, &wetR);
    inl = wetL * revLevel;
    inr = wetR * revLevel;
}

void ProcessControls() {
    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();

    ProcessButtons();
    ProcessEncoder();
    ProcessKnobs();

    pod.encoder.Debounce();
    pod.button1.Debounce();
    pod.button2.Debounce();
}
void MidiUpdate(){
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
                }
                break;
                default: break;
            }
        }
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
        display.WriteString(bufferMin, Font_7x10, true);
        display.SetCursor(1, 19);
        display.WriteString(bufferAv, Font_7x10, true);
        display.SetCursor(1, 36);
        display.WriteString(bufferMax, Font_7x10, true);

        pod.seed.PrintLine(bufferMin);
        pod.seed.PrintLine(bufferAv);
        pod.seed.PrintLine(bufferMax);
        }
    else 
        {
        char menuLabel[16];
        char param1[32];
        char param2[32];
        char param3[32];
        const char* wave_types[] = {"Sine", "Triangle", "Saw", "Ramp", "Square", "Poliblep Tri", "Poliblep Sqr", "Last"};

        switch (current_menu)
        {
            case OSCILLATOR:
                snprintf(menuLabel, 16, "    OSC");
                snprintf(param1, 32, "Volume: %.1f", volume);
                snprintf(param2, 32, "Tune: %.1f", tune);
                snprintf(param3, sizeof(param3), "Wave: %s", wave_types[wave_type]);
                break;
            case FILTER:
                snprintf(menuLabel, 16, "   FILTER");
                snprintf(param1, 32, "Cutoff: %.1f Hz", cutoff);
                snprintf(param2, 32, "Res: %.1f", resonance);
                snprintf(param3, 32, "Level: %.1f", revLevel);
                break;
            case ENVELOPE:
                snprintf(menuLabel, 16, "    ENV");
                snprintf(param1, 32, "Attack: %.1f s", attack);
                snprintf(param2, 32, "Decay: %.1f s", decay);
                snprintf(param3, 32, "Level: %.1f", revLevel);
                break;
            case DELAY:
                snprintf(menuLabel, 16, "   DELAY");
                snprintf(param1, 32, "Rate: %.1f", delay_rate);
                snprintf(param2, 32, "Feedback: %.1f", delay_feedback);
                snprintf(param3, 32, "Level: %.1f", revLevel);
                break;
            case REVERB:
                snprintf(menuLabel, 16, "   REVERB");
                snprintf(param1, 32, "Feedback: %.1f", revFeedback);
                snprintf(param2, 32, "Filter: %.1f Hz", revFilter);
                snprintf(param3, 32, "Level: %.1f", revLevel);
                break;
            default:
                snprintf(menuLabel, 16, "Menu: UNKNOWN");
                param1[0] = '\0';
                param2[0] = '\0';
                break;
        }
        display.SetCursor(1, 0);
        display.WriteString(menuLabel, Font_11x18, true);
        display.SetCursor(1, 19);
        display.WriteString(param1, Font_7x10, true);
        display.SetCursor(1, 30);
        display.WriteString(param2, Font_7x10, true);
        display.SetCursor(1, 41);
        display.WriteString(param3, Font_7x10, true);

        }
    display.Update();
}
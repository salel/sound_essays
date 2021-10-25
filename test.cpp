#include <iostream>
#include <alsa/asoundlib.h>
#include <vector>
#define _USE_MATH_DEFINES
#include <cmath>
#include <string>
#include <fstream>
#include <csignal>
#include <thread>

#define PCM_DEVICE "default"

using namespace std;

void error(unsigned int e) {
    if (e < 0) {
        cout << snd_strerror(e) << endl;
        exit(0);
    }
}

float t_freq(float t, float freq) {
    return fmod(t*freq,1.f);
}

float square_wave(float t) {
    return t<0.5?1:-1;
}

float sine_wave(float t) {
    return sin(t*M_PI);
}

float saw_wave(float t) {
    return t*2-1;
}

float triangle_wave(float t) {
    return t<0.5?t*4-1:3-t*4;
}

int16_t convert(float s, float volume) {
    return (min(1.f, max(-1.f,s*volume)))*0x7FFE;
}

// build each root frequency of each note on a keyboard
vector<float> gen_keyboard() {
    vector<float> keyboard(88);
    // A0 freq
    const float a0 = 440.0/16.0;
    for (size_t i=0;i<keyboard.size();i++) {
        keyboard[i] = a0*pow(2, (float)i/12.0);
    }
    return keyboard;
}

int keyboard_note_index(const char* s) {
    int len = strlen(s);
    if (len == 3 && (tolower(s[1])!='b'&&s[1]!='#')) throw "malformed note";
    int mod = (len==2)?0:(s[1]=='#'?1:-1);

    char note = toupper(s[0]);
    char oct = (len==2)?s[1]:s[2];
    if (note < 'A' || note > 'G') throw "bad note";
    if (oct  < '0' || oct  > '7') throw "bad octave";
    vector<int> note_map = {0, 2, 3, 5, 7, 8, 10};
    int index = note_map[(note-'A')] + mod + (oct - '0')*12;
    if (index < 0 || index >= 88) throw "bad range";
    return index;
}

vector<int16_t> full_buffer;

// on sigint save wav
void signalHandler(int signum) {
    cout << full_buffer.size() << endl;

    ofstream file("output.wav", ios::out | ios::binary);

    file << "RIFF";

    int32_t file_size = full_buffer.size()*2 + 44;
    int32_t fmt_len = 16;
    int16_t fmt_type = 1;
    int16_t fmt_channels = 1;
    int32_t fmt_rate = 48000;
    int32_t fmt_bits_per_sample = 16;
    int32_t fmt_bytes_per_sample = fmt_bits_per_sample*fmt_channels/8;
    int32_t fmt_bytes_sec = fmt_rate*fmt_bytes_per_sample;
    int32_t data_size = full_buffer.size()*2;

    file.write((char*)&file_size, sizeof(int32_t));
    file << "WAVEfmt ";
    file.write((char*)&fmt_len, sizeof(int32_t));
    file.write((char*)&fmt_type, sizeof(int16_t));
    file.write((char*)&fmt_channels, sizeof(int16_t));
    file.write((char*)&fmt_rate, sizeof(int32_t));
    file.write((char*)&fmt_bytes_sec, sizeof(int32_t));
    file.write((char*)&fmt_bytes_per_sample, sizeof(int16_t));
    file.write((char*)&fmt_bits_per_sample, sizeof(int16_t));
    file << "data";
    file.write((char*)&data_size, sizeof(int32_t));

    file.write((char*)full_buffer.data(), full_buffer.size()*sizeof(int16_t));

    file.flush();

    exit(0);
}

int main() {
    signal(SIGINT, signalHandler);

    // Initialize the shits
    unsigned int rate = 48000;
    unsigned int channels = 1;

    snd_pcm_t *pcm_handle;

    error(snd_pcm_open(&pcm_handle, 
        PCM_DEVICE, SND_PCM_STREAM_PLAYBACK, 0));

    snd_pcm_hw_params_t *params;
    snd_pcm_hw_params_alloca(&params);
    snd_pcm_hw_params_any(pcm_handle, params);

    error(snd_pcm_hw_params_set_access(
        pcm_handle, params, SND_PCM_ACCESS_RW_INTERLEAVED));

    error(snd_pcm_hw_params_set_format(
        pcm_handle, params, SND_PCM_FORMAT_S16_LE));

    error(snd_pcm_hw_params_set_channels(
        pcm_handle, params, channels));

    error(snd_pcm_hw_params_set_rate_near(
        pcm_handle, params, &rate, 0));

    unsigned int periods = 4;

    error(snd_pcm_hw_params_set_periods_near(pcm_handle, params, &periods, 0));

    unsigned int period_time = 5e5;

    error(snd_pcm_hw_params_set_period_time(pcm_handle, params, period_time, 0));

    error(snd_pcm_hw_params(pcm_handle, params));

    snd_pcm_hw_params_get_channels(params, &channels);
    snd_pcm_hw_params_get_rate(params, &rate, 0);

    snd_pcm_uframes_t frames;
    snd_pcm_hw_params_get_period_size(params, &frames, 0);

    vector<int16_t> buffer(frames * channels);

    // ouch owie my ears
    float volume = 0.1;

    const auto kb = gen_keyboard();

    for (int loop = 0; true; loop++) {
        cout << loop << endl;

        for (size_t i=0;i<buffer.size();i++) {
            float t = (float)(loop*frames+i)/(float)rate;

            /*
            float val = 
                triangle_wave(t_freq(t, kb[keyboard_note_index("A2")]))
                /*
                sine_wave(t_freq(t, kb[keyboard_note_index("A3")]))+
                sine_wave(t_freq(t, kb[keyboard_note_index("E3")]))+
                sine_wave(t_freq(t, kb[keyboard_note_index("A4")]))+
                sine_wave(t_freq(t, kb[keyboard_note_index("C4")]))*/;

            float freq = pow(220, 1.0+t*0.1);
            float val = square_wave(t_freq(t,freq));

            buffer[i] = convert(val, volume);
        }

        full_buffer.insert(full_buffer.end(), buffer.begin(), buffer.end());

        int result = snd_pcm_writei(pcm_handle, buffer.data(), frames);
        error(result);
        if (result == -EPIPE) snd_pcm_prepare(pcm_handle);
    }

    snd_pcm_drain(pcm_handle);    
    snd_pcm_close(pcm_handle);
    return 0;
}

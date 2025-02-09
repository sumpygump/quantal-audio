#if !defined(DAISY_CONSTANTS_H)
#define DAISY_CONSTANTS_H 1

// Hypothetically the max number of channels that could be chained
constexpr float DAISY_DIVISOR = 16.f;

// How frequently the light draw step is processed
constexpr int DAISY_LIGHT_DIVISION = 512;

// How frequently the UI step is processed
constexpr int DAISY_UI_DIVISION = 128;

/**
 * Object to hold stereo polyphonic voltages
 */
struct StereoVoltages {
    int channels = 0;
    float voltages_l[16] = {};
    float voltages_r[16] = {};

    /**
     * Copies an array of size at least `channels` to this obj's voltages.
     * Remember to set the number of channels *before* calling this method.
     */
    void writeVoltages(const float* v_l, const float* v_r) {
        for (int c = 0; c < channels; c++) {
            voltages_l[c] = v_l[c];
            voltages_r[c] = v_r[c];
        }
    }

    void writeVoltages(const StereoVoltages sv, int channels) {
        this->channels = channels;
        for (int c = 0; c < channels; c++) {
            voltages_l[c] = sv.voltages_l[c];
            voltages_r[c] = sv.voltages_r[c];
        }
    }

    /**
     * Copies this objs voltages to an array of size at least `channels`
     */
    void sendVoltages(float* v_l, float* v_r) {
        for (int c = 0; c < channels; c++) {
            v_l[c] = voltages_l[c];
            v_r[c] = voltages_r[c];
        }
    }
};

struct DaisyMessage {
    // Daisy-chained mix signal
    StereoVoltages signals = {};

    // Single module's signal
    StereoVoltages singleSignals = {};

    // Aux 1 send signal
    StereoVoltages aux1Signals = {};

    // Aux 2 send signal
    StereoVoltages aux2Signals = {};

    // Solo signals
    StereoVoltages soloSignals = {};

    // Meta data about this daisy chain
    int channel_strip_id = 1;
    float first_pos_x = 0.0f;
    float first_pos_y = 0.0f;
};

#endif

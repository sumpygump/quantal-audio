#if !defined(DAISY_CONSTANTS_H)
#define DAISY_CONSTANTS_H 1

// Hypothetically the max number of channels that could be chained
constexpr float DAISY_DIVISOR = 16.f;

// How frequently the UI step is processed
constexpr int DAISY_UI_DIVISION = 128;

struct DaisyMessage {
    // Daisy-chained mix signal
    int channels = 1;
    float voltages_l[16] = {};
    float voltages_r[16] = {};

    // Single module's signal
    int single_channels = 1;
    float single_voltages_l[16] = {};
    float single_voltages_r[16] = {};

    // Aux 1 send signal
    int aux1_channels = 1;
    float aux1_voltages_l[16] = {};
    float aux1_voltages_r[16] = {};

    // Aux 2 send signal
    int aux2_channels = 1;
    float aux2_voltages_l[16] = {};
    float aux2_voltages_r[16] = {};

    // Meta data about this daisy chain
    float first_pos_x = 0.0f;
    float first_pos_y = 0.0f;
};

#endif
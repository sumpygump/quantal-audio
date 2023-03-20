#if !defined(DAISY_CONSTANTS_H)
#define DAISY_CONSTANTS_H 1

// Hypothetically the max number of channels that could be chained
const float DAISY_DIVISOR = 16.f;

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
};

#endif

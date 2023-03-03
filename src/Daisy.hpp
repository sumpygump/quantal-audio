
struct DaisyMessage {
    int channels;
    float voltages_l[16];
    float voltages_r[16];

    DaisyMessage() {
        // Init defaults
        channels = 1;
        for (int c = 0; c < 16; c++) {
            voltages_l[c] = 0.0f;
            voltages_r[c] = 0.0f;
        }
    }
};

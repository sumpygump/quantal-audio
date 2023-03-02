#include "QuantalAudio.hpp"

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

struct DaisyMaster2 : Module {
    enum ParamIds {
        MIX_LVL_PARAM,
        MUTE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        MIX_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        MIX_OUTPUT_1, // Left
        MIX_OUTPUT_2, // Right
        NUM_OUTPUTS
    };
    enum LightsIds {
        MUTE_LIGHT,
        LINK_LIGHT_L,
        NUM_LIGHTS
    };

    // Hypothetically the max number of channels that could be chained
    // Needs to match the divisor in the daisy channel class
    float DAISY_DIVISOR = 16.f;
    bool muted = false;
    float link_l = 0.f;
    dsp::SchmittTrigger muteTrigger;

    DaisyMessage daisyMessages[2][1];

    DaisyMaster2() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(MIX_LVL_PARAM, 0.0f, 2.0f, 1.0f, "Mix level", " dB", -10, 20);
        configButton(MUTE_PARAM, "Mute");

        configInput(MIX_CV_INPUT, "Level CV");
        configOutput(MIX_OUTPUT_1, "Mix L");
        configOutput(MIX_OUTPUT_2, "Mix R");

        configLight(LINK_LIGHT_L, "Daisy chain link input");

        // Set the left expander message instances
        leftExpander.producerMessage = daisyMessages[0];
        leftExpander.consumerMessage = daisyMessages[1];
    }

    json_t *dataToJson() override {
        json_t *rootJ = json_object();

        // mute
        json_object_set_new(rootJ, "muted", json_boolean(muted));

        return rootJ;
    }

    void dataFromJson(json_t *rootJ) override {
        // mute
        json_t *mutedJ = json_object_get(rootJ, "muted");
        if (mutedJ)
            muted = json_is_true(mutedJ);
    }

    void process(const ProcessArgs &args) override {
        muted = params[MUTE_PARAM].getValue() > 0.f;

        int channels = 1;
        float mix_l[16] = {};
        float mix_r[16] = {};

        if (!muted) {
            // Get daisy-chained data from left-side linked module
            if (leftExpander.module && (leftExpander.module->model == modelDaisyChannel2 || leftExpander.module->model == modelDaisyChannelSends2)) {
                DaisyMessage *msgFromExpander = (DaisyMessage*)(leftExpander.module->rightExpander.consumerMessage);

                channels = msgFromExpander->channels;
                for (int c = 0; c < channels; c++) {
                    mix_l[c] = msgFromExpander->voltages_l[c];
                    mix_r[c] = msgFromExpander->voltages_r[c];
                }

                link_l = 0.8f;
            } else {
                link_l = 0.0f;
            }

            float gain = params[MIX_LVL_PARAM].getValue();

            // Bring the voltage back up from the chained low voltage
            for (int c = 0; c < channels; c++) {
                mix_l[c] = clamp(mix_l[c] * DAISY_DIVISOR, -12.f, 12.f) * gain;
                mix_r[c] = clamp(mix_r[c] * DAISY_DIVISOR, -12.f, 12.f) * gain;
            }

            float mix_cv = 1.f;
            if (inputs[MIX_CV_INPUT].isConnected()) {
                mix_cv = clamp(inputs[MIX_CV_INPUT].getVoltage() / 10.f, 0.f, 1.f);
                for (int c = 0; c < channels; c++) {
                    mix_l[c] *= mix_cv;
                    mix_r[c] *= mix_cv;
                }
            }
        }

        outputs[MIX_OUTPUT_1].setChannels(channels);
        outputs[MIX_OUTPUT_1].writeVoltages(mix_l);
        outputs[MIX_OUTPUT_2].setChannels(channels);
        outputs[MIX_OUTPUT_2].writeVoltages(mix_r);

        lights[MUTE_LIGHT].value = (muted);
        lights[LINK_LIGHT_L].setBrightness(link_l);
    }
};

struct DaisyMasterWidget2 : ModuleWidget {
    DaisyMasterWidget2(DaisyMaster2 *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DaisyMaster2.svg")));

        // Screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Level & CV
        addParam(createParam<RoundLargeBlackKnob>(Vec(RACK_GRID_WIDTH * 1.5 - (36.0 / 2), 52.0), module, DaisyMaster2::MIX_LVL_PARAM));
        addInput(createInput<PJ301MPort>(Vec(RACK_GRID_WIDTH * 1.5 - (25.0 / 2), 96.0), module, DaisyMaster2::MIX_CV_INPUT));

        // Mute
        addParam(createLightParam<VCVLightLatch<MediumSimpleLight<RedLight>>>(Vec(RACK_GRID_WIDTH * 1.5 - 9.0, 254.0), module, DaisyMaster2::MUTE_PARAM, DaisyMaster2::MUTE_LIGHT));
        // addParam(createParam<LEDButton>(Vec(RACK_GRID_WIDTH * 1.5 - 9.0, 254.0), module, DaisyMaster2::MUTE_PARAM));
        // addChild(createLight<MediumLight<RedLight>>(Vec(RACK_GRID_WIDTH * 1.5 - 4.5, 258.25f), module, DaisyMaster2::MUTE_LIGHT));

        // Mix output
        addOutput(createOutput<PJ301MPort>(Vec((RACK_GRID_WIDTH * 1.5) - (25.0 / 2), 290.0), module, DaisyMaster2::MIX_OUTPUT_1));
        addOutput(createOutput<PJ301MPort>(Vec((RACK_GRID_WIDTH * 1.5) - (25.0 / 2), 316.0), module, DaisyMaster2::MIX_OUTPUT_2));

        // Link light
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH - 6, 361.0f), module, DaisyMaster2::LINK_LIGHT_L));
    }
};

Model *modelDaisyMaster2 = createModel<DaisyMaster2, DaisyMasterWidget2>("DaisyMaster2");

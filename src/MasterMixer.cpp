#include "QuantalAudio.hpp"
#include "Daisy.hpp"

constexpr float SLEW_SPEED = 6.f; // For smoothing out CV

struct MasterMixer : Module {
    enum ParamIds {
        MIX_LVL_PARAM,
        MONO_PARAM,
        ENUMS(LVL_PARAM, 2),
        NUM_PARAMS
    };
    enum InputIds {
        MIX_CV_INPUT,
        ENUMS(CH_INPUT, 2),
        NUM_INPUTS
    };
    enum OutputIds {
        MIX_OUTPUT,
        MIX_OUTPUT_2,
        ENUMS(CH_OUTPUT, 2),
        NUM_OUTPUTS
    };

    bool levelSlew = true;
    SimpleSlewer levelSlewer;

    MasterMixer() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS);
        configParam(MIX_LVL_PARAM, 0.0f, 2.0f, 1.0f, "Mix level", " dB", -10, 20);
        configSwitch(MONO_PARAM, 0.0f, 1.0f, 1.0f, "Mode", {"Stereo", "Mono"});
        configParam(LVL_PARAM + 0, 0.0f, M_SQRT2, 1.0f, "Channel 1 level", " dB", -10, 40);
        configParam(LVL_PARAM + 1, 0.0f, M_SQRT2, 1.0f, "Channel 2 level", " dB", -10, 40);

        configInput(MIX_CV_INPUT, "Mix CV");
        configInput(CH_INPUT + 0, "Channel 1");
        configInput(CH_INPUT + 1, "Channel 2");

        configOutput(CH_OUTPUT + 0, "Channel 1");
        configOutput(CH_OUTPUT + 1, "Channel 2");
        configOutput(MIX_OUTPUT, "Mix 1");
        configOutput(MIX_OUTPUT_2, "Mix 2");
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_object_set_new(rootJ, "level_slew", json_boolean(levelSlew));

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        // level slew
        const json_t* levelSlewJ = json_object_get(rootJ, "level_slew");
        if (levelSlewJ) {
            levelSlew = json_is_true(levelSlewJ);
        }
    }

    /**
     * When user resets this module
     */
    void onReset() override {
        levelSlew = true;
    }

    void onSampleRateChange() override {
        levelSlewer.setSlewSpeed(SLEW_SPEED);
    }

    void process(const ProcessArgs &args) override {
        float mix[16] = {};
        float mix_cv[16] = {};
        float mix_out[2][16] = {};

        int maxChannels = 1;
        const bool is_mono = (params[MONO_PARAM].getValue() > 0.0f);
        const float master_gain = params[MIX_LVL_PARAM].getValue();

        for (int i = 0; i < 2; i++) {
            int channels = 1;
            float ch[16] = {};

            if (inputs[CH_INPUT + i].isConnected()) {
                channels = inputs[CH_INPUT + i].getChannels();
                maxChannels = std::max(maxChannels, channels);

                inputs[CH_INPUT + i].readVoltages(ch);

                const float gain = std::pow(params[LVL_PARAM + i].getValue(), 2.f);
                for (int c = 0; c < channels; c++) {
                    ch[c] *= gain;
                    mix[c] += ch[c];
                    mix_out[i][c] += ch[c];
                }
            }

            if (outputs[CH_OUTPUT + i].isConnected()) {
                outputs[CH_OUTPUT + i].setChannels(channels);
                outputs[CH_OUTPUT + i].writeVoltages(ch);
            }
        }

        // Gather poly values from CV input
        if (inputs[MIX_CV_INPUT].isConnected()) {
            for (int c = 0; c < maxChannels; c++) {
                mix_cv[c] = clamp(inputs[MIX_CV_INPUT].getPolyVoltage(c) / 10.f, 0.f, 1.f);
                if (levelSlew) {
                    mix_cv[c] = levelSlewer.process(mix_cv[c]);
                }
            }
        } else {
            for (int c = 0; c < maxChannels; c++) {
                mix_cv[c] = 1.f;
            }
        }

        if (!is_mono && (inputs[CH_INPUT + 0].isConnected() && inputs[CH_INPUT + 1].isConnected())) {
            // If the ch 2 jack is active use stereo mode
            for (int c = 0; c < maxChannels; c++) {
                const float attenuate = master_gain * mix_cv[c];
                mix_out[0][c] *= attenuate;
                mix_out[1][c] *= attenuate;
            }
            outputs[MIX_OUTPUT].setChannels(maxChannels);
            outputs[MIX_OUTPUT].writeVoltages(mix_out[0]);
            outputs[MIX_OUTPUT_2].setChannels(maxChannels);
            outputs[MIX_OUTPUT_2].writeVoltages(mix_out[1]);
        } else {
            // Otherwise use mono->stereo mode
            for (int c = 0; c < maxChannels; c++) {
                const float attenuate = master_gain * mix_cv[c];
                mix[c] *= attenuate;
            }
            outputs[MIX_OUTPUT].setChannels(maxChannels);
            outputs[MIX_OUTPUT].writeVoltages(mix);
            outputs[MIX_OUTPUT_2].setChannels(maxChannels);
            outputs[MIX_OUTPUT_2].writeVoltages(mix);
        }
    }
};

struct MasterMixerWidget : ModuleWidget {
    explicit MasterMixerWidget(MasterMixer *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/MasterMixer.svg"),
                asset::plugin(pluginInstance, "res/MasterMixer-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Level & CV
        addParam(createParam<RoundLargeBlackKnob>(Vec(RACK_GRID_WIDTH * 2.5f - (38.0f / 2), 52.0), module, MasterMixer::MIX_LVL_PARAM));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH * 2.5f - (25.0f / 2), 96.0), module, MasterMixer::MIX_CV_INPUT));

        // Mono/stereo switch
        addParam(createParam<CKSS>(Vec(RACK_GRID_WIDTH * 2.5f - 7.0f, 162.0f), module, MasterMixer::MONO_PARAM));

        // LED faders
        addParam(createParam<LEDSliderGreen>(Vec(RACK_GRID_WIDTH * 2.5f - (21.0f + 7.0f), 130.4), module, MasterMixer::LVL_PARAM + 0));
        addParam(createParam<LEDSliderGreen>(Vec(RACK_GRID_WIDTH * 2.5f + 7.0f, 130.4), module, MasterMixer::LVL_PARAM + 1));

        // Channel inputs
        addInput(createInput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5f) - (25.0f + 5.0f), 232.0), module, MasterMixer::CH_INPUT + 0));
        addInput(createInput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5f) + 5.0f, 232.0), module, MasterMixer::CH_INPUT + 1));

        // Channel outputs
        addOutput(createOutput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5f) - (25.0f + 5.0f), 276.0), module, MasterMixer::CH_OUTPUT + 0));
        addOutput(createOutput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5f) + 5.0f, 276.0), module, MasterMixer::CH_OUTPUT + 1));

        // Mix outputs
        addOutput(createOutput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5f) - (25.0f + 5.0f), 320.0), module, MasterMixer::MIX_OUTPUT));
        addOutput(createOutput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 2.5f) + 5.0f, 320.0), module, MasterMixer::MIX_OUTPUT_2));
    }

    void appendContextMenu(Menu *menu) override {
        MasterMixer* module = getModule<MasterMixer>();

        menu->addChild(new MenuSeparator);
        menu->addChild(createBoolPtrMenuItem("Smooth level CV", "", &module->levelSlew));
    }
};

Model* modelMasterMixer = createModel<MasterMixer, MasterMixerWidget>("Mixer2");
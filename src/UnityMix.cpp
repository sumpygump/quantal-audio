#include "QuantalAudio.hpp"

struct polysignal {
    float signals[16] = {};
    int channels = 1;
};

struct UnityMix : Module {
    enum ParamIds {
        CONNECT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        ENUMS(CH_INPUT, 6),
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(CH_OUTPUT, 2),
        NUM_OUTPUTS
    };
    enum LightsIds {
        NUM_LIGHTS
    };

    UnityMix() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(CONNECT_PARAM, 0.0f, 1.0f, 1.0f, "Connect mode", {"Group All (6:1)", "Groups A, B (3:1 x 2)"});

        configInput(CH_INPUT + 0, "A1");
        configInput(CH_INPUT + 1, "A2");
        configInput(CH_INPUT + 2, "A3");
        configInput(CH_INPUT + 3, "B1");
        configInput(CH_INPUT + 4, "B2");
        configInput(CH_INPUT + 5, "B3");

        configOutput(CH_OUTPUT + 0, "Group A");
        configOutput(CH_OUTPUT + 1, "Group B");
    }

    polysignal mixchannels(int in_start, int in_end) {
        float in[16] = {};
        struct polysignal mix;
        float channels = 0.f;
        int maxChannels = 1;

        for (int i = in_start; i <= in_end; i++) {
            if (inputs[CH_INPUT + i].isConnected()) {
                mix.channels = inputs[CH_INPUT + i].getChannels();
                maxChannels = std::max(mix.channels, maxChannels);
                inputs[CH_INPUT + i].readVoltages(in);

                for (int c = 0; c < mix.channels; c++) {
                    mix.signals[c] += in[c];
                }

                // Keep track of number of plugs so we can average
                channels++;
            }
        }

        if (channels > 0.f) {
            mix.channels = maxChannels;
            for (int c = 0; c < maxChannels; c++) {
                mix.signals[c] = mix.signals[c] / channels;
            }
        }

        return mix;
    }

    void process(const ProcessArgs &args) override {
        bool unconnect = (params[CONNECT_PARAM].getValue() > 0.0f);

        if (unconnect) {
            // Group A : Inputs 0 1 2 -> Output 0
            polysignal mix1 = mixchannels(0, 2);
            outputs[CH_OUTPUT + 0].setChannels(mix1.channels);
            outputs[CH_OUTPUT + 0].writeVoltages(mix1.signals);
            // Group B : Inputs 3 4 5 -> Output 1
            polysignal mix2 = mixchannels(3, 5);
            outputs[CH_OUTPUT + 1].setChannels(mix2.channels);
            outputs[CH_OUTPUT + 1].writeVoltages(mix2.signals);
        } else {
            // Combined : Inputs 0-5 -> Output 1 & 2
            polysignal mix = mixchannels(0, 5);
            outputs[CH_OUTPUT + 0].setChannels(mix.channels);
            outputs[CH_OUTPUT + 0].writeVoltages(mix.signals);
            outputs[CH_OUTPUT + 1].setChannels(mix.channels);
            outputs[CH_OUTPUT + 1].writeVoltages(mix.signals);
        }
    }
};

struct UnityMixWidget : ModuleWidget {
    UnityMixWidget(UnityMix *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/UnityMix.svg"),
                asset::plugin(pluginInstance, "res/UnityMix-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Connect switch
        addParam(createParam<CKSS>(Vec(RACK_GRID_WIDTH - 7.0, 182.0), module, UnityMix::CONNECT_PARAM));

        // Group A
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 50.0), module, UnityMix::CH_INPUT + 0));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 78.0), module, UnityMix::CH_INPUT + 1));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 106.0), module, UnityMix::CH_INPUT + 2));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 148.0), module, UnityMix::CH_OUTPUT + 0));

        // Group B
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 222.0), module, UnityMix::CH_INPUT + 3));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 250.0), module, UnityMix::CH_INPUT + 4));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 278.0), module, UnityMix::CH_INPUT + 5));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 320.0), module, UnityMix::CH_OUTPUT + 1));
    }
};

Model *modelUnityMix = createModel<UnityMix, UnityMixWidget>("UnityMix");

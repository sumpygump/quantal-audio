#include "QuantalAudio.hpp"

struct BufferedMult : Module {
    enum ParamIds {
        CONNECT_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        ENUMS(CH_INPUT, 2),
        NUM_INPUTS
    };
    enum OutputIds {
        ENUMS(CH_OUTPUT, 6),
        NUM_OUTPUTS
    };
    enum LightsIds {
        NUM_LIGHTS
    };

    BufferedMult() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configSwitch(CONNECT_PARAM, 0.0f, 1.0f, 1.0f, "connect mode", {"Group All (2:6)", "Groups A, B (1:3 x 2)"});

        configInput(CH_INPUT + 0, "A");
        configInput(CH_INPUT + 1, "B");

        configOutput(CH_OUTPUT + 0, "A1");
        configOutput(CH_OUTPUT + 1, "A2");
        configOutput(CH_OUTPUT + 2, "A3");
        configOutput(CH_OUTPUT + 3, "B1");
        configOutput(CH_OUTPUT + 4, "B2");
        configOutput(CH_OUTPUT + 5, "B3");
    }

    void process(const ProcessArgs &args) override {
        bool unconnect = (params[CONNECT_PARAM].getValue() > 0.0f);

        // Input 0 -> Outputs 0 1 2
        float signals[16] = {};
        inputs[CH_INPUT + 0].readVoltages(signals);
        int channels = inputs[CH_INPUT + 0].getChannels();
        for (int i = 0; i <= 2; i++) {
            outputs[CH_OUTPUT + i].setChannels(channels);
            outputs[CH_OUTPUT + i].writeVoltages(signals);
        }

        // Input 1 -> Outputs 3 4 5
        // otherwise Outputs 3 4 5 is copy of Input 0
        if (unconnect) {
            inputs[CH_INPUT + 1].readVoltages(signals);
            channels = inputs[CH_INPUT + 1].getChannels();
        }

        for (int i = 3; i <= 5; i++) {
            outputs[CH_OUTPUT + i].setChannels(channels);
            outputs[CH_OUTPUT + i].writeVoltages(signals);
        }
    }
};

struct BufferedMultWidget : ModuleWidget {
    BufferedMultWidget(BufferedMult *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/BufferedMult.svg"),
                asset::plugin(pluginInstance, "res/BufferedMult-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Connect switch
        addParam(createParam<CKSS>(Vec(RACK_GRID_WIDTH - 7.0, 182.0), module, BufferedMult::CONNECT_PARAM));

        // Group A
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 50.0), module, BufferedMult::CH_INPUT + 0));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 92.0), module, BufferedMult::CH_OUTPUT + 0));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 120.0), module, BufferedMult::CH_OUTPUT + 1));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 148.0), module, BufferedMult::CH_OUTPUT + 2));

        // Group B
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 222.0), module, BufferedMult::CH_INPUT + 1));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 264.0), module, BufferedMult::CH_OUTPUT + 3));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 292.0), module, BufferedMult::CH_OUTPUT + 4));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 320.0), module, BufferedMult::CH_OUTPUT + 5));
    }
};

Model *modelBufferedMult = createModel<BufferedMult, BufferedMultWidget>("BufferedMult");

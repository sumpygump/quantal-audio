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
        configParam(CONNECT_PARAM, 0.0f, 1.0f, 1.0f);
    }

    void process(const ProcessArgs &args) override {
        bool unconnect = (params[CONNECT_PARAM].getValue() > 0.0f);

        // Input 0 -> Outputs 0 1 2
        float ch = inputs[CH_INPUT + 0].getVoltage();
        for (int i = 0; i <= 2; i++) {
            outputs[CH_OUTPUT + i].setVoltage(ch);
        }

        // Input 1 -> Outputs 3 4 5
        // otherwise Outputs 3 4 5 is copy of Input 0
        if (unconnect) {
            ch = inputs[CH_INPUT + 1].getVoltage();
        }

        for (int i = 3; i <= 5; i++) {
            outputs[CH_OUTPUT + i].setVoltage(ch);
        }
    }
};

struct BufferedMultWidget : ModuleWidget {
    BufferedMultWidget(BufferedMult *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/BufferedMult.svg")));

        // Screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Connect switch
        addParam(createParam<CKSS>(Vec(RACK_GRID_WIDTH - 7.0, 182.0), module, BufferedMult::CONNECT_PARAM));

        // Group A
        addInput(createInput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 50.0), module, BufferedMult::CH_INPUT + 0));
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 92.0), module, BufferedMult::CH_OUTPUT + 0));
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 120.0), module, BufferedMult::CH_OUTPUT + 1));
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 148.0), module, BufferedMult::CH_OUTPUT + 2));

        // Group B
        addInput(createInput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 222.0), module, BufferedMult::CH_INPUT + 1));
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 264.0), module, BufferedMult::CH_OUTPUT + 3));
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 292.0), module, BufferedMult::CH_OUTPUT + 4));
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 320.0), module, BufferedMult::CH_OUTPUT + 5));
    }
};

Model *modelBufferedMult = createModel<BufferedMult, BufferedMultWidget>("BufferedMult");

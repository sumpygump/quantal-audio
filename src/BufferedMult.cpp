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
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);}

    void process(const ProcessArgs &args) override {
        bool unconnect = (params[CONNECT_PARAM].value > 0.0f);

        // Input 0 -> Outputs 0 1 2
        float ch = inputs[CH_INPUT + 0].value;
        for (int i = 0; i <= 2; i++) {
            outputs[CH_OUTPUT + i].value = ch;
        }

        // Input 1 -> Outputs 3 4 5
        // otherwise Outputs 3 4 5 is copy of Input 0
        if (unconnect) {
            ch = inputs[CH_INPUT + 1].value;
        }

        for (int i = 3; i <= 5; i++) {
            outputs[CH_OUTPUT + i].value = ch;
        }
    }
};

struct BufferedMultWidget : ModuleWidget {
    BufferedMultWidget(BufferedMult *module) {
        setModule(module);
        setPanel(SVG::load(assetPlugin(pluginInstance, "res/BufferedMult.svg")));

        // Screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Connect switch
        addParam(createParam<CKSS>(Vec(RACK_GRID_WIDTH - 7.0, 182.0), module, BufferedMult::CONNECT_PARAM, 0.0f, 1.0f, 1.0f));

        // Group A
        addInput(createPort<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 50.0), PortWidget::INPUT, module, BufferedMult::CH_INPUT + 0));
        addOutput(createPort<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 92.0), PortWidget::OUTPUT, module, BufferedMult::CH_OUTPUT + 0));
        addOutput(createPort<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 120.0), PortWidget::OUTPUT, module, BufferedMult::CH_OUTPUT + 1));
        addOutput(createPort<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 148.0), PortWidget::OUTPUT, module, BufferedMult::CH_OUTPUT + 2));

        // Group B
        addInput(createPort<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 222.0), PortWidget::INPUT, module, BufferedMult::CH_INPUT + 1));
        addOutput(createPort<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 264.0), PortWidget::OUTPUT, module, BufferedMult::CH_OUTPUT + 3));
        addOutput(createPort<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 292.0), PortWidget::OUTPUT, module, BufferedMult::CH_OUTPUT + 4));
        addOutput(createPort<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 320.0), PortWidget::OUTPUT, module, BufferedMult::CH_OUTPUT + 5));
    }
};

Model *modelBufferedMult = createModel<BufferedMult, BufferedMultWidget>("BufferedMult");

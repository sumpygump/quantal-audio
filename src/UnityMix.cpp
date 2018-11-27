#include "QuantalAudio.hpp"

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

    UnityMix() : Module(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS) {}

    void step() override {
        bool unconnect = (params[CONNECT_PARAM].value > 0.0f);

        // Group A
        // Inputs 0 1 2 -> Output 0
        float mix = 0.f;
        float channels = 0.f;
        for (int i = 0; i <= 2; i++) {
            if (inputs[CH_INPUT + i].active) {
                mix += inputs[CH_INPUT + i].normalize(0.0);
                channels += 0.9f;
            }
        }
        if (channels > 0.f)
            mix = mix / channels;
        outputs[CH_OUTPUT + 0].value = mix;

        // Inputs 3 4 5 -> Output 1
        // otherwise Inputs 3 4 5 are also summed and put in output 1
        if (unconnect) {
            mix = 0.f;
            channels = 0.f;
        }

        // Group B
        bool add_group_b = false;
        for (int i = 3; i <= 5; i++) {
            if (inputs[CH_INPUT + i].active) {
                mix += inputs[CH_INPUT + i].normalize(0.0);
                channels += 0.9f;
                add_group_b = true;
            }
        }
        if (add_group_b && channels > 0.f)
            mix = mix / channels;
        outputs[CH_OUTPUT + 1].value = mix;

        if (!unconnect) {
            outputs[CH_OUTPUT + 0].value = mix;
        }
    }
};

struct UnityMixWidget : ModuleWidget {
    UnityMixWidget(UnityMix *module) : ModuleWidget(module) {
        setPanel(SVG::load(assetPlugin(plugin, "res/UnityMix.svg")));

        // Screws
        addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(Widget::create<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Connect switch
        addParam(ParamWidget::create<CKSS>(Vec(RACK_GRID_WIDTH - 7.0, 182.0), module, UnityMix::CONNECT_PARAM, 0.0f, 1.0f, 1.0f));

        // Group A
        addInput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 50.0), Port::INPUT, module, UnityMix::CH_INPUT + 0));
        addInput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 78.0), Port::INPUT, module, UnityMix::CH_INPUT + 1));
        addInput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 106.0), Port::INPUT, module, UnityMix::CH_INPUT + 2));
        addOutput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 148.0), Port::OUTPUT, module, UnityMix::CH_OUTPUT + 0));

        // Group B
        addInput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 222.0), Port::INPUT, module, UnityMix::CH_INPUT + 3));
        addInput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 250.0), Port::INPUT, module, UnityMix::CH_INPUT + 4));
        addInput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 278.0), Port::INPUT, module, UnityMix::CH_INPUT + 5));
        addOutput(Port::create<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 320.0), Port::OUTPUT, module, UnityMix::CH_OUTPUT + 1));
    }
};

Model *modelUnityMix = Model::create<UnityMix, UnityMixWidget>("QuantalAudio", "UnityMix", "Unity Mix | 2HP", MULTIPLE_TAG);

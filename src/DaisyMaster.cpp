#include "QuantalAudio.hpp"
#include "Daisy.hpp"

struct DaisyMaster : Module {
    enum ParamIds {
        MIX_LVL_PARAM,
        MUTE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        MIX_CV_INPUT,
        CHAIN_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        MIX_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightsIds {
        MUTE_LIGHT,
        NUM_LIGHTS
    };

    bool muted = false;
    dsp::SchmittTrigger muteTrigger;

    DaisyMaster() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(MIX_LVL_PARAM, 0.0f, 2.0f, 1.0f, "Mix level", " dB", -10, 20);
        configButton(MUTE_PARAM, "Mute");

        configInput(MIX_CV_INPUT, "Level CV");
        configInput(CHAIN_INPUT, "Daisy Channel");
        configOutput(MIX_OUTPUT, "Mix");
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        // mute
        json_object_set_new(rootJ, "muted", json_boolean(muted));

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        // mute
        json_t* mutedJ = json_object_get(rootJ, "muted");
        if (mutedJ) {
            muted = json_is_true(mutedJ);
        }
    }

    void process(const ProcessArgs &args) override {
        if (muteTrigger.process(params[MUTE_PARAM].getValue())) {
            muted = !muted;
        }

        float mix[16] = {};
        int channels = 1;

        if (!muted) {
            float gain = params[MIX_LVL_PARAM].getValue();

            channels = inputs[CHAIN_INPUT].getChannels();

            // Bring the voltage back up from the chained low voltage
            inputs[CHAIN_INPUT].readVoltages(mix);
            for (int c = 0; c < channels; c++) {
                mix[c] = clamp(mix[c] * DAISY_DIVISOR, -12.f, 12.f) * gain;
            }

            // mix = clamp(inputs[CHAIN_INPUT].getVoltage() * DAISY_DIVISOR, -12.f, 12.f);

            if (inputs[MIX_CV_INPUT].isConnected()) {
                for (int c = 0; c < channels; c++) {
                    const float mix_cv = clamp(inputs[MIX_CV_INPUT].getPolyVoltage(c) / 10.f, 0.f, 1.f);
                    mix[c] *= mix_cv;
                }
            }
        }

        outputs[MIX_OUTPUT].setChannels(channels);
        outputs[MIX_OUTPUT].writeVoltages(mix);
        lights[MUTE_LIGHT].value = (muted);
    }
};

struct DaisyMasterWidget : ModuleWidget {
    explicit DaisyMasterWidget(DaisyMaster *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DaisyMaster.svg")));
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/DaisyMaster.svg"),
                asset::plugin(pluginInstance, "res/DaisyMaster-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Level & CV
        addParam(createParam<RoundLargeBlackKnob>(Vec(RACK_GRID_WIDTH * 1.5f - (36.0f / 2), 52.0), module, DaisyMaster::MIX_LVL_PARAM));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH * 1.5f - (25.0f / 2), 96.0), module, DaisyMaster::MIX_CV_INPUT));

        // Mute
        addParam(createParam<LEDButton>(Vec(RACK_GRID_WIDTH * 1.5f - 9.0f, 206.0), module, DaisyMaster::MUTE_PARAM));
        addChild(createLight<MediumLight<RedLight>>(Vec(RACK_GRID_WIDTH * 1.5f - 4.5f, 210.25f), module, DaisyMaster::MUTE_LIGHT));

        // Mix output
        addOutput(createOutput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 1.5f) - (25.0f / 2), 245.0), module, DaisyMaster::MIX_OUTPUT));

        // Chain input
        addInput(createInput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 1.5f) - (25.0f / 2), 290.5), module, DaisyMaster::CHAIN_INPUT));
    }
};

Model* modelDaisyMaster = createModel<DaisyMaster, DaisyMasterWidget>("DaisyMaster");

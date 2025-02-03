#include "QuantalAudio.hpp"
#include "Daisy.hpp"

struct DaisyChannel : Module {
    enum ParamIds {
        CH_LVL_PARAM,
        MUTE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH_INPUT,
        LVL_CV_INPUT,
        CHAIN_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CH_OUTPUT,
        CHAIN_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightsIds {
        MUTE_LIGHT,
        NUM_LIGHTS
    };

    bool muted = false;
    dsp::SchmittTrigger muteTrigger;

    DaisyChannel() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CH_LVL_PARAM, 0.0f, 1.0f, 1.0f, "Channel level", " dB", -10, 20);
        configButton(MUTE_PARAM, "Mute");

        configInput(CH_INPUT, "Channel");
        configInput(LVL_CV_INPUT, "Level CV");
        configInput(CHAIN_INPUT, "Daisy chain");

        configOutput(CH_OUTPUT, "Channel");
        configOutput(CHAIN_OUTPUT, "Daisy chain");
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

        float signals[16] = {};
        int channels = 1;
        if (!muted) {
            channels = inputs[CH_INPUT].getChannels();
            inputs[CH_INPUT].readVoltages(signals);
            const float gain = params[CH_LVL_PARAM].getValue();
            for (int c = 0; c < channels; c++) {
                signals[c] *= std::pow(gain, 2.f);
            }

            if (inputs[LVL_CV_INPUT].isConnected()) {
                for (int c = 0; c < channels; c++) {
                    const float _cv = clamp(inputs[LVL_CV_INPUT].getPolyVoltage(c) / 10.f, 0.f, 1.f);
                    signals[c] *= _cv;
                }
            }
        }

        outputs[CH_OUTPUT].setChannels(channels);
        outputs[CH_OUTPUT].writeVoltages(signals);

        float daisySignals[16] = {};
        const int maxChannels = std::max(inputs[CHAIN_INPUT].getChannels(), channels);

        // Make the voltage small to the chain by dividing by the divisor;
        inputs[CHAIN_INPUT].readVoltages(daisySignals);
        for (int c = 0; c < maxChannels; c++) {
            daisySignals[c] += signals[c] / DAISY_DIVISOR;
        }

        outputs[CHAIN_OUTPUT].setChannels(maxChannels);
        outputs[CHAIN_OUTPUT].writeVoltages(daisySignals);

        lights[MUTE_LIGHT].value = (muted);
    }
};

struct DaisyChannelWidget : ModuleWidget {
    explicit DaisyChannelWidget(DaisyChannel *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/DaisyChannel.svg"),
                asset::plugin(pluginInstance, "res/DaisyChannel-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Channel Input/Output
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 50.0), module, DaisyChannel::CH_INPUT));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 245.0), module, DaisyChannel::CH_OUTPUT));

        // Level & CV
        addParam(createParam<LEDSliderGreen>(Vec(RACK_GRID_WIDTH - 10.5f, 121.4), module, DaisyChannel::CH_LVL_PARAM));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 89.0), module, DaisyChannel::LVL_CV_INPUT));

        // Mute
        addParam(createParam<LEDButton>(Vec(RACK_GRID_WIDTH - 9.0f, 206.0), module, DaisyChannel::MUTE_PARAM));
        addChild(createLight<MediumLight<RedLight>>(Vec(RACK_GRID_WIDTH - 4.5f, 210.25f), module, DaisyChannel::MUTE_LIGHT));

        // Chain Input/Output
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 290.5), module, DaisyChannel::CHAIN_INPUT));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 320.0), module, DaisyChannel::CHAIN_OUTPUT));
    }
};

Model* modelDaisyChannel = createModel<DaisyChannel, DaisyChannelWidget>("DaisyChannel");
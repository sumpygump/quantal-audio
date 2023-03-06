#include "QuantalAudio.hpp"
#include "Daisy.hpp"

struct DaisyChannelSends2 : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        CH_OUTPUT_1, // Left
        CH_OUTPUT_2, // Right
        NUM_OUTPUTS
    };
    enum LightsIds {
        LINK_LIGHT_L,
        LINK_LIGHT_R,
        NUM_LIGHTS
    };

    bool muted = false;
    float link_l = 0.f;
    float link_r = 0.f;
    dsp::ClockDivider lightDivider;

    DaisyMessage daisyInputMessage[2][1];
    DaisyMessage daisyOutputMessage[2][1];

    DaisyChannelSends2() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configOutput(CH_OUTPUT_1, "Channel L");
        configOutput(CH_OUTPUT_2, "Channel R");

        configLight(LINK_LIGHT_L, "Daisy chain link input");
        configLight(LINK_LIGHT_R, "Daisy chain link output");

        // Set the expander messages
        leftExpander.producerMessage = &daisyInputMessage[0];
        leftExpander.consumerMessage = &daisyInputMessage[1];
        rightExpander.producerMessage = &daisyOutputMessage[0];
        rightExpander.consumerMessage = &daisyOutputMessage[1];

        lightDivider.setDivision(512);
    }

    void process(const ProcessArgs &args) override {
        float mix_l[16] = {};
        float mix_r[16] = {};
        int chainChannels = 1;

        // Get daisy-chained data from left-side linked module
        if (leftExpander.module && (
            leftExpander.module->model == modelDaisyChannel2
            || leftExpander.module->model == modelDaisyChannelVu
            || leftExpander.module->model == modelDaisyChannelSends2
        )) {
            DaisyMessage *msgFromModule = (DaisyMessage *)(leftExpander.consumerMessage);

            chainChannels = msgFromModule->channels;
            for (int c = 0; c < chainChannels; c++) {
                mix_l[c] = msgFromModule->voltages_l[c];
                mix_r[c] = msgFromModule->voltages_r[c];
            }
            link_l = 0.8f;
        } else {
            link_l = 0.0f;
        }

        // Set daisy-chained output to right-side linked module
        if (rightExpander.module && (
            rightExpander.module->model == modelDaisyMaster2
            || rightExpander.module->model == modelDaisyChannel2
            || rightExpander.module->model == modelDaisyChannelVu
            || rightExpander.module->model == modelDaisyChannelSends2
        )) {
            DaisyMessage *msgToModule = (DaisyMessage *)(rightExpander.module->leftExpander.producerMessage);
            msgToModule->channels = chainChannels;
            for (int c = 0; c < chainChannels; c++) {
                msgToModule->voltages_l[c] = mix_l[c];
                msgToModule->voltages_r[c] = mix_r[c];
            }

            link_r = 0.8f;
        } else {
            link_r = 0.0f;
        }

        // Bring the voltage back up from the chained low voltage
        for (int c = 0; c < chainChannels; c++) {
            mix_l[c] = clamp(mix_l[c] * DAISY_DIVISOR, -12.f, 12.f) * 1;
            mix_r[c] = clamp(mix_r[c] * DAISY_DIVISOR, -12.f, 12.f) * 1;
        }

        // Set daisy-chained output to right-side linked module
        if (rightExpander.module && rightExpander.module->model == modelDaisyChannelVu) {
            // Write this module's output to the producer message
            DaisyMessage *msgToModule = (DaisyMessage *)(rightExpander.module->leftExpander.producerMessage);
            msgToModule->single_channels = chainChannels;
            for (int c = 0; c < chainChannels; c++) {
                msgToModule->single_voltages_l[c] = mix_l[c];
                msgToModule->single_voltages_r[c] = mix_r[c];
            }
        }

        if (link_r > 0.0) {
            rightExpander.module->leftExpander.messageFlipRequested = true;
        }

        // Set aggregated decoded output
        outputs[CH_OUTPUT_1].setChannels(chainChannels);
        outputs[CH_OUTPUT_1].writeVoltages(mix_l);
        outputs[CH_OUTPUT_2].setChannels(chainChannels);
        outputs[CH_OUTPUT_2].writeVoltages(mix_r);

        // Set lights
        if (lightDivider.process()) {
            lights[LINK_LIGHT_L].setBrightness(link_l);
            lights[LINK_LIGHT_R].setBrightness(link_r);
        }
    }
};

struct DaisyChannelSendsWidget2 : ModuleWidget {
    DaisyChannelSendsWidget2(DaisyChannelSends2 *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/DaisyChannelSends2.svg")));

        // Screws
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Channel Input/Output
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 290.0), module, DaisyChannelSends2::CH_OUTPUT_1));
        addOutput(createOutput<PJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 316.0), module, DaisyChannelSends2::CH_OUTPUT_2));

        // Link lights
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH - 4, 361.0f), module, DaisyChannelSends2::LINK_LIGHT_L));
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH + 4, 361.0f), module, DaisyChannelSends2::LINK_LIGHT_R));
    }
};

Model *modelDaisyChannelSends2 = createModel<DaisyChannelSends2, DaisyChannelSendsWidget2>("DaisyChannelSends2");

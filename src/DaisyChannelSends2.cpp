#include "QuantalAudio.hpp"
#include "Daisy.hpp"

struct DaisyChannelSends2 : Module {
    enum ParamIds {
        GROUP_PARAM,
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
        GROUP_BTN_LIGHT,
        GROUP1_LIGHT,
        GROUP2_LIGHT,
        NUM_LIGHTS
    };

    bool muted = false;
    int group = 1;
    float link_l = 0.f;
    float link_r = 0.f;

    Vec widgetPos;

    dsp::ClockDivider lightDivider;
    dsp::SchmittTrigger groupChangeTrigger;

    DaisyMessage daisyInputMessage[2][1];
    DaisyMessage daisyOutputMessage[2][1];

    DaisyChannelSends2() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configButton(GROUP_PARAM, "Change group");

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

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        // mute
        json_object_set_new(rootJ, "group", json_integer(group));

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        // mute
        json_t* groupJ = json_object_get(rootJ, "group");
        if (groupJ) {
            group = json_integer_value(groupJ);
        }
    }

    void setWidgetPosition(Vec pos) {
        widgetPos = pos;
    }

    void process(const ProcessArgs &args) override {
        int chainChannels = 1;
        float mix_l[16] = {};
        float mix_r[16] = {};
        int auxChannels = 1;
        float aux_l[16] = {};
        float aux_r[16] = {};
        int aux1Channels = 1;
        float aux1Signals_l[16] = {};
        float aux1Signals_r[16] = {};
        int aux2Channels = 1;
        float aux2Signals_l[16] = {};
        float aux2Signals_r[16] = {};

        // Assume this module is the first in the chain; it will get
        // overwritten if we receive a value from the left expander
        Vec firstPos = widgetPos;

        bool groupButton = params[GROUP_PARAM].getValue() > 0.f;
        if (groupChangeTrigger.process(params[GROUP_PARAM].getValue())) {
            group = group + 1;
            if (group > 2) {
                group = 1;
            }
        }

        // Get daisy-chained data from left-side linked module
        if (leftExpander.module && (
                leftExpander.module->model == modelDaisyChannel2
                || leftExpander.module->model == modelDaisyChannelVu
                || leftExpander.module->model == modelDaisyChannelSends2
                || leftExpander.module->model == modelDaisyBlank
            )) {
            DaisyMessage* msgFromModule = (DaisyMessage*)(leftExpander.consumerMessage);

            chainChannels = msgFromModule->channels;
            for (int c = 0; c < chainChannels; c++) {
                mix_l[c] = msgFromModule->voltages_l[c];
                mix_r[c] = msgFromModule->voltages_r[c];
            }

            aux1Channels = msgFromModule->aux1_channels;
            for (int c = 0; c < aux1Channels; c++) {
                aux1Signals_l[c] = msgFromModule->aux1_voltages_l[c];
                aux1Signals_r[c] = msgFromModule->aux1_voltages_r[c];
            }

            aux2Channels = msgFromModule->aux2_channels;
            for (int c = 0; c < aux2Channels; c++) {
                aux2Signals_l[c] = msgFromModule->aux2_voltages_l[c];
                aux2Signals_r[c] = msgFromModule->aux2_voltages_r[c];
            }

            if (group == 1) {
                auxChannels = aux1Channels;
                for (int c = 0; c < auxChannels; c++) {
                    aux_l[c] = aux1Signals_l[c];
                    aux_r[c] = aux1Signals_r[c];
                }
            } else {
                auxChannels = aux2Channels;
                for (int c = 0; c < auxChannels; c++) {
                    aux_l[c] = aux2Signals_l[c];
                    aux_r[c] = aux2Signals_r[c];
                }
            }

            firstPos = Vec(msgFromModule->first_pos_x, msgFromModule->first_pos_y);

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
                || rightExpander.module->model == modelDaisyBlank
            )) {
            DaisyMessage* msgToModule = (DaisyMessage*)(rightExpander.module->leftExpander.producerMessage);

            msgToModule->channels = chainChannels;
            for (int c = 0; c < chainChannels; c++) {
                msgToModule->voltages_l[c] = mix_l[c];
                msgToModule->voltages_r[c] = mix_r[c];
            }

            // Combine/pass through the aux send voltages
            msgToModule->aux1_channels = aux1Channels;
            for (int c = 0; c < aux1Channels; c++) {
                msgToModule->aux1_voltages_l[c] = aux1Signals_l[c];
                msgToModule->aux1_voltages_r[c] = aux1Signals_r[c];
            }
            msgToModule->aux2_channels = aux2Channels;
            for (int c = 0; c < aux2Channels; c++) {
                msgToModule->aux2_voltages_l[c] = aux2Signals_l[c];
                msgToModule->aux2_voltages_r[c] = aux2Signals_r[c];
            }

            msgToModule->first_pos_x = firstPos.x;
            msgToModule->first_pos_y = firstPos.y;

            link_r = 0.8f;
        } else {
            link_r = 0.0f;
        }

        // Bring the voltage back up from the chained low voltage
        for (int c = 0; c < chainChannels; c++) {
            mix_l[c] = clamp(mix_l[c] * DAISY_DIVISOR, -12.f, 12.f) * 1;
            mix_r[c] = clamp(mix_r[c] * DAISY_DIVISOR, -12.f, 12.f) * 1;
        }

        // Set this channel's output to right-side linked VU module
        if (rightExpander.module && rightExpander.module->model == modelDaisyChannelVu) {
            // Write this module's output to the single channel message
            DaisyMessage* msgToModule = (DaisyMessage*)(rightExpander.module->leftExpander.producerMessage);
            msgToModule->single_channels = chainChannels;
            for (int c = 0; c < chainChannels; c++) {
                msgToModule->single_voltages_l[c] = aux_l[c];
                msgToModule->single_voltages_r[c] = aux_r[c];
            }
        }

        if (link_r > 0.0) {
            rightExpander.module->leftExpander.messageFlipRequested = true;
        }

        // Set aggregated decoded output
        outputs[CH_OUTPUT_1].setChannels(auxChannels);
        outputs[CH_OUTPUT_1].writeVoltages(aux_l);
        outputs[CH_OUTPUT_2].setChannels(auxChannels);
        outputs[CH_OUTPUT_2].writeVoltages(aux_r);

        // Set lights
        if (lightDivider.process()) {
            lights[LINK_LIGHT_L].setBrightness(link_l);
            lights[LINK_LIGHT_R].setBrightness(link_r);

            lights[GROUP_BTN_LIGHT].setBrightness(groupButton);
            if (group == 1) {
                lights[GROUP1_LIGHT].setBrightness(1);
                lights[GROUP2_LIGHT].setBrightness(0);
            } else {
                lights[GROUP1_LIGHT].setBrightness(0);
                lights[GROUP2_LIGHT].setBrightness(1);
            }
        }
    }
};

struct DaisyChannelSendsWidget2 : ModuleWidget {

    dsp::ClockDivider uiDivider;

    DaisyChannelSendsWidget2(DaisyChannelSends2 *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/DaisyChannelSends2.svg"),
                asset::plugin(pluginInstance, "res/DaisyChannelSends2-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Switch
        addParam(createLightParamCentered<VCVLightButton<MediumSimpleLight<WhiteLight>>>(Vec(RACK_GRID_WIDTH - 0, 57.5f), module, DaisyChannelSends2::GROUP_PARAM, DaisyChannelSends2::GROUP_BTN_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(RACK_GRID_WIDTH - 2, 80.0f), module, DaisyChannelSends2::GROUP1_LIGHT));
        addChild(createLightCentered<SmallLight<BlueLight>>(Vec(RACK_GRID_WIDTH - 2, 90.0f), module, DaisyChannelSends2::GROUP2_LIGHT));

        // Channel Output
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 290.0), module, DaisyChannelSends2::CH_OUTPUT_1));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5, 316.0), module, DaisyChannelSends2::CH_OUTPUT_2));

        // Link lights
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH - 4, 361.0f), module, DaisyChannelSends2::LINK_LIGHT_L));
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH + 4, 361.0f), module, DaisyChannelSends2::LINK_LIGHT_R));
    }

    void step() override {
        if (uiDivider.process()) {
            DaisyChannelSends2 *module = getModule<DaisyChannelSends2>();

            if (this->box.pos.x > 0.00) {
                module->setWidgetPosition(this->box.pos);
            }
        }

        ModuleWidget::step();
    }
};

Model* modelDaisyChannelSends2 = createModel<DaisyChannelSends2, DaisyChannelSendsWidget2>("DaisyChannelSends2");

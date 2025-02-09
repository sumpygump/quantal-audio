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

    int channelStripId = 1;

    Vec widgetPos;

    dsp::ClockDivider lightDivider;
    dsp::SchmittTrigger groupChangeTrigger;

    DaisyMessage daisyInputMessage[2][1];
    DaisyMessage daisyOutputMessage[2][1];

    StereoVoltages daisySignals = {};
    StereoVoltages auxSignals = {};
    StereoVoltages aux1Signals = {};
    StereoVoltages aux2Signals = {};
    StereoVoltages soloSignals = {};

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

        lightDivider.setDivision(DAISY_LIGHT_DIVISION);
    }

    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        // mute
        json_object_set_new(rootJ, "group", json_integer(group));

        return rootJ;
    }

    void dataFromJson(json_t* rootJ) override {
        // mute
        const json_t* groupJ = json_object_get(rootJ, "group");
        if (groupJ) {
            group = json_integer_value(groupJ);
        }
    }

    void setWidgetPosition(Vec pos) {
        widgetPos = pos;
    }

    void process(const ProcessArgs &args) override {
        daisySignals = {};
        auxSignals = {};
        aux1Signals = {};
        aux2Signals = {};
        soloSignals = {};

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
            DaisyMessage* msgFromModule = static_cast<DaisyMessage*>(leftExpander.consumerMessage);

            daisySignals = msgFromModule->signals;
            aux1Signals = msgFromModule->aux1Signals;
            aux2Signals = msgFromModule->aux2Signals;

            if (group == 1) {
                auxSignals = aux1Signals;
            } else {
                auxSignals = aux2Signals;
            }

            soloSignals = msgFromModule->soloSignals;

            firstPos = Vec(msgFromModule->first_pos_x, msgFromModule->first_pos_y);
            channelStripId = msgFromModule->channel_strip_id;

            link_l = 0.8f;
        } else {
            channelStripId = 1;
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
            DaisyMessage* msgToModule = static_cast<DaisyMessage*>(rightExpander.module->leftExpander.producerMessage);

            msgToModule->signals = daisySignals;
            msgToModule->aux1Signals = aux1Signals;
            msgToModule->aux2Signals = aux2Signals;
            msgToModule->soloSignals = soloSignals;

            msgToModule->first_pos_x = firstPos.x;
            msgToModule->first_pos_y = firstPos.y;
            msgToModule->channel_strip_id = channelStripId;

            link_r = 0.8f;
        } else {
            link_r = 0.0f;
        }

        // Set this channel's output to right-side linked VU module
        if (rightExpander.module && rightExpander.module->model == modelDaisyChannelVu) {
            // Write this module's output to the single channel message
            DaisyMessage* msgToModule = static_cast<DaisyMessage*>(rightExpander.module->leftExpander.producerMessage);
            msgToModule->singleSignals = auxSignals;
        }

        if (rightExpander.module && link_r > 0.0f) {
            rightExpander.module->leftExpander.messageFlipRequested = true;
        }

        // Set aggregated decoded output
        outputs[CH_OUTPUT_1].setChannels(auxSignals.channels);
        outputs[CH_OUTPUT_1].writeVoltages(auxSignals.voltages_l);
        outputs[CH_OUTPUT_2].setChannels(auxSignals.channels);
        outputs[CH_OUTPUT_2].writeVoltages(auxSignals.voltages_r);

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

    explicit DaisyChannelSendsWidget2(DaisyChannelSends2 *module) {
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
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 290.0), module, DaisyChannelSends2::CH_OUTPUT_1));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 316.0), module, DaisyChannelSends2::CH_OUTPUT_2));

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

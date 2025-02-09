#include "QuantalAudio.hpp"
#include "Daisy.hpp"

struct DaisyBlank : Module {
    enum ParamIds {
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightsIds {
        LINK_LIGHT_L,
        LINK_LIGHT_R,
        NUM_LIGHTS
    };

    float link_l = 0.f;
    float link_r = 0.f;
    int channelStripId = 1;

    Vec widgetPos;

    dsp::ClockDivider lightDivider;

    DaisyMessage daisyInputMessage[2][1];
    DaisyMessage daisyOutputMessage[2][1];

    DaisyBlank() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        configLight(LINK_LIGHT_L, "Daisy chain link input");
        configLight(LINK_LIGHT_R, "Daisy chain link output");

        // Set the expander messages
        leftExpander.producerMessage = &daisyInputMessage[0];
        leftExpander.consumerMessage = &daisyInputMessage[1];
        rightExpander.producerMessage = &daisyOutputMessage[0];
        rightExpander.consumerMessage = &daisyOutputMessage[1];

        lightDivider.setDivision(DAISY_LIGHT_DIVISION);
    }

    void setWidgetPosition(Vec pos) {
        widgetPos = pos;
    }

    void process(const ProcessArgs &args) override {

        // Assume this module is the first in the chain; it will get
        // overwritten if we receive a value from the left expander
        Vec firstPos = widgetPos;

        // Get daisy-chained data from left-side linked module
        if (leftExpander.module && (
                leftExpander.module->model == modelDaisyChannel2
                || leftExpander.module->model == modelDaisyChannelVu
                || leftExpander.module->model == modelDaisyChannelSends2
                || leftExpander.module->model == modelDaisyBlank
            )) {
            const DaisyMessage* msgFromModule = static_cast<DaisyMessage*>(leftExpander.consumerMessage);

            firstPos = Vec(msgFromModule->first_pos_x, msgFromModule->first_pos_y);
            channelStripId = msgFromModule->channel_strip_id;

            // Set daisy-chained output to right-side linked module
            if (rightExpander.module && (
                    rightExpander.module->model == modelDaisyMaster2
                    || rightExpander.module->model == modelDaisyChannel2
                    || rightExpander.module->model == modelDaisyChannelVu
                    || rightExpander.module->model == modelDaisyChannelSends2
                    || rightExpander.module->model == modelDaisyBlank
                )) {
                DaisyMessage* msgToModule = static_cast<DaisyMessage*>(rightExpander.module->leftExpander.producerMessage);

                msgToModule->signals = msgFromModule->signals;
                msgToModule->aux1Signals = msgFromModule->aux1Signals;
                msgToModule->aux2Signals = msgFromModule->aux2Signals;
                msgToModule->soloSignals = msgFromModule->soloSignals;

                msgToModule->first_pos_x = firstPos.x;
                msgToModule->first_pos_y = firstPos.y;
                msgToModule->channel_strip_id = channelStripId;

                rightExpander.module->leftExpander.messageFlipRequested = true;
            }

            link_l = 0.8f;
        } else {
            channelStripId = 1;
            link_l = 0.0f;
        }

        // Make sure link light to the right is correct
        if (rightExpander.module && (
                rightExpander.module->model == modelDaisyMaster2
                || rightExpander.module->model == modelDaisyChannel2
                || rightExpander.module->model == modelDaisyChannelVu
                || rightExpander.module->model == modelDaisyChannelSends2
                || rightExpander.module->model == modelDaisyBlank
            )) {
            link_r = 0.8f;
        } else {
            link_r = 0.0f;
        }

        // Set lights
        if (lightDivider.process()) {
            lights[LINK_LIGHT_L].setBrightness(link_l);
            lights[LINK_LIGHT_R].setBrightness(link_r);
        }
    }
};

struct DaisyBlankWidget : ModuleWidget {

    dsp::ClockDivider uiDivider;

    explicit DaisyBlankWidget(DaisyBlank *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/DaisyBlank.svg"),
                asset::plugin(pluginInstance, "res/DaisyBlank-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Link lights
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH - 4, 361.0f), module, DaisyBlank::LINK_LIGHT_L));
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH + 4, 361.0f), module, DaisyBlank::LINK_LIGHT_R));

        uiDivider.setDivision(DAISY_UI_DIVISION);
    }

    void step() override {
        if (uiDivider.process()) {
            DaisyBlank *module = getModule<DaisyBlank>();

            if (this->box.pos.x > 0.00) {
                module->setWidgetPosition(this->box.pos);
            }
        }

        ModuleWidget::step();
    }
};

Model* modelDaisyBlank = createModel<DaisyBlank, DaisyBlankWidget>("DaisyBlank");

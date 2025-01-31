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
    float aux1_send_amt = 0.f;
    float aux2_send_amt = 0.f;

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

        lightDivider.setDivision(512);
    }

    void setWidgetPosition(Vec pos) {
        widgetPos = pos;
    }

    void process(const ProcessArgs &args) override {

        int chainChannels = 1;
        float daisySignals_l[16] = {};
        float daisySignals_r[16] = {};
        int aux1Channels = 1;
        float aux1Signals_l[16] = {};
        float aux1Signals_r[16] = {};
        int aux2Channels = 1;
        float aux2Signals_l[16] = {};
        float aux2Signals_r[16] = {};

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
            DaisyMessage *msgFromModule = (DaisyMessage *)(leftExpander.consumerMessage);
            chainChannels = msgFromModule->channels;
            for (int c = 0; c < chainChannels; c++) {
                daisySignals_l[c] = msgFromModule->voltages_l[c];
                daisySignals_r[c] = msgFromModule->voltages_r[c];
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
            DaisyMessage *msgToModule = (DaisyMessage *)(rightExpander.module->leftExpander.producerMessage);

            msgToModule->channels = chainChannels;
            for (int c = 0; c < chainChannels; c++) {
                msgToModule->voltages_l[c] = daisySignals_l[c];
                msgToModule->voltages_r[c] = daisySignals_r[c];
            }

            // Send along aux signals
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

            rightExpander.module->leftExpander.messageFlipRequested = true;

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

    DaisyBlankWidget(DaisyBlank *module) {
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

Model *modelDaisyBlank = createModel<DaisyBlank, DaisyBlankWidget>("DaisyBlank");

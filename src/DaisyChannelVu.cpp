#include "QuantalAudio.hpp"
#include "Daisy.hpp"

static constexpr int VU_LIGHT_COUNT = 32;

/** Returns the sum of all voltages. */
float getVoltageSum(const int channels, const float voltages[16]) {
    float sum = 0.f;
    for (int c = 0; c < channels; c++) {
        sum += voltages[c];
    }
    return sum;
}

struct DaisyChannelVu : Module {
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
        ENUMS(VU_LIGHTS_L, VU_LIGHT_COUNT + 8 + 4),
        ENUMS(VU_LIGHTS_R, VU_LIGHT_COUNT + 8 + 4),
        NUM_LIGHTS
    };

    float link_l = 0.f;
    float link_r = 0.f;
    int channelStripId = 1;

    Vec widgetPos;

    dsp::ClockDivider lightDivider;
    dsp::VuMeter2 vuMeter[2];

    DaisyMessage daisyInputMessage[2][1];
    DaisyMessage daisyOutputMessage[2][1];

    DaisyChannelVu() {
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
                || leftExpander.module->model == modelDaisyMaster2
                || leftExpander.module->model == modelDaisyBlank
            )) {
            const DaisyMessage* msgFromModule = static_cast<DaisyMessage*>(leftExpander.consumerMessage);

            // Use the single channel to display in VU meter
            vuMeter[0].process(
                args.sampleTime,
                getVoltageSum(msgFromModule->singleSignals.channels, msgFromModule->singleSignals.voltages_l) / 10.f
            );
            vuMeter[1].process(
                args.sampleTime,
                getVoltageSum(msgFromModule->singleSignals.channels, msgFromModule->singleSignals.voltages_r) / 10.f
            );

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
            vuMeter[0].process(args.sampleTime, 0.0f);
            vuMeter[1].process(args.sampleTime, 0.0f);
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
            for (int i = VU_LIGHT_COUNT + 8 + 3; i >= 0; i--) {
                const float dbMax = -60.f + 1.5f * static_cast<float>(i);
                const float dbMin = dbMax + 1;
                lights[VU_LIGHTS_L + i].setBrightness(vuMeter[0].getBrightness(dbMin, dbMax));
                lights[VU_LIGHTS_R + i].setBrightness(vuMeter[1].getBrightness(dbMin, dbMax));
            }
            lights[LINK_LIGHT_L].setBrightness(link_l);
            lights[LINK_LIGHT_R].setBrightness(link_r);
        }
    }
};

struct DaisyChannelVuWidget : ModuleWidget {

    dsp::ClockDivider uiDivider;

    explicit DaisyChannelVuWidget(DaisyChannelVu *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/DaisyChannelVu.svg"),
                asset::plugin(pluginInstance, "res/DaisyChannelVu-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Link lights
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH / 2 - 3, 361.0f), module, DaisyChannelVu::LINK_LIGHT_L));
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH / 2 + 3, 361.0f), module, DaisyChannelVu::LINK_LIGHT_R));

        for (int i = 0; i < VU_LIGHT_COUNT; i++) {
            float distance = static_cast<float>(i) * 7;
            addChild(createLightCentered<VCVSliderLight<GreenLight>>(Vec(RACK_GRID_WIDTH / 2 - 3.f, 339.f - distance), module, DaisyChannelVu::VU_LIGHTS_L + i));
            addChild(createLightCentered<VCVSliderLight<GreenLight>>(Vec(RACK_GRID_WIDTH / 2 + 3.f, 339.f - distance), module, DaisyChannelVu::VU_LIGHTS_R + i));
        }
        for (int i = VU_LIGHT_COUNT; i < VU_LIGHT_COUNT + 8; i++) {
            float distance = static_cast<float>(i) * 7;
            addChild(createLightCentered<VCVSliderLight<YellowLight>>(Vec(RACK_GRID_WIDTH / 2 - 3.f, 339.f - distance), module, DaisyChannelVu::VU_LIGHTS_L + i));
            addChild(createLightCentered<VCVSliderLight<YellowLight>>(Vec(RACK_GRID_WIDTH / 2 + 3.f, 339.f - distance), module, DaisyChannelVu::VU_LIGHTS_R + i));
        }
        for (int i = VU_LIGHT_COUNT + 8; i < VU_LIGHT_COUNT + 12; i++) {
            float distance = static_cast<float>(i) * 7;
            addChild(createLightCentered<VCVSliderLight<RedLight>>(Vec(RACK_GRID_WIDTH / 2 - 3.f, 339.f - distance), module, DaisyChannelVu::VU_LIGHTS_L + i));
            addChild(createLightCentered<VCVSliderLight<RedLight>>(Vec(RACK_GRID_WIDTH / 2 + 3.f, 339.f - distance), module, DaisyChannelVu::VU_LIGHTS_R + i));
        }

        uiDivider.setDivision(DAISY_UI_DIVISION);
    }

    void step() override {
        if (uiDivider.process()) {
            DaisyChannelVu *module = getModule<DaisyChannelVu>();

            if (this->box.pos.x > 0.00) {
                module->setWidgetPosition(this->box.pos);
            }
        }

        ModuleWidget::step();
    }
};

Model* modelDaisyChannelVu = createModel<DaisyChannelVu, DaisyChannelVuWidget>("DaisyChannelVu");

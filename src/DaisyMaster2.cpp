#include "QuantalAudio.hpp"
#include "Daisy.hpp"

struct DaisyMaster2 : Module {
    enum ParamIds {
        MIX_LVL_PARAM,
        MUTE_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        MIX_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        MIX_OUTPUT_1, // Left
        MIX_OUTPUT_2, // Right
        NUM_OUTPUTS
    };
    enum LightsIds {
        MUTE_LIGHT,
        LINK_LIGHT_L,
        NUM_LIGHTS
    };

    bool muted = false;
    float link_l = 0.f;
    dsp::ClockDivider lightDivider;

    enum DaisyModelIds {
        CHANNEL_2,
        CHANNEL_VU,
        CHANNEL_AUX,
        CHANNEL_SEP,
        NUM_MODELS
    };
    Model* daisyModels[NUM_MODELS] {};
    Vec widgetPos;

    DaisyMessage daisyMessages[2][1];
    StereoVoltages signals = {};
    StereoVoltages soloSignals = {};

    DaisyMaster2() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(MIX_LVL_PARAM, 0.0f, 2.0f, 1.0f, "Mix level", " dB", -10, 20);
        configSwitch(MUTE_PARAM, 0.f, 1.f, 0.f, "Mute", {"Not muted", "Muted"});

        configInput(MIX_CV_INPUT, "Level CV");
        configOutput(MIX_OUTPUT_1, "Mix L");
        configOutput(MIX_OUTPUT_2, "Mix R");

        configLight(LINK_LIGHT_L, "Daisy chain link input");

        // Set the left expander message instances
        leftExpander.producerMessage = &daisyMessages[0];
        leftExpander.consumerMessage = &daisyMessages[1];

        lightDivider.setDivision(512);

        // Store all the related daisy models
        daisyModels[CHANNEL_2] = rack::plugin::getModel("QuantalAudio", "DaisyChannel2");
        daisyModels[CHANNEL_VU] = rack::plugin::getModel("QuantalAudio", "DaisyChannelVu");
        daisyModels[CHANNEL_AUX] = rack::plugin::getModel("QuantalAudio", "DaisyChannelSends2");
        daisyModels[CHANNEL_SEP] = rack::plugin::getModel("QuantalAudio", "DaisyBlank");
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
        muted = params[MUTE_PARAM].getValue() > 0.f;

        widgetPos = Vec(0, 0);

        if (!muted) {
            // Get daisy-chained data from left-side linked module
            if (leftExpander.module && (
                    leftExpander.module->model == modelDaisyChannel2
                    || leftExpander.module->model == modelDaisyChannelVu
                    || leftExpander.module->model == modelDaisyChannelSends2
                    || leftExpander.module->model == modelDaisyBlank
                )) {
                DaisyMessage* msgFromExpander = static_cast<DaisyMessage*>(leftExpander.consumerMessage);

                signals = msgFromExpander->signals;
                soloSignals = msgFromExpander->soloSignals;

                widgetPos = Vec(msgFromExpander->first_pos_x, msgFromExpander->first_pos_y);

                link_l = 0.8f;
            } else {
                link_l = 0.0f;
            }

            float gain = params[MIX_LVL_PARAM].getValue();

            if (soloSignals.channels > 0) {
                for (int c = 0; c < soloSignals.channels; c++) {
                    soloSignals.voltages_l[c] = clamp(soloSignals.voltages_l[c], -12.f, 12.f) * gain;
                    soloSignals.voltages_r[c] = clamp(soloSignals.voltages_r[c], -12.f, 12.f) * gain;
                }

                if (inputs[MIX_CV_INPUT].isConnected()) {
                    for (int c = 0; c < soloSignals.channels; c++) {
                        const float mix_cv = clamp(inputs[MIX_CV_INPUT].getPolyVoltage(c) / 10.f, 0.f, 1.f);
                        soloSignals.voltages_l[c] *= mix_cv;
                        soloSignals.voltages_r[c] *= mix_cv;
                    }
                }

                outputs[MIX_OUTPUT_1].setChannels(soloSignals.channels);
                outputs[MIX_OUTPUT_1].writeVoltages(soloSignals.voltages_l);
                outputs[MIX_OUTPUT_2].setChannels(soloSignals.channels);
                outputs[MIX_OUTPUT_2].writeVoltages(soloSignals.voltages_r);
            } else {
                // Bring the voltage back up from the chained low voltage
                for (int c = 0; c < signals.channels; c++) {
                    signals.voltages_l[c] = clamp(signals.voltages_l[c] * DAISY_DIVISOR, -12.f, 12.f) * gain;
                    signals.voltages_r[c] = clamp(signals.voltages_r[c] * DAISY_DIVISOR, -12.f, 12.f) * gain;
                }

                if (inputs[MIX_CV_INPUT].isConnected()) {
                    for (int c = 0; c < signals.channels; c++) {
                        const float mix_cv = clamp(inputs[MIX_CV_INPUT].getPolyVoltage(c) / 10.f, 0.f, 1.f);
                        signals.voltages_l[c] *= mix_cv;
                        signals.voltages_r[c] *= mix_cv;
                    }
                }
                outputs[MIX_OUTPUT_1].setChannels(signals.channels);
                outputs[MIX_OUTPUT_1].writeVoltages(signals.voltages_l);
                outputs[MIX_OUTPUT_2].setChannels(signals.channels);
                outputs[MIX_OUTPUT_2].writeVoltages(signals.voltages_r);
            }

            // Set output to right-side linked VU meter module
            if (rightExpander.module && rightExpander.module->model == modelDaisyChannelVu) {
                DaisyMessage* msgToModule = static_cast<DaisyMessage*>(rightExpander.module->leftExpander.producerMessage);
                msgToModule->singleSignals = signals;
                rightExpander.module->leftExpander.messageFlipRequested = true;
            }
        }

        // Set lights
        if (lightDivider.process()) {
            lights[MUTE_LIGHT].value = (muted);
            lights[LINK_LIGHT_L].setBrightness(link_l);
        }
    }

    static Vec spawnModule(Vec pos, Model *model) {
        Module *newModule = model->createModule();

        // Create module widget
        ModuleWidget *newWidget = model->createModuleWidget(newModule);
        if (!newWidget) {
            WARN("Cannot spawn module %s.", model->slug.c_str());
            return pos;
        }
        APP->engine->addModule(newModule);
        APP->scene->rack->updateModuleOldPositions();
        APP->scene->rack->setModulePosNearest(newWidget, Vec(pos.x, pos.y));
        APP->scene->rack->addModule(newWidget);

        // Record history
        auto *h = new history::ModuleAdd;
        h->name = "create module";
        h->setModule(newWidget);
        APP->history->push(h);

        return newWidget->box.pos;
    }

    /**
     * Add channel strips
     *
     * Looks at the value that has come through the chain indicating the
     * position of the leftmost channel strip. Use the position of that module
     * to determine where the next channel strips should be placed.
     */
    void addChannelStrips(const ModuleWidget *parentWidget, const int channelStripCount, const int channelAuxCount, const bool includeVuMeters) const {
        Vec next;
        const Vec pos = parentWidget->box.pos;

        if (widgetPos.x == 0) {
            next = pos;
        } else {
            next = widgetPos;
        }

        if (includeVuMeters) {
            // Add return channel strips
            for (int i = 0; i < channelAuxCount; i++) {
                next = spawnModule(next, daisyModels[CHANNEL_VU]);
                next = spawnModule(next, daisyModels[CHANNEL_2]);
            }

            // Add aux send channel strips
            for (int i = 0; i < channelAuxCount; i++) {
                next = spawnModule(next, daisyModels[CHANNEL_AUX]);
            }

            // Add a separator
            if (channelAuxCount > 0) {
                next = spawnModule(next, daisyModels[CHANNEL_SEP]);
            }

            // Add input channel strips
            for (int i = 0; i < channelStripCount; i++) {
                next = spawnModule(next, daisyModels[CHANNEL_VU]);
                next = spawnModule(next, daisyModels[CHANNEL_2]);
            }
        } else {
            // Add return channel strips
            for (int i = 0; i < channelAuxCount; i++) {
                next = spawnModule(next, daisyModels[CHANNEL_2]);
            }

            // Add aux send channel strips
            for (int i = 0; i < channelAuxCount; i++) {
                next = spawnModule(next, daisyModels[CHANNEL_AUX]);
            }

            // Add a separator
            if (channelAuxCount > 0) {
                next = spawnModule(next, daisyModels[CHANNEL_SEP]);
            }

            // Add input channel strips
            for (int i = 0; i < channelStripCount; i++) {
                next = spawnModule(next, daisyModels[CHANNEL_2]);
            }
        }
    }
};

struct DaisyMasterWidget2 : ModuleWidget {

    dsp::ClockDivider uiDivider;

    explicit DaisyMasterWidget2(DaisyMaster2 *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/DaisyMaster2.svg"),
                asset::plugin(pluginInstance, "res/DaisyMaster2-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Level & CV
        addParam(createParam<RoundLargeBlackKnob>(Vec(RACK_GRID_WIDTH * 1.5f - (36.0f / 2), 52.0), module, DaisyMaster2::MIX_LVL_PARAM));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH * 1.5f - (25.0f / 2), 96.0), module, DaisyMaster2::MIX_CV_INPUT));

        // Mute
        addParam(createLightParam<VCVLightLatch<MediumSimpleLight<RedLight>>>(Vec(RACK_GRID_WIDTH * 1.5f - 9.0f, 254.0), module, DaisyMaster2::MUTE_PARAM, DaisyMaster2::MUTE_LIGHT));

        // Mix output
        addOutput(createOutput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 1.5f) - (25.0f / 2), 290.0), module, DaisyMaster2::MIX_OUTPUT_1));
        addOutput(createOutput<ThemedPJ301MPort>(Vec((RACK_GRID_WIDTH * 1.5f) - (25.0f / 2), 316.0), module, DaisyMaster2::MIX_OUTPUT_2));

        // Link light
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH - 6, 361.0), module, DaisyMaster2::LINK_LIGHT_L));

        uiDivider.setDivision(24);
    }

    void appendContextMenu(Menu *menu) override {
        const DaisyMaster2* module = getModule<DaisyMaster2>();

        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Create 1 channel", "", [ = ]() {
            module->addChannelStrips(this, 1, 0, false);
        }));
        menu->addChild(createMenuItem("Create 1 channel with vu meter", "", [ = ]() {
            module->addChannelStrips(this, 1, 0, true);
        }));
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Create 2 channels", "", [ = ]() {
            module->addChannelStrips(this, 2, 0, false);
        }));
        menu->addChild(createMenuItem("Create 2 channels with vu meters", "", [ = ]() {
            module->addChannelStrips(this, 2, 0, true);
        }));
        menu->addChild(createMenuItem("Create 2 channels with vu meters + aux sends", "", [ = ]() {
            module->addChannelStrips(this, 2, 2, true);
        }));
        menu->addChild(new MenuSeparator);
        menu->addChild(createMenuItem("Create 4 channels", "", [ = ]() {
            module->addChannelStrips(this, 4, 0, false);
        }));
        menu->addChild(createMenuItem("Create 4 channels with vu meters", "", [ = ]() {
            module->addChannelStrips(this, 4, 0, true);
        }));
        menu->addChild(createMenuItem("Create 4 channels with vu meters + aux sends", "", [ = ]() {
            module->addChannelStrips(this, 4, 2, true);
        }));
    }

    void onHoverKey(const HoverKeyEvent& e) override {
        if (e.action == GLFW_RELEASE) {
            if (e.keyName[0] == 'm') {
                DaisyMaster2 *module = getModule<DaisyMaster2>();
                // Toggle mute
                module->params[DaisyMaster2::MUTE_PARAM].setValue(module->muted ? 0.f : 1.f);
                e.consume(this);
            }
        }
        ModuleWidget::onHoverKey(e);
    }
};

Model* modelDaisyMaster2 = createModel<DaisyMaster2, DaisyMasterWidget2>("DaisyMaster2");

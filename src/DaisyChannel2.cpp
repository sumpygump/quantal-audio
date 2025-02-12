#include "QuantalAudio.hpp"
#include "Daisy.hpp"

constexpr float SLEW_SPEED = 6.f; // For smoothing out CV
constexpr float VALUE_MUTE = 1.f;
constexpr float VALUE_SOLO = -1.f;
constexpr float VALUE_OFF = 0.f;

constexpr int HOLD_TRIGGER_DURATION = 50;

struct DaisyChannel2 : Module {
    enum ParamIds {
        CH_LVL_PARAM,
        MUTE_PARAM,
        PAN_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        CH_INPUT_1, // Left
        CH_INPUT_2, // Right
        LVL_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        CH_OUTPUT_1, // Left
        CH_OUTPUT_2, // Right
        NUM_OUTPUTS
    };
    enum LightsIds {
        MUTE_LIGHT,
        MUTE2_LIGHT,
        LINK_LIGHT_L,
        LINK_LIGHT_R,
        AUX1_LIGHT,
        AUX2_LIGHT,
        NUM_LIGHTS
    };

    bool muted = false;
    bool solo = false;
    bool directOutsPremute = false;
    bool levelSlew = true;
    float link_l = 0.f;
    float link_r = 0.f;
    float aux1_send_amt = 0.f;
    float aux2_send_amt = 0.f;

    int channelStripId = 1;
    std::string label;

    Vec widgetPos;

    dsp::ClockDivider lightDivider;

    DaisyMessage daisyInputMessage[2][1];
    DaisyMessage daisyOutputMessage[2][1];
    SimpleSlewer levelSlewer;

    /**
     * Constructor
     */
    DaisyChannel2() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(CH_LVL_PARAM, 0.0f, 1.0f, 1.0f, "Channel level", " dB", -10, 20);
        configParam(PAN_PARAM, -1.0f, 1.0f, 0.0f, "Panning", "%", 0.f, 100.f);
        configSwitch(MUTE_PARAM, VALUE_SOLO, VALUE_MUTE, VALUE_OFF, "Mute", {"Solo", "Not muted", "Muted"});

        configInput(CH_INPUT_1, "Channel L");
        configInput(CH_INPUT_2, "Channel R");
        configInput(LVL_CV_INPUT, "Level CV");

        configOutput(CH_OUTPUT_1, "Channel L");
        configOutput(CH_OUTPUT_2, "Channel R");

        configLight(LINK_LIGHT_L, "Daisy chain link input");
        configLight(LINK_LIGHT_R, "Daisy chain link output");

        levelSlewer.setSlewSpeed(SLEW_SPEED);

        // Set the expander messages
        leftExpander.producerMessage = &daisyInputMessage[0];
        leftExpander.consumerMessage = &daisyInputMessage[1];
        rightExpander.producerMessage = &daisyOutputMessage[0];
        rightExpander.consumerMessage = &daisyOutputMessage[1];

        lightDivider.setDivision(DAISY_LIGHT_DIVISION);
    }

    /**
     * Persist state settings for this module
     */
    json_t* dataToJson() override {
        json_t* rootJ = json_object();

        json_object_set_new(rootJ, "muted", json_boolean(muted));
        json_object_set_new(rootJ, "solo", json_boolean(solo));
        json_object_set_new(rootJ, "direct_outs_prefader", json_boolean(directOutsPremute));
        json_object_set_new(rootJ, "level_slew", json_boolean(levelSlew));
        json_object_set_new(rootJ, "aux1_send_amt", json_real(aux1_send_amt));
        json_object_set_new(rootJ, "aux2_send_amt", json_real(aux2_send_amt));

        return rootJ;
    }

    /**
     * Load persisted state data for this module
     */
    void dataFromJson(json_t* rootJ) override {
        // mute
        const json_t* mutedJ = json_object_get(rootJ, "muted");
        if (mutedJ) {
            muted = json_is_true(mutedJ);
        }

        // solo
        const json_t* soloJ = json_object_get(rootJ, "solo");
        if (soloJ) {
            solo = json_is_true(soloJ);
        }

        // directOutsPremute
        const json_t* directOutsPremuteJ = json_object_get(rootJ, "direct_outs_prefader");
        if (directOutsPremuteJ) {
            directOutsPremute = json_is_true(directOutsPremuteJ);
        }

        // level slew
        const json_t* levelSlewJ = json_object_get(rootJ, "level_slew");
        if (levelSlewJ) {
            levelSlew = json_is_true(levelSlewJ);
        }

        // aux 1
        const json_t* aux1_send_amtJ = json_object_get(rootJ, "aux1_send_amt");
        if (aux1_send_amtJ) {
            aux1_send_amt = std::max(0.0f, static_cast<float>(json_real_value(aux1_send_amtJ)));
        }

        // aux 2
        const json_t* aux2_send_amtJ = json_object_get(rootJ, "aux2_send_amt");
        if (aux2_send_amtJ) {
            aux2_send_amt = std::max(0.0f, static_cast<float>(json_real_value(aux2_send_amtJ)));
        }
    }

    /**
     * Tell the module where the widget is located on the rack , so we can pass
     * it along the chain to the daisy master
     */
    void setWidgetPosition(const Vec pos) {
        widgetPos = pos;
    }

    /**
     * When user resets this module
     */
    void onReset() override {
        muted = false;
        solo = false;
        directOutsPremute = false;
        levelSlew = true;
        aux1_send_amt = 0.0f;
        aux2_send_amt = 0.0f;
    }

    void onSampleRateChange() override {
        levelSlewer.setSlewSpeed(SLEW_SPEED);
    }

    StereoVoltages signals = {};
    StereoVoltages daisySignals = {};
    StereoVoltages aux1Signals = {};
    StereoVoltages aux2Signals = {};
    StereoVoltages soloSignals = {};

    /**
     * PROCESS
     *
     * Called at sample rate
     */
    void process(const ProcessArgs &args) override {
        muted = params[MUTE_PARAM].getValue() > VALUE_OFF;
        solo = params[MUTE_PARAM].getValue() < VALUE_OFF;

        signals = {};
        daisySignals = {};
        aux1Signals = {};
        aux2Signals = {};
        soloSignals = {};

        // Assume this module is the first in the chain; it will get
        // overwritten if we receive a value from the left expander
        Vec firstPos = widgetPos;

        // Get inputs from this channel strip
        if (!muted || directOutsPremute) {
            const float gain = params[CH_LVL_PARAM].getValue();
            const float pan = params[PAN_PARAM].getValue();

            signals.channels = std::max(inputs[CH_INPUT_1].getChannels(), inputs[CH_INPUT_2].getChannels());
            if (solo && signals.channels == 0) {
                // Force it to be one if this channel strip is solo so we pull
                // in even blank input
                signals.channels = 1;
            }

            inputs[CH_INPUT_1].readVoltages(signals.voltages_l);
            if (inputs[CH_INPUT_2].isConnected()) {
                inputs[CH_INPUT_2].readVoltages(signals.voltages_r);
            } else {
                // Copy signals from ch1 into ch2
                inputs[CH_INPUT_1].readVoltages(signals.voltages_r);
            }

            for (int c = 0; c < signals.channels; c++) {
                signals.voltages_l[c] *= std::cos(M_PI * (pan + 1.0f) / 4.0f) * std::pow(gain, 2.f);
                signals.voltages_r[c] *= std::sin(M_PI * (pan + 1.0f) / 4.0f) * std::pow(gain, 2.f);
            }

            if (inputs[LVL_CV_INPUT].isConnected()) {
                for (int c = 0; c < signals.channels; c++) {
                    float _cv = clamp(inputs[LVL_CV_INPUT].getPolyVoltage(c) / 10.f, 0.f, 1.f);
                    if (levelSlew) {
                        _cv = levelSlewer.process(_cv);
                    }
                    signals.voltages_l[c] *= _cv;
                    signals.voltages_r[c] *= _cv;
                }
            }
        }

        // Set output for this channel strip
        outputs[CH_OUTPUT_1].setChannels(signals.channels);
        outputs[CH_OUTPUT_1].writeVoltages(signals.voltages_l);

        outputs[CH_OUTPUT_2].setChannels(signals.channels);
        outputs[CH_OUTPUT_2].writeVoltages(signals.voltages_r);

        if (muted && directOutsPremute) {
            signals = {};
        }

        // Get daisy-chained data from left-side linked module
        if (leftExpander.module && (
                leftExpander.module->model == modelDaisyChannel2
                || leftExpander.module->model == modelDaisyChannelVu
                || leftExpander.module->model == modelDaisyChannelSends2
                || leftExpander.module->model == modelDaisyBlank
            )) {
            const DaisyMessage* msgFromModule = static_cast<DaisyMessage*>(leftExpander.consumerMessage);

            daisySignals = msgFromModule->signals;
            aux1Signals = msgFromModule->aux1Signals;
            aux2Signals = msgFromModule->aux2Signals;
            soloSignals = msgFromModule->soloSignals;

            firstPos = Vec(msgFromModule->first_pos_x, msgFromModule->first_pos_y);
            channelStripId = msgFromModule->channel_strip_id;

            link_l = 0.8f;
        } else {
            channelStripId = 1;
            link_l = 0.0f;
        }

        const int maxChannels = std::max(daisySignals.channels, signals.channels);
        daisySignals.channels = maxChannels;

        // Set daisy-chained output to right-side linked module
        if (rightExpander.module && (
                rightExpander.module->model == modelDaisyMaster2
                || rightExpander.module->model == modelDaisyChannel2
                || rightExpander.module->model == modelDaisyChannelVu
                || rightExpander.module->model == modelDaisyChannelSends2
                || rightExpander.module->model == modelDaisyBlank
            )) {
            DaisyMessage* msgToModule = static_cast<DaisyMessage*>(rightExpander.module->leftExpander.producerMessage);

            // Write this module's output along to single voltages pipe
            msgToModule->singleSignals = signals;

            msgToModule->aux1Signals.channels = maxChannels;
            msgToModule->aux2Signals.channels = maxChannels;

            for (int c = 0; c < maxChannels; c++) {
                // Combine this module's signal with daisy-chain
                daisySignals.voltages_l[c] += signals.voltages_l[c] / DAISY_DIVISOR;
                daisySignals.voltages_r[c] += signals.voltages_r[c] / DAISY_DIVISOR;

                msgToModule->aux1Signals.voltages_l[c] = aux1Signals.voltages_l[c]
                    + (aux1_send_amt * signals.voltages_l[c]);
                msgToModule->aux1Signals.voltages_r[c] = aux1Signals.voltages_r[c]
                    + (aux1_send_amt * signals.voltages_r[c]);

                msgToModule->aux2Signals.voltages_l[c] = aux2Signals.voltages_l[c]
                    + (aux2_send_amt * signals.voltages_l[c]);
                msgToModule->aux2Signals.voltages_r[c] = aux2Signals.voltages_r[c]
                    + (aux2_send_amt * signals.voltages_r[c]);
            }

            // Write daisy-chain signal to producer message
            msgToModule->signals.writeVoltages(daisySignals, maxChannels * 1);

            if (solo) {
                // Sum the daisy received solo signals with this module's signals
                msgToModule->soloSignals.channels = std::max(signals.channels, soloSignals.channels);
                for (int c = 0; c < msgToModule->soloSignals.channels; c++) {
                    msgToModule->soloSignals.voltages_l[c] = soloSignals.voltages_l[c] + signals.voltages_l[c];
                    msgToModule->soloSignals.voltages_r[c] = soloSignals.voltages_r[c] + signals.voltages_r[c];
                }
            } else {
                msgToModule->soloSignals = soloSignals;
            }

            msgToModule->first_pos_x = firstPos.x;
            msgToModule->first_pos_y = firstPos.y;
            msgToModule->channel_strip_id = channelStripId + 1;

            rightExpander.module->leftExpander.messageFlipRequested = true;

            link_r = 0.8f;
        } else {
            link_r = 0.0f;
        }

        if (link_l > 0.0f || link_r > 0.0f) {
            label = std::to_string(channelStripId);
        } else {
            channelStripId = 1;
            label = "";
        }

        // Set lights
        if (lightDivider.process()) {
            lights[MUTE_LIGHT].value = (muted);
            lights[MUTE2_LIGHT].value = (solo);
            lights[LINK_LIGHT_L].setBrightness(link_l);
            lights[LINK_LIGHT_R].setBrightness(link_r);
            lights[AUX1_LIGHT].setBrightness(aux1_send_amt);
            lights[AUX2_LIGHT].setBrightness(aux2_send_amt);
        }
    }
};

/**
 * Struct for keeping track of the value in the right click menu for the aux sends
 */
struct SendQuantity : Quantity {
    DaisyChannel2* _module;
    int _group;

    SendQuantity(DaisyChannel2 *m, int g) : _module(m), _group(g) {}

    void setValue(float value) override {
        value = clamp(value, getMinValue(), getMaxValue());
        if (_module && _group == 1) {
            _module->aux1_send_amt = value;
        }
        if (_module && _group == 2) {
            _module->aux2_send_amt = value;
        }
    }

    float getValue() override {
        if (_module && _group == 1) {
            return _module->aux1_send_amt;
        }
        if (_module && _group == 2) {
            return _module->aux2_send_amt;
        }
        return getDefaultValue();
    }

    float getMinValue() override {
        return 0.0f;
    }
    float getMaxValue() override {
        return 1.0f;
    }
    float getDefaultValue() override {
        return 0.f;
    }
    float getDisplayValue() override {
        return roundf(100.0f * getValue());
    }
    void setDisplayValue(float displayValue) override {
        setValue(displayValue / 100.0f);
    }
    std::string getLabel() override {
        return string::f("Aux Group %d Send Amt", _group);
    }
    std::string getUnit() override {
        return "%";
    }
};

/**
 * Slider used in menu for group aux send amounts
 */
template<class Q, int g>
struct DaisyMenuSlider : ui::Slider {
    explicit DaisyMenuSlider(DaisyChannel2 *module) {
        quantity = new Q(module, g);
        box.size.x = 200.0f;
    }
    virtual ~DaisyMenuSlider() {
        delete quantity;
    }
};

/**
 * Display at top of module to show the channel strip number
 */
struct DaisyDisplay : LedDisplay {
    DaisyChannel2* module {};
    std::string fontPath = asset::plugin(pluginInstance, "res/fonts/EnvyCodeR-Bold.ttf");
    std::string text;

    NVGcolor color = nvgRGB(0xff, 0xff, 0xff);
    NVGcolor bgColor = nvgRGB(0xff, 0x00, 0x00);

    void setModule(DaisyChannel2* module) {
        if (!module) {
            return;
        }
        this->module = module;
    }

    void draw(const DrawArgs& args) override {
        if (!module) {
            return;
        }
        text = module->label;

        if (text.empty()) {
            return;
        }

        // Background
        nvgBeginPath(args.vg);
        nvgRoundedRect(args.vg, 0, 0, box.size.x, box.size.y, 0);
        nvgFillColor(args.vg, nvgRGB(0xc9, 0x18, 0x47));
        nvgFill(args.vg);

        const std::shared_ptr<Font> font = APP->window->loadFont(fontPath);
        if (!font) {
            return;
        }

        nvgFontFaceId(args.vg, font->handle);
        nvgFontSize(args.vg, 14);
        nvgTextLetterSpacing(args.vg, 0.0);
        nvgTextAlign(args.vg, NVG_ALIGN_CENTER);

        // Foreground text
        nvgFillColor(args.vg, nvgRGB(0xff, 0xff, 0xff));
        nvgText(args.vg, RACK_GRID_WIDTH - 1, 12, text.c_str(), nullptr);
    }
};

/** Reads two adjacent lightIds, so `lightId` and `lightId + 1` must be defined */
template <typename TBase = GrayModuleLightWidget>
struct TRedGreenLight : TBase {
    TRedGreenLight() {
        this->addBaseColor(SCHEME_RED);
        this->addBaseColor(SCHEME_GREEN);
    }
};
using RedGreenLight = TRedGreenLight<>;

/**
 * QuantalDualLatch
 *
 * A switch button that can have multiple states. The way to get to the max
 * state is to hold down the button for a few moments.
 */
template <typename TLight>
struct QuantalDualLatch : VCVLatch {
    // Keep track of how long user kept mouse button down
    int holdDuration = 0;

    // Whether hold duration was long enough to trigger max latch state
    bool holdSucceed = false;

    app::ModuleLightWidget* light;

    QuantalDualLatch() {
        light = new TLight;
        // Move center of light to center of box
        light->box.pos = box.size.div(2).minus(light->box.size.div(2));
        addChild(light);
    }

    app::ModuleLightWidget* getLight() {
        return light;
    }

    /**
     * When user starts clicking on latch
     */
    void onDragStart(const DragStartEvent& e) override {
        if (e.button != GLFW_MOUSE_BUTTON_LEFT) {
            return;
        }
    }

    /**
     * While user is holding down mouse button
     */
    void onDragMove(const DragMoveEvent& e) override {
        holdDuration++;
        if (!holdSucceed && holdDuration >= HOLD_TRIGGER_DURATION) {
            engine::ParamQuantity* pq = getParamQuantity();
            float oldValue = pq->getValue();
            pq->setValue(VALUE_SOLO);
            saveHistory(oldValue, pq);
            holdSucceed = true;
        }
    }

    /**
     * When user stops mouse click on latch
     */
    void onDragEnd(const DragEndEvent& e) override {
        ParamWidget::onDragEnd(e);

        if (e.button != GLFW_MOUSE_BUTTON_LEFT) {
            return;
        }

        engine::ParamQuantity* pq = getParamQuantity();
        if (pq && holdDuration < HOLD_TRIGGER_DURATION) {
            // Don't do this logic if user JUST triggered hold state

            float oldValue = pq->getValue();
            if (pq->getValue() == VALUE_SOLO || pq->getValue() == VALUE_MUTE) {
                // Going off mute or solo
                pq->setValue(VALUE_OFF);
            } else {
                // Muting
                pq->setValue(VALUE_MUTE);
            }
            saveHistory(oldValue, pq);
        }

        holdDuration = 0;
        holdSucceed = false;
    }

    /**
     * Save history of param change (for undo/redo integration)
     */
    void saveHistory(float oldValue, ParamQuantity* pq) {
        float newValue = pq->getValue();
        if (oldValue != newValue) {
            // Push ParamChange history action
            history::ParamChange* h = new history::ParamChange;
            h->name = "move switch";
            h->moduleId = module->id;
            h->paramId = paramId;
            h->oldValue = oldValue;
            h->newValue = newValue;
            APP->history->push(h);
        }
    }
};

struct DaisyChannelWidget2 : ModuleWidget {

    dsp::ClockDivider uiDivider;

    /**
     * Constructor
     */
    explicit DaisyChannelWidget2(DaisyChannel2 *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/DaisyChannel2.svg"),
                asset::plugin(pluginInstance, "res/DaisyChannel2-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(0, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Label
        DaisyDisplay* display = createWidget<DaisyDisplay>(Vec(1, 16));
        display->box.size = Vec(RACK_GRID_WIDTH * 2 - 2, 16);
        display->setModule(module);
        addChild(display);

        // Channel Input/Output
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 45.0), module, DaisyChannel2::CH_INPUT_1));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 71.0), module, DaisyChannel2::CH_INPUT_2));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 290.0), module, DaisyChannel2::CH_OUTPUT_1));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 316.0), module, DaisyChannel2::CH_OUTPUT_2));

        // Level & CV
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 12.5f, 110.0), module, DaisyChannel2::LVL_CV_INPUT));
        addParam(createParam<LEDSliderGreen>(Vec(RACK_GRID_WIDTH - 10.5f, 138.4), module, DaisyChannel2::CH_LVL_PARAM));
        addParam(createParamCentered<Trimpot>(Vec(RACK_GRID_WIDTH - 0, 240.0), module, DaisyChannel2::PAN_PARAM));

        // Mute
        addParam(createLightParam<QuantalDualLatch<MediumSimpleLight<RedGreenLight>>>(Vec(RACK_GRID_WIDTH - 9.0f, 254.0), module, DaisyChannel2::MUTE_PARAM, DaisyChannel2::MUTE_LIGHT));

        // Link lights
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH - 4, 361.0f), module, DaisyChannel2::LINK_LIGHT_L));
        addChild(createLightCentered<TinyLight<YellowLight>>(Vec(RACK_GRID_WIDTH + 4, 361.0f), module, DaisyChannel2::LINK_LIGHT_R));

        // Aux lights
        addChild(createLightCentered<TinyLight<BlueLight>>(Vec(RACK_GRID_WIDTH - 10, 6.0f), module, DaisyChannel2::AUX1_LIGHT));
        addChild(createLightCentered<TinyLight<BlueLight>>(Vec(RACK_GRID_WIDTH - 10, 11.0f), module, DaisyChannel2::AUX2_LIGHT));

        uiDivider.setDivision(DAISY_UI_DIVISION);
    }

    /**
     * Step function for widget
     */
    void step() override {
        if (uiDivider.process()) {
            DaisyChannel2 *module = getModule<DaisyChannel2>();

            if (this->box.pos.x > 0.00) {
                module->setWidgetPosition(this->box.pos);
            }
        }

        ModuleWidget::step();
    }

    /**
     * Make context menu items
     */
    void appendContextMenu(Menu *menu) override {
        DaisyChannel2 *module = getModule<DaisyChannel2>();

        menu->addChild(new MenuSeparator);
        menu->addChild(new DaisyMenuSlider<SendQuantity, 1>(module)); // Aux send group 1
        menu->addChild(new DaisyMenuSlider<SendQuantity, 2>(module)); // Aux send group 2
        menu->addChild(createBoolPtrMenuItem("Direct outs pre-mute", "", &module->directOutsPremute));
        menu->addChild(createBoolPtrMenuItem("Smooth level CV", "", &module->levelSlew));
    }

    /**
     * Handle key presses while mouse is hovering this widget
     */
    void onHoverKey(const HoverKeyEvent& e) override {
        if (e.action == GLFW_RELEASE) {
            if (e.keyName[0] == 'm') {
                DaisyChannel2 *module = getModule<DaisyChannel2>();
                // Toggle mute
                module->params[DaisyChannel2::MUTE_PARAM].setValue(module->muted ? VALUE_OFF : VALUE_MUTE);
                e.consume(this);
            }
            if (e.keyName[0] == 's') {
                DaisyChannel2 *module = getModule<DaisyChannel2>();
                // Toggle solo
                module->params[DaisyChannel2::MUTE_PARAM].setValue(module->solo ? VALUE_OFF : VALUE_SOLO);
                e.consume(this);
            }
        }
        ModuleWidget::onHoverKey(e);
    }
};

Model* modelDaisyChannel2 = createModel<DaisyChannel2, DaisyChannelWidget2>("DaisyChannel2");

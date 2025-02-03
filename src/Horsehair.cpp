#include "QuantalAudio.hpp"

using simd::float_4;

template <typename T>
T sin2pi_pade_05_5_4(T x) {
    x -= 0.5f;
    return (T(-6.283185307) * x + T(33.19863968) * simd::pow(x, 3) - T(32.44191367) * simd::pow(x, 5))
           / (1 + T(1.296008659) * simd::pow(x, 2) + T(0.7028072946) * simd::pow(x, 4));
}

template <typename T>
T expCurve(T x) {
    return (3 + x * (-13 + 5 * x)) / (3 + 2 * x);
}

template <int OVERSAMPLE, int QUALITY, typename T>
struct VoltageControlledOscillator {
    bool analog = true;
    bool soft = false;
    bool syncEnabled = false;
    // For optimizing in serial code
    int channels = 0;

    T lastSyncValue = 0.f;
    T phase = 0.f;
    T freq{};
    T pulseWidth = 0.5f;
    T syncDirection = 1.f;

    dsp::TRCFilter<T> sqrFilter;

    dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sqrMinBlep;
    dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sawMinBlep;
    dsp::MinBlepGenerator<QUALITY, OVERSAMPLE, T> sinMinBlep;

    T sqrValue = 0.f;
    T sawValue = 0.f;
    T sinValue = 0.f;

    void setPitch(T pitch) {
        freq = dsp::FREQ_C4 * dsp::approxExp2_taylor5(pitch + 30.f) / std::pow(2.f, 30.f);
    }

    void setPulseWidth(T pulseWidth) {
        constexpr float pwMin = 0.01f;
        this->pulseWidth = simd::clamp(pulseWidth, pwMin, 1.f - pwMin);
    }

    void process(float deltaTime, T syncValue) {
        // Advance phase
        T deltaPhase = simd::clamp(freq * deltaTime, 1e-6f, 0.35f);
        if (soft) {
            // Reverse direction
            deltaPhase *= syncDirection;
        } else {
            // Reset back to forward
            syncDirection = 1.f;
        }
        phase += deltaPhase;
        // Wrap phase
        phase -= simd::floor(phase);

        // Jump sqr when crossing 0, or 1 if backwards
        T wrapPhase = (syncDirection == -1.f) & 1.f;
        T wrapCrossing = (wrapPhase - (phase - deltaPhase)) / deltaPhase;
        int wrapMask = simd::movemask((0 < wrapCrossing) & (wrapCrossing <= 1.f));
        if (wrapMask) {
            for (int i = 0; i < channels; i++) {
                if (wrapMask & (1 << i)) {
                    T mask = simd::movemaskInverse<T>(1 << i);
                    float p = wrapCrossing[i] - 1.f;
                    T x = mask & (2.f * syncDirection);
                    sqrMinBlep.insertDiscontinuity(p, x);
                }
            }
        }

        // Jump sqr when crossing `pulseWidth`
        T pulseCrossing = (pulseWidth - (phase - deltaPhase)) / deltaPhase;
        int pulseMask = simd::movemask((0 < pulseCrossing) & (pulseCrossing <= 1.f));
        if (pulseMask) {
            for (int i = 0; i < channels; i++) {
                if (pulseMask & (1 << i)) {
                    T mask = simd::movemaskInverse<T>(1 << i);
                    float p = pulseCrossing[i] - 1.f;
                    T x = mask & (-2.f * syncDirection);
                    sqrMinBlep.insertDiscontinuity(p, x);
                }
            }
        }

        // Jump saw when crossing 0.5
        T halfCrossing = (0.5f - (phase - deltaPhase)) / deltaPhase;
        const int halfMask = simd::movemask((0 < halfCrossing) & (halfCrossing <= 1.f));
        if (halfMask) {
            for (int i = 0; i < channels; i++) {
                if (halfMask & (1 << i)) {
                    T mask = simd::movemaskInverse<T>(1 << i);
                    float p = halfCrossing[i] - 1.f;
                    T x = mask & (-2.f * syncDirection);
                    sawMinBlep.insertDiscontinuity(p, x);
                }
            }
        }

        // Square
        sqrValue = sqr(phase);
        sqrValue += sqrMinBlep.process();

        if (analog) {
            sqrFilter.setCutoffFreq(20.f * deltaTime);
            sqrFilter.process(sqrValue);
            sqrValue = sqrFilter.highpass() * 0.95f;
        }

        // Saw
        sawValue = saw(phase);
        sawValue += sawMinBlep.process();

        // Sin
        sinValue = sin(phase);
        sinValue += sinMinBlep.process();
    }

    T sin(T phase) {
        T v{};
        if (analog) {
            // Quadratic approximation of sine, slightly richer harmonics
            T halfPhase = (phase < 0.5f);
            T x = phase - simd::ifelse(halfPhase, 0.25f, 0.75f);
            v = 1.f - 16.f * simd::pow(x, 2);
            v *= simd::ifelse(halfPhase, 1.f, -1.f);
        } else {
            v = sin2pi_pade_05_5_4(phase);
        }
        return v;
    }
    T sin() {
        return sinValue;
    }

    T saw(T phase) {
        T v{};
        T x = phase + 0.5f;
        x -= simd::trunc(x);
        if (analog) {
            v = -expCurve(x);
        } else {
            v = 2 * x - 1;
        }
        return v;
    }
    T saw() {
        return sawValue;
    }

    T sqr(T phase) {
        T v = simd::ifelse(phase < pulseWidth, 1.f, -1.f);
        return v;
    }
    T sqr() {
        return sqrValue;
    }
};

struct Horsehair : Module {
    enum ParamIds {
        PITCH_PARAM,
        ENUMS(OCTAVE_PARAM, 2),
        ENUMS(SHAPE_PARAM, 2),
        ENUMS(PW_PARAM, 2),
        MIX_PARAM,
        NUM_PARAMS
    };
    enum InputIds {
        PITCH_INPUT,
        ENUMS(SHAPE_CV_INPUT, 2),
        ENUMS(PW_CV_INPUT, 2),
        MIX_CV_INPUT,
        NUM_INPUTS
    };
    enum OutputIds {
        SIN_OUTPUT,
        MIX_OUTPUT,
        NUM_OUTPUTS
    };
    enum LightsIds {
        OSC_LIGHT,
        NUM_LIGHTS
    };

    VoltageControlledOscillator<16, 16, float_4> oscillators[4];
    VoltageControlledOscillator<16, 16, float_4> oscillators2[4];

    Horsehair() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        configParam(PITCH_PARAM, -2.0f, 2.0f, 0.0f, "Pitch Tune");
        configParam(OCTAVE_PARAM + 0, -5.0f, 4.0f, -2.0f, "Osc A Octave", "");
        configParam(OCTAVE_PARAM + 1, -5.0f, 4.0f, -1.0f, "Osc B Octave", "");
        configParam(SHAPE_PARAM + 0, 0.0f, 1.0f, 0.0f, "Osc A Shape");
        configParam(SHAPE_PARAM + 1, 0.0f, 1.0f, 1.0f, "Osc B Shape");
        configParam(PW_PARAM + 0, 0.0f, 1.0f, 0.5f, "Osc A PW");
        configParam(PW_PARAM + 1, 0.0f, 1.0f, 0.5f, "Osc B PW");
        configParam(MIX_PARAM, 0.0f, 1.0f, 0.5f, "Mix level", "%", 0, 100);

        configInput(PITCH_INPUT, "Pitch");
        configInput(SHAPE_CV_INPUT + 0, "Osc A Shape CV");
        configInput(SHAPE_CV_INPUT + 1, "Osc B Shape CV");
        configInput(PW_CV_INPUT + 0, "Osc A PW CV");
        configInput(PW_CV_INPUT + 1, "Osc B PW CV");
        configInput(MIX_CV_INPUT, "Mix CV");
        configOutput(SIN_OUTPUT, "Sine");
        configOutput(MIX_OUTPUT, "Osc Mix");
    }

    void process(const ProcessArgs &args) override {
        int channels = std::max(inputs[PITCH_INPUT].getChannels(), 1);

        float octave = params[OCTAVE_PARAM + 0].getValue();
        float octave2 = params[OCTAVE_PARAM + 1].getValue();
        float pitch_fine = params[PITCH_PARAM].getValue() / 4.0f;
        float pw = params[PW_PARAM + 0].getValue();
        float pw2 = params[PW_PARAM + 1].getValue();

        float shape = clamp(params[SHAPE_PARAM + 0].getValue(), 0.0f, 1.0f);
        if (inputs[SHAPE_CV_INPUT + 0].isConnected()) {
            shape += inputs[SHAPE_CV_INPUT + 0].getVoltage() / 10.0f;
            shape = clamp(shape, 0.0f, 1.0f);
        }

        float shape2 = clamp(params[SHAPE_PARAM + 1].getValue(), 0.0f, 1.0f);
        if (inputs[SHAPE_CV_INPUT + 1].isConnected()) {
            shape2 += inputs[SHAPE_CV_INPUT + 1].getVoltage() / 10.0f;
            shape2 = clamp(shape2, 0.0f, 1.0f);
        }

        float_4 out = 0.0f;
        float_4 out2 = 0.0f;

        for (int c = 0; c < channels; c += 4) {
            auto *oscillator = &oscillators[c / 4];
            oscillator->channels = std::min(channels - c, 4);
            float_4 pitch = 1.0f + roundf(octave) + pitch_fine;
            pitch += inputs[PITCH_INPUT].getVoltageSimd<float_4>(c);
            oscillator->setPitch(pitch);
            oscillator->setPulseWidth(pw + inputs[PW_CV_INPUT + 0].getPolyVoltageSimd<float_4>(c) / 10.f);
            oscillator->process(args.sampleTime, 0.0);

            auto *oscillator2 = &oscillators2[c / 4];
            oscillator2->channels = std::min(channels - c, 4);
            float_4 pitch2 = 1.0f + roundf(octave2) + pitch_fine;
            pitch2 += inputs[PITCH_INPUT].getVoltageSimd<float_4>(c);
            oscillator2->setPitch(pitch2);
            oscillator2->setPulseWidth(pw2 + inputs[PW_CV_INPUT + 1].getPolyVoltageSimd<float_4>(c) / 10.f);
            oscillator2->process(args.sampleTime, 0.0);

            if (outputs[MIX_OUTPUT].isConnected()) {
                out = simd::crossfade(oscillator->sqr(), oscillator->saw(), shape);
                out2 = simd::crossfade(oscillator2->sqr(), oscillator2->saw(), shape2);
                const float_4 mix = clamp(params[MIX_PARAM].getValue() + inputs[MIX_CV_INPUT].getPolyVoltageSimd<float_4>(c) / 10.0, 0.0f, 1.0f);
                outputs[MIX_OUTPUT].setChannels(channels);
                outputs[MIX_OUTPUT].setVoltageSimd(5.0f * simd::crossfade(out, out2, mix), c);
            }

            if (outputs[SIN_OUTPUT].isConnected()) {
                outputs[SIN_OUTPUT].setChannels(channels);
                outputs[SIN_OUTPUT].setVoltageSimd(5.0f * oscillator->sin(), c);
            }
        }
    }
};

struct HorsehairWidget : ModuleWidget {
    explicit HorsehairWidget(Horsehair *module) {
        setModule(module);
        setPanel(
            createPanel(
                asset::plugin(pluginInstance, "res/Horsehair.svg"),
                asset::plugin(pluginInstance, "res/Horsehair-dark.svg")
            )
        );

        // Screws
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ThemedScrew>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ThemedScrew>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        // Pitch & CThemedV
        addParam(createParam<RoundSmallBlackKnob>(Vec(RACK_GRID_WIDTH * 4 + 3, 50.0), module, Horsehair::PITCH_PARAM));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH + 3, 50.0), module, Horsehair::PITCH_INPUT));

        // Octave
        addParam(createParam<RoundBlackSnapKnob>(Vec(RACK_GRID_WIDTH, 93.0), module, Horsehair::OCTAVE_PARAM + 0));
        addParam(createParam<RoundBlackSnapKnob>(Vec(RACK_GRID_WIDTH * 4, 93.0), module, Horsehair::OCTAVE_PARAM + 1));

        // Shape
        addParam(createParam<RoundBlackKnob>(Vec(RACK_GRID_WIDTH, 142.0), module, Horsehair::SHAPE_PARAM + 0));
        addParam(createParam<RoundBlackKnob>(Vec(RACK_GRID_WIDTH * 4, 142.0), module, Horsehair::SHAPE_PARAM + 1));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 11.5f, 172.0), module, Horsehair::SHAPE_CV_INPUT + 0));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH * 4 + 16.5f, 172.0), module, Horsehair::SHAPE_CV_INPUT + 1));

        // Pulse width
        addParam(createParam<RoundBlackKnob>(Vec(RACK_GRID_WIDTH, 215.0), module, Horsehair::PW_PARAM + 0));
        addParam(createParam<RoundBlackKnob>(Vec(RACK_GRID_WIDTH * 4, 215.0), module, Horsehair::PW_PARAM + 1));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 11.5f, 245.0), module, Horsehair::PW_CV_INPUT + 0));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH * 4 + 16.5f, 245.0), module, Horsehair::PW_CV_INPUT + 1));

        // Osc Mix
        addParam(createParam<RoundLargeBlackKnob>(Vec(RACK_GRID_WIDTH * 3.5f - (38.0f / 2), 264.0), module, Horsehair::MIX_PARAM));
        addInput(createInput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH - 8, 277.0), module, Horsehair::MIX_CV_INPUT));

        // Output
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH + 3, 320.0), module, Horsehair::MIX_OUTPUT));
        addOutput(createOutput<ThemedPJ301MPort>(Vec(RACK_GRID_WIDTH * 4 + 3, 320.0), module, Horsehair::SIN_OUTPUT));
    }
};

Model* modelHorsehair = createModel<Horsehair, HorsehairWidget>("Horsehair");
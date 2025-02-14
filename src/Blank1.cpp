#include "QuantalAudio.hpp"

struct Blank1Widget : ModuleWidget {
    explicit Blank1Widget(Module *module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/blank-1.svg")));
    }
};

Model* modelBlank1 = createModel<Module, Blank1Widget>("Blank1");

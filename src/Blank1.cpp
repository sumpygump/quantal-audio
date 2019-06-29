#include "QuantalAudio.hpp"


struct Blank1Widget : ModuleWidget {
	Blank1Widget(Module *module) {
		setModule(module);
		setPanel(APP->window->loadSvg(assetPlugin(pluginInstance, "res/blank-1.svg")));
	}
};


Model *modelBlank1 = createModel<Module, Blank1Widget>("Blank1");

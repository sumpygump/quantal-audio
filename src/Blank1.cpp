#include "QuantalAudio.hpp"


struct Blank1Widget : ModuleWidget {
	Blank1Widget(Module *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/blank-1.svg")));
	}
};


Model *modelBlank1 = Model::create<Module, Blank1Widget>("Blank1");

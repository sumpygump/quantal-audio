#include "QuantalAudio.hpp"


struct Blank3Widget : ModuleWidget {
	Blank3Widget(Module *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(pluginInstance, "res/blank-3.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	}
};


Model *modelBlank3 = Model::create<Module, Blank3Widget>("Blank3");

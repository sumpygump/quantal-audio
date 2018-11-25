#include "QuantalAudio.hpp"


struct Blank3Widget : ModuleWidget {
	Blank3Widget(Module *module) : ModuleWidget(module) {
		setPanel(SVG::load(assetPlugin(plugin, "res/blank-3.svg")));

		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(Widget::create<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	}
};


Model *modelBlank3 = Model::create<Module, Blank3Widget>("QuantalAudio", "Blank3", "Blank Panel - 3 Slots Wide", BLANK_TAG);

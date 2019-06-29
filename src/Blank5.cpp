#include "QuantalAudio.hpp"


struct Blank5Widget : ModuleWidget {
	Blank5Widget(Module *module) {
		setModule(module);
		setPanel(SVG::load(assetPlugin(pluginInstance, "res/blank-5.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
	}
};


Model *modelBlank5 = createModel<Module, Blank5Widget>("Blank5");

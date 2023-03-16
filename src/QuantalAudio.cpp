#include "QuantalAudio.hpp"

Plugin *pluginInstance;

void init(Plugin *p) {
    pluginInstance = p;

    // Add all Models defined throughout the pluginInstance
    p->addModel(modelMasterMixer);
    p->addModel(modelBufferedMult);
    p->addModel(modelUnityMix);
    p->addModel(modelDaisyChannel);
    p->addModel(modelDaisyChannel2);
    p->addModel(modelDaisyChannelSends2);
    p->addModel(modelDaisyChannelVu);
    p->addModel(modelDaisyMaster);
    p->addModel(modelDaisyMaster2);
    p->addModel(modelHorsehair);
    p->addModel(modelBlank1);
    p->addModel(modelBlank3);
    p->addModel(modelBlank5);

    // Any other pluginInstance initialization may go here.
    // As an alternative, consider lazy-loading assets and lookup tables when your module is created to reduce startup times of Rack.
}

#pragma once
#include "rack.hpp"

using namespace rack;

// Forward-declare the Plugin, defined in Template.cpp
extern Plugin *pluginInstance;

// Forward-declare each Model, defined in each module source file
extern Model *modelMasterMixer;
extern Model *modelBufferedMult;
extern Model *modelUnityMix;
extern Model *modelDaisyChannel;
extern Model *modelDaisyChannel2;
extern Model *modelDaisyMaster;
extern Model *modelDaisyMaster2;
extern Model *modelHorsehair;
extern Model *modelBlank1;
extern Model *modelBlank3;
extern Model *modelBlank5;

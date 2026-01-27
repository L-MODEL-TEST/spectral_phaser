#include "plugin_ui.hpp"

#include "../PluginProcessor.h"

PluginUi::PluginUi(EmptyAudioProcessor& p)
    : preset_(*p.preset_manager_) {
    addAndMakeVisible(preset_);

    auto& apvt = *p.value_tree_;
    pitch_.BindParam(apvt, "pitch");
    addAndMakeVisible(pitch_);
    phase_.BindParam(apvt, "phase");
    addAndMakeVisible(phase_);
    morph_.BindParam(apvt, "morph");
    addAndMakeVisible(morph_);
}

void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(30));

    auto line = b.removeFromTop(65);
    pitch_.setBounds(line.removeFromLeft(50));
    phase_.setBounds(line.removeFromLeft(50));
    morph_.setBounds(line.removeFromLeft(50));
}

void PluginUi::paint(juce::Graphics& g) {
    
}

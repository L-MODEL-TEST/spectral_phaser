#include "plugin_ui.hpp"

#include "../PluginProcessor.h"

PluginUi::PluginUi(EmptyAudioProcessor& p)
    : processor_(p)
    , preset_(*p.preset_manager_) {
    addAndMakeVisible(preset_);

    for (size_t i = 0; i < phaser::SpectralPhaser::kNumLayers; ++i) {
        phaser_layer_.AddCube(juce::String{i});
    }
    addAndMakeVisible(phaser_layer_);
    phaser_layer_.on_value_changed = [this](size_t i) { OnLayerSelect(i); };

    addAndMakeVisible(enable_);
    addAndMakeVisible(drywet_);
    addAndMakeVisible(pitch_);
    addAndMakeVisible(phase_);
    addAndMakeVisible(morph_);
    addAndMakeVisible(freq_);
    phasy_.BindParam(*p.value_tree_, "phasy");
    addAndMakeVisible(phasy_);

    phaser_layer_.Set(0);
}

void PluginUi::resized() {
    auto b = getLocalBounds();
    preset_.setBounds(b.removeFromTop(30));

    {
        int w = 30;
        auto line = b.removeFromTop(w);
        phasy_.setBounds(line.removeFromRight(50).reduced(2, 0));
        phaser_layer_.setBounds(line);
        auto& all_layer = phaser_layer_.GetAllCubes();

        line = phaser_layer_.getLocalBounds().withSize(w * static_cast<int>(all_layer.size()), w);
        for (auto& layer : all_layer) {
            layer->setBounds(line.removeFromLeft(w).reduced(2, 2));
        }
    }

    {
        auto line = b.removeFromTop(65);
        enable_.setBounds(line.removeFromLeft(30).withSizeKeepingCentre(30, 30).reduced(2, 2));
        drywet_.setBounds(line.removeFromLeft(50));
        pitch_.setBounds(line.removeFromLeft(50));
        phase_.setBounds(line.removeFromLeft(50));
        morph_.setBounds(line.removeFromLeft(50));
        freq_.setBounds(line.removeFromLeft(50));
    }
}

void PluginUi::paint(juce::Graphics& g) {}

void PluginUi::OnLayerSelect(size_t i) {
    enable_.BindParam(*processor_.value_tree_, "enable" + juce::String{i});
    drywet_.BindParam(*processor_.value_tree_, "drywet" + juce::String{i});
    pitch_.BindParam(*processor_.value_tree_, "pitch" + juce::String{i});
    phase_.BindParam(*processor_.value_tree_, "phase" + juce::String{i});
    morph_.BindParam(*processor_.value_tree_, "morph" + juce::String{i});
    freq_.BindParam(processor_.layer_lfo_[i]);
}

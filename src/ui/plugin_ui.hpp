#pragma once

#include "pluginshared/component.hpp"
#include "pluginshared/preset_panel.hpp"

class EmptyAudioProcessor;

class PluginUi : public juce::Component {
public:
    static constexpr int kWidth = 480;
    static constexpr int kHeight = 320;

    explicit PluginUi(EmptyAudioProcessor& p);

    void resized() override;
    void paint(juce::Graphics& g) override;
private:
    pluginshared::PresetPanel preset_;

    ui::Dial pitch_{"pitch"};
    ui::Dial phase_{"phase"};
    ui::Dial morph_{"morph"};
    ui::Switch phasy_{"phasy"};
};

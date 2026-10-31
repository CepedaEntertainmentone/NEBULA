#pragma once
#include "PluginProcessor.h"
#include <array>
#include <memory>

// Aspecto de las perillas y menus
class NebulaLookAndFeel : public juce::LookAndFeel_V4
{
public:
    NebulaLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

// El cielo donde se dibuja la constelacion
class ConstellationView : public juce::Component
{
public:
    explicit ConstellationView (NebulaProcessor& p);

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;

private:
    void setFromMouse (const juce::MouseEvent&);
    void setStar (int index, float value);
    float getStar (int index) const;
    float xAt (float u) const;
    float yAt (float v) const;

    NebulaProcessor& proc;
    std::array<std::atomic<float>*, 16> starVals {};
    std::array<juce::RangedAudioParameter*, 16> starParams {};
    std::array<bool, 16> touched {};
    int lastIdx = -1;
    float lastVal = 0.0f;
    float levelSm = 0.0f;
    static constexpr float pad = 30.0f;
};

class NebulaEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit NebulaEditor (NebulaProcessor&);
    ~NebulaEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override { repaint(); }
    void bigBang();

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
    };

    struct BgStar { float x, y, size, phase, speed; };

    NebulaProcessor& proc;
    NebulaLookAndFeel lnf;
    ConstellationView sky;

    juce::ComboBox presetBox, scaleBox, divBox, cycleBox;
    juce::TextButton bigBangButton { "BIG BANG" };
    std::array<Knob, 10> knobs;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> presetAtt, scaleAtt, divAtt, cycleAtt;

    juce::Image bgImage;
    std::array<BgStar, 240> bgStars;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NebulaEditor)
};

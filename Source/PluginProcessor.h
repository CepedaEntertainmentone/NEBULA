#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <array>
#include <atomic>

class NebulaProcessor : public juce::AudioProcessor,
                        private juce::AudioProcessorValueTreeState::Listener,
                        private juce::AsyncUpdater
{
public:
    NebulaProcessor();
    ~NebulaProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Nebula"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.5; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    std::atomic<float> uiPhase { 0.0f }, uiLevel { 0.0f };

private:
    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void handleAsyncUpdate() override;
    void applyPreset (int index);

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static float readInterp (const juce::AudioBuffer<float>& ring, int ch, int writeIdx, int size, float delay);

    juce::AudioProcessorValueTreeState apvts;
    std::array<std::atomic<float>*, 16> starParams {};

    // Buffer 1: audio original (para el stutter y la senal seca)
    juce::AudioBuffer<float> preRing;
    int preSize = 0, preIdx = 0;

    // Buffer 2: audio que pasa por el cambio de pitch
    juce::AudioBuffer<float> pitchRing;
    int pSize = 0, pIdx = 0;

    double sr = 44100.0;
    float windowSamples = 3528.0f;
    float baseDelay = 3528.0f;

    float phaseA = 0.0f;
    float delayA = 0.0f, delayB = 0.0f;

    double cyclePhase = 0.0;
    float cs = 1.0f;         // curva suavizada (volumen y filtro)
    float semisSm = 0.0f;    // semitonos suavizados (glide)

    bool stutterActive = false;
    int loopStart = 0, loopLen = 1, loopPhase = 0;

    float ic1eq[2] = { 0.0f, 0.0f };
    float ic2eq[2] = { 0.0f, 0.0f };

    std::atomic<int> pendingPreset { 0 };
    int lastAppliedPreset = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NebulaProcessor)
};

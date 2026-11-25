#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Parameters.h"
#include "ScopeBuffer.h"

class PhosphorProcessor : public juce::AudioProcessor
{
public:
    PhosphorProcessor();
    ~PhosphorProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Phosphor XY"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    ScopeBuffer& getScope (int channel) noexcept { return scope[channel]; }

    std::atomic<double> currentSampleRate { 44100.0 };
    juce::AudioProcessorValueTreeState apvts;

private:
    ScopeBuffer scope[2];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhosphorProcessor)
};

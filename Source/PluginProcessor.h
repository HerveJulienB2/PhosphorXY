#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <memory>
#include <vector>
#include "Parameters.h"
#include "ScopeBuffer.h"
#include "GenTable.h"
#include "Generator.h"

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

    // Texte du generateur (interface uniquement). Il est sauvegarde avec le projet.
    void setGeneratorText (const juce::String& text);
    juce::String getGeneratorText() const;
    const GenTable* getTable() const noexcept { return currentTable.load (std::memory_order_acquire); }

    std::atomic<double> currentSampleRate { 44100.0 };
    juce::AudioProcessorValueTreeState apvts;

private:
    void rebuildTable();
    void scheduleRebuild();

    ScopeBuffer scope[2];

    // Les tables sont creees par l'interface et lues sans verrou par le thread audio.
    // Les plus anciennes sont gardees en vie quelques mises a jour avant d'etre liberees.
    std::atomic<const GenTable*> currentTable { nullptr };
    std::vector<std::unique_ptr<GenTable>> tableStore;

    GeneratorEngine engine;
    juce::SmoothedValue<float> gainIn, gainGen;
    std::atomic<float> *pSrc = nullptr, *pMode = nullptr, *pScale = nullptr,
                       *pLevel = nullptr, *pFreq = nullptr, *pStep = nullptr;

    JUCE_DECLARE_WEAK_REFERENCEABLE (PhosphorProcessor)
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PhosphorProcessor)
};

#include "PluginProcessor.h"
#include "PluginEditor.h"

PhosphorProcessor::PhosphorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", Params::createLayout())
{
}

void PhosphorProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate.store (sampleRate);
}

bool PhosphorProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
}

void PhosphorProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    // Le plugin est transparent : le signal passe sans modification, on ne fait que l'observer.
    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numCh < 1 || n < 1)
        return;

    scope[0].push (buffer.getReadPointer (0), n);
    scope[1].push (buffer.getReadPointer (numCh > 1 ? 1 : 0), n);
}

juce::AudioProcessorEditor* PhosphorProcessor::createEditor()
{
    return new PhosphorEditor (*this);
}

void PhosphorProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PhosphorProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhosphorProcessor();
}

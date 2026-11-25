#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "TextShape.h"

static const char* const kDefaultText = "PHOSPHOR";

PhosphorProcessor::PhosphorProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", Params::createLayout())
{
    pSrc   = apvts.getRawParameterValue ("gen_src");
    pMode  = apvts.getRawParameterValue ("gen_mode");
    pScale = apvts.getRawParameterValue ("gen_scale");
    pLevel = apvts.getRawParameterValue ("gen_level");
    pFreq  = apvts.getRawParameterValue ("gen_freq");
    pStep  = apvts.getRawParameterValue ("gen_step");

    scheduleRebuild();
}

void PhosphorProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate.store (sampleRate);
    engine.prepare (sampleRate);

    gainIn.reset (sampleRate, 0.03);
    gainGen.reset (sampleRate, 0.03);
    gainIn.setCurrentAndTargetValue (1.0f);
    gainGen.setCurrentAndTargetValue (0.0f);
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

    const int n = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();
    if (numCh < 1 || n < 1)
        return;

    // Source : 0 = entree seule (le plugin est transparent), 1 = entree + generateur, 2 = generateur seul.
    const int src = juce::roundToInt (pSrc->load());
    gainIn.setTargetValue (src == 2 ? 0.0f : 1.0f);
    gainGen.setTargetValue (src == 0 ? 0.0f : pLevel->load());

    GeneratorEngine::Params gp;
    gp.mode   = juce::roundToInt (pMode->load());
    gp.freq   = pFreq->load();
    gp.stepMs = pStep->load();
    gp.scale  = juce::roundToInt (pScale->load());
    engine.setTable (currentTable.load (std::memory_order_acquire));

    const bool needGen = src != 0 || gainGen.isSmoothing();

    if (needGen || gainIn.isSmoothing())
    {
        float* left  = buffer.getWritePointer (0);
        float* right = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

        for (int i = 0; i < n; ++i)
        {
            const float gi = gainIn.getNextValue();
            const float gg = gainGen.getNextValue();

            float gl = 0.0f, gr = 0.0f;
            if (needGen)
                engine.process (gp, gl, gr);

            if (right != nullptr)
            {
                left[i]  = left[i]  * gi + gl * gg;
                right[i] = right[i] * gi + gr * gg;
            }
            else
            {
                left[i] = left[i] * gi + 0.5f * (gl + gr) * gg;
            }
        }
    }

    // l'oscilloscope observe ce que l'on entend : le signal de sortie
    scope[0].push (buffer.getReadPointer (0), n);
    scope[1].push (buffer.getReadPointer (numCh > 1 ? 1 : 0), n);
}

//==============================================================================
juce::String PhosphorProcessor::getGeneratorText() const
{
    return apvts.state.getProperty ("text", kDefaultText).toString();
}

void PhosphorProcessor::setGeneratorText (const juce::String& text)
{
    apvts.state.setProperty ("text", text, nullptr);
    rebuildTable();
}

void PhosphorProcessor::rebuildTable()
{
    auto table = TextShape::build (getGeneratorText());
    const GenTable* raw = table.get();
    tableStore.push_back (std::move (table));
    currentTable.store (raw, std::memory_order_release);

    // le thread audio ne peut pas lire une table vieille de plus de quelques mises a jour
    if (tableStore.size() > 24)
        tableStore.erase (tableStore.begin(), tableStore.begin() + 8);
}

void PhosphorProcessor::scheduleRebuild()
{
    if (juce::MessageManager::getInstance()->isThisTheMessageThread())
    {
        rebuildTable();
        return;
    }

    juce::WeakReference<PhosphorProcessor> weak (this);
    juce::MessageManager::callAsync ([weak]
    {
        if (auto* self = weak.get())
            self->rebuildTable();
    });
}

//==============================================================================
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
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            scheduleRebuild();
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PhosphorProcessor();
}

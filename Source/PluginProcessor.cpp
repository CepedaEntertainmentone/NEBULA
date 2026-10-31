#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

namespace
{
    // ---------- PRESETS ----------
    struct Preset
    {
        const char* name;
        float stars[16];
        float pdepth, pcenter;
        int   scale;
        float glide;
        float vol, filt, res;
        float smix;
        int   sdiv;
        float sthresh;
        float smooth;
        int   cycle;
        float mix;
    };

    const Preset kPresets[] =
    {
        { "Agujero negro",
          { 1.00f, 0.93f, 0.87f, 0.80f, 0.73f, 0.67f, 0.60f, 0.53f, 0.47f, 0.40f, 0.33f, 0.27f, 0.20f, 0.13f, 0.07f, 0.00f },
          12.0f, 1.0f, 0, 0.0f,   0.9f, 0.7f, 0.25f,   0.0f, 2, 0.7f,   1.0f, 3, 1.0f },

        { "Supernova",
          { 0.00f, 0.10f, 0.20f, 0.30f, 0.40f, 0.50f, 0.60f, 0.70f, 0.75f, 0.80f, 0.85f, 0.90f, 0.95f, 1.00f, 1.00f, 1.00f },
          12.0f, 0.0f, 0, 0.0f,   0.0f, 0.0f, 0.20f,   1.0f, 2, 0.8f,   0.8f, 2, 1.0f },

        { "Eclipse",
          { 1.00f, 0.00f, 1.00f, 1.00f, 0.00f, 1.00f, 0.00f, 1.00f, 1.00f, 0.00f, 1.00f, 1.00f, 0.00f, 1.00f, 0.00f, 0.50f },
          0.0f, 1.0f, 0, 0.0f,   1.0f, 0.0f, 0.20f,   0.0f, 2, 0.7f,   0.1f, 2, 1.0f },

        { "Lluvia de meteoros",
          { 0.90f, 0.20f, 0.70f, 0.10f, 0.95f, 0.30f, 0.60f, 0.15f, 0.85f, 0.40f, 0.75f, 0.05f, 0.90f, 0.25f, 0.65f, 0.35f },
          24.0f, 0.5f, 4, 0.0f,   0.0f, 0.5f, 0.40f,   0.8f, 2, 0.85f,   0.0f, 2, 1.0f },

        { "Orbita lunar",
          { 0.50f, 0.69f, 0.85f, 0.96f, 1.00f, 0.96f, 0.85f, 0.69f, 0.50f, 0.31f, 0.15f, 0.04f, 0.00f, 0.04f, 0.15f, 0.31f },
          3.0f, 0.5f, 0, 0.0f,   0.0f, 0.3f, 0.20f,   0.0f, 2, 0.7f,   1.0f, 3, 1.0f },

        { "Arpegio estelar",
          { 0.50f, 0.67f, 0.83f, 1.00f, 0.83f, 0.67f, 0.50f, 0.33f, 0.50f, 0.67f, 0.83f, 1.00f, 0.83f, 0.50f, 0.33f, 0.00f },
          24.0f, 0.5f, 4, 10.0f,   0.0f, 0.0f, 0.20f,   0.0f, 2, 0.7f,   0.0f, 2, 1.0f },

        { "Cinta rota",
          { 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, 0.90f, 0.70f, 0.50f, 0.30f, 0.15f, 0.05f, 0.00f, 0.00f },
          12.0f, 1.0f, 0, 0.0f,   1.0f, 0.0f, 0.20f,   0.0f, 2, 0.7f,   1.0f, 3, 1.0f }
    };
    constexpr int kNumPresets = (int) (sizeof (kPresets) / sizeof (kPresets[0]));

    juce::StringArray presetNames()
    {
        juce::StringArray a;
        a.add ("Manual");
        for (const auto& p : kPresets)
            a.add (p.name);
        return a;
    }

    // ---------- UTILIDADES ----------
    float curveAt (float u, const float* stars, float smoothAmt)
    {
        const float x = u * 16.0f;
        int i = (int) x;
        if (i > 15) i = 15;
        if (i < 0) i = 0;
        const float f = x - (float) i;
        const int j = (i + 1) & 15;
        const float a = stars[i];
        const float b = stars[j];
        const float sm = f * f * (3.0f - 2.0f * f);
        const float interp = a + (b - a) * sm;
        return a + (interp - a) * smoothAmt;
    }

    float quantizeSemis (float s, int scale)
    {
        static const int chromatic[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };
        static const int major[]     = { 0, 2, 4, 5, 7, 9, 11 };
        static const int minorS[]    = { 0, 2, 3, 5, 7, 8, 10 };
        static const int pentMinor[] = { 0, 3, 5, 7, 10 };
        static const int wholeTone[] = { 0, 2, 4, 6, 8, 10 };

        const int* notes = nullptr;
        int count = 0;
        switch (scale)
        {
            case 1: notes = chromatic; count = 12; break;
            case 2: notes = major;     count = 7;  break;
            case 3: notes = minorS;    count = 7;  break;
            case 4: notes = pentMinor; count = 5;  break;
            case 5: notes = wholeTone; count = 6;  break;
            default: return s;
        }

        const int oct = (int) std::floor (s / 12.0f);
        float best = s;
        float bestDist = 1.0e9f;
        for (int o = oct - 1; o <= oct + 1; ++o)
            for (int k = 0; k < count; ++k)
            {
                const float cand = (float) (notes[k] + 12 * o);
                const float d = std::abs (cand - s);
                if (d < bestDist) { bestDist = d; best = cand; }
            }
        return best;
    }
}

// ---------- PROCESADOR ----------
NebulaProcessor::NebulaProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int k = 0; k < 16; ++k)
        starParams[(size_t) k] = apvts.getRawParameterValue (juce::String ("s") + juce::String (k + 1));

    apvts.addParameterListener ("preset", this);
}

NebulaProcessor::~NebulaProcessor()
{
    apvts.removeParameterListener ("preset", this);
    cancelPendingUpdate();
}

juce::AudioProcessorValueTreeState::ParameterLayout NebulaProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    using F = juce::AudioParameterFloat;
    using C = juce::AudioParameterChoice;
    using R = juce::NormalisableRange<float>;

    layout.add (std::make_unique<C> (juce::ParameterID { "preset", 1 }, "Preset", presetNames(), 0));
    layout.add (std::make_unique<C> (juce::ParameterID { "cycle", 1 }, "Ciclo",
                                     juce::StringArray { "1 pulso", "2 pulsos", "1 compas", "2 compases" }, 2));
    layout.add (std::make_unique<F> (juce::ParameterID { "smooth", 1 }, "Suavidad (0=escalones)", R (0.0f, 1.0f, 0.001f), 1.0f));

    for (int k = 0; k < 16; ++k)
        layout.add (std::make_unique<F> (juce::ParameterID { juce::String ("s") + juce::String (k + 1), 1 },
                                         "Estrella " + juce::String (k + 1), R (0.0f, 1.0f, 0.001f), 1.0f));

    layout.add (std::make_unique<F> (juce::ParameterID { "pdepth", 1 }, "Pitch profundidad (st)", R (-24.0f, 24.0f, 0.01f), 0.0f));
    layout.add (std::make_unique<F> (juce::ParameterID { "pcenter", 1 }, "Pitch centro", R (0.0f, 1.0f, 0.001f), 1.0f));
    layout.add (std::make_unique<C> (juce::ParameterID { "scale", 1 }, "Pitch escala",
                                     juce::StringArray { "Libre", "Cromatica", "Mayor", "Menor", "Pentatonica menor", "Tonos enteros" }, 0));
    layout.add (std::make_unique<F> (juce::ParameterID { "glide", 1 }, "Pitch deslizamiento (ms)", R (0.0f, 500.0f, 0.1f), 0.0f));

    layout.add (std::make_unique<F> (juce::ParameterID { "vol", 1 }, "Volumen cantidad", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (juce::ParameterID { "filt", 1 }, "Filtro cantidad", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<F> (juce::ParameterID { "res", 1 }, "Filtro resonancia", R (0.0f, 1.0f, 0.001f), 0.2f));

    layout.add (std::make_unique<F> (juce::ParameterID { "smix", 1 }, "Stutter cantidad", R (0.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<C> (juce::ParameterID { "sdiv", 1 }, "Stutter division",
                                     juce::StringArray { "1/4", "1/8", "1/16", "1/32" }, 2));
    layout.add (std::make_unique<F> (juce::ParameterID { "sthresh", 1 }, "Stutter umbral", R (0.0f, 1.0f, 0.001f), 0.7f));

    layout.add (std::make_unique<F> (juce::ParameterID { "mix", 1 }, "Mezcla", R (0.0f, 1.0f, 0.001f), 1.0f));
    return layout;
}

void NebulaProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    if (parameterID == "preset")
    {
        pendingPreset = (int) newValue;
        triggerAsyncUpdate();
    }
}

void NebulaProcessor::handleAsyncUpdate()
{
    const int idx = pendingPreset.load();
    if (idx == lastAppliedPreset)
        return;
    lastAppliedPreset = idx;
    if (idx > 0 && idx <= kNumPresets)
        applyPreset (idx - 1);
}

void NebulaProcessor::applyPreset (int index)
{
    const Preset& p = kPresets[index];
    auto setF = [this] (const juce::String& id, float v)
    {
        if (auto* prm = apvts.getParameter (id))
            prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    };

    for (int k = 0; k < 16; ++k)
        setF (juce::String ("s") + juce::String (k + 1), p.stars[k]);

    setF ("pdepth", p.pdepth);
    setF ("pcenter", p.pcenter);
    setF ("scale", (float) p.scale);
    setF ("glide", p.glide);
    setF ("vol", p.vol);
    setF ("filt", p.filt);
    setF ("res", p.res);
    setF ("smix", p.smix);
    setF ("sdiv", (float) p.sdiv);
    setF ("sthresh", p.sthresh);
    setF ("smooth", p.smooth);
    setF ("cycle", (float) p.cycle);
    setF ("mix", p.mix);
}

void NebulaProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    windowSamples = (float) std::floor (0.08 * sr);
    baseDelay = windowSamples;

    preSize = (int) (sr * 2.0) + 8;
    pSize = (int) (windowSamples * 5.5f) + 8;

    preRing.setSize (2, preSize);
    pitchRing.setSize (2, pSize);
    preRing.clear();
    pitchRing.clear();
    preIdx = pIdx = 0;

    phaseA = 0.0f;
    delayA = delayB = baseDelay;
    cyclePhase = 0.0;
    cs = 1.0f;
    semisSm = 0.0f;
    stutterActive = false;
    loopPhase = 0;
    ic1eq[0] = ic1eq[1] = ic2eq[0] = ic2eq[1] = 0.0f;

    setLatencySamples ((int) baseDelay);
}

bool NebulaProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return out == layouts.getMainInputChannelSet();
}

float NebulaProcessor::readInterp (const juce::AudioBuffer<float>& ring, int ch, int writeIdx, int size, float d)
{
    float pos = (float) writeIdx - d;
    while (pos < 0.0f)
        pos += (float) size;

    int i0 = (int) pos;
    const float frac = pos - (float) i0;
    if (i0 >= size) i0 -= size;
    int i1 = i0 + 1;
    if (i1 >= size) i1 -= size;

    const float* x = ring.getReadPointer (ch);
    return x[i0] + frac * (x[i1] - x[i0]);
}

void NebulaProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int n = buffer.getNumSamples();
    if (numCh == 0 || pSize == 0)
        return;

    auto P = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };

    float stars[16];
    for (int k = 0; k < 16; ++k)
        stars[k] = starParams[(size_t) k]->load();

    static const float cycleTable[4] = { 1.0f, 2.0f, 4.0f, 8.0f };
    static const float divTable[4]   = { 1.0f, 0.5f, 0.25f, 0.125f };

    const float cycleBeats = cycleTable[juce::jlimit (0, 3, (int) P ("cycle"))];
    const float divBeats   = divTable[juce::jlimit (0, 3, (int) P ("sdiv"))];
    const float smoothAmt  = P ("smooth");
    const float pdepth     = P ("pdepth");
    const float pcenter    = P ("pcenter");
    const int   scale      = (int) P ("scale");
    const float glideMs    = P ("glide");
    const float vol        = P ("vol");
    const float filt       = P ("filt");
    const float res        = P ("res");
    const float smix       = P ("smix");
    const float sthresh    = P ("sthresh");
    const float mix        = P ("mix");

    // Tempo del host
    double bpm = 120.0;
    bool playing = false, havePpq = false;
    double ppq = 0.0;
    if (auto* head = getPlayHead())
        if (auto pos = head->getPosition())
        {
            if (auto b = pos->getBpm())
                bpm = *b;
            playing = pos->getIsPlaying();
            if (auto q = pos->getPpqPosition())
            {
                ppq = *q;
                havePpq = true;
            }
        }
    if (bpm < 20.0) bpm = 120.0;

    if (playing && havePpq)
    {
        double ph = std::fmod (ppq / (double) cycleBeats, 1.0);
        if (ph < 0.0) ph += 1.0;
        cyclePhase = ph;
    }
    const double dPhase = bpm / (60.0 * sr * (double) cycleBeats);

    const float cCoef = 1.0f - std::exp (-1.0f / (0.002f * (float) sr));
    const float glideCoef = glideMs < 0.5f ? 1.0f : 1.0f - std::exp (-1.0f / (glideMs * 0.001f * (float) sr));
    const int L = juce::jlimit (64, preSize - 2, (int) ((double) divBeats * 60.0 / bpm * sr));
    const float phaseStep = 1.0f / windowSamples;
    const int dryOffset = (int) baseDelay;
    const float kRes = 1.0f / (0.7f + res * 7.0f);
    float peak = 0.0f;

    if (filt <= 0.001f)
        ic1eq[0] = ic1eq[1] = ic2eq[0] = ic2eq[1] = 0.0f;

    for (int i = 0; i < n; ++i)
    {
        // Curva de la constelacion en este instante
        const float c = curveAt ((float) cyclePhase, stars, smoothAmt);
        cyclePhase += dPhase;
        if (cyclePhase >= 1.0) cyclePhase -= 1.0;
        cs += (c - cs) * cCoef;

        // Pitch objetivo
        float semis = juce::jlimit (-12.0f, 12.0f, pdepth * (c - pcenter));
        semis = juce::jlimit (-12.0f, 12.0f, quantizeSemis (semis, scale));
        semisSm += (semis - semisSm) * glideCoef;
        const float rate = std::exp2 (semisSm / 12.0f);

        // Entrada y grabacion del audio original
        float in[2] = { 0.0f, 0.0f };
        for (int ch = 0; ch < numCh; ++ch)
        {
            in[ch] = buffer.getReadPointer (ch)[i];
            preRing.setSample (ch, preIdx, in[ch]);
        }

        // Stutter: cuando la estrella supera el umbral, se repite el ultimo fragmento
        const bool gate = smix > 0.001f && c > sthresh;
        if (gate && ! stutterActive)
        {
            stutterActive = true;
            loopLen = L;
            loopStart = preIdx - L;
            if (loopStart < 0) loopStart += preSize;
            loopPhase = 0;
        }
        else if (! gate)
        {
            stutterActive = false;
        }

        float s[2] = { in[0], in[1] };
        if (stutterActive)
        {
            const int fade = juce::jmax (1, juce::jmin (64, loopLen / 4));
            float g = 1.0f;
            if (loopPhase < fade)                   g = (float) loopPhase / (float) fade;
            else if (loopPhase > loopLen - fade)    g = (float) (loopLen - loopPhase) / (float) fade;

            int idx = loopStart + loopPhase;
            if (idx >= preSize) idx -= preSize;

            for (int ch = 0; ch < numCh; ++ch)
            {
                const float ls = preRing.getSample (ch, idx) * g;
                s[ch] = in[ch] + (ls - in[ch]) * smix;
            }
            if (++loopPhase >= loopLen)
                loopPhase = 0;
        }

        // Cambio de pitch con dos cabezales de lectura
        for (int ch = 0; ch < numCh; ++ch)
            pitchRing.setSample (ch, pIdx, s[ch]);

        const float prevA = phaseA;
        phaseA += phaseStep;
        if (phaseA >= 1.0f) phaseA -= 1.0f;
        if (phaseA < prevA) delayA = baseDelay;
        float phaseB = phaseA + 0.5f;
        if (phaseB >= 1.0f) phaseB -= 1.0f;
        if (prevA < 0.5f && phaseA >= 0.5f) delayB = baseDelay;

        delayA += (1.0f - rate);
        delayB += (1.0f - rate);
        delayA = juce::jlimit (1.0f, (float) (pSize - 2), delayA);
        delayB = juce::jlimit (1.0f, (float) (pSize - 2), delayB);

        const float sA = std::sin (juce::MathConstants<float>::pi * phaseA);
        const float sB = std::sin (juce::MathConstants<float>::pi * phaseB);
        const float wA = sA * sA;
        const float wB = sB * sB;

        // Filtro (coeficientes segun la curva)
        float a1 = 0.0f, a2 = 0.0f, a3 = 0.0f;
        if (filt > 0.001f)
        {
            float fc = 20000.0f * std::pow (0.01f, filt * (1.0f - cs));
            fc = juce::jlimit (20.0f, 0.45f * (float) sr, fc);
            const float g = std::tan (juce::MathConstants<float>::pi * fc / (float) sr);
            a1 = 1.0f / (1.0f + g * (g + kRes));
            a2 = g * a1;
            a3 = g * a2;
        }

        const float gain = 1.0f - vol * (1.0f - cs);

        int di = preIdx - dryOffset;
        if (di < 0) di += preSize;

        for (int ch = 0; ch < numCh; ++ch)
        {
            float w = wA * readInterp (pitchRing, ch, pIdx, pSize, delayA)
                    + wB * readInterp (pitchRing, ch, pIdx, pSize, delayB);

            if (filt > 0.001f)
            {
                const float v3 = w - ic2eq[ch];
                const float v1 = a1 * ic1eq[ch] + a2 * v3;
                const float v2 = ic2eq[ch] + a2 * ic1eq[ch] + a3 * v3;
                ic1eq[ch] = 2.0f * v1 - ic1eq[ch];
                ic2eq[ch] = 2.0f * v2 - ic2eq[ch];
                w = v2;
            }

            w *= gain;

            const float dry = preRing.getSample (ch, di);
            const float o = dry * (1.0f - mix) + w * mix;
            buffer.getWritePointer (ch)[i] = o;
            peak = juce::jmax (peak, std::abs (o));
        }

        if (++preIdx >= preSize) preIdx = 0;
        if (++pIdx >= pSize) pIdx = 0;
    }

    uiPhase.store ((float) cyclePhase);
    uiLevel.store (juce::jlimit (0.0f, 1.0f, peak));
}

juce::AudioProcessorEditor* NebulaProcessor::createEditor()
{
    return new NebulaEditor (*this);
}

void NebulaProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void NebulaProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        // Evita que al cargar un proyecto el preset pise los valores guardados
        lastAppliedPreset = (int) apvts.getRawParameterValue ("preset")->load();
        pendingPreset = lastAppliedPreset;
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new NebulaProcessor();
}

#include "PluginEditor.h"
#include <cmath>

// =====================================================================
//  LOOK AND FEEL
// =====================================================================
NebulaLookAndFeel::NebulaLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, juce::Colour (0xffcfd8ff));
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Label::textColourId, juce::Colour (0xffa9b4e8));

    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xcc14113a));
    setColour (juce::ComboBox::textColourId, juce::Colour (0xffe8ecff));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff5b4fd0));
    setColour (juce::ComboBox::arrowColourId, juce::Colour (0xff9fd8ff));

    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff15123a));
    setColour (juce::PopupMenu::textColourId, juce::Colour (0xffe8ecff));
    setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xff5b4fd0));
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);

    setColour (juce::TextButton::buttonColourId, juce::Colour (0xff6b3fd6));
    setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffd63fa0));
    setColour (juce::TextButton::textColourOffId, juce::Colours::white);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
}

void NebulaLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                          float pos, float startAngle, float endAngle, juce::Slider&)
{
    const auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (8.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f;
    const auto centre = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);

    // resplandor
    g.setColour (juce::Colour (0xff7b5cff).withAlpha (0.14f));
    g.fillEllipse (centre.x - radius - 5.0f, centre.y - radius - 5.0f, radius * 2.0f + 10.0f, radius * 2.0f + 10.0f);

    // cuerpo
    juce::ColourGradient body (juce::Colour (0xff2e2a6e), centre.x, centre.y - radius,
                               juce::Colour (0xff0c0a22), centre.x, centre.y + radius, false);
    g.setGradientFill (body);
    g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);

    // arco de fondo y arco de valor
    const float arcR = radius - 3.0f;
    juce::Path track;
    track.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff3a3680));
    g.strokePath (track, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val;
    val.addCentredArc (centre.x, centre.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (juce::Colour (0xff6ef0ff));
    g.strokePath (val, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // punto indicador
    const float pr = arcR - 9.0f;
    const float tx = centre.x + pr * std::sin (angle);
    const float ty = centre.y - pr * std::cos (angle);
    g.setColour (juce::Colours::white);
    g.fillEllipse (tx - 2.5f, ty - 2.5f, 5.0f, 5.0f);
}

// =====================================================================
//  CIELO / CONSTELACION
// =====================================================================
ConstellationView::ConstellationView (NebulaProcessor& p) : proc (p)
{
    for (int k = 0; k < 16; ++k)
    {
        const juce::String id = juce::String ("s") + juce::String (k + 1);
        starVals[(size_t) k] = proc.getAPVTS().getRawParameterValue (id);
        starParams[(size_t) k] = proc.getAPVTS().getParameter (id);
    }
}

float ConstellationView::getStar (int index) const
{
    return starVals[(size_t) index]->load();
}

float ConstellationView::xAt (float u) const
{
    return pad + u * ((float) getWidth() - 2.0f * pad);
}

float ConstellationView::yAt (float v) const
{
    return pad + (1.0f - v) * ((float) getHeight() - 2.0f * pad);
}

void ConstellationView::setStar (int index, float value)
{
    auto* prm = starParams[(size_t) index];
    if (prm == nullptr)
        return;

    if (! touched[(size_t) index])
    {
        prm->beginChangeGesture();
        touched[(size_t) index] = true;
    }
    prm->setValueNotifyingHost (juce::jlimit (0.0f, 1.0f, value));
}

void ConstellationView::setFromMouse (const juce::MouseEvent& e)
{
    const float usable = (float) getWidth() - 2.0f * pad;
    const float u = (e.position.x - pad) / usable;
    const int idx = juce::jlimit (0, 15, juce::roundToInt (u * 16.0f));
    const float v = juce::jlimit (0.0f, 1.0f, 1.0f - (e.position.y - pad) / ((float) getHeight() - 2.0f * pad));

    // si el mouse salta varias columnas, se rellenan las del medio
    if (lastIdx >= 0 && std::abs (idx - lastIdx) > 1)
    {
        const int step = idx > lastIdx ? 1 : -1;
        for (int k = lastIdx + step; k != idx; k += step)
        {
            const float t = (float) (k - lastIdx) / (float) (idx - lastIdx);
            setStar (k, lastVal + (v - lastVal) * t);
        }
    }
    setStar (idx, v);
    lastIdx = idx;
    lastVal = v;
}

void ConstellationView::mouseDown (const juce::MouseEvent& e)
{
    lastIdx = -1;
    setFromMouse (e);
}

void ConstellationView::mouseDrag (const juce::MouseEvent& e)
{
    setFromMouse (e);
}

void ConstellationView::mouseUp (const juce::MouseEvent&)
{
    for (size_t k = 0; k < 16; ++k)
        if (touched[k])
        {
            starParams[k]->endChangeGesture();
            touched[k] = false;
        }
    lastIdx = -1;
}

void ConstellationView::paint (juce::Graphics& g)
{
    const auto b = getLocalBounds().toFloat();
    const float w = (float) getWidth();
    const float h = (float) getHeight();

    // panel
    g.setColour (juce::Colour (0x66070520));
    g.fillRoundedRectangle (b, 14.0f);
    g.setColour (juce::Colour (0x667b5cff));
    g.drawRoundedRectangle (b.reduced (0.5f), 14.0f, 1.2f);

    // lineas guia
    for (int k = 1; k < 4; ++k)
    {
        g.setColour (juce::Colour (0x14ffffff));
        g.drawHorizontalLine ((int) yAt ((float) k * 0.25f), pad, w - pad);
    }
    for (int k = 0; k <= 16; ++k)
    {
        g.setColour (juce::Colour (k % 4 == 0 ? 0x22ffffff : 0x0cffffff));
        g.drawVerticalLine ((int) xAt ((float) k / 16.0f), pad * 0.5f, h - pad * 0.5f);
    }

    // posicion de las estrellas (la 17 es la primera repetida al final del ciclo)
    juce::Point<float> pts[17];
    float vals[17];
    for (int i = 0; i < 16; ++i)
    {
        vals[i] = getStar (i);
        pts[i] = { xAt ((float) i / 16.0f), yAt (vals[i]) };
    }
    vals[16] = vals[0];
    pts[16] = { xAt (1.0f), yAt (vals[0]) };

    // linea de la constelacion
    const bool smooth = proc.getAPVTS().getRawParameterValue ("smooth")->load() > 0.3f;
    juce::Path path;
    path.startNewSubPath (pts[0]);
    if (smooth)
    {
        for (int i = 0; i < 16; ++i)
        {
            const auto mid = (pts[i] + pts[i + 1]) * 0.5f;
            path.quadraticTo (pts[i], mid);
        }
        path.lineTo (pts[16]);
    }
    else
    {
        for (int i = 0; i < 16; ++i)
        {
            path.lineTo (pts[i + 1].x, pts[i].y);
            path.lineTo (pts[i + 1]);
        }
    }
    g.setColour (juce::Colour (0xff7b5cff).withAlpha (0.25f));
    g.strokePath (path, juce::PathStrokeType (7.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colour (0xffb8a8ff).withAlpha (0.85f));
    g.strokePath (path, juce::PathStrokeType (1.8f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // nivel de audio para que las estrellas "latan"
    levelSm += (proc.uiLevel.load() - levelSm) * 0.4f;

    // estrellas
    for (int i = 0; i < 17; ++i)
    {
        const bool ghost = (i == 16);
        const float v = vals[i];
        const auto col = juce::Colour (0xffb04dff).interpolatedWith (juce::Colour (0xff5ef2ff), v);
        const float coreR = 3.5f + 2.5f * v + 3.0f * levelSm;
        const float glowR = coreR * 4.2f;
        const float a = ghost ? 0.35f : 1.0f;
        const auto p = pts[i];

        juce::ColourGradient glow (col.withAlpha (0.6f * a), p.x, p.y, col.withAlpha (0.0f), p.x + glowR, p.y, true);
        g.setGradientFill (glow);
        g.fillEllipse (p.x - glowR, p.y - glowR, glowR * 2.0f, glowR * 2.0f);

        g.setColour (juce::Colours::white.withAlpha (0.45f * a));
        g.drawLine (p.x - glowR * 0.75f, p.y, p.x + glowR * 0.75f, p.y, 1.0f);
        g.drawLine (p.x, p.y - glowR * 0.75f, p.x, p.y + glowR * 0.75f, 1.0f);

        g.setColour (juce::Colours::white.withAlpha (a));
        g.fillEllipse (p.x - coreR, p.y - coreR, coreR * 2.0f, coreR * 2.0f);
    }

    // cometa: posicion actual dentro del ciclo
    const float px = xAt (proc.uiPhase.load());
    juce::ColourGradient trail (juce::Colour (0x00aef0ff), px - 70.0f, 0.0f, juce::Colour (0x30aef0ff), px, 0.0f, false);
    g.setGradientFill (trail);
    g.fillRect (px - 70.0f, 6.0f, 70.0f, h - 12.0f);
    g.setColour (juce::Colour (0xaaaef0ff));
    g.drawLine (px, 6.0f, px, h - 6.0f, 1.5f);

    g.setColour (juce::Colour (0x88a9b4e8));
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.drawText ("ARRASTRA PARA DIBUJAR TU CONSTELACION", getLocalBounds().reduced (16, 8), juce::Justification::bottomLeft);
}

// =====================================================================
//  EDITOR
// =====================================================================
NebulaEditor::NebulaEditor (NebulaProcessor& p)
    : juce::AudioProcessorEditor (&p), proc (p), sky (p)
{
    setLookAndFeel (&lnf);
    auto& apvts = proc.getAPVTS();

    addAndMakeVisible (sky);

    // --- menus desplegables ---
    auto setupCombo = [this, &apvts] (juce::ComboBox& box, const char* id)
    {
        if (auto* prm = dynamic_cast<juce::AudioParameterChoice*> (apvts.getParameter (id)))
            box.addItemList (prm->choices, 1);
        addAndMakeVisible (box);
    };
    setupCombo (presetBox, "preset");
    setupCombo (scaleBox, "scale");
    setupCombo (divBox, "sdiv");
    setupCombo (cycleBox, "cycle");

    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    presetAtt = std::make_unique<CA> (apvts, "preset", presetBox);
    scaleAtt  = std::make_unique<CA> (apvts, "scale", scaleBox);
    divAtt    = std::make_unique<CA> (apvts, "sdiv", divBox);
    cycleAtt  = std::make_unique<CA> (apvts, "cycle", cycleBox);

    // --- perillas ---
    static const char* ids[10]   = { "pdepth", "pcenter", "glide", "vol", "filt", "res", "smix", "sthresh", "smooth", "mix" };
    static const char* names[10] = { "PITCH", "CENTRO", "GLIDE", "VOLUMEN", "FILTRO", "RESON.", "STUTTER", "UMBRAL", "SUAVIDAD", "MEZCLA" };

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    for (size_t k = 0; k < 10; ++k)
    {
        auto& kn = knobs[k];
        kn.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        kn.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 16);
        addAndMakeVisible (kn.slider);

        kn.label.setText (names[k], juce::dontSendNotification);
        kn.label.setJustificationType (juce::Justification::centred);
        kn.label.setFont (juce::Font (juce::FontOptions (11.0f, juce::Font::bold)));
        kn.label.attachToComponent (&kn.slider, false);

        kn.att = std::make_unique<SA> (apvts, ids[k], kn.slider);
    }

    bigBangButton.onClick = [this] { bigBang(); };
    addAndMakeVisible (bigBangButton);

    // --- estrellas del fondo ---
    juce::Random rng (12345);
    for (auto& s : bgStars)
    {
        s.x = rng.nextFloat();
        s.y = rng.nextFloat();
        s.size = 0.8f + rng.nextFloat() * (rng.nextFloat() < 0.1f ? 2.6f : 1.2f);
        s.phase = rng.nextFloat() * 6.28f;
        s.speed = 0.5f + rng.nextFloat() * 2.5f;
    }

    setResizable (false, false);
    setSize (920, 540);
    startTimerHz (30);
}

NebulaEditor::~NebulaEditor()
{
    setLookAndFeel (nullptr);
}

void NebulaEditor::bigBang()
{
    auto& apvts = proc.getAPVTS();
    auto setP = [&apvts] (const juce::String& id, float plain)
    {
        if (auto* prm = apvts.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (prm->convertTo0to1 (plain));
            prm->endChangeGesture();
        }
    };

    juce::Random rng;
    float v = 0.2f + 0.8f * rng.nextFloat();
    for (int k = 0; k < 16; ++k)
    {
        if (rng.nextFloat() < 0.3f)
            v = rng.nextFloat();
        else
            v = juce::jlimit (0.0f, 1.0f, v + (rng.nextFloat() - 0.5f) * 0.6f);
        setP (juce::String ("s") + juce::String (k + 1), v);
    }

    setP ("preset", 0.0f);
    setP ("pdepth", 8.0f + 16.0f * rng.nextFloat());
    setP ("pcenter", 0.5f);
    setP ("scale", (float) (1 + rng.nextInt (5)));
    setP ("glide", rng.nextFloat() < 0.5f ? 0.0f : 40.0f * rng.nextFloat());
    setP ("smooth", rng.nextFloat() < 0.5f ? 0.0f : 1.0f);
}

void NebulaEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();
    const float h = (float) getHeight();

    if (bgImage.isValid())
        g.drawImageAt (bgImage, 0, 0);

    // estrellas titilando
    const double t = juce::Time::getMillisecondCounterHiRes() * 0.001;
    for (const auto& s : bgStars)
    {
        const float a = 0.25f + 0.75f * (0.5f + 0.5f * (float) std::sin (t * (double) s.speed + (double) s.phase));
        g.setColour (juce::Colours::white.withAlpha (a * 0.9f));
        g.fillEllipse (s.x * w, s.y * h, s.size, s.size);
    }

    // titulo
    g.setFont (juce::Font (juce::FontOptions (36.0f, juce::Font::bold)));
    g.setColour (juce::Colour (0xffe9e4ff));
    g.drawText ("N E B U L A", 30, 14, 320, 44, juce::Justification::centredLeft);
    g.setFont (juce::Font (juce::FontOptions (11.0f)));
    g.setColour (juce::Colour (0xff8f9be0));
    g.drawText ("CONSTELACION RITMICA", 32, 54, 300, 16, juce::Justification::centredLeft);

    // titulos de grupos
    g.setFont (juce::Font (juce::FontOptions (13.0f, juce::Font::bold)));
    g.setColour (juce::Colour (0xff9fd8ff));
    g.drawText ("TONO", 40, 336, 240, 18, juce::Justification::centredLeft);
    g.drawText ("VOLUMEN Y FILTRO", 292, 336, 240, 18, juce::Justification::centredLeft);
    g.drawText ("STUTTER", 544, 336, 160, 18, juce::Justification::centredLeft);
    g.drawText ("GLOBAL", 712, 336, 160, 18, juce::Justification::centredLeft);

    g.setFont (juce::Font (juce::FontOptions (10.0f)));
    g.setColour (juce::Colour (0xff7f8ac8));
    g.drawText ("ESCALA", 40, 356, 200, 14, juce::Justification::centredLeft);
    g.drawText ("DIVISION", 544, 356, 150, 14, juce::Justification::centredLeft);
    g.drawText ("CICLO", 712, 356, 150, 14, juce::Justification::centredLeft);
}

void NebulaEditor::resized()
{
    const int w = getWidth();

    sky.setBounds (30, 84, w - 60, 240);
    presetBox.setBounds (w - 30 - 260, 22, 260, 30);
    bigBangButton.setBounds (w - 30 - 260 - 130, 22, 120, 30);

    scaleBox.setBounds (40, 374, 200, 26);
    divBox.setBounds (544, 374, 150, 26);
    cycleBox.setBounds (712, 374, 150, 26);

    static const int kx[10] = { 40, 124, 208, 292, 376, 460, 544, 628, 712, 796 };
    for (size_t k = 0; k < 10; ++k)
        knobs[k].slider.setBounds (kx[k], 432, 84, 90);

    // fondo (nebulosa) dibujado una sola vez
    bgImage = juce::Image (juce::Image::ARGB, juce::jmax (1, w), juce::jmax (1, getHeight()), true);
    juce::Graphics gg (bgImage);
    const float fw = (float) w;
    const float fh = (float) getHeight();

    juce::ColourGradient sky0 (juce::Colour (0xff04031a), 0.0f, 0.0f, juce::Colour (0xff160a3a), 0.0f, fh, false);
    gg.setGradientFill (sky0);
    gg.fillAll();

    struct Blob { float cx, cy, r; juce::uint32 colour; float alpha; };
    const Blob blobs[] = {
        { 0.20f, 0.30f, 0.45f, 0xff6a2fd6, 0.35f },
        { 0.80f, 0.25f, 0.40f, 0xff1fa7d6, 0.22f },
        { 0.55f, 0.85f, 0.50f, 0xffd62fa0, 0.22f },
        { 0.10f, 0.90f, 0.35f, 0xff3047d6, 0.25f }
    };
    for (const auto& bl : blobs)
    {
        const float cx = bl.cx * fw, cy = bl.cy * fh, r = bl.r * fw;
        juce::ColourGradient gr (juce::Colour (bl.colour).withAlpha (bl.alpha), cx, cy,
                                 juce::Colour (bl.colour).withAlpha (0.0f), cx + r, cy, true);
        gg.setGradientFill (gr);
        gg.fillEllipse (cx - r, cy - r, r * 2.0f, r * 2.0f);
    }
}

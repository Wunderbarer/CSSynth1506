#include "PluginEditor.h"

using namespace cs;
using juce::Rectangle;

namespace
{
constexpr int kLayerX[2] = { 20, 670 };
constexpr int kLayerY = 92, kLayerW = 630, kLayerH = 410;
constexpr int kGlobalY = 512, kGlobalH = 138;

void drawWood (juce::Graphics& g, Rectangle<int> r)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff7a4626), (float) r.getX(), 0.f,
                                             juce::Colour (0xff3f2412), (float) r.getRight(), 0.f, false));
    g.fillRect (r);
    juce::Random rng (11);
    for (int i = 0; i < 26; ++i)
    {
        const float x = (float) r.getX() + rng.nextFloat() * (float) r.getWidth();
        g.setColour ((i % 3 == 0 ? juce::Colours::white : juce::Colours::black).withAlpha (0.05f + rng.nextFloat() * 0.08f));
        g.drawLine (x, (float) r.getY(), x + (rng.nextFloat() - 0.5f) * 3.f, (float) r.getBottom(), 1.f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRect (r, 1);
}

void drawPanel (juce::Graphics& g, Rectangle<int> r, const juce::String& title)
{
    const auto rf = r.toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff33302c), rf.getX(), rf.getY(),
                                             juce::Colour (0xff25221f), rf.getX(), rf.getBottom(), false));
    g.fillRoundedRectangle (rf, 6.f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (rf.reduced (0.5f), 6.f, 1.f);

    if (title.isNotEmpty())
    {
        const auto bar = Rectangle<float> (rf.getX() + 1.f, rf.getY() + 1.f, rf.getWidth() - 2.f, 24.f);
        g.setColour (juce::Colour (0xff171513));
        g.fillRoundedRectangle (bar, 5.f);
        g.setColour (col::orange);
        g.fillRect (bar.getX() + 10.f, bar.getBottom() - 3.f, 34.f, 3.f);
        g.setColour (col::amber);
        g.fillRect (bar.getX() + 46.f, bar.getBottom() - 3.f, 20.f, 3.f);
        g.setColour (col::rust);
        g.fillRect (bar.getX() + 68.f, bar.getBottom() - 3.f, 10.f, 3.f);
        g.setColour (col::cream);
        g.setFont (VintageLook::labelFont (13.f));
        g.drawText (title, (int) bar.getX() + 90, (int) bar.getY(), (int) bar.getWidth() - 100, 22, juce::Justification::centredLeft, false);
    }
}

void drawGroupLabel (juce::Graphics& g, Rectangle<int> r, const juce::String& text)
{
    g.setColour (col::cream);
    g.setFont (VintageLook::labelFont (10.f));
    g.drawFittedText (text, r.getX(), r.getY(), r.getWidth(), 14, juce::Justification::centredLeft, 1, 0.8f);
    g.setColour (col::orange);
    g.fillRect (r.getX(), r.getY() + 16, 22, 2);
    g.setColour (col::amber);
    g.fillRect (r.getX() + 24, r.getY() + 16, 12, 2);
    g.setColour (col::rust);
    g.fillRect (r.getX() + 38, r.getY() + 16, 6, 2);
}
} // namespace

//==============================================================================
MainPanel::MainPanel (CSSynthProcessor& p) : proc (p)
{
    setLookAndFeel (&look);
    setSize (W, H);

    buildLayer (0);
    buildLayer (1);
    buildGlobal();

    // Kopfzeile: Preset-Auswahl
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedItemIndex()); };
    addAndMakeVisible (presetBox);

    prevBtn.onClick = [this]
    {
        const int n = proc.getNumPrograms();
        proc.setCurrentProgram ((proc.getCurrentProgram() + n - 1) % n);
    };
    nextBtn.onClick = [this] { proc.setCurrentProgram ((proc.getCurrentProgram() + 1) % proc.getNumPrograms()); };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    resetMidiBtn.onClick = [this]
    {
        proc.cancelLearn();
        proc.resetMidiMapToDefaults();
    };
    addAndMakeVisible (resetMidiBtn);

    startTimerHz (20);
}

MainPanel::~MainPanel()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

KnobCell* MainPanel::knob (int idx, const juce::String& label, Rectangle<int> b, int ks, int lh)
{
    auto* k = add<KnobCell> (proc, idx, label, ks, lh);
    k->setBounds (b);
    learners.push_back (&k->getSlider());
    return k;
}

FaderCell* MainPanel::fader (int idx, const juce::String& label, Rectangle<int> b, bool accent)
{
    auto* f = add<FaderCell> (proc, idx, label, accent);
    f->setBounds (b);
    learners.push_back (&f->getSlider());
    return f;
}

void MainPanel::buildLayer (int L)
{
    const int x0 = kLayerX[L], y0 = kLayerY;
    const int o = L * LP_COUNT;
    const int cx = x0 + 92;   // erste Zelle
    const int cw = 88, ch = 84;
    const juce::String name = L == 0 ? "LAYER  I" : "LAYER  II";
    groups.push_back ({ name, { x0, y0, kLayerW, kLayerH } });

    // --- Oszillator ---
    const int oy = y0 + 34;
    groups.push_back ({ "OSCILLATOR", { x0 + 12, oy + 6, 76, 20 } });
    {
        auto* w = add<ChoiceButtons> (proc, o + LP_WAVE, juce::StringArray { "Saw", "Square", "Pulse" }, "WAVEFORM");
        w->setBounds (cx, oy, cw, 3 * ChoiceButtons::rowH + 18);
        learners.push_back (w);
    }
    knob (o + LP_OCT,    "OCTAVE", { cx + cw * 1, oy, cw, ch }, 62);
    knob (o + LP_DETUNE, "DETUNE", { cx + cw * 2, oy, cw, ch }, 62);
    knob (o + LP_PW,     "PULSE W.", { cx + cw * 3, oy, cw, ch }, 62);
    knob (o + LP_SUB,    "SUB OSC", { cx + cw * 4, oy, cw, ch }, 62);
    knob (o + LP_NOISE,  "NOISE", { cx + cw * 5, oy, cw, ch }, 62);

    // --- Filter ---
    const int fy = y0 + 126;
    groups.push_back ({ "FILTER", { x0 + 12, fy + 6, 76, 20 } });
    knob (o + LP_HPF,   "HP CUTOFF", { cx + cw * 0, fy, cw, ch }, 62);
    knob (o + LP_LPF,   "LP CUTOFF", { cx + cw * 1, fy, cw, ch }, 62);
    knob (o + LP_RES,   "RESONANCE", { cx + cw * 2, fy, cw, ch }, 62);
    knob (o + LP_BRILL, "BRILLIANCE", { cx + cw * 3, fy, cw, ch }, 62);
    knob (o + LP_KEY,   "KEY TRACK", { cx + cw * 4, fy, cw, ch }, 62);
    knob (o + LP_AMT,   "EG AMOUNT", { cx + cw * 5, fy, cw, ch }, 62);

    // --- Hüllkurven ---
    const int ey = y0 + 220;
    groups.push_back ({ "ENVELOPES", { x0 + 12, ey + 24, 76, 20 } });
    const int sy = ey + 22, sh = 166, fw = 40;
    const char* fl[6] = { "IL", "A", "AL", "D", "SL", "R" };
    const int fi[6] = { LP_FIL, LP_FA, LP_FAL, LP_FD, LP_FSL, LP_FR };
    for (int i = 0; i < 6; ++i)
        fader (o + fi[i], fl[i], { cx + i * fw, sy, fw, sh });

    const int ax = cx + 6 * fw + 18;
    const char* al[4] = { "A", "D", "S", "R" };
    const int ai[4] = { LP_AA, LP_AD, LP_AS, LP_AR };
    for (int i = 0; i < 4; ++i)
        fader (o + ai[i], al[i], { ax + i * fw, sy, fw, sh });

    const int lx = ax + 4 * fw + 18;
    fader (o + LP_LEVEL, "LEVEL", { lx, sy, fw + 6, sh }, true);

    groups.push_back ({ "FILTER EG", { cx, ey + 1, 6 * fw, 16 } });
    groups.push_back ({ "AMPLIFIER EG", { ax, ey + 1, 4 * fw, 16 } });
}

void MainPanel::buildGlobal()
{
    struct Cell { int idx; const char* label; bool toggle; };
    struct Section { const char* title; std::vector<Cell> cells; int cellW; int knobSize; };

    std::vector<Section> secs = {
        { "MODULATION", { { GP_LFO_RATE, "RATE", false }, { GP_LFO_SHAPE, "SHAPE", false }, { GP_LFO_PITCH, "PITCH", false },
                          { GP_LFO_CUT, "CUTOFF", false }, { GP_LFO_PWM, "PWM", false },
                          { GP_AT_FILTER, "AT>FLT", false }, { GP_AT_VIB, "AT>VIB", false } }, 50, 44 },
        { "PERFORMANCE", { { GP_GLIDE, "GLIDE", false }, { GP_BEND, "BEND", false },
                           { GP_VEL_AMP, "VEL>VOL", false }, { GP_VEL_FLT, "VEL>FLT", false } }, 52, 44 },
        { "CHORUS", { { GP_CH_RATE, "RATE", false }, { GP_CH_DEPTH, "DEPTH", false }, { GP_CH_MIX, "MIX", false } }, 50, 44 },
        { "DELAY", { { GP_DL_TIME, "TIME", false }, { GP_DL_FB, "FDBK", false }, { GP_DL_MIX, "MIX", false },
                     { GP_DL_PING, "PING-PONG", true } }, 54, 44 },
        { "REVERB", { { GP_RV_SIZE, "SIZE", false }, { GP_RV_DAMP, "DAMP", false }, { GP_RV_MIX, "MIX", false } }, 50, 44 },
        { "MASTER", { { GP_MASTER, "VOLUME", false } }, 84, 66 },
    };

    int total = 0;
    for (auto& s : secs) total += 16 + (int) s.cells.size() * s.cellW;
    const int margin = 8;
    const int avail = (W - 40) - 2 * margin;
    const float gap = (float) (avail - total) / (float) (secs.size() - 1);

    float x = (float) (20 + margin);
    const int y = kGlobalY;
    groups.push_back ({ "", { 20, kGlobalY, W - 40, kGlobalH } });

    for (auto& s : secs)
    {
        const int sw = 16 + (int) s.cells.size() * s.cellW;
        sections.push_back ({ (int) x, y + 8, sw, kGlobalH - 16 });
        groups.push_back ({ s.title, { (int) x + 8, y + 12, sw - 16, 14 } });

        int cx = (int) x + 8;
        for (auto& c : s.cells)
        {
            const Rectangle<int> b (cx, y + 34, s.cellW, kGlobalH - 42);
            if (c.toggle)
            {
                auto* t = add<ToggleLed> (proc, c.idx, c.label);
                t->setBounds (b.withHeight (62));
                learners.push_back (t);
            }
            else
            {
                const bool big = s.knobSize > 50;
                const int ks = s.knobSize;
                auto* k = knob (c.idx, c.label, b.withHeight (ks + 16 + (big ? 6 : 0)), ks, big ? 16 : 14);
                (void) k;
            }
            cx += s.cellW;
        }
        x += (float) sw + gap;
    }
}

//==============================================================================
void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff100f0d));

    // Holzwangen
    drawWood (g, { 0, 0, 16, H });
    drawWood (g, { W - 16, 0, 16, H });

    // Gehäuse
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2724), 0.f, 0.f, col::body, 0.f, (float) H, false));
    g.fillRect (16, 0, W - 32, H);

    // Kopfzeile
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff211f1c), 0.f, 0.f, juce::Colour (0xff131210), 0.f, 82.f, false));
    g.fillRect (16, 0, W - 32, 82);
    g.setColour (col::orange);
    g.fillRect (16, 82, W - 32, 2);
    g.setColour (col::amber);
    g.fillRect (16, 84, W - 32, 1);

    // Logo
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.setFont (juce::Font (juce::FontOptions (46.f, juce::Font::bold | juce::Font::italic)).withExtraKerningFactor (0.02f));
    g.drawText ("CSSynth", 50, 10, 240, 52, juce::Justification::centredLeft, false);
    g.setColour (col::cream);
    g.drawText ("CSSynth", 48, 8, 240, 52, juce::Justification::centredLeft, false);

    g.setColour (col::creamDim);
    g.setFont (VintageLook::labelFont (10.5f));
    g.drawText ("DUAL-LAYER POLYPHONIC SYNTHESIZER", 50, 58, 330, 16, juce::Justification::centredLeft, false);

    const Rectangle<float> badge (292.f, 18.f, 104.f, 44.f);
    g.setColour (juce::Colour (0xff0d0c0b));
    g.fillRoundedRectangle (badge, 6.f);
    g.setColour (col::orange);
    g.drawRoundedRectangle (badge.reduced (1.f), 6.f, 2.f);
    g.setFont (juce::Font (juce::FontOptions (32.f, juce::Font::bold)).withExtraKerningFactor (0.08f));
    g.drawText ("1506", badge.toNearestInt(), juce::Justification::centred, false);

    // Streifen
    const juce::Colour stripe[3] = { col::orange, col::amber, col::rust };
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (stripe[i]);
        g.fillRect (430, 26 + i * 10, 360, 6);
    }

    g.setColour (col::cream);
    g.setFont (VintageLook::labelFont (10.5f));
    g.drawText ("PRESET", 842, 14, 80, 14, juce::Justification::centredLeft, false);

    // Bereiche
    for (auto& grp : groups)
    {
        if (grp.r.getWidth() >= 300) drawPanel (g, grp.r, grp.title);
    }
    for (auto& s : sections)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (s.toFloat(), 4.f);
        g.setColour (col::edge.withAlpha (0.7f));
        g.drawRoundedRectangle (s.toFloat().reduced (0.5f), 4.f, 1.f);
    }
    for (auto& grp : groups)
        if (grp.r.getWidth() < 300 && grp.title.isNotEmpty()) drawGroupLabel (g, grp.r, grp.title);

    // Fußzeile
    g.setColour (col::creamDim);
    g.setFont (VintageLook::labelFont (10.5f, false));
    g.drawText ("RIGHT-CLICK ANY CONTROL: MIDI LEARN   |   DOUBLE-CLICK: DEFAULT VALUE   |   MOUSE WHEEL: FINE ADJUST",
                36, 662, 760, 26, juce::Justification::centredLeft, false);
    g.setColour (col::amber);
    g.setFont (VintageLook::labelFont (11.f));
    g.drawText (midiText, 800, 662, 300, 26, juce::Justification::centredRight, false);
}

void MainPanel::resized()
{
    presetBox.setBounds (842, 30, 250, 30);
    prevBtn.setBounds (1100, 30, 34, 30);
    nextBtn.setBounds (1140, 30, 34, 30);
    resetMidiBtn.setBounds (1150, 664, 140, 24);
}

void MainPanel::timerCallback()
{
    for (auto* l : learners) l->refreshLearn();

    if (presetBox.getSelectedItemIndex() != proc.getCurrentProgram())
        presetBox.setSelectedItemIndex (proc.getCurrentProgram(), juce::dontSendNotification);

    juce::String t;
    if (proc.getLearnIndex() >= 0)
        t = "MIDI LEARN: MOVE A CONTROLLER ...";
    else if (proc.getLastCc() >= 0)
        t = "LAST CC " + juce::String (proc.getLastCc()) + " = " + juce::String (proc.getLastCcValue());
    else
        t = "NO MIDI CC RECEIVED YET";
    if (t != midiText)
    {
        midiText = t;
        repaint (800, 662, 300, 26);
    }
}

//==============================================================================
CSSynthEditor::CSSynthEditor (CSSynthProcessor& p) : juce::AudioProcessorEditor (&p), panel (p)
{
    addAndMakeVisible (panel);
    setResizable (true, true);
    setResizeLimits (MainPanel::W / 2, MainPanel::H / 2, MainPanel::W * 2, MainPanel::H * 2);
    getConstrainer()->setFixedAspectRatio ((double) MainPanel::W / (double) MainPanel::H);
    setSize (MainPanel::W, MainPanel::H);
}

void CSSynthEditor::resized()
{
    panel.setBounds (0, 0, MainPanel::W, MainPanel::H);
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) MainPanel::W));
}

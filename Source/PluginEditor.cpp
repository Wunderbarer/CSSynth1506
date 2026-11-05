#include "PluginEditor.h"

using namespace cs;
using juce::Rectangle;

namespace
{
constexpr int kLayerX[2] = { 20, 670 };   // linke / rechte Spalte (Layer L liegt in Spalte L % 2)
constexpr int kLayerY = 92, kLayerW = 630, kLayerH = 410;
constexpr int kGlobalY = 512, kGlobalH = 138;

// Seitenwangen: dunkles Metall mit neonfarbener Kante
void drawSide (juce::Graphics& g, Rectangle<int> r, bool leftSide)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff12142b), (float) r.getX(), 0.f,
                                             juce::Colour (0xff05060d), (float) r.getRight(), 0.f, false));
    g.fillRect (r);
    const int ex = leftSide ? r.getRight() - 2 : r.getX();
    g.setColour (col::cyan.withAlpha (0.18f)); g.fillRect (ex - (leftSide ? 2 : -2), r.getY(), 4, r.getHeight());
    g.setColour (col::cyan.withAlpha (0.9f));  g.fillRect (ex, r.getY(), 2, r.getHeight());
    g.setColour (col::magenta.withAlpha (0.8f));
    for (int y = 40; y < r.getHeight(); y += 140) g.fillRect (r.getX() + 4, y, r.getWidth() - 8, 2);
}

void drawPanel (juce::Graphics& g, Rectangle<int> r, const juce::String& title)
{
    const auto rf = r.toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff10132a), rf.getX(), rf.getY(),
                                             juce::Colour (0xff080a17), rf.getX(), rf.getBottom(), false));
    g.fillRoundedRectangle (rf, 6.f);
    g.setColour (col::edge);
    g.drawRoundedRectangle (rf.reduced (0.5f), 6.f, 1.f);
    // abgeschnittene Neon-Ecke oben rechts
    g.setColour (col::cyan.withAlpha (0.75f));
    g.fillRect (rf.getRight() - 40.f, rf.getY() + 0.5f, 38.f, 1.5f);
    g.fillRect (rf.getRight() - 1.5f, rf.getY() + 2.f, 1.f, 14.f);

    if (title.isNotEmpty())
    {
        const auto bar = Rectangle<float> (rf.getX() + 1.f, rf.getY() + 1.f, rf.getWidth() - 2.f, 24.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b1f45), bar.getX(), 0.f, juce::Colour (0xff0a0b1a), bar.getRight(), 0.f, false));
        g.fillRoundedRectangle (bar, 5.f);
        g.setColour (col::cyan.withAlpha (0.25f)); g.fillRect (bar.getX() + 10.f, bar.getBottom() - 4.f, 34.f, 5.f);
        g.setColour (col::cyan);    g.fillRect (bar.getX() + 10.f, bar.getBottom() - 3.f, 34.f, 2.f);
        g.setColour (col::magenta); g.fillRect (bar.getX() + 46.f, bar.getBottom() - 3.f, 20.f, 2.f);
        g.setColour (col::violet);  g.fillRect (bar.getX() + 68.f, bar.getBottom() - 3.f, 10.f, 2.f);
        g.setColour (col::ice);
        g.setFont (CyberLook::labelFont (13.f));
        g.drawText (title, (int) bar.getX() + 90, (int) bar.getY(), (int) bar.getWidth() - 100, 22, juce::Justification::centredLeft, false);
    }
}

void drawGroupLabel (juce::Graphics& g, Rectangle<int> r, const juce::String& text)
{
    g.setColour (col::ice);
    g.setFont (CyberLook::labelFont (10.f));
    g.drawFittedText (text, r.getX(), r.getY(), r.getWidth(), 14, juce::Justification::centredLeft, 1, 0.8f);
    g.setColour (col::cyan);    g.fillRect (r.getX(), r.getY() + 16, 22, 2);
    g.setColour (col::magenta); g.fillRect (r.getX() + 24, r.getY() + 16, 12, 2);
    g.setColour (col::violet);  g.fillRect (r.getX() + 38, r.getY() + 16, 6, 2);
}
} // namespace

//==============================================================================
MainPanel::MainPanel (CSSynthProcessor& p) : proc (p)
{
    setLookAndFeel (&look);
    setSize (W, H);

    for (int L = 0; L < NUM_LAYERS; ++L)
        buildLayer (L);
    buildGlobal();

    for (int i = 0; i < 2; ++i)
    {
        pageBtn[i].setClickingTogglesState (false);
        pageBtn[i].onClick = [this, i] { setPage (i); };
        addAndMakeVisible (pageBtn[i]);
    }
    setPage (0);

    // Kopfzeile: Preset-Auswahl (Werks-Presets: Id 1..N, Anwender-Presets: Id 1000+)
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= 1000)
        {
            const size_t k = (size_t) (id - 1000);
            if (k < userFiles.size() && ! proc.loadPresetFile (userFiles[k]))
            {
                showNotice ("COULD NOT READ PRESET FILE");
                syncPresetBox();
            }
        }
        else if (id >= 1)
            proc.setCurrentProgram (id - 1);
    };
    addAndMakeVisible (presetBox);
    rebuildPresetBox();

    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    importBtn.onClick = [this] { startImport(); };
    saveBtn.onClick   = [this] { startSave(); };
    addAndMakeVisible (importBtn);
    addAndMakeVisible (saveBtn);

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
    const int x0 = kLayerX[L % 2], y0 = kLayerY;
    const int o = L * LP_COUNT;
    const bool formant = (L == FORMANT_LAYER);
    const int page = L / 2;
    const size_t ownedStart = (size_t) owned.size(), groupStart = groups.size();
    const int cx = x0 + 92;   // erste Zelle
    const int cw = 88, ch = 84;
    static const char* const names[4] = { "LAYER  I", "LAYER  II", "LAYER  III", "LAYER  IV  -  FORMANT" };
    const juce::String name = names[juce::jlimit (0, 3, L)];
    groups.push_back ({ name, { x0, y0, kLayerW, kLayerH } });

    // --- Oszillator ---
    const int oy = y0 + 34;
    groups.push_back ({ "OSCILLATOR", { x0 + 12, oy + 6, 76, 20 } });
    if (formant)
    {
        knob (o + LP_WAVE,   "VOWEL",   { cx + cw * 0, oy, cw, ch }, 62);
        knob (o + LP_OCT,    "OCTAVE",  { cx + cw * 1, oy, cw, ch }, 62);
        knob (o + LP_DETUNE, "DETUNE",  { cx + cw * 2, oy, cw, ch }, 62);
        knob (o + LP_PW,     "MORPH",   { cx + cw * 3, oy, cw, ch }, 62);
        knob (o + LP_SUB,    "F.SHIFT", { cx + cw * 4, oy, cw, ch }, 62);
        knob (o + LP_NOISE,  "BREATH",  { cx + cw * 5, oy, cw, ch }, 62);
    }
    else
    {
        auto* w = add<ChoiceButtons> (proc, o + LP_WAVE, juce::StringArray { "Saw", "Square", "Pulse" }, "WAVEFORM");
        w->setBounds (cx, oy, cw, 3 * ChoiceButtons::rowH + 18);
        learners.push_back (w);
        knob (o + LP_OCT,    "OCTAVE", { cx + cw * 1, oy, cw, ch }, 62);
        knob (o + LP_DETUNE, "DETUNE", { cx + cw * 2, oy, cw, ch }, 62);
        knob (o + LP_PW,     "PULSE W.", { cx + cw * 3, oy, cw, ch }, 62);
        knob (o + LP_SUB,    "SUB OSC", { cx + cw * 4, oy, cw, ch }, 62);
        knob (o + LP_NOISE,  "NOISE", { cx + cw * 5, oy, cw, ch }, 62);
    }

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

    for (size_t i = ownedStart; i < (size_t) owned.size(); ++i)
        pageComps[page].push_back (owned[(int) i]);
    for (size_t i = groupStart; i < groups.size(); ++i)
        groups[i].page = page;
}

void MainPanel::setPage (int page)
{
    curPage = page;
    for (int p = 0; p < 2; ++p)
        for (auto* c : pageComps[p])
            c->setVisible (p == page);
    for (int i = 0; i < 2; ++i)
        pageBtn[i].setToggleState (i == page, juce::dontSendNotification);
    repaint();
}

void MainPanel::buildGlobal()
{
    struct Cell { int idx; const char* label; bool toggle; };
    struct Section { const char* title; std::vector<Cell> cells; int cellW; int knobSize; };

    std::vector<Section> secs = {
        { "MODULATION", { { GP_LFO_RATE, "RATE", false }, { GP_LFO_SHAPE, "SHAPE", false }, { GP_LFO_PITCH, "PITCH", false },
                          { GP_LFO_CUT, "CUTOFF", false }, { GP_LFO_PWM, "PWM", false }, { GP_LFO_VOWEL, "VOWEL", false },
                          { GP_AT_FILTER, "AT>FLT", false }, { GP_AT_VIB, "AT>VIB", false } }, 46, 44 },
        { "PERFORMANCE", { { GP_GLIDE, "GLIDE", false }, { GP_BEND, "BEND", false },
                           { GP_VEL_AMP, "VEL>VOL", false }, { GP_VEL_FLT, "VEL>FLT", false } }, 48, 44 },
        { "CHORUS", { { GP_CH_RATE, "RATE", false }, { GP_CH_DEPTH, "DEPTH", false }, { GP_CH_MIX, "MIX", false } }, 50, 44 },
        { "DELAY", { { GP_DL_TIME, "TIME", false }, { GP_DL_FB, "FDBK", false }, { GP_DL_MIX, "MIX", false },
                     { GP_DL_PING, "PING-PONG", true } }, 50, 44 },
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
                if (c.idx == GP_MASTER) k->getSlider().getProperties().set ("accent", true);
            }
            cx += s.cellW;
        }
        x += (float) sw + gap;
    }
}

//==============================================================================
void MainPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff04050a));

    // Seitenwangen
    drawSide (g, { 0, 0, 16, H }, true);
    drawSide (g, { W - 16, 0, 16, H }, false);

    // Gehäuse mit feinem Raster
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b0d20), 0.f, 0.f, col::body, 0.f, (float) H, false));
    g.fillRect (16, 0, W - 32, H);
    g.setColour (col::cyan.withAlpha (0.035f));
    for (int gx = 16; gx < W - 16; gx += 32) g.fillRect (gx, 0, 1, H);
    for (int gy = 0; gy < H; gy += 32)       g.fillRect (16, gy, W - 32, 1);

    // Kopfzeile
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff15183a), 0.f, 0.f, juce::Colour (0xff070815), 0.f, 82.f, false));
    g.fillRect (16, 0, W - 32, 82);
    g.setColour (col::cyan.withAlpha (0.25f)); g.fillRect (16, 80, W - 32, 6);
    g.setColour (col::cyan);                  g.fillRect (16, 82, W - 32, 2);
    g.setColour (col::magenta);               g.fillRect (16, 84, W - 32, 1);

    // Name: DIPLOMAT (Orbitron) mit Leuchten und Farbverlauf
    {
        const auto f = fonts::title (38.f).withExtraKerningFactor (0.04f);
        juce::GlyphArrangement ga;
        ga.addLineOfText (f, "DIPLOMAT", 0.f, 0.f);
        juce::Path p;
        ga.createPath (p);
        const auto bb = p.getBounds();
        const auto place = juce::AffineTransform::translation (46.f - bb.getX(), 52.f);
        p.applyTransform (place);
        const auto pb = p.getBounds();
        for (int i = 4; i >= 1; --i)
        {
            g.setColour (col::cyan.withAlpha (0.05f * (float) (5 - i)));
            g.strokePath (p, juce::PathStrokeType ((float) i * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        }
        juce::ColourGradient grad (col::cyan.brighter (0.2f), pb.getX(), 0.f, col::magenta, pb.getRight(), 0.f, false);
        g.setGradientFill (grad);
        g.fillPath (p);
        titleRight = (int) pb.getRight();
    }
    g.setColour (col::iceDim);
    g.setFont (CyberLook::labelFont (10.5f));
    g.drawText ("FOUR-LAYER POLYPHONIC SYNTHESIZER", 48, 58, 330, 16, juce::Justification::centredLeft, false);

    // Modellnummer (Audiowide) im Neon-Rahmen
    const Rectangle<float> badge ((float) titleRight + 18.f, 16.f, 112.f, 46.f);
    glowRect (g, badge, col::magenta, 6.f, 0.9f);
    g.setColour (juce::Colour (0xff070815));
    g.fillRoundedRectangle (badge, 6.f);
    g.setColour (col::magenta);
    g.drawRoundedRectangle (badge.reduced (1.f), 6.f, 2.f);
    g.setColour (col::ice);
    g.setFont (fonts::model (30.f).withExtraKerningFactor (0.06f));
    g.drawText ("1506", badge.toNearestInt(), juce::Justification::centred, false);

    // Neon-Streifen
    const juce::Colour stripe[3] = { col::cyan, col::magenta, col::violet };
    const int sx = titleRight + 150;
    for (int i = 0; i < 3; ++i)
    {
        g.setColour (stripe[i].withAlpha (0.25f));
        g.fillRect (sx, 25 + i * 10, 790 - sx, 8);
        g.setColour (stripe[i]);
        g.fillRect (sx, 26 + i * 10, 790 - sx, 4);
    }

    g.setColour (col::ice);
    g.setFont (CyberLook::labelFont (10.5f));
    g.drawText ("PRESET", 842, 14, 80, 14, juce::Justification::centredLeft, false);

    // Bereiche
    for (auto& grp : groups)
    {
        if (grp.page >= 0 && grp.page != curPage) continue;
        if (grp.r.getWidth() >= 500) drawPanel (g, grp.r, grp.title);
    }
    for (auto& s : sections)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (s.toFloat(), 4.f);
        g.setColour (col::edge.withAlpha (0.7f));
        g.drawRoundedRectangle (s.toFloat().reduced (0.5f), 4.f, 1.f);
    }
    for (auto& grp : groups)
        if ((grp.page < 0 || grp.page == curPage) && grp.r.getWidth() < 500 && grp.title.isNotEmpty()) drawGroupLabel (g, grp.r, grp.title);

    // Fußzeile
    g.setColour (col::iceDim);
    g.setFont (CyberLook::labelFont (10.5f, false));
    g.drawText ("RIGHT-CLICK ANY CONTROL: MIDI LEARN   |   DOUBLE-CLICK: DEFAULT VALUE   |   MOUSE WHEEL: FINE ADJUST",
                36, 662, 760, 26, juce::Justification::centredLeft, false);
    g.setColour (col::magenta);
    g.setFont (CyberLook::labelFont (11.f));
    g.drawText (midiText, 800, 662, 300, 26, juce::Justification::centredRight, false);
}

void MainPanel::resized()
{
    presetBox.setBounds (842, 30, 220, 30);
    prevBtn.setBounds (1068, 30, 34, 30);
    nextBtn.setBounds (1106, 30, 34, 30);
    pageBtn[0].setBounds (430, 58, 178, 20);
    pageBtn[1].setBounds (612, 58, 178, 20);
    importBtn.setBounds (1150, 30, 70, 30);
    saveBtn.setBounds (1224, 30, 70, 30);
    resetMidiBtn.setBounds (1150, 664, 140, 24);
}

//==============================================================================
// Preset-Auswahl, Import, Export
void MainPanel::rebuildPresetBox()
{
    userFiles = CSSynthProcessor::scanUserPresets();

    presetBox.clear (juce::dontSendNotification);
    for (int i = 0; i < proc.getNumPrograms(); ++i)
        presetBox.addItem (proc.getProgramName (i), i + 1);

    if (! userFiles.empty())
    {
        presetBox.addSeparator();
        presetBox.addSectionHeading ("USER PRESETS");
        for (size_t k = 0; k < userFiles.size(); ++k)
            presetBox.addItem (userFiles[k].getFileNameWithoutExtension(), 1000 + (int) k);
    }
    syncPresetBox();
}

void MainPanel::syncPresetBox()
{
    int want = 0;
    const auto user = proc.getCurrentUserPreset();
    if (user.isNotEmpty())
        for (size_t k = 0; k < userFiles.size(); ++k)
            if (userFiles[k].getFileNameWithoutExtension() == user) { want = 1000 + (int) k; break; }

    if (want == 0) want = proc.getCurrentProgram() + 1;
    if (presetBox.getSelectedId() != want)
        presetBox.setSelectedId (want, juce::dontSendNotification);
}

void MainPanel::stepPreset (int direction)
{
    std::vector<int> ids;
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemId (i) > 0) ids.push_back (presetBox.getItemId (i));
    if (ids.empty()) return;

    const auto it = std::find (ids.begin(), ids.end(), presetBox.getSelectedId());
    const int cur = it == ids.end() ? 0 : (int) (it - ids.begin());
    const int n = (int) ids.size();
    presetBox.setSelectedId (ids[(size_t) ((cur + direction + n) % n)], juce::sendNotificationSync);
}

void MainPanel::showNotice (const juce::String& text)
{
    notice = text;
    noticeTicks = 120;   // ca. 6 Sekunden bei 20 Hz
}

void MainPanel::startImport()
{
    chooser = std::make_unique<juce::FileChooser> ("Import CSSynth 1506 presets",
                                                   juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                   "*.cs1506;*.xml");
    juce::Component::SafePointer<MainPanel> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::canSelectMultipleItems,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe != nullptr) safe->importFiles (fc.getResults());
                          });
}

void MainPanel::startSave()
{
    const auto dir = CSSynthProcessor::getUserPresetDir();
    dir.createDirectory();

    chooser = std::make_unique<juce::FileChooser> ("Save CSSynth 1506 preset",
                                                   dir.getChildFile ("My Preset" + juce::String (cs::presetfile::extension())),
                                                   "*.cs1506");
    juce::Component::SafePointer<MainPanel> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr) return;
                              auto f = fc.getResult();
                              if (f == juce::File()) return;           // abgebrochen

                              f = f.withFileExtension (cs::presetfile::extension());
                              if (safe->proc.exportPresetToFile (f, f.getFileNameWithoutExtension()))
                              {
                                  safe->rebuildPresetBox();
                                  safe->showNotice ("SAVED: " + f.getFileName().toUpperCase());
                              }
                              else
                                  safe->showNotice ("COULD NOT SAVE PRESET");
                          });
}

void MainPanel::importFiles (const juce::Array<juce::File>& files)
{
    int good = 0, bad = 0;
    juce::File first;

    for (const auto& f : files)
    {
        const auto dest = CSSynthProcessor::importPresetFile (f);
        if (dest == juce::File()) { ++bad; continue; }
        ++good;
        if (first == juce::File()) first = dest;
    }

    if (first != juce::File())
        proc.loadPresetFile (first);      // das erste importierte Preset sofort laden
    rebuildPresetBox();

    if (good == 0 && bad == 0) return;    // Dialog ohne Auswahl geschlossen
    juce::String msg = "IMPORTED " + juce::String (good) + (good == 1 ? " PRESET" : " PRESETS");
    if (bad > 0) msg += " | " + juce::String (bad) + " SKIPPED";
    showNotice (msg);
}

bool MainPanel::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
    {
        const auto ext = juce::File (f).getFileExtension().toLowerCase();
        if (ext == cs::presetfile::extension() || ext == ".xml") return true;
    }
    return false;
}

void MainPanel::filesDropped (const juce::StringArray& files, int, int)
{
    juce::Array<juce::File> list;
    for (const auto& f : files) list.add (juce::File (f));
    importFiles (list);
}

void MainPanel::timerCallback()
{
    for (auto* l : learners) l->refreshLearn();

    syncPresetBox();

    juce::String t;
    if (noticeTicks > 0)
    {
        --noticeTicks;
        t = notice;
    }
    else if (proc.getLearnIndex() >= 0)
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

#include "PluginEditor.h"

using namespace cs;
using juce::Rectangle;

namespace
{
constexpr int kLayerX[2] = { 20, 670 };   // linke / rechte Spalte (Layer L liegt in Spalte L % 2)
constexpr int kLayerY = 92, kLayerW = 630, kLayerH = 410;
constexpr int kGlobalY = 512, kGlobalH = 112;
constexpr int kFxY = kGlobalY + kGlobalH + 10;   // Effekt-Zeile

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

    buildSamplePanel();

    for (int i = 0; i < kPages; ++i)
    {
        pageBtn[i].setClickingTogglesState (false);
        pageBtn[i].onClick = [this, i] { setPage (i); };
        addAndMakeVisible (pageBtn[i]);
    }
    setPage (0);

    // Kopfzeile: zwei Bänke. SOUNDBANKS = Werks-Bank + geladene Bank-Dateien (Id 1..N je Bank),
    // USER = eigene Sounds (Id 1000+).
    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (proc.getBankMode() == CSSynthProcessor::UserMode || shownMode == CSSynthProcessor::UserMode)
        {
            if (id >= 1000)
            {
                const size_t k = (size_t) (id - 1000);
                juce::String info;
                if (k < userFiles.size() && ! proc.loadPresetFile (userFiles[k], &info))
                {
                    showNotice ("COULD NOT READ SOUND FILE");
                    syncPresetBox();
                }
                else if (info.isNotEmpty())
                    showNotice ("LOADED " + userFiles[k].getFileNameWithoutExtension().toUpperCase() + "  " + info);
                updateBankControls();
            }
            return;
        }
        if (id < 1) return;
        if (viewBank == nullptr)
            proc.setCurrentProgram (id - 1);
        else if ((size_t) (id - 1) < viewBank->presets.size())
        {
            juce::String info;
            proc.applyBankPreset (viewBankId, viewBank->presets[(size_t) (id - 1)], &info);
            if (info.isNotEmpty()) showNotice ("LOADED " + viewBank->presets[(size_t) (id - 1)].name.toUpperCase() + "  " + info);
        }
        updateBankControls();
    };
    addAndMakeVisible (presetBox);

    bankBox.onChange = [this]
    {
        const int id = bankBox.getSelectedId();
        if (id < 1) return;
        viewBankId = id == 1 ? juce::String() : (size_t) (id - 2) < bankFiles.size() ? bankFiles[(size_t) (id - 2)].getFileName() : juce::String();
        rebuildPresetBox();
    };
    addAndMakeVisible (bankBox);

    for (int i = 0; i < 2; ++i)
    {
        tabBtn[i].setClickingTogglesState (false);
        tabBtn[i].onClick = [this, i] { applyMode (i); };
        addAndMakeVisible (tabBtn[i]);
    }
    prevBtn.onClick = [this] { stepPreset (-1); };
    nextBtn.onClick = [this] { stepPreset (+1); };
    addAndMakeVisible (prevBtn);
    addAndMakeVisible (nextBtn);

    btnA.onClick = [this] { if (shownMode == CSSynthProcessor::UserMode) startImport(); else startLoadBank(); };
    btnB.onClick = [this] { if (shownMode == CSSynthProcessor::UserMode) startSave();   else startRemoveBank(); };
    deleteBtn.onClick = [this] { startDeleteUserSound(); };
    exportBtn.onClick = [this] { startExportBank(); };
    addAndMakeVisible (btnA);
    addAndMakeVisible (btnB);
    addAndMakeVisible (deleteBtn);
    addAndMakeVisible (exportBtn);

    viewBankId = proc.getSoundbankId();
    applyMode (proc.getBankMode());

    resetMidiBtn.onClick = [this]
    {
        proc.cancelLearn();
        proc.resetMidiMapToDefaults();
        notice = "MIDI MAP RESET TO DEFAULTS"; noticeTicks = 60;
    };
    addAndMakeVisible (resetMidiBtn);

    // Learn-Modus: bei "ON" genügt ein Linksklick auf ein Bedienelement, danach den Controller am Keyboard bewegen
    learnBtn.setClickingTogglesState (true);
    learnBtn.onClick = [this]
    {
        proc.setLearnArmed (learnBtn.getToggleState());
        learnBtn.setButtonText (learnBtn.getToggleState() ? "MIDI LEARN: ON" : "MIDI LEARN: OFF");
        repaint();
    };
    addAndMakeVisible (learnBtn);

    seenLearnEvent = proc.getLearnEvent();
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
    const bool sampler = (L == SAMPLE_LAYER);
    const int page = L / 2;
    const size_t ownedStart = (size_t) owned.size(), groupStart = groups.size();
    const int cx = x0 + 92;   // erste Zelle
    const int cw = 88, ch = 84;
    static const char* const names[5] = { "LAYER  I", "LAYER  II", "LAYER  III", "LAYER  IV  -  FORMANT", "LAYER  V  -  SAMPLE" };
    const juce::String name = names[juce::jlimit (0, 4, L)];
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
        auto* w = add<ChoiceButtons> (proc, o + LP_WAVE,
                                      sampler ? juce::StringArray { "Loop", "One-Shot", "Ping-Pong" } : juce::StringArray { "Saw", "Square", "Pulse", "Sine", "Triangle" },
                                      sampler ? "PLAYBACK" : "WAVEFORM");
        w->setBounds (cx, oy, cw, w->preferredHeight());
        learners.push_back (w);
        knob (o + LP_OCT,    "OCTAVE", { cx + cw * 1, oy, cw, ch }, 62);
        knob (o + LP_DETUNE, "DETUNE", { cx + cw * 2, oy, cw, ch }, 62);
        knob (o + LP_PW,     sampler ? "START" : "PULSE W.", { cx + cw * 3, oy, cw, ch }, 62);
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

void MainPanel::buildSamplePanel()
{
    const int x0 = kLayerX[1], y0 = kLayerY;
    const size_t ownedStart = (size_t) owned.size(), groupStart = groups.size();
    groups.push_back ({ "SAMPLE  -  AUDIO FILE OSCILLATOR", { x0, y0, kLayerW, kLayerH } });

    sampleView = add<SampleView> (proc, [this] { startLoadSample(); });
    sampleView->setBounds (x0 + 16, y0 + 36, kLayerW - 32, 196);

    auto* load = add<juce::TextButton> ("LOAD FILE ...");
    load->setBounds (x0 + 16, y0 + 242, 128, 28);
    load->onClick = [this] { startLoadSample(); };
    auto* clear = add<juce::TextButton> ("CLEAR");
    clear->setBounds (x0 + 150, y0 + 242, 80, 28);
    clear->onClick = [this]
    {
        proc.clearSample();
        showNotice ("SAMPLE REMOVED");
    };
    auto* info = add<TextBlock> ("DROP A WAV, AIFF, FLAC, OGG OR MP3 FILE ANYWHERE IN THIS WINDOW. THE FILE IS STORED IN THE PROJECT AND PLAYS LIKE A FIFTH OSCILLATOR (OCTAVE, DETUNE, FILTER, ENVELOPES, LEVEL ON THE LEFT).", 11.f);
    info->setBounds (x0 + 246, y0 + 238, kLayerW - 262, 40);

    const int ky = y0 + 296, kw = 100;
    groups.push_back ({ "SAMPLE PITCH & LOOP", { x0 + 16, ky - 4, 400, 20 } });
    knob (GP_SMP_ROOT,   "ROOT NOTE",  { x0 + 16 + kw * 0, ky + 20, kw, 84 }, 62);
    knob (GP_SMP_LSTART, "LOOP START", { x0 + 16 + kw * 1, ky + 20, kw, 84 }, 62);
    knob (GP_SMP_LEND,   "LOOP END",   { x0 + 16 + kw * 2, ky + 20, kw, 84 }, 62);

    {
        auto* t = add<ToggleLed> (proc, GP_SMP_SYNC, "HOST SYNC");
        t->setBounds (x0 + 16 + kw * 3, ky + 20, kw, 62);
        learners.push_back (t);
    }
    knob (GP_SMP_BEATS, "LOOP LENGTH", { x0 + 16 + kw * 4, ky + 20, kw, 84 }, 62);

    auto* hint = add<TextBlock> ("HOST SYNC: the loop lasts the chosen number of beats at the DAW tempo. Root note = in sync, other keys transpose.", 11.f);
    hint->setBounds (x0 + 16 + kw * 5 + 10, ky + 22, kLayerW - 16 - kw * 5 - 26, 80);

    for (size_t i = ownedStart; i < (size_t) owned.size(); ++i)
        pageComps[2].push_back (owned[(int) i]);
    for (size_t i = groupStart; i < groups.size(); ++i)
        groups[i].page = 2;
}

void MainPanel::startLoadSample()
{
    chooser = std::make_unique<juce::FileChooser> ("Load audio file into layer V",
                                                   juce::File::getSpecialLocation (juce::File::userMusicDirectory),
                                                   proc.getSampleWildcard());
    juce::Component::SafePointer<MainPanel> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr) return;
                              const auto f = fc.getResult();
                              if (f != juce::File()) safe->loadSampleFromFile (f);
                          });
}

void MainPanel::loadSampleFromFile (const juce::File& f)
{
    juce::String msg;
    if (proc.loadSampleFile (f, msg))
    {
        // Damit man das Sample sofort hört: Layer V einschalten, falls sein Pegel noch auf 0 steht
        if (auto* lvl = proc.getParam (SAMPLE_LAYER * LP_COUNT + LP_LEVEL))
            if (lvl->getValue() < 0.001f)
                lvl->setValueNotifyingHost (lvl->convertTo0to1 (0.8f));
        setPage (2);
    }
    showNotice (msg);
    if (sampleView != nullptr) sampleView->refresh();
}

bool MainPanel::anyAudioFile (const juce::StringArray& files) const
{
    for (const auto& f : files)
        if (proc.isSampleFile (juce::File (f))) return true;
    return false;
}

void MainPanel::setPage (int page)
{
    curPage = page;
    for (int p = 0; p < kPages; ++p)
        for (auto* c : pageComps[p])
            c->setVisible (p == page);
    for (int i = 0; i < kPages; ++i)
        pageBtn[i].setToggleState (i == page, juce::dontSendNotification);
    repaint();
}

void MainPanel::buildGlobal()
{
    struct Cell { int idx; const char* label; bool toggle; };
    struct Section { const char* title; std::vector<Cell> cells; int cellW; int knobSize; };

    // Zeile 1: Modulation, Spielhilfen, Master | Zeile 2: Effekte (Distortion, Phaser, Flanger, Chorus, Delay, Reverb)
    const std::vector<Section> row1 = {
        { "MODULATION", { { GP_LFO_RATE, "RATE", false }, { GP_LFO_SHAPE, "SHAPE", false }, { GP_LFO_PITCH, "PITCH", false },
                          { GP_LFO_CUT, "CUTOFF", false }, { GP_LFO_PWM, "PWM", false }, { GP_LFO_VOWEL, "VOWEL", false },
                          { GP_AT_FILTER, "AT>FLT", false }, { GP_AT_VIB, "AT>VIB", false } }, 80, 46 },
        { "PERFORMANCE", { { GP_GLIDE, "GLIDE", false }, { GP_BEND, "BEND", false },
                           { GP_VEL_AMP, "VEL>VOL", false }, { GP_VEL_FLT, "VEL>FLT", false } }, 80, 46 },
        { "MASTER", { { GP_MASTER, "VOLUME", false } }, 110, 56 },
    };
    const std::vector<Section> row2 = {
        { "DISTORTION", { { GP_DS_TYPE, "TYPE", false }, { GP_DS_DRIVE, "DRIVE", false }, { GP_DS_TONE, "TONE", false },
                          { GP_DS_MIX, "MIX", false } }, 48, 44 },
        { "PHASER", { { GP_PH_RATE, "RATE", false }, { GP_PH_DEPTH, "DEPTH", false }, { GP_PH_FB, "FDBK", false },
                      { GP_PH_MIX, "MIX", false } }, 48, 44 },
        { "FLANGER", { { GP_FL_RATE, "RATE", false }, { GP_FL_DEPTH, "DEPTH", false }, { GP_FL_FB, "FDBK", false },
                       { GP_FL_MIX, "MIX", false } }, 48, 44 },
        { "CHORUS", { { GP_CH_RATE, "RATE", false }, { GP_CH_DEPTH, "DEPTH", false }, { GP_CH_MIX, "MIX", false } }, 48, 44 },
        { "DELAY", { { GP_DL_SYNC, "HOST SYNC", true }, { GP_DL_TIME, "TIME", false }, { GP_DL_DIV, "NOTE", false },
                     { GP_DL_FB, "FDBK", false }, { GP_DL_MIX, "MIX", false }, { GP_DL_PING, "PING-PONG", true } }, 50, 44 },
        { "REVERB", { { GP_RV_SIZE, "SIZE", false }, { GP_RV_DAMP, "DAMP", false }, { GP_RV_MIX, "MIX", false } }, 48, 44 },
    };

    auto buildRow = [&] (const std::vector<Section>& secs, int y)
    {
        int total = 0;
        for (auto& s : secs) total += 16 + (int) s.cells.size() * s.cellW;
        const int margin = 8;
        const int avail = (W - 40) - 2 * margin;
        const float gap = secs.size() > 1 ? (float) (avail - total) / (float) (secs.size() - 1) : 0.f;

        float x = (float) (20 + margin);
        groups.push_back ({ "", { 20, y, W - 40, kGlobalH } });

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
    };

    buildRow (row1, kGlobalY);
    buildRow (row2, kFxY);
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
    g.drawText ("FIVE-LAYER POLYPHONIC SYNTHESIZER", 48, 55, 330, 14, juce::Justification::centredLeft, false);
    g.setColour (col::iceDim.withAlpha (0.8f));
    g.setFont (CyberLook::labelFont (8.f));
    g.drawText (juce::String (juce::CharPointer_UTF8 ("\xc2\xa9")) + " by Christian Saak", 48, 67, 330, 11, juce::Justification::centredLeft, false);

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
    g.drawText (bankCaption, 842, 65, 452, 14, juce::Justification::centredLeft, false);

    // Bereiche
    for (auto& grp : groups)
    {
        if (grp.page >= 0 && grp.page != curPage) continue;
        if (grp.r.getHeight() > 30) drawPanel (g, grp.r, grp.title);
    }
    for (auto& s : sections)
    {
        g.setColour (juce::Colours::black.withAlpha (0.25f));
        g.fillRoundedRectangle (s.toFloat(), 4.f);
        g.setColour (col::edge.withAlpha (0.7f));
        g.drawRoundedRectangle (s.toFloat().reduced (0.5f), 4.f, 1.f);
    }
    for (auto& grp : groups)
        if ((grp.page < 0 || grp.page == curPage) && grp.r.getHeight() <= 30 && grp.title.isNotEmpty()) drawGroupLabel (g, grp.r, grp.title);

    // Fußzeile
    g.setColour (col::iceDim);
    g.setFont (CyberLook::labelFont (10.5f, false));
    g.drawText ("RIGHT-CLICK: MIDI LEARN   |   DOUBLE-CLICK: DEFAULT   |   WHEEL: FINE",
                36, 756, 560, 26, juce::Justification::centredLeft, false);
    g.setColour (col::magenta);
    g.setFont (CyberLook::labelFont (11.f));
    g.drawText (midiText, 600, 756, 390, 26, juce::Justification::centredRight, false);
}

void MainPanel::resized()
{
    tabBtn[0].setBounds (842, 8, 108, 20);
    tabBtn[1].setBounds (954, 8, 108, 20);
    bankBox.setBounds (1068, 8, 226, 20);
    deleteBtn.setBounds (1068, 8, 110, 20);
    exportBtn.setBounds (1182, 8, 112, 20);
    presetBox.setBounds (842, 32, 220, 30);
    prevBtn.setBounds (1068, 32, 34, 30);
    nextBtn.setBounds (1106, 32, 34, 30);
    pageBtn[0].setBounds (430, 58, 106, 20);
    pageBtn[1].setBounds (540, 58, 112, 20);
    pageBtn[2].setBounds (656, 58, 134, 20);
    btnA.setBounds (1146, 32, 78, 30);
    btnB.setBounds (1228, 32, 66, 30);
    learnBtn.setBounds (1000, 758, 140, 24);
    resetMidiBtn.setBounds (1150, 758, 140, 24);
}

//==============================================================================
// Preset-Auswahl, Import, Export
void MainPanel::applyMode (int mode)
{
    mode = mode == CSSynthProcessor::UserMode ? CSSynthProcessor::UserMode : CSSynthProcessor::SoundbankMode;
    proc.setBankMode (mode);
    shownMode = mode;
    for (int i = 0; i < 2; ++i) tabBtn[i].setToggleState (i == mode, juce::dontSendNotification);

    const bool user = mode == CSSynthProcessor::UserMode;
    bankBox.setVisible (! user);
    deleteBtn.setVisible (user);
    exportBtn.setVisible (user);

    if (! user) rebuildBankBox();
    rebuildPresetBox();
}

void MainPanel::rebuildBankBox()
{
    bankFiles = CSSynthProcessor::scanBankFiles();
    bankBox.clear (juce::dontSendNotification);
    bankBox.addItem ("FACTORY  (" + juce::String (proc.getNumPrograms()) + " SOUNDS)", 1);
    for (size_t k = 0; k < bankFiles.size(); ++k)
        bankBox.addItem (bankFiles[k].getFileNameWithoutExtension(), 2 + (int) k);

    int want = 1;
    if (viewBankId.isNotEmpty())
    {
        bool found = false;
        for (size_t k = 0; k < bankFiles.size(); ++k)
            if (bankFiles[k].getFileName() == viewBankId) { want = 2 + (int) k; found = true; break; }
        if (! found) viewBankId = {};          // Datei existiert nicht mehr: Werks-Bank zeigen
    }
    bankBox.setSelectedId (want, juce::dontSendNotification);
}

void MainPanel::rebuildPresetBox()
{
    presetBox.clear (juce::dontSendNotification);

    if (shownMode == CSSynthProcessor::UserMode)
    {
        userFiles = CSSynthProcessor::scanUserPresets();
        presetBox.setTextWhenNothingSelected (userFiles.empty() ? "NO USER SOUNDS YET - SAVE ONE" : "SELECT A USER SOUND");
        for (size_t k = 0; k < userFiles.size(); ++k)
            presetBox.addItem (userFiles[k].getFileNameWithoutExtension(), 1000 + (int) k);
    }
    else
    {
        viewBank.reset();
        if (viewBankId.isNotEmpty())
        {
            auto b = std::make_unique<cs::presetfile::Bank>();
            if (CSSynthProcessor::readBank (CSSynthProcessor::getBankDir().getChildFile (viewBankId), *b))
                viewBank = std::move (b);
            else
            {
                showNotice ("COULD NOT READ SOUNDBANK - SHOWING FACTORY");
                viewBankId = {};
                bankBox.setSelectedId (1, juce::dontSendNotification);
            }
        }
        presetBox.setTextWhenNothingSelected ("SELECT A SOUND");
        if (viewBank == nullptr)
            for (int i = 0; i < proc.getNumPrograms(); ++i)
                presetBox.addItem (proc.getProgramName (i), i + 1);
        else
            for (size_t i = 0; i < viewBank->presets.size(); ++i)
                presetBox.addItem (viewBank->presets[i].name, (int) i + 1);
    }
    syncPresetBox();
    updateBankControls();
}

void MainPanel::syncPresetBox()
{
    if (shownMode != proc.getBankMode())       // z. B. Programmwechsel durch die DAW oder Zustand geladen
    {
        viewBankId = proc.getSoundbankId();
        applyMode (proc.getBankMode());
        return;
    }

    int want = 0;
    if (shownMode == CSSynthProcessor::UserMode)
    {
        const auto user = proc.getCurrentUserPreset();
        if (user.isNotEmpty())
            for (size_t k = 0; k < userFiles.size(); ++k)
                if (userFiles[k].getFileNameWithoutExtension() == user) { want = 1000 + (int) k; break; }
    }
    else if (viewBankId == proc.getSoundbankId() && proc.getCurrentUserPreset().isEmpty())   // geladener Sound stammt aus dieser Soundbank
    {
        if (viewBank == nullptr)
            want = proc.getCurrentProgram() + 1;
        else
        {
            const auto name = proc.getSoundbankPreset();
            for (size_t i = 0; i < viewBank->presets.size(); ++i)
                if (viewBank->presets[i].name == name) { want = (int) i + 1; break; }
        }
    }
    if (presetBox.getSelectedId() != want)
    {
        presetBox.setSelectedId (want, juce::dontSendNotification);
        updateBankControls();
    }
}

void MainPanel::updateBankControls()
{
    const bool user = shownMode == CSSynthProcessor::UserMode;
    btnA.setButtonText (user ? "IMPORT" : "LOAD BANK");
    btnB.setButtonText (user ? "SAVE" : "REMOVE");
    btnB.setEnabled (user || viewBankId.isNotEmpty());       // die Werks-Bank lässt sich nicht entfernen
    deleteBtn.setEnabled (user && proc.getCurrentUserPreset().isNotEmpty() && presetBox.getSelectedId() >= 1000);
    exportBtn.setEnabled (user && ! userFiles.empty());

    juce::String c;
    if (user)
        c = "USER BANK  -  " + juce::String ((int) userFiles.size()) + (userFiles.size() == 1 ? " SOUND" : " SOUNDS")
            + "  -  YOUR OWN SOUNDS (SAVE)";
    else if (viewBank == nullptr)
        c = "SOUNDBANK: FACTORY  -  " + juce::String (proc.getNumPrograms()) + " SOUNDS";
    else
        c = "SOUNDBANK: " + viewBank->name.toUpperCase() + "  -  " + juce::String ((int) viewBank->presets.size()) + " SOUNDS";
    if (c != bankCaption)
    {
        bankCaption = c;
        repaint (842, 64, 452, 16);
    }
}

void MainPanel::stepPreset (int direction)
{
    std::vector<int> ids;
    for (int i = 0; i < presetBox.getNumItems(); ++i)
        if (presetBox.getItemId (i) > 0) ids.push_back (presetBox.getItemId (i));
    if (ids.empty()) return;

    const auto it = std::find (ids.begin(), ids.end(), presetBox.getSelectedId());
    const int n = (int) ids.size();
    int next;
    if (it == ids.end()) next = direction > 0 ? 0 : n - 1;      // noch nichts gewählt: erster bzw. letzter Sound
    else                 next = ((int) (it - ids.begin()) + direction + n) % n;
    presetBox.setSelectedId (ids[(size_t) next], juce::sendNotificationSync);
}

void MainPanel::showNotice (const juce::String& text)
{
    notice = text;
    noticeTicks = 120;   // ca. 6 Sekunden bei 20 Hz
}

//------------------------------------------------------------------------------
// kleine Dialoge (Text eingeben / bestätigen)
void MainPanel::askText (const juce::String& title, const juce::String& message, const juce::String& initial,
                         const juce::String& okText, std::function<void (const juce::String&)> onOk)
{
    auto* aw = new juce::AlertWindow (title, message, juce::MessageBoxIconType::NoIcon, this);
    aw->addTextEditor ("name", initial, "");
    aw->addButton (okText, 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([aw, onOk] (int r)
    {
        if (r == 1) onOk (aw->getTextEditorContents ("name"));
    }), true);
}

void MainPanel::askSave (const juce::String& initial, bool sampleLoaded,
                         std::function<void (const juce::String&, bool, bool)> onOk)
{
    auto* aw = new juce::AlertWindow ("SAVE TO USER BANK", "Name of your sound:", juce::MessageBoxIconType::NoIcon, this);
    aw->addTextEditor ("name", initial, "");
    aw->addComboBox ("midi", { "SAVE MIDI MAPPING WITH THE SOUND", "DO NOT SAVE MIDI MAPPING" }, "");
    if (sampleLoaded)
        aw->addComboBox ("sample", { "SAVE LOADED SAMPLE (LAYER V) WITH THE SOUND", "DO NOT SAVE THE SAMPLE" }, "");
    aw->addButton ("SAVE", 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([aw, onOk, sampleLoaded] (int r)
    {
        if (r != 1) return;
        const bool midi = aw->getComboBoxComponent ("midi")->getSelectedItemIndex() == 0;
        const bool smp  = sampleLoaded && aw->getComboBoxComponent ("sample")->getSelectedItemIndex() == 0;
        onOk (aw->getTextEditorContents ("name"), midi, smp);
    }), true);
}

void MainPanel::askConfirm (const juce::String& title, const juce::String& message, const juce::String& okText,
                            std::function<void()> onOk)
{
    auto* aw = new juce::AlertWindow (title, message, juce::MessageBoxIconType::QuestionIcon, this);
    aw->addButton (okText, 1, juce::KeyPress (juce::KeyPress::returnKey));
    aw->addButton ("CANCEL", 0, juce::KeyPress (juce::KeyPress::escapeKey));
    aw->enterModalState (true, juce::ModalCallbackFunction::create ([onOk] (int r)
    {
        if (r == 1) onOk();
    }), true);
}

//------------------------------------------------------------------------------
// USER-Bank
void MainPanel::startImport()
{
    chooser = std::make_unique<juce::FileChooser> ("Import sounds into the USER bank",
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
    juce::String initial = proc.getCurrentUserPreset();
    if (initial.isEmpty()) initial = proc.getSoundbankPreset();
    if (initial.isEmpty()) initial = proc.getProgramName (proc.getCurrentProgram());
    if (initial.isEmpty()) initial = "My Sound";

    juce::Component::SafePointer<MainPanel> safe (this);
    askSave (initial, proc.getSample() != nullptr, [safe] (const juce::String& name, bool withMidi, bool withSample)
    {
        if (safe == nullptr) return;
        const auto clean = name.trim();
        if (clean.isEmpty()) { safe->showNotice ("PLEASE ENTER A NAME"); return; }

        if (CSSynthProcessor::userPresetFileFor (clean).exists())
            safe->askConfirm ("SOUND ALREADY EXISTS", "\"" + clean + "\" is already in your USER bank. Replace it?", "REPLACE",
                              [safe, clean, withMidi, withSample]
                              { if (safe != nullptr) safe->saveUserSound (clean, true, withMidi, withSample); });
        else
            safe->saveUserSound (clean, false, withMidi, withSample);
    });
}

void MainPanel::saveUserSound (const juce::String& name, bool overwrite, bool withMidi, bool withSample)
{
    juce::File saved;
    const auto extras = proc.captureExtras (withMidi, withSample);
    if (proc.saveUserPreset (name, overwrite, saved, &extras))
    {
        applyMode (CSSynthProcessor::UserMode);
        juce::String msg = "SAVED TO USER BANK: " + saved.getFileNameWithoutExtension().toUpperCase();
        if (extras.hasMidi)   msg += "  + MIDI MAP";
        if (extras.hasSample) msg += "  + SAMPLE";
        showNotice (msg);
    }
    else
        showNotice ("COULD NOT SAVE SOUND");
}

void MainPanel::startDeleteUserSound()
{
    const auto id = presetBox.getSelectedId();
    if (id < 1000 || (size_t) (id - 1000) >= userFiles.size()) return;
    const auto file = userFiles[(size_t) (id - 1000)];

    juce::Component::SafePointer<MainPanel> safe (this);
    askConfirm ("DELETE SOUND", "Move \"" + file.getFileNameWithoutExtension() + "\" to the recycle bin?", "DELETE",
                [safe, file]
                {
                    if (safe == nullptr) return;
                    if (CSSynthProcessor::trashFile (file))
                    {
                        safe->rebuildPresetBox();
                        safe->showNotice ("DELETED: " + file.getFileNameWithoutExtension().toUpperCase());
                    }
                    else
                        safe->showNotice ("COULD NOT DELETE SOUND");
                });
}

void MainPanel::startExportBank()
{
    chooser = std::make_unique<juce::FileChooser> ("Export USER bank as soundbank file",
                                                   juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                                       .getChildFile ("My Diplomat Bank" + juce::String (cs::presetfile::bankExtension())),
                                                   "*" + juce::String (cs::presetfile::bankExtension()));
    juce::Component::SafePointer<MainPanel> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                              | juce::FileBrowserComponent::warnAboutOverwriting,
                          [safe] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr) return;
                              auto f = fc.getResult();
                              if (f == juce::File()) return;
                              f = f.withFileExtension (cs::presetfile::bankExtension());
                              int n = 0;
                              if (safe->proc.exportUserBank (f, f.getFileNameWithoutExtension(), n))
                                  safe->showNotice ("EXPORTED " + juce::String (n) + " SOUNDS: " + f.getFileName().toUpperCase());
                              else
                                  safe->showNotice ("NOTHING TO EXPORT");
                          });
}

//------------------------------------------------------------------------------
// SOUNDBANKS
void MainPanel::startLoadBank()
{
    chooser = std::make_unique<juce::FileChooser> ("Load a soundbank",
                                                   juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                   "*" + juce::String (cs::presetfile::bankExtension()));
    juce::Component::SafePointer<MainPanel> safe (this);
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe] (const juce::FileChooser& fc)
                          {
                              auto f = fc.getResult();
                              if (safe != nullptr && f != juce::File()) safe->loadBankFile (f);
                          });
}

void MainPanel::loadBankFile (const juce::File& f)
{
    juce::String msg;
    const auto dest = CSSynthProcessor::importBankFile (f, msg);
    if (dest == juce::File())
    {
        showNotice (msg.isNotEmpty() ? msg : juce::String ("NOT A VALID SOUNDBANK FILE"));
        return;
    }
    viewBankId = dest.getFileName();
    applyMode (CSSynthProcessor::SoundbankMode);
    showNotice ("LOADED SOUNDBANK: " + dest.getFileNameWithoutExtension().toUpperCase()
                + (viewBank != nullptr ? "  (" + juce::String ((int) viewBank->presets.size()) + " SOUNDS)" : juce::String()));
}

void MainPanel::startRemoveBank()
{
    if (viewBankId.isEmpty()) return;
    const auto file = CSSynthProcessor::getBankDir().getChildFile (viewBankId);
    juce::Component::SafePointer<MainPanel> safe (this);
    askConfirm ("REMOVE SOUNDBANK", "Move \"" + file.getFileNameWithoutExtension() + "\" to the recycle bin?\n"
                "(Your USER sounds are not affected.)", "REMOVE",
                [safe, file]
                {
                    if (safe == nullptr) return;
                    if (CSSynthProcessor::trashFile (file))
                    {
                        safe->viewBankId = {};
                        safe->applyMode (CSSynthProcessor::SoundbankMode);
                        safe->showNotice ("REMOVED SOUNDBANK: " + file.getFileNameWithoutExtension().toUpperCase());
                    }
                    else
                        safe->showNotice ("COULD NOT REMOVE SOUNDBANK");
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
    {
        proc.loadPresetFile (first);      // den ersten importierten Sound sofort laden (wechselt in die USER-Bank)
        applyMode (CSSynthProcessor::UserMode);
    }

    if (good == 0 && bad == 0) return;    // Dialog ohne Auswahl geschlossen
    juce::String msg = "IMPORTED " + juce::String (good) + (good == 1 ? " SOUND" : " SOUNDS") + " INTO USER BANK";
    if (bad > 0) msg += " | " + juce::String (bad) + " SKIPPED";
    showNotice (msg);
}

bool MainPanel::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (const auto& f : files)
    {
        const auto ext = juce::File (f).getFileExtension().toLowerCase();
        if (ext == cs::presetfile::extension() || ext == ".xml" || ext == cs::presetfile::bankExtension()) return true;
    }
    return anyAudioFile (files);
}

void MainPanel::fileDragEnter (const juce::StringArray& files, int, int)
{
    dropHint = anyAudioFile (files);
    if (sampleView != nullptr) sampleView->setDropHighlight (dropHint);
    repaint();
}

void MainPanel::fileDragExit (const juce::StringArray&)
{
    dropHint = false;
    if (sampleView != nullptr) sampleView->setDropHighlight (false);
    repaint();
}

void MainPanel::filesDropped (const juce::StringArray& files, int, int)
{
    dropHint = false;
    if (sampleView != nullptr) sampleView->setDropHighlight (false);

    juce::Array<juce::File> presets;
    juce::File audio, bank;
    for (const auto& name : files)
    {
        const juce::File f (name);
        const auto ext = f.getFileExtension().toLowerCase();
        if (ext == cs::presetfile::bankExtension()) bank = f;
        else if (ext == cs::presetfile::extension() || ext == ".xml") presets.add (f);
        else if (audio == juce::File() && proc.isSampleFile (f)) audio = f;
    }
    if (bank != juce::File()) loadBankFile (bank);
    if (presets.size() > 0) importFiles (presets);
    if (audio != juce::File()) loadSampleFromFile (audio);
    repaint();
}

void MainPanel::paintOverChildren (juce::Graphics& g)
{
    if (! dropHint) return;
    const auto r = juce::Rectangle<float> (20.f, 92.f, (float) W - 40.f, 560.f);
    g.setColour (juce::Colour (0xff04050a).withAlpha (0.55f));
    g.fillRoundedRectangle (r, 8.f);
    glowRect (g, r.reduced (2.f), col::magenta, 8.f, 0.8f);
    g.setColour (col::magenta);
    g.drawRoundedRectangle (r.reduced (2.f), 8.f, 2.5f);
    g.setColour (col::ice);
    g.setFont (CyberLook::labelFont (30.f));
    g.drawText ("DROP AUDIO FILE TO LOAD INTO LAYER V", r.toNearestInt(), juce::Justification::centred, false);
}

void MainPanel::timerCallback()
{
    for (auto* l : learners) l->refreshLearn();
    if (sampleView != nullptr && curPage == 2) sampleView->refresh();

    syncPresetBox();

    if (const int ev = proc.getLearnEvent(); ev != seenLearnEvent)     // MIDI-Learn hat gerade eine Zuweisung vorgenommen
    {
        seenLearnEvent = ev;
        const int pi = proc.getLearnedParam();
        if (pi >= 0)
        {
            notice = "LEARNED:  CC " + juce::String (proc.getLearnedCc()) + "  >  " + juce::String (cs::specs()[(size_t) pi].name).toUpperCase();
            noticeTicks = 60;
        }
    }

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
        repaint (600, 756, 390, 26);
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

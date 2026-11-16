// Diplomat 1506 - Wellenform-Anzeige für den Sample-Layer (Layer V):
// zeigt die geladene Audiodatei, Startposition und Loop-Bereich; Marker lassen sich mit der Maus ziehen.
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"
#include "Look.h"

class SampleView : public juce::Component
{
public:
    SampleView (CSSynthProcessor& p, std::function<void()> onEmptyClick)
        : proc (p), openRequest (std::move (onEmptyClick))
    {
        setMouseCursor (juce::MouseCursor::PointingHandCursor);
    }

    // Regelmäßig aufrufen (Timer): baut die Anzeige neu, wenn sich Sample oder Marker geändert haben
    void refresh()
    {
        const int rev = proc.getSampleRevision();
        const auto key = markerKey();
        if (rev != shownRevision || key != shownKey)
        {
            shownRevision = rev; shownKey = key;
            peaksWidth = -1;
            repaint();
        }
    }

    void setDropHighlight (bool on) { if (on != dropHighlight) { dropHighlight = on; repaint(); } }

    void paint (juce::Graphics& g) override
    {
        const auto r = getLocalBounds().toFloat();
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff0b0d22), 0.f, 0.f, juce::Colour (0xff04050c), 0.f, r.getBottom(), false));
        g.fillRoundedRectangle (r, 5.f);

        // Raster
        g.setColour (col::cyan.withAlpha (0.06f));
        for (int i = 1; i < 8; ++i) g.fillRect (r.getX() + r.getWidth() * (float) i / 8.f, r.getY(), 1.f, r.getHeight());
        g.setColour (col::cyan.withAlpha (0.18f));
        g.fillRect (r.getX(), r.getCentreY(), r.getWidth(), 1.f);

        const auto s = proc.getSample();
        if (s == nullptr || s->empty())
        {
            const auto c = dropHighlight ? col::magenta : col::cyan;
            if (dropHighlight) glowRect (g, r.reduced (3.f), c, 5.f, 0.8f);
            juce::Path outline; outline.addRoundedRectangle (r.reduced (4.f), 5.f);
            juce::Path dashed;
            const float dash[2] = { 7.f, 5.f };
            juce::PathStrokeType (1.4f).createDashedStroke (dashed, outline, dash, 2);
            g.setColour (c.withAlpha (0.85f));
            g.fillPath (dashed);

            g.setColour (col::ice);
            g.setFont (CyberLook::labelFont (17.f));
            g.drawText ("DROP AN AUDIO FILE HERE", getLocalBounds().withTrimmedBottom (26), juce::Justification::centred, false);
            g.setColour (col::iceDim);
            g.setFont (CyberLook::labelFont (11.5f, false));
            g.drawText ("or click to browse   |   WAV  AIFF  FLAC  OGG  MP3", getLocalBounds().withTrimmedTop (34), juce::Justification::centred, false);
            return;
        }

        const int w = getWidth(), h = getHeight();
        const float mid = (float) h * 0.5f, amp = (float) h * 0.5f - 18.f;
        updatePeaks (*s, w);

        const float startX = markerX (kStart), ls = markerX (kLoopStart), le = markerX (kLoopEnd);
        const int mode = (int) std::lround (valueOf (cs::SAMPLE_LAYER * cs::LP_COUNT + cs::LP_WAVE));
        const bool looping = mode != 1;

        // Loop-Bereich hervorheben
        if (looping && le > ls)
        {
            g.setColour (col::magenta.withAlpha (0.12f));
            g.fillRect (ls, 0.f, le - ls, (float) h);
        }

        // Wellenform: Kern hell, Hülle dunkler; Bereiche außerhalb von Start/Loop abgedunkelt
        for (int x = 0; x < w; ++x)
        {
            const auto pk = peaks[(size_t) x];
            const float y0 = mid - pk.second * amp, y1 = mid - pk.first * amp;
            const bool active = (float) x >= startX && (! looping || ((float) x >= ls && (float) x <= le) || (float) x < ls);
            const float a = std::max (std::fabs (pk.first), std::fabs (pk.second));
            const auto c = col::value (0.25f + 0.75f * a);
            g.setColour (c.withAlpha (active ? 0.9f : 0.35f));
            g.fillRect ((float) x, y0, 1.f, std::max (1.f, y1 - y0));
        }

        // Marker
        drawMarker (g, startX, col::cyan, "START", true, h);
        if (looping)
        {
            drawMarker (g, ls, col::magenta, "LOOP", true, h);
            drawMarker (g, le, col::magenta, "END", false, h);
        }

        // Kopfzeile: Name links, Länge/Samplerate rechts
        g.setFont (CyberLook::labelFont (12.f));
        g.setColour (col::ice);
        g.drawText (juce::String (s->name).toUpperCase(), 10, 4, w / 2, 16, juce::Justification::centredLeft, true);
        g.setColour (col::iceDim);
        g.drawText (juce::String ((double) s->data.size() / s->rate, 2) + " S   |   "
                        + juce::String (s->rate / 1000.0, 1) + " KHZ   |   MONO",
                    w / 2, 4, w / 2 - 10, 16, juce::Justification::centredRight, false);

        if (dropHighlight)
        {
            g.setColour (col::magenta);
            g.drawRoundedRectangle (r.reduced (1.5f), 5.f, 2.f);
        }
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (hitMarker (e.x) >= 0 ? juce::MouseCursor::LeftRightResizeCursor : juce::MouseCursor::PointingHandCursor);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        const auto s = proc.getSample();
        if (s == nullptr || s->empty()) { if (openRequest) openRequest(); return; }
        dragging = hitMarker (e.x);
        if (dragging >= 0)
            if (auto* p = proc.getParam (indexOf (dragging))) p->beginChangeGesture();
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragging < 0) return;
        const float v = juce::jlimit (0.f, 1.f, (float) e.x / (float) juce::jmax (1, getWidth() - 1));
        if (auto* p = proc.getParam (indexOf (dragging))) p->setValueNotifyingHost (v);
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        if (dragging >= 0)
            if (auto* p = proc.getParam (indexOf (dragging))) p->endChangeGesture();
        dragging = -1;
    }

    void mouseDoubleClick (const juce::MouseEvent&) override
    {
        const auto s = proc.getSample();
        if (s != nullptr && ! s->empty() && openRequest) openRequest();     // Doppelklick: andere Datei laden
    }

private:
    enum Marker { kStart, kLoopStart, kLoopEnd };

    static int indexOf (int marker)
    {
        return marker == kStart ? cs::SAMPLE_LAYER * cs::LP_COUNT + cs::LP_PW
             : marker == kLoopStart ? (int) cs::GP_SMP_LSTART : (int) cs::GP_SMP_LEND;
    }
    float valueOf (int idx) const
    {
        if (auto* p = proc.getParam (idx)) return p->convertFrom0to1 (p->getValue());
        return 0.f;
    }
    float markerX (int m) const { return valueOf (indexOf (m)) * (float) (getWidth() - 1); }
    juce::int64 markerKey() const
    {
        return (juce::int64) std::lround (valueOf (indexOf (kStart)) * 1000.f) * 1000003
             + (juce::int64) std::lround (valueOf (indexOf (kLoopStart)) * 1000.f) * 1009
             + (juce::int64) std::lround (valueOf (indexOf (kLoopEnd)) * 1000.f)
             + (juce::int64) std::lround (valueOf (cs::SAMPLE_LAYER * cs::LP_COUNT + cs::LP_WAVE)) * 7919;
    }

    int hitMarker (int x) const
    {
        const auto s = proc.getSample();
        if (s == nullptr || s->empty()) return -1;
        const bool loop = std::lround (valueOf (cs::SAMPLE_LAYER * cs::LP_COUNT + cs::LP_WAVE)) != 1;
        int best = -1; float bestD = 9.f;
        for (int m = 0; m < 3; ++m)
        {
            if (m != kStart && ! loop) continue;
            const float d = std::fabs ((float) x - markerX (m));
            if (d < bestD) { bestD = d; best = m; }
        }
        return best;
    }

    void drawMarker (juce::Graphics& g, float x, juce::Colour c, const char* label, bool labelRight, int h) const
    {
        glowRect (g, juce::Rectangle<float> (x - 0.5f, 0.f, 1.f, (float) h), c, 0.5f, 0.6f);
        g.setColour (c);
        g.fillRect (x - 0.75f, 0.f, 1.5f, (float) h);
        juce::Path tri;
        tri.addTriangle (x - 5.f, (float) h - 1.f, x + 5.f, (float) h - 1.f, x, (float) h - 9.f);
        g.fillPath (tri);
        g.setFont (CyberLook::labelFont (9.5f));
        const float tw = 34.f;
        g.drawText (label, juce::Rectangle<float> (labelRight ? x + 4.f : x - 4.f - tw, (float) h - 22.f, tw, 12.f),
                    labelRight ? juce::Justification::centredLeft : juce::Justification::centredRight, false);
    }

    void updatePeaks (const cs::SampleData& s, int w)
    {
        if (peaksWidth == w && peaksRevision == shownRevision) return;
        peaksWidth = w; peaksRevision = shownRevision;
        peaks.assign ((size_t) w, { 0.f, 0.f });
        const size_t n = s.data.size();
        for (int x = 0; x < w; ++x)
        {
            const size_t a = (size_t) ((double) x * (double) n / (double) w);
            size_t b = (size_t) ((double) (x + 1) * (double) n / (double) w);
            b = std::min (std::max (b, a + 1), n);
            float lo = 0.f, hi = 0.f;
            for (size_t i = a; i < b; ++i) { lo = std::min (lo, s.data[i]); hi = std::max (hi, s.data[i]); }
            peaks[(size_t) x] = { lo, hi };
        }
    }

    CSSynthProcessor& proc;
    std::function<void()> openRequest;
    std::vector<std::pair<float, float>> peaks;
    int peaksWidth = -1, peaksRevision = -1;
    int shownRevision = -1;
    juce::int64 shownKey = 0;
    int dragging = -1;
    bool dropHighlight = false;
};

// Einfacher mehrzeiliger Text (für Hinweise)
class TextBlock : public juce::Component
{
public:
    TextBlock (juce::String t, float size, juce::Colour c = col::iceDim) : text (std::move (t)), fontSize (size), colour (c)
    {
        setInterceptsMouseClicks (false, false);
    }
    void paint (juce::Graphics& g) override
    {
        g.setColour (colour);
        g.setFont (CyberLook::labelFont (fontSize, false));
        g.drawFittedText (text, getLocalBounds(), juce::Justification::topLeft, 12, 1.f);
    }
private:
    juce::String text;
    float fontSize;
    juce::Colour colour;
};

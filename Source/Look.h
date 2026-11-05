// Diplomat 1506 - Cyberpunk-Optik: Neon-Drehregler und -Fader (Farbe/Helligkeit folgt dem Wert),
// Taster mit Leuchtanzeige, MIDI-Learn-Widgets, eingebettete Schriftarten.
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "BinaryData.h"
#include "PluginProcessor.h"

namespace col
{
const juce::Colour ice     { 0xffd9f4ff };   // Haupttext
const juce::Colour iceDim  { 0xff7f90b8 };   // Nebentext
const juce::Colour cyan    { 0xff12e6ff };   // Hauptakzent
const juce::Colour magenta { 0xffff2fd0 };   // zweiter Akzent
const juce::Colour violet  { 0xff8a4dff };   // dritter Akzent
const juce::Colour yellow  { 0xfffff23a };   // MIDI-Learn-Markierung
const juce::Colour panel   { 0xff0e1022 };
const juce::Colour panelHi { 0xff1a1e3c };
const juce::Colour body    { 0xff080913 };
const juce::Colour edge    { 0xff2b3366 };

inline juce::Colour mix (juce::Colour a, juce::Colour b, float t) { return a.interpolatedWith (b, juce::jlimit (0.f, 1.f, t)); }

// Farbe eines Bedienelements in Abhängigkeit vom Wert (0..1): dunkles Violett -> Cyan -> fast Weiß.
// Je höher der Wert, desto heller und kräftiger leuchtet das Element.
inline juce::Colour value (float t)
{
    t = juce::jlimit (0.f, 1.f, t);
    const juce::Colour lo { 0xff5a34d6 }, mid { 0xff12d8ff }, hi { 0xffeafeff };
    return t < 0.6f ? mix (lo, mid, t / 0.6f) : mix (mid, hi, (t - 0.6f) / 0.4f);
}
// Wie value(), aber in Magenta (für die LEVEL-Fader und den Master-Regler).
inline juce::Colour accent (float t)
{
    t = juce::jlimit (0.f, 1.f, t);
    const juce::Colour lo { 0xff6a2bbd }, mid { 0xffff2fd0 }, hi { 0xffffe3fa };
    return t < 0.6f ? mix (lo, mid, t / 0.6f) : mix (mid, hi, (t - 0.6f) / 0.4f);
}
} // namespace col

//==============================================================================
// Eingebettete Schriftarten (SIL Open Font License): Orbitron (Name), Audiowide (Modellnummer),
// Rajdhani (Beschriftungen).
namespace fonts
{
inline juce::Typeface::Ptr make (const char* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
}
inline juce::Font title (float h)
{
    static const auto t = make (BinaryData::FontTitle_ttf, BinaryData::FontTitle_ttfSize);
    return juce::Font (juce::FontOptions (t)).withHeight (h);
}
inline juce::Font model (float h)
{
    static const auto t = make (BinaryData::FontModel_ttf, BinaryData::FontModel_ttfSize);
    return juce::Font (juce::FontOptions (t)).withHeight (h);
}
inline juce::Font label (float h, bool bold = true)
{
    static const auto a = make (BinaryData::FontLabel_ttf, BinaryData::FontLabel_ttfSize);
    static const auto b = make (BinaryData::FontLabelBold_ttf, BinaryData::FontLabelBold_ttfSize);
    return juce::Font (juce::FontOptions (bold ? b : a)).withHeight (h * 1.14f).withExtraKerningFactor (0.05f);
}
} // namespace fonts

// Leuchteffekt: mehrere weiche Schichten übereinander
inline void glowEllipse (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float strength = 1.f)
{
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (c.withAlpha (0.07f * strength * (float) (5 - i)));
        g.fillEllipse (r.expanded ((float) i * 1.6f));
    }
}
inline void glowRect (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour c, float corner, float strength = 1.f)
{
    for (int i = 4; i >= 1; --i)
    {
        g.setColour (c.withAlpha (0.06f * strength * (float) (5 - i)));
        g.fillRoundedRectangle (r.expanded ((float) i * 1.4f), corner + (float) i);
    }
}

//==============================================================================
// Gemeinsames Verhalten aller bedienbaren Elemente: Rechtsklick-Menü für MIDI-Learn und
// Anzeige des Learn-Zustands (Rahmen um das Element, solange es auf einen Controller wartet).
class LearnWidget
{
public:
    LearnWidget (CSSynthProcessor& p, int idx, juce::Component& c) : proc (p), paramIdx (idx), self (c) {}
    virtual ~LearnWidget() = default;

    void refreshLearn()
    {
        const bool l = proc.getLearnIndex() == paramIdx;
        if (l != shownLearn)
        {
            shownLearn = l;
            self.repaint();
        }
    }

    void showLearnMenu()
    {
        const bool learning = proc.getLearnIndex() == paramIdx;
        const int cc = proc.getCcForParam (paramIdx);

        juce::PopupMenu m;
        m.addItem (1, learning ? "Waiting for MIDI CC ...  (click to cancel)" : "MIDI Learn", true, learning);
        m.addItem (2, cc >= 0 ? "Clear MIDI mapping  (CC " + juce::String (cc) + ")" : juce::String ("Clear MIDI mapping"), cc >= 0);
        m.addSeparator();
        m.addItem (3, "Reset to default");

        juce::Component::SafePointer<juce::Component> sp (&self);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (&self), [this, sp, learning] (int r)
        {
            if (sp == nullptr) return;
            if (r == 1)      { if (learning) proc.cancelLearn(); else proc.startLearn (paramIdx); }
            else if (r == 2) proc.clearMappingForParam (paramIdx);
            else if (r == 3) resetToDefault();
            refreshLearn();
        });
    }

    void resetToDefault()
    {
        if (auto* p = proc.getParam (paramIdx))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->getDefaultValue());
            p->endChangeGesture();
        }
    }

    CSSynthProcessor& proc;
    const int paramIdx;
    bool shownLearn = false;
    int steps = 0; // >1: gerasteter Regler mit so vielen Positionen

private:
    juce::Component& self;
};

//==============================================================================
class ParamSlider : public juce::Slider, public LearnWidget
{
public:
    ParamSlider (CSSynthProcessor& p, int idx, juce::Slider::SliderStyle style)
        : juce::Slider (style, juce::Slider::NoTextBox), LearnWidget (p, idx, *this)
    {
        setPopupDisplayEnabled (true, true, nullptr);
        setScrollWheelEnabled (true);
        setVelocityBasedMode (false);
        setMouseDragSensitivity (180);
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showLearnMenu(); return; }
        juce::Slider::mouseDown (e);
    }
    void mouseDrag (const juce::MouseEvent& e) override { if (! e.mods.isPopupMenu()) juce::Slider::mouseDrag (e); }
    void mouseUp (const juce::MouseEvent& e) override   { if (! e.mods.isPopupMenu()) juce::Slider::mouseUp (e); }
};

//==============================================================================
class CyberLook : public juce::LookAndFeel_V4
{
public:
    CyberLook()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff070816));
        setColour (juce::ComboBox::textColourId, col::ice);
        setColour (juce::ComboBox::outlineColourId, col::cyan.withAlpha (0.55f));
        setColour (juce::ComboBox::arrowColourId, col::cyan);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff0a0b1c));
        setColour (juce::PopupMenu::textColourId, col::ice);
        setColour (juce::PopupMenu::headerTextColourId, col::magenta);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::cyan);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colour (0xff04050c));
        setColour (juce::TextButton::buttonColourId, col::panelHi);
        setColour (juce::TextButton::buttonOnColourId, col::cyan);
        setColour (juce::TextButton::textColourOffId, col::ice);
        setColour (juce::TextButton::textColourOnId, juce::Colour (0xff04050c));
        setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf007081a));
        setColour (juce::BubbleComponent::outlineColourId, col::cyan);
        setColour (juce::TooltipWindow::textColourId, col::ice);
    }

    static juce::Font labelFont (float h, bool bold = true) { return fonts::label (h, bold); }

    int getSliderThumbRadius (juce::Slider& s) override
    {
        return s.isHorizontal() || s.isVertical() ? 9 : 10;
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return labelFont (11.f); }
    juce::Font getPopupMenuFont() override { return labelFont (14.f, false); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return labelFont (14.f); }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down) override
    {
        const auto r = b.getLocalBounds().toFloat().reduced (1.f);
        const bool on = b.getToggleState();
        const auto c = on ? col::cyan : col::edge.brighter (0.3f);
        if (on) glowRect (g, r, col::cyan, 4.f, 0.8f);
        g.setGradientFill (juce::ColourGradient (on ? col::cyan.withAlpha (0.9f) : col::panelHi.brighter (down ? 0.3f : over ? 0.15f : 0.f), r.getX(), r.getY(),
                                                 on ? col::cyan.darker (0.4f) : juce::Colour (0xff0b0c1d), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 4.f);
        g.setColour (c.withAlpha (on ? 1.f : 0.9f));
        g.drawRoundedRectangle (r, 4.f, 1.2f);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        auto* ps = dynamic_cast<ParamSlider*> (&s);
        const auto c = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
        const float R = (float) juce::jmin (w, h) * 0.5f;
        const float kr = R - 10.f;
        const float ang = startAngle + pos * (endAngle - startAngle);
        const bool accentKnob = (bool) s.getProperties().getWithDefault ("accent", false);
        const auto vc = accentKnob ? col::accent (pos) : col::value (pos);

        auto dir = [&] (float a) { return juce::Point<float> (std::sin (a), -std::cos (a)); };

        // Skalenstriche: bis zum aktuellen Wert leuchtend in der Wertfarbe
        const int steps = ps != nullptr ? ps->steps : 0;
        const int nt = steps > 1 ? steps : 11;
        for (int i = 0; i < nt; ++i)
        {
            const float f = (float) i / (float) (nt - 1);
            const float a = startAngle + (endAngle - startAngle) * f;
            const bool major = steps > 1 || i % 5 == 0;
            const bool lit = f <= pos + 0.001f;
            g.setColour (lit ? (accentKnob ? col::accent (f) : col::value (f)) : col::iceDim.withAlpha (0.35f));
            const auto d = dir (a);
            g.drawLine (juce::Line<float> (c + d * (kr + 6.f), c + d * (kr + (major ? 10.f : 8.f))), major ? 1.8f : 1.1f);
        }

        // Schatten + Korpus
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillEllipse (c.x - kr + 1.f, c.y - kr + 2.5f, kr * 2.f, kr * 2.f);

        // Wertbogen (Glühen) um den Korpus
        {
            const float ar = kr + 2.5f;
            juce::Path track, val;
            track.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, endAngle, true);
            val.addCentredArc (c.x, c.y, ar, ar, 0.f, startAngle, juce::jmax (startAngle + 0.001f, ang), true);
            g.setColour (juce::Colour (0xff1c2146));
            g.strokePath (track, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            if (pos > 0.002f)
            {
                g.setColour (vc.withAlpha (0.10f + 0.12f * pos));
                g.strokePath (val, juce::PathStrokeType (8.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour (vc.withAlpha (0.25f + 0.2f * pos));
                g.strokePath (val, juce::PathStrokeType (5.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
                g.setColour (vc);
                g.strokePath (val, juce::PathStrokeType (3.f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
            }
        }

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2f57), c.x - kr, c.y - kr,
                                                 juce::Colour (0xff05060e), c.x + kr, c.y + kr, false));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
        g.setColour (vc.withAlpha (0.35f));
        g.drawEllipse (c.x - kr + 0.5f, c.y - kr + 0.5f, kr * 2.f - 1.f, kr * 2.f - 1.f, 1.f);

        // Kappe
        const float cr = kr * 0.74f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1d2145), c.x - cr, c.y - cr,
                                                 juce::Colour (0xff090a16), c.x + cr, c.y + cr, false));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (float f = 0.3f; f < 1.f; f += 0.22f)
            g.drawEllipse (c.x - cr * f, c.y - cr * f, cr * f * 2.f, cr * f * 2.f, 0.8f);

        // Zeiger mit Leuchtpunkt
        const auto d = dir (ang);
        g.setColour (vc.withAlpha (0.35f));
        g.drawLine (juce::Line<float> (c + d * (cr * 0.18f), c + d * (cr * 0.98f)), 5.f);
        g.setColour (vc.brighter (0.25f));
        g.drawLine (juce::Line<float> (c + d * (cr * 0.18f), c + d * (cr * 0.98f)), 2.2f);

        // MIDI-Learn-Anzeige
        if (ps != nullptr && ps->shownLearn)
        {
            g.setColour (col::yellow);
            g.drawEllipse (c.x - kr - 5.f, c.y - kr - 5.f, kr * 2.f + 10.f, kr * 2.f + 10.f, 2.f);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        auto* ps = dynamic_cast<ParamSlider*> (&s);
        const float cx = (float) x + (float) w * 0.5f;
        const float top = (float) y + 9.f, bottom = (float) (y + h) - 9.f;
        const bool accentFader = (bool) s.getProperties().getWithDefault ("accent", false);
        const float t = juce::jlimit (0.f, 1.f, (float) s.valueToProportionOfLength (s.getValue()));
        auto cf = [&] (float f) { return accentFader ? col::accent (f) : col::value (f); };
        const auto vc = cf (t);

        // Teilung links/rechts: bis zum Wert leuchtend
        for (int i = 0; i <= 10; ++i)
        {
            const float f = (float) i / 10.f;
            const float ty = bottom - (bottom - top) * f;
            const float len = (i % 5 == 0) ? 5.f : 3.f;
            g.setColour (f <= t + 0.001f ? cf (f) : col::iceDim.withAlpha (0.35f));
            g.drawLine (cx - 10.f - len, ty, cx - 10.f, ty, 1.1f);
            g.drawLine (cx + 10.f, ty, cx + 10.f + len, ty, 1.1f);
        }

        // Führungsschlitz mit leuchtender Füllung bis zur Kappe
        const juce::Rectangle<float> slot (cx - 2.5f, top - 4.f, 5.f, bottom - top + 8.f);
        g.setColour (juce::Colour (0xff03040a));
        g.fillRoundedRectangle (slot, 2.f);
        g.setColour (col::edge.withAlpha (0.8f));
        g.drawRoundedRectangle (slot.expanded (0.5f), 2.f, 1.f);

        if (t > 0.002f)
        {
            const juce::Rectangle<float> fill (cx - 1.8f, pos, 3.6f, (bottom + 4.f) - pos);
            g.setGradientFill (juce::ColourGradient (vc, 0.f, pos, cf (0.f).withAlpha (0.85f), 0.f, bottom, false));
            glowRect (g, fill, vc, 1.5f, 0.5f + t);
            g.setGradientFill (juce::ColourGradient (vc, 0.f, pos, cf (0.f).withAlpha (0.85f), 0.f, bottom, false));
            g.fillRoundedRectangle (fill, 1.5f);
        }

        // Kappe: dunkles Metall mit leuchtender Kante und Markierung in der Wertfarbe
        const juce::Rectangle<float> cap (cx - 12.f, pos - 9.f, 24.f, 18.f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (cap.translated (1.5f, 2.f), 3.f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a4070), cap.getX(), cap.getY(),
                                                 juce::Colour (0xff0d0f22), cap.getX(), cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 3.f);
        g.setColour (vc.withAlpha (0.55f + 0.35f * t));
        g.drawRoundedRectangle (cap.reduced (0.5f), 3.f, 1.f);
        g.setColour (vc.withAlpha (0.4f));
        g.fillRect (cap.getX() + 3.f, pos - 2.f, cap.getWidth() - 6.f, 4.f);
        g.setColour (vc.brighter (0.3f));
        g.fillRect (cap.getX() + 3.f, pos - 0.75f, cap.getWidth() - 6.f, 1.5f);

        if (ps != nullptr && ps->shownLearn)
        {
            g.setColour (col::yellow);
            g.drawRoundedRectangle (cap.expanded (3.f), 4.f, 2.f);
        }
    }
};

//==============================================================================
class KnobCell : public juce::Component
{
public:
    KnobCell (CSSynthProcessor& p, int idx, const juce::String& text, int knobSize, int labelHeight = 16)
        : label (text), ks (knobSize), lh (labelHeight),
          slider (p, idx, juce::Slider::RotaryHorizontalVerticalDrag)
    {
        const auto& spec = cs::specs()[(size_t) idx];
        auto* param = p.getParam (idx);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f, juce::MathConstants<float>::pi * 2.8f, true);
        attach = std::make_unique<juce::SliderParameterAttachment> (*param, slider, nullptr);
        slider.setDoubleClickReturnValue (true, spec.def);
        if (spec.kind == cs::Kind::Choice) slider.steps = (int) spec.choices.size();
        addAndMakeVisible (slider);
    }

    ParamSlider& getSlider() { return slider; }

    void resized() override
    {
        slider.setBounds ((getWidth() - ks) / 2, 0, ks, ks);
    }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::ice);
        g.setFont (CyberLook::labelFont (lh > 14 ? 11.f : 9.5f));
        g.drawText (label, 0, getHeight() - lh, getWidth(), lh, juce::Justification::centred, false);
    }

private:
    juce::String label;
    int ks, lh;
    ParamSlider slider;
    std::unique_ptr<juce::SliderParameterAttachment> attach;
};

//==============================================================================
class FaderCell : public juce::Component
{
public:
    FaderCell (CSSynthProcessor& p, int idx, const juce::String& text, bool accent = false)
        : label (text), slider (p, idx, juce::Slider::LinearVertical)
    {
        auto* param = p.getParam (idx);
        attach = std::make_unique<juce::SliderParameterAttachment> (*param, slider, nullptr);
        slider.setDoubleClickReturnValue (true, cs::specs()[(size_t) idx].def);
        slider.getProperties().set ("accent", accent);
        addAndMakeVisible (slider);
    }

    ParamSlider& getSlider() { return slider; }
    void resized() override { slider.setBounds (getLocalBounds().withTrimmedBottom (16)); }

    void paint (juce::Graphics& g) override
    {
        g.setColour (col::ice);
        g.setFont (CyberLook::labelFont (10.5f));
        g.drawText (label, 0, getHeight() - 16, getWidth(), 16, juce::Justification::centred, false);
    }

private:
    juce::String label;
    ParamSlider slider;
    std::unique_ptr<juce::SliderParameterAttachment> attach;
};

//==============================================================================
// Senkrechte Reihe von Tasten mit LED, z.B. Wellenform-Wahl. Zeigt Auswahl einer Choice-Parameters.
class ChoiceButtons : public juce::Component, public LearnWidget
{
public:
    ChoiceButtons (CSSynthProcessor& p, int idx, juce::StringArray names, const juce::String& text)
        : LearnWidget (p, idx, *this), items (std::move (names)), label (text),
          attach (*p.getParam (idx), [this] (float v) { current = (int) std::lround (v); repaint(); }, nullptr)
    {
        attach.sendInitialUpdate();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showLearnMenu(); return; }
        const int i = e.y / rowH;
        if (juce::isPositiveAndBelow (i, items.size()))
            attach.setValueAsCompleteGesture ((float) i);
    }

    void paint (juce::Graphics& g) override
    {
        for (int i = 0; i < items.size(); ++i)
        {
            const auto r = juce::Rectangle<float> (4.f, (float) (i * rowH) + 2.f, (float) getWidth() - 8.f, (float) rowH - 4.f);
            const bool on = i == current;
            if (on) glowRect (g, r, col::cyan, 3.f, 0.7f);
            g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff1f3a5e) : juce::Colour (0xff1a1e3c), r.getX(), r.getY(),
                                                     juce::Colour (0xff090a18), r.getX(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 3.f);
            g.setColour (on ? col::cyan : col::edge);
            g.drawRoundedRectangle (r.reduced (0.5f), 3.f, on ? 1.4f : 1.f);

            // LED
            const auto led = juce::Rectangle<float> (r.getX() + 7.f, r.getCentreY() - 3.5f, 7.f, 7.f);
            if (on) glowEllipse (g, led, col::cyan);
            g.setColour (on ? col::cyan.brighter (0.4f) : juce::Colour (0xff1a2744));
            g.fillEllipse (led);

            g.setColour (on ? col::ice : col::iceDim);
            g.setFont (CyberLook::labelFont (10.5f));
            g.drawText (items[i].toUpperCase(), (int) r.getX() + 18, (int) r.getY(), (int) r.getWidth() - 20, (int) r.getHeight(),
                        juce::Justification::centredLeft, false);
        }
        g.setColour (col::ice);
        g.setFont (CyberLook::labelFont (11.f));
        g.drawText (label, 0, items.size() * rowH + 1, getWidth(), 16, juce::Justification::centred, false);

        if (shownLearn)
        {
            g.setColour (col::yellow);
            g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 4.f, 2.f);
        }
    }

    static constexpr int rowH = 22;

private:
    juce::StringArray items;
    juce::String label;
    int current = 0;
    juce::ParameterAttachment attach;
};

//==============================================================================
// Einzelner Taster mit LED für einen Ein/Aus-Parameter (z.B. Ping-Pong).
class ToggleLed : public juce::Component, public LearnWidget
{
public:
    ToggleLed (CSSynthProcessor& p, int idx, const juce::String& text)
        : LearnWidget (p, idx, *this), label (text),
          attach (*p.getParam (idx), [this] (float v) { on = v > 0.5f; repaint(); }, nullptr)
    {
        attach.sendInitialUpdate();
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        if (e.mods.isPopupMenu()) { showLearnMenu(); return; }
        attach.setValueAsCompleteGesture (on ? 0.f : 1.f);
    }

    void paint (juce::Graphics& g) override
    {
        const auto r = juce::Rectangle<float> (7.f, 6.f, (float) getWidth() - 14.f, 28.f);
        if (on) glowRect (g, r, col::cyan, 3.f, 0.7f);
        g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff1f3a5e) : juce::Colour (0xff1a1e3c), r.getX(), r.getY(),
                                                 juce::Colour (0xff090a18), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 3.f);
        g.setColour (on ? col::cyan : col::edge);
        g.drawRoundedRectangle (r.reduced (0.5f), 3.f, on ? 1.4f : 1.f);

        const auto led = juce::Rectangle<float> (r.getCentreX() - 4.f, r.getCentreY() - 4.f, 8.f, 8.f);
        if (on) glowEllipse (g, led, col::cyan);
        g.setColour (on ? col::cyan.brighter (0.4f) : juce::Colour (0xff1a2744));
        g.fillEllipse (led);

        g.setColour (col::ice);
        g.setFont (CyberLook::labelFont (9.5f));
        g.drawText (label, 0, getHeight() - 16, getWidth(), 16, juce::Justification::centred, false);

        if (shownLearn)
        {
            g.setColour (col::yellow);
            g.drawRoundedRectangle (r.expanded (2.f), 4.f, 2.f);
        }
    }

private:
    juce::String label;
    bool on = false;
    juce::ParameterAttachment attach;
};

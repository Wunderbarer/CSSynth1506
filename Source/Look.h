// CSSynth 1506 - Vintage-Optik: Drehregler, Fader, LED-Taster, MIDI-Learn-Widgets
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace col
{
const juce::Colour cream   { 0xffe9dfc6 };
const juce::Colour creamDim{ 0xffb3a98f };
const juce::Colour orange  { 0xffe0732a };
const juce::Colour amber   { 0xffe8a33a };
const juce::Colour rust    { 0xffb8422e };
const juce::Colour panel   { 0xff2c2926 };
const juce::Colour panelHi { 0xff3a3631 };
const juce::Colour body    { 0xff1e1c19 };
const juce::Colour edge    { 0xff4d473f };
} // namespace col

//==============================================================================
// Gemeinsames Verhalten aller bedienbaren Elemente: Rechtsklick-Menü für MIDI-Learn und
// Anzeige von Learn-Zustand bzw. zugewiesener CC-Nummer.
class LearnWidget
{
public:
    LearnWidget (CSSynthProcessor& p, int idx, juce::Component& c) : proc (p), paramIdx (idx), self (c) {}
    virtual ~LearnWidget() = default;

    void refreshLearn()
    {
        const bool l = proc.getLearnIndex() == paramIdx;
        const int cc = proc.getCcForParam (paramIdx);
        if (l != shownLearn || cc != shownCc)
        {
            shownLearn = l; shownCc = cc;
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
    int shownCc = -1;
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
class VintageLook : public juce::LookAndFeel_V4
{
public:
    VintageLook()
    {
        setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff15130f));
        setColour (juce::ComboBox::textColourId, col::cream);
        setColour (juce::ComboBox::outlineColourId, col::edge);
        setColour (juce::ComboBox::arrowColourId, col::orange);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1b1815));
        setColour (juce::PopupMenu::textColourId, col::cream);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, col::orange);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::black);
        setColour (juce::TextButton::buttonColourId, col::panelHi);
        setColour (juce::TextButton::textColourOffId, col::cream);
        setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xf0121110));
        setColour (juce::BubbleComponent::outlineColourId, col::orange);
        setColour (juce::TooltipWindow::textColourId, col::cream);
    }

    static juce::Font labelFont (float h, bool bold = true)
    {
        return juce::Font (juce::FontOptions (h, bold ? juce::Font::bold : juce::Font::plain)).withExtraKerningFactor (0.06f);
    }

    int getSliderThumbRadius (juce::Slider& s) override
    {
        return s.isHorizontal() || s.isVertical() ? 9 : 10;
    }

    juce::Font getPopupMenuFont() override { return labelFont (14.f, false); }
    juce::Font getComboBoxFont (juce::ComboBox&) override { return labelFont (14.f); }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider& s) override
    {
        using juce::MathConstants;
        auto* ps = dynamic_cast<ParamSlider*> (&s);
        const auto c = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
        const float R = (float) juce::jmin (w, h) * 0.5f;
        const float kr = R - 9.f;
        const float ang = startAngle + pos * (endAngle - startAngle);

        auto dir = [&] (float a) { return juce::Point<float> (std::sin (a), -std::cos (a)); };

        // Skalenstriche
        const int steps = ps != nullptr ? ps->steps : 0;
        const int nt = steps > 1 ? steps : 11;
        g.setColour (col::cream.withAlpha (0.85f));
        for (int i = 0; i < nt; ++i)
        {
            const float a = startAngle + (endAngle - startAngle) * (float) i / (float) (nt - 1);
            const bool major = steps > 1 || i % 5 == 0;
            const auto d = dir (a);
            g.drawLine (juce::Line<float> (c + d * (kr + 3.f), c + d * (kr + (major ? 8.f : 6.f))), major ? 1.6f : 1.f);
        }

        // Schatten + Korpus
        g.setColour (juce::Colours::black.withAlpha (0.55f));
        g.fillEllipse (c.x - kr + 1.5f, c.y - kr + 3.f, kr * 2.f, kr * 2.f);

        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4641), c.x - kr, c.y - kr,
                                                 juce::Colour (0xff0c0b0a), c.x + kr, c.y + kr, false));
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2.f, kr * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.18f));
        g.drawEllipse (c.x - kr + 0.5f, c.y - kr + 0.5f, kr * 2.f - 1.f, kr * 2.f - 1.f, 1.f);

        // Kappe mit feiner Riffelung
        const float cr = kr * 0.80f;
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3d3a36), c.x - cr, c.y - cr,
                                                 juce::Colour (0xff171615), c.x + cr, c.y + cr, false));
        g.fillEllipse (c.x - cr, c.y - cr, cr * 2.f, cr * 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.05f));
        for (float f = 0.25f; f < 1.f; f += 0.15f)
            g.drawEllipse (c.x - cr * f, c.y - cr * f, cr * f * 2.f, cr * f * 2.f, 0.8f);

        // Zeiger
        const auto d = dir (ang);
        g.setColour (juce::Colour (0xfff6efdc));
        g.drawLine (juce::Line<float> (c + d * (cr * 0.12f), c + d * (cr * 0.97f)), 2.6f);

        // MIDI-Learn-Anzeige
        if (ps != nullptr && ps->shownLearn)
        {
            g.setColour (col::orange);
            g.drawEllipse (c.x - kr - 3.f, c.y - kr - 3.f, kr * 2.f + 6.f, kr * 2.f + 6.f, 2.5f);
        }
        else if (ps != nullptr && ps->shownCc >= 0)
        {
            g.setColour (col::amber.withAlpha (0.9f));
            g.setFont (labelFont (8.5f));
            g.drawText ("CC" + juce::String (ps->shownCc), x + w - 26, y + h - 11, 26, 11, juce::Justification::centredRight, false);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int w, int h, float pos, float, float,
                           juce::Slider::SliderStyle, juce::Slider& s) override
    {
        auto* ps = dynamic_cast<ParamSlider*> (&s);
        const float cx = (float) x + (float) w * 0.5f;
        const float top = (float) y + 9.f, bottom = (float) (y + h) - 9.f;

        // Teilung links/rechts
        g.setColour (col::cream.withAlpha (0.7f));
        for (int i = 0; i <= 10; ++i)
        {
            const float ty = bottom - (bottom - top) * (float) i / 10.f;
            const float len = (i % 5 == 0) ? 5.f : 3.f;
            g.drawLine (cx - 9.f - len, ty, cx - 9.f, ty, 1.f);
            g.drawLine (cx + 9.f, ty, cx + 9.f + len, ty, 1.f);
        }

        // Führungsschlitz
        const juce::Rectangle<float> slot (cx - 2.5f, top - 4.f, 5.f, bottom - top + 8.f);
        g.setColour (juce::Colours::black);
        g.fillRoundedRectangle (slot, 2.f);
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (slot.expanded (0.5f), 2.f, 1.f);

        // Kappe
        const bool accent = (bool) s.getProperties().getWithDefault ("accent", false);
        const juce::Rectangle<float> cap (cx - 12.f, pos - 9.f, 24.f, 18.f);
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillRoundedRectangle (cap.translated (1.5f, 2.f), 3.f);

        const auto top1 = accent ? juce::Colour (0xfff08a45) : juce::Colour (0xff8d8982);
        const auto bot1 = accent ? juce::Colour (0xffa84a14) : juce::Colour (0xff3a3835);
        g.setGradientFill (juce::ColourGradient (top1, cap.getX(), cap.getY(), bot1, cap.getX(), cap.getBottom(), false));
        g.fillRoundedRectangle (cap, 3.f);
        g.setColour (juce::Colours::white.withAlpha (0.25f));
        g.drawRoundedRectangle (cap.reduced (0.5f), 3.f, 1.f);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.fillRect (cap.getX() + 3.f, pos - 1.f, cap.getWidth() - 6.f, 2.f);

        if (ps != nullptr && ps->shownLearn)
        {
            g.setColour (col::orange);
            g.drawRoundedRectangle (cap.expanded (3.f), 4.f, 2.f);
        }
        else if (ps != nullptr && ps->shownCc >= 0)
        {
            g.setColour (col::amber.withAlpha (0.9f));
            g.setFont (labelFont (8.f));
            g.drawText ("CC" + juce::String (ps->shownCc), x - 4, y - 1, w + 8, 10, juce::Justification::centred, false);
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
        g.setColour (col::cream);
        g.setFont (VintageLook::labelFont (lh > 14 ? 11.f : 9.5f));
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
        g.setColour (col::cream);
        g.setFont (VintageLook::labelFont (10.5f));
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
            g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff55504a) : juce::Colour (0xff3d3a36), r.getX(), r.getY(),
                                                     juce::Colour (0xff1c1b19), r.getX(), r.getBottom(), false));
            g.fillRoundedRectangle (r, 3.f);
            g.setColour (juce::Colours::white.withAlpha (on ? 0.25f : 0.1f));
            g.drawRoundedRectangle (r.reduced (0.5f), 3.f, 1.f);

            // LED
            const auto led = juce::Rectangle<float> (r.getX() + 7.f, r.getCentreY() - 3.5f, 7.f, 7.f);
            if (on)
            {
                g.setColour (col::orange.withAlpha (0.35f));
                g.fillEllipse (led.expanded (3.f));
            }
            g.setColour (on ? col::orange : juce::Colour (0xff3b2216));
            g.fillEllipse (led);

            g.setColour (on ? col::cream : col::creamDim);
            g.setFont (VintageLook::labelFont (10.5f));
            g.drawText (items[i].toUpperCase(), (int) r.getX() + 18, (int) r.getY(), (int) r.getWidth() - 20, (int) r.getHeight(),
                        juce::Justification::centredLeft, false);
        }
        g.setColour (col::cream);
        g.setFont (VintageLook::labelFont (11.f));
        g.drawText (label, 0, items.size() * rowH + 1, getWidth(), 16, juce::Justification::centred, false);

        if (shownLearn)
        {
            g.setColour (col::orange);
            g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (1.f), 4.f, 2.f);
        }
        else if (shownCc >= 0)
        {
            g.setColour (col::amber.withAlpha (0.9f));
            g.setFont (VintageLook::labelFont (8.5f));
            g.drawText ("CC" + juce::String (shownCc), getWidth() - 26, items.size() * rowH + 3, 26, 11, juce::Justification::centredRight, false);
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
        g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff55504a) : juce::Colour (0xff3d3a36), r.getX(), r.getY(),
                                                 juce::Colour (0xff1c1b19), r.getX(), r.getBottom(), false));
        g.fillRoundedRectangle (r, 3.f);
        g.setColour (juce::Colours::white.withAlpha (on ? 0.25f : 0.1f));
        g.drawRoundedRectangle (r.reduced (0.5f), 3.f, 1.f);

        const auto led = juce::Rectangle<float> (r.getCentreX() - 4.f, r.getCentreY() - 4.f, 8.f, 8.f);
        if (on) { g.setColour (col::orange.withAlpha (0.35f)); g.fillEllipse (led.expanded (3.f)); }
        g.setColour (on ? col::orange : juce::Colour (0xff3b2216));
        g.fillEllipse (led);

        g.setColour (col::cream);
        g.setFont (VintageLook::labelFont (9.5f));
        g.drawText (label, 0, getHeight() - 16, getWidth(), 16, juce::Justification::centred, false);

        if (shownLearn)
        {
            g.setColour (col::orange);
            g.drawRoundedRectangle (r.expanded (2.f), 4.f, 2.f);
        }
        else if (shownCc >= 0)
        {
            g.setColour (col::amber.withAlpha (0.9f));
            g.setFont (VintageLook::labelFont (8.5f));
            g.drawText ("CC" + juce::String (shownCc), getWidth() - 26, 36, 26, 11, juce::Justification::centredRight, false);
        }
    }

private:
    juce::String label;
    bool on = false;
    juce::ParameterAttachment attach;
};

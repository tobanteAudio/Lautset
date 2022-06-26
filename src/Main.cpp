#include <memory>

#include "MainComponent.hpp"

namespace ta
{

struct LautsetApplication final : juce::JUCEApplication
{
    LautsetApplication() = default;

    auto getApplicationName() -> const juce::String override { return JUCE_APPLICATION_NAME_STRING; }
    auto getApplicationVersion() -> const juce::String override { return JUCE_APPLICATION_VERSION_STRING; }
    auto moreThanOneInstanceAllowed() -> bool override { return true; }

    void initialise(juce::String const& /*arg*/) override { _win = std::make_unique<MainWindow>(getApplicationName()); }
    void shutdown() override { _win.reset(nullptr); }
    void systemRequestedQuit() override { quit(); }
    void anotherInstanceStarted(juce::String const& /*arg*/) override {}

    struct MainWindow final : juce::DocumentWindow
    {

        explicit MainWindow(juce::String const& name)
            : DocumentWindow(
                name,
                juce::Desktop::getInstance().getDefaultLookAndFeel().findColour(ResizableWindow::backgroundColourId),
                DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setContentOwned(std::make_unique<MainComponent>().release(), true);

            setResizable(true, true);
            centreWithSize(getWidth(), getHeight());

            setVisible(true);
        }

        void closeButtonPressed() override { JUCEApplication::getInstance()->systemRequestedQuit(); }
    };

private:
    std::unique_ptr<MainWindow> _win;
};

}  // namespace ta

START_JUCE_APPLICATION(ta::LautsetApplication)  // NOLINT

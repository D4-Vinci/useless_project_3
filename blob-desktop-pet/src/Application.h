#pragma once
#include "DesktopWindow.h"
#include "ScreenInfo.h"
#include "MovementController.h"
#include "EmotionSystem.h"
#include "Blob.h"
#include "BehaviorController.h"
#include "CommandInterface.h"
#include <memory>

class Application {
public:
    // Returns the process exit code.
    int Run(int argc, char** argv);

private:
    int RunAsClientCommand(const std::string& command);
    int RunServer();
    void MainLoop();
    void InstallSignalHandlers();

    std::unique_ptr<DesktopWindow> window_;
    std::unique_ptr<ScreenInfo> screen_;
    std::unique_ptr<MovementController> movement_;
    std::unique_ptr<EmotionSystem> emotion_;
    std::unique_ptr<Blob> blob_;
    std::unique_ptr<BehaviorController> behavior_;
    CommandInterface commandInterface_;

    volatile bool running_ = true;
};

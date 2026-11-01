// All state below is owned by the UI thread; network jobs return data only.
ClientScanner scanner;
std::atomic<bool> probeCancel{false};
std::future<ProbeResult> probeTask;
RemoteClientConfig pendingClient;
bool connectionOpen = false, scanOpen = false, canScreen = false;
int connectionStage = 0, connectionSelection = 0, scanSelection = 0;
void beginConnect(RemoteClientConfig) {
    status = "Outgoing remote control is unavailable on Prospero Hi-Fi.";
}

void closeConnection() {
    connectionOpen = false;
    probeCancel = true;
}
void activateRemote(bool screen) {
    connectionOpen = false;
    disconnectTarget();
    targetView = std::make_unique<RemoteView>(
        ui, renderer, pendingClient,
        [](const auto &label, auto &value, bool secret) { return keyboard(label, value, secret); },
        settings.remoteClients, screen);
    cooldown = SDL_GetTicks64() + 120;
}
void connectionAction() {
    if (connectionStage == 0) {
        closeConnection();
        return;
    }
    if (connectionStage == 1) {
        if (connectionSelection == 0)
            beginConnect(pendingClient);
        else
            closeConnection();
        return;
    }
    if (connectionSelection == 0)
        activateRemote(false);
    else if (connectionSelection == 1 && canScreen)
        activateRemote(true);
    else if (connectionSelection == 2)
        closeConnection();
}
void startScan() {}

void scanAction() {
    auto found = scanner.results();
    if (scanSelection < int(found.size())) {
        auto c = found[scanSelection].client;
        choose(c.name + " · " + c.host, {0, 1, 2}, {"Connect", "Save / edit client", "Cancel"}, 0,
               [this, c](auto v) {
                   if (v == 0)
                       beginConnect(c);
                   if (v == 1) {
                       scanOpen = false;
                       page = 4;
                       int index = -1;
                       for (int i = 0; i < int(settings.remoteClients.size()); ++i)
                           if (settings.remoteClients[i].host == c.host) {
                               index = i;
                               break;
                           }
                       beginClientEdit(index);
                       editClient = c;
                   }
               });
    } else if (scanSelection == int(found.size())) {
        if (!scanner.running())
            scanner.start(localSubnetHosts());
    } else {
        scanner.cancel();
        scanOpen = false;
    }
}
void updateConnections() {
    if (probeTask.valid() &&
        probeTask.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
        auto result = probeTask.get();
        if (connectionOpen) {
            if (!result.online) {
                connectionStage = 1;
                connectionSelection = 0;
            } else {
                pendingClient = result.client;
                canScreen = result.screen;
                if (pendingClient.type == "mister")
                    activateRemote(false);
                else {
                    connectionStage = 2;
                    connectionSelection = 0;
                }
            }
        }
    }
}
void drawConnection() {
    ui.begin();
    header();
    ui.text(80, 125, pendingClient.name.empty() ? pendingClient.host : pendingClient.name, 28,
            ACCENT, 1100);
    ui.text(80, 174, pendingClient.host + ":8182", 20, MUTED);
    if (connectionStage == 0) {
        ui.text(80, 240, "Checking remote client...", 26, TEXT);
        ui.button(80, 330, 1120, 54, "Back", true);
    }
    if (connectionStage == 1) {
        ui.text(80, 240, "Remote client is offline", 30, TEXT);
        ui.text(80, 292, "Check that Hi-Fi and its web remote are running.", 20, MUTED, 1100);
        ui.button(80, 360, 1120, 54, "Retry", connectionSelection == 0);
        ui.button(80, 430, 1120, 54, "Back", connectionSelection == 1);
    }
    if (connectionStage == 2) {
        ui.text(80, 235, "Choose remote mode", 28, TEXT);
        ui.button(80, 310, 1120, 58, "Independent control", connectionSelection == 0);
        ui.text(98, 375, "Browse and control music separately from the target's screen.", 19, MUTED,
                1050);
        ui.button(80, 420, 1120, 58,
                  canScreen ? "Screen control" : "Screen control · requires NX Hi-Fi v0.9.0",
                  connectionSelection == 1);
        ui.text(98, 485, "View and operate the target's interface.", 19, MUTED, 1050);
        ui.button(80, 535, 1120, 54, "Cancel", connectionSelection == 2);
    }
    ui.text(80, layout().bottom(665), "Up/Down Select   A Confirm   B Back", 17, MUTED, 1100);
    finishFrame();
}
void drawScan() {
    ui.begin();
    header();
    auto found = scanner.results();
    ui.text(46, 110, "SCAN FOR REMOTE CLIENTS", 25, ACCENT);
    auto message = scanner.running() ? "Scanning " + std::to_string(scanner.completed()) + " / " +
                                           std::to_string(scanner.total())
                   : found.empty()   ? "No clients found. Keep Hi-Fi and its web remote open."
                                     : std::to_string(found.size()) + " clients found";
    ui.text(46, 155, message, 18, MUTED, 1180);
    int first = std::max(0, scanSelection - layout().rows() + 1);
    for (int row = 0; row < layout().rows(); ++row) {
        int index = first + row;
        if (index >= int(found.size()))
            break;
        auto &c = found[index].client;
        bool saved = false;
        for (auto &s : settings.remoteClients)
            if (s.host == c.host)
                saved = true;
        ui.button(46, 198 + row * 44, 1180, 40,
                  c.name + " · " + c.host + (c.type == "nx" ? " · NX" : " · MiSTer") +
                      (saved ? " · Saved" : ""),
                  scanSelection == index);
    }
    ui.button(46, layout().bottom(572), 570, 48, scanner.running() ? "Scanning..." : "Scan again",
              scanSelection == int(found.size()));
    ui.button(636, layout().bottom(572), 590, 48, "Cancel / Back",
              scanSelection == int(found.size()) + 1);
    ui.text(46, layout().bottom(660), "A Connect / Save   B Back   Up/Down Select   Port 8182", 17,
            MUTED, 1170);
    drawOptions();
    finishFrame();
}
bool dialogInput(u64 down, u64 held, bool injected) {
    if (!connectionOpen && !scanOpen)
        return false;
    bool up = held & (HidNpadButton_Up | HidNpadButton_StickLUp),
         dn = held & (HidNpadButton_Down | HidNpadButton_StickLDown);
    int delta = 0;
    int direction=up?-1:dn?1:0;
    if(verticalRepeat.fire(direction,down&(HidNpadButton_Up|HidNpadButton_Down|HidNpadButton_StickLUp|HidNpadButton_StickLDown),SDL_GetTicks64()))delta=direction;
    if (optionApply) {
        if (delta)
            move(delta);
        if (down & HidNpadButton_A)
            action();
        if (down & HidNpadButton_B)
            optionApply = nullptr;
    } else if (connectionOpen) {
        int count = connectionStage == 0 ? 1 : connectionStage == 1 ? 2 : 3;
        connectionSelection = (connectionSelection + delta + count) % count;
        if (down & HidNpadButton_A)
            connectionAction();
        if (down & HidNpadButton_B)
            closeConnection();
    } else {
        int count = scanner.results().size() + 2;
        scanSelection = (scanSelection + delta + count) % count;
        if (down & HidNpadButton_A)
            scanAction();
        if (down & HidNpadButton_B) {
            scanner.cancel();
            scanOpen = false;
        }
    }
    if (!injected) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                running = false;
            if (e.type == SDL_FINGERUP)
                dialogClick(e.tfinger.x * 1280, e.tfinger.y * layout().height());
            if (e.type == SDL_MOUSEBUTTONDOWN && e.button.which != SDL_TOUCH_MOUSEID)
                dialogClick(ui.pointerX(e.button.x), e.button.y);
        }
    }
    return true;
}
void dialogClick(int x, int y) {
    if (optionApply) {
        int first = layout().optionFirst(optionIndex);
        if (inside(x, y, 170, 170, 940, layout().optionRows() * 48)) {
            int i = first + (y - 170) / 48;
            if (i < int(optionValues.size())) {
                optionIndex = i;
                action();
            }
        }
        return;
    }
    if (connectionOpen) {
        if (connectionStage == 0 && inside(x, y, 80, 330, 1120, 54))
            closeConnection();
        if (connectionStage == 1)
            for (int i = 0; i < 2; ++i)
                if (inside(x, y, 80, 360 + i * 70, 1120, 54)) {
                    connectionSelection = i;
                    connectionAction();
                    return;
                }
        if (connectionStage == 2) {
            int ys[] = {310, 420, 535};
            for (int i = 0; i < 3; ++i)
                if (inside(x, y, 80, ys[i], 1120, 58)) {
                    connectionSelection = i;
                    connectionAction();
                    return;
                }
        }
    } else if (scanOpen) {
        int first = std::max(0, scanSelection - layout().rows() + 1), n = scanner.results().size();
        if (inside(x, y, 46, 198, 1180, layout().rows() * 44)) {
            int i = first + (y - 198) / 44;
            if (i < n) {
                scanSelection = i;
                scanAction();
                return;
            }
        }
        if (inside(x, y, 46, layout().bottom(572), 570, 48)) {
            scanSelection = n;
            scanAction();
        }
        if (inside(x, y, 636, layout().bottom(572), 590, 48)) {
            scanSelection = n + 1;
            scanAction();
        }
    }
}
void finishFrame(bool present = true) {
    if (present) {
        auto start=SDL_GetPerformanceCounter();
        if(SDL_RenderFlush(renderer)!=0)frameDiagnostics.renderError=SDL_GetError();
        auto flushed=SDL_GetPerformanceCounter();
        SDL_RenderPresent(renderer);
        frameDiagnostics.flushTicks=flushed-start;
        frameDiagnostics.presentTicks=SDL_GetPerformanceCounter()-flushed;
    }
}

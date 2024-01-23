import Cocoa
import GUIBridge

class AppDelegate: NSObject, NSApplicationDelegate {
    let connectionsSubMenu = NSMenu()
    let noConnections = NSMenuItem(title: "No Connections", action: nil, keyEquivalent: "")

    let serversSubMenu = NSMenu()
    let noServers = NSMenuItem(title: "No Servers", action: nil, keyEquivalent: "")

    @objc private func windowItemClicked(_ sender: NSMenuItem) {
        GUIBridge.setWindowFocus(std.string(sender.representedObject as! String))
    }

    @objc private func openSettingsWindow() {
        GUIBridge.openSettingsWindow()
    }

    @objc private func openNewConnectionWindow() {
        GUIBridge.openNewConnectionWindow()
    }

    @objc private func openNewServerWindow() {
        GUIBridge.openNewServerWindow()
    }

    @objc private func openNotificationsWindow() {
        GUIBridge.openNotificationsWindow()
    }

    @objc private func openAboutWindow() {
        GUIBridge.openAboutWindow()
    }

    @objc private func showChangelog() {
        let url = URL(string: "https://www.github.com/NSTerminal/terminal/blob/main/docs/changelog.md")!
        NSWorkspace.shared.open(url)
    }

    private func buildViewMenu() -> NSMenuItem {
        let subMenu = NSMenu()
        subMenu.addItem(withTitle: "New Connection", action: #selector(openNewConnectionWindow), keyEquivalent: "")
        subMenu.addItem(withTitle: "New Server", action: #selector(openNewServerWindow), keyEquivalent: "")
        subMenu.addItem(withTitle: "Notifications", action: #selector(openNotificationsWindow), keyEquivalent: "")

        let viewMenu = NSMenuItem(title: "View", action: nil, keyEquivalent: "")
        viewMenu.submenu = subMenu
        return viewMenu
    }

    private func buildConnectionsMenu() -> NSMenuItem {
        noConnections.isEnabled = false
        connectionsSubMenu.addItem(noConnections)

        let connectionsMenu = NSMenuItem(title: "Connections", action: nil, keyEquivalent: "")
        connectionsMenu.submenu = connectionsSubMenu
        return connectionsMenu
    }

    private func buildServersMenu() -> NSMenuItem {
        noServers.isEnabled = false
        serversSubMenu.addItem(noServers)

        let serversMenu = NSMenuItem(title: "Servers", action: nil, keyEquivalent: "")
        serversMenu.submenu = serversSubMenu
        return serversMenu
    }

    private func buildHelpMenu() -> NSMenuItem {
        let subMenu = NSMenu()
        subMenu.addItem(withTitle: "Show Changelog", action: #selector(showChangelog), keyEquivalent: "")

        NSApp.helpMenu = subMenu
        let helpMenu = NSMenuItem(title: "Help", action: nil, keyEquivalent: "")
        helpMenu.submenu = NSApp.helpMenu
        return helpMenu
    }

    private func addMenuItem(name: String, subMenu: NSMenu, emptyLabel: NSMenuItem) {
        if subMenu.index(of: emptyLabel) != -1 {
            subMenu.removeItem(emptyLabel)
        }

        let newItem = NSMenuItem(
            title: name.components(separatedBy: "##")[0], // Segment of title before double hash
            action: #selector(windowItemClicked),
            keyEquivalent: ""
        )

        newItem.representedObject = name
        subMenu.addItem(newItem)
    }

    private func removeMenuItem(name: String, subMenu: NSMenu, emptyLabel: NSMenuItem) {
        let idx = subMenu.indexOfItem(withRepresentedObject: name)
        if idx != -1 {
            subMenu.removeItem(at: idx)
        }

        if subMenu.numberOfItems == 0 {
            subMenu.addItem(emptyLabel)
        }
    }

    func addWindowMenuItem(name: String) {
        addMenuItem(name: name, subMenu: connectionsSubMenu, emptyLabel: noConnections)
    }

    func removeWindowMenuItem(name: String) {
        removeMenuItem(name: name, subMenu: connectionsSubMenu, emptyLabel: noConnections)
    }

    func addServerMenuItem(name: String) {
        addMenuItem(name: name, subMenu: serversSubMenu, emptyLabel: noServers)
    }

    func removeServerMenuItem(name: String) {
        removeMenuItem(name: name, subMenu: serversSubMenu, emptyLabel: noServers)
    }

    func applicationDidFinishLaunching(_: Notification) {
        NSApp.mainMenu?.insertItem(buildViewMenu(), at: 1)
        NSApp.mainMenu?.insertItem(buildConnectionsMenu(), at: 2)
        NSApp.mainMenu?.insertItem(buildServersMenu(), at: 3)
        NSApp.mainMenu?.insertItem(buildHelpMenu(), at: 5) // After "Windows" at index 4

        // Build settings menu
        let appMenu = NSApp.mainMenu!.item(at: 0)!.submenu!
        let settingsMenu = appMenu.item(at: 2)!
        settingsMenu.isEnabled = true
        settingsMenu.action = #selector(openSettingsWindow)

        // "About" menu item
        appMenu.item(at: 0)!.action = #selector(openAboutWindow)
    }
}

let delegate = AppDelegate()

public func setupMenuBar() {
    NSApplication.shared.delegate = delegate
}

public func addWindowMenuItem(name: String) {
    delegate.addWindowMenuItem(name: name)
}

public func removeWindowMenuItem(name: String) {
    delegate.removeWindowMenuItem(name: name)
}

public func addServerMenuItem(name: String) {
    delegate.addServerMenuItem(name: name)
}

public func removeServerMenuItem(name: String) {
    delegate.removeServerMenuItem(name: name)
}

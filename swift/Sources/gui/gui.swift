import Cocoa
import Menu

class MenuHandler {
    let connectionsSubMenu = NSMenu()
    let noConnections = NSMenuItem(title: "No Connections", action: nil, keyEquivalent: "")

    let serversSubMenu = NSMenu()
    let noServers = NSMenuItem(title: "No Servers", action: nil, keyEquivalent: "")

    @objc private func windowItemClicked(_ sender: NSMenuItem) {
        Menu.setWindowFocus(sender.representedObject as! String)
    }

    @objc private func openSettingsWindow() {
        Menu.settingsOpen = true
    }

    @objc private func openNewConnectionWindow() {
        Menu.newConnectionOpen = true
    }

    @objc private func openNewServerWindow() {
        Menu.newServerOpen = true
    }

    @objc private func openNotificationsWindow() {
        Menu.notificationsOpen = true
    }

    @objc private func openAboutWindow() {
        Menu.aboutOpen = true
    }

    @objc private func openLinksWindow() {
        Menu.linksOpen = true
    }

    private func addItem(menu: NSMenu, title: String, action: Selector) {
        let item = menu.addItem(withTitle: title, action: action, keyEquivalent: "")
        item.target = self
    }

    private func buildViewMenu() -> NSMenuItem {
        let subMenu = NSMenu()
        addItem(menu: subMenu, title: "New Connection", action: #selector(openNewConnectionWindow))
        addItem(menu: subMenu, title: "New Server", action: #selector(openNewServerWindow))
        addItem(menu: subMenu, title: "Notifications", action: #selector(openNotificationsWindow))

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
        addItem(menu: subMenu, title: "Links", action: #selector(openLinksWindow))

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
        newItem.target = self
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

    func buildMenus() {
        NSApp.mainMenu?.insertItem(buildViewMenu(), at: 1)
        NSApp.mainMenu?.insertItem(buildConnectionsMenu(), at: 2)
        NSApp.mainMenu?.insertItem(buildServersMenu(), at: 3)
        NSApp.mainMenu?.insertItem(buildHelpMenu(), at: 5) // After "Windows" at index 4

        // Build settings menu
        let appMenu = NSApp.mainMenu!.item(at: 0)!.submenu!

        let settingsItem = NSMenuItem(title: "Settings...", action: #selector(openSettingsWindow), keyEquivalent: ",")
        settingsItem.target = self
        appMenu.insertItem(settingsItem, at: 1)

        // "About" menu item
        let aboutItem = appMenu.item(at: 0)!
        aboutItem.action = #selector(openAboutWindow)
        aboutItem.target = self
    }
}

let menu = MenuHandler()

public func setupMenuBar() {
    menu.buildMenus()
}

public func addWindowMenuItem(name: String) {
    menu.addWindowMenuItem(name: name)
}

public func removeWindowMenuItem(name: String) {
    menu.removeWindowMenuItem(name: name)
}

public func addServerMenuItem(name: String) {
    menu.addServerMenuItem(name: name)
}

public func removeServerMenuItem(name: String) {
    menu.removeServerMenuItem(name: name)
}

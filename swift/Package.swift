// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "terminal",
    products: [
        .library(name: "BluetoothMacOS", type: .static, targets: ["BluetoothMacOS"]),
        .library(name: "GUIMacOS", type: .static, targets: ["GUIMacOS"]),
    ],
    targets: [
        .target(
            name: "BluetoothMacOS",
            dependencies: [],
            path: "./Sources",
            exclude: ["gui.swift"],
            sources: ["bthandle.swift", "btutils.swift"],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .unsafeFlags(["-I", "./bridge"]),
            ]
        ),
        .target(
            name: "GUIMacOS",
            dependencies: [],
            path: "./Sources",
            exclude: ["bthandle.swift", "btutils.swift"],
            sources: ["gui.swift"],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .unsafeFlags(["-I", "./bridge"]),
            ]
        ),
    ]
)

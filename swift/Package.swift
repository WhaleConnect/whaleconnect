// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "WhaleConnect",
    platforms: [
        .macOS(.v14)
    ],
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
                .unsafeFlags(["-I", "../src", "-Xcc", "-std=c++2b", "-Xcc", "-DNO_EXPOSE_INTERNAL"]),
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
                .unsafeFlags(["-I", "../src", "-Xcc", "-std=c++2b"]),
            ]
        ),
    ]
)

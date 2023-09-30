// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "BluetoothMacOS",
    products: [
        .library(name: "BluetoothMacOS", type: .static, targets: ["BluetoothMacOS"]),
    ],
    targets: [
        .target(
            name: "BluetoothMacOS",
            dependencies: [],
            path: "./Sources",
            sources: ["bthandle.swift", "btutils.swift"],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .unsafeFlags(["-I", "./bridge"]),
            ]
        ),
    ]
)

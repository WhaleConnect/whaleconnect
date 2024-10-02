// swift-tools-version: 5.9
import PackageDescription

let package = Package(
    name: "WhaleConnect",
    platforms: [
        .macOS(.v14),
    ],
    products: [
        .library(name: "BluetoothMacOS", type: .static, targets: ["BluetoothMacOS"]),
        .library(name: "GUIMacOS", type: .static, targets: ["GUIMacOS"]),
    ],
    targets: [
        .target(
            name: "BluetoothMacOS",
            dependencies: [],
            path: "./Sources/bluetooth",
            cxxSettings: [
                .define("NO_EXPOSE_INTERNAL"),
            ],
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .unsafeFlags(["-I", "../src"]),
            ]
        ),
        .target(
            name: "GUIMacOS",
            dependencies: [],
            path: "./Sources/gui",
            swiftSettings: [
                .interoperabilityMode(.Cxx),
                .unsafeFlags(["-I", "../src"]),
            ]
        ),
    ],
    cxxLanguageStandard: .cxx2b
)

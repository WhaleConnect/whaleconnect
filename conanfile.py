# Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain

class App(ConanFile):
    settings = ("os", "arch", "compiler", "build_type")

    def requirements(self):
        # Common dependencies
        self.requires("imgui/cci.20220621+1.88.docking")
        self.requires("magic_enum/0.8.1")
        self.requires("sdl/2.24.1")

        # Dependencies on Linux
        if self.settings.os == "Linux":
            self.requires("liburing/2.2")

        # Backports of modern Standard Library features for certain vendors
        if self.settings.compiler != "Visual Studio":
            self.requires("out_ptr/cci.20211119")

    def imports(self):
        # Copy the Dear ImGui backend files into the project
        self.copy("imgui_impl_sdl.*", dst="bindings", src="res/bindings")
        self.copy("imgui_impl_opengl3*", dst="bindings", src="res/bindings")

    def layout(self):
        self.folders.build = "build"
        self.folders.generators = "build"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.generator = "Ninja"
        tc.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

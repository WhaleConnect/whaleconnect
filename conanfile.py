# Copyright 2021-2022 Aidan Sun and the Network Socket Terminal contributors
# SPDX-License-Identifier: GPL-3.0-or-later

from conan import ConanFile
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain
from conan.tools.files import save

# CMakeSettings.json file for SonarLint analysis (it doesn't support Presets yet)
cmake_settings_file = r"""{
  "configurations": [
    {
      "name": "'debug' config",
      "buildRoot": "${projectDir}\\build"
    },
    {
      "name": "'release' config",
      "buildRoot": "${projectDir}\\build"
    }
  ]
}"""

class App(ConanFile):
    settings = ("os", "arch", "compiler", "build_type")

    def requirements(self):
        # Common dependencies
        self.requires("fmt/9.1.0")
        self.requires("glfw/3.3.8")
        self.requires("imgui/cci.20220621+1.88.docking")
        self.requires("magic_enum/0.8.1")

        # Dependencies on Linux
        if self.settings.os == "Linux":
            self.requires("liburing/2.2")

        # Backports of modern Standard Library features for certain vendors
        if self.settings.compiler != "Visual Studio":
            self.requires("out_ptr/cci.20211119")

    def imports(self):
        # Copy the Dear ImGui backend files into the project
        self.copy("imgui_impl_glfw*", dst="bindings", src="res/bindings")
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

        # Generate CMake Settings
        save(self, "../CMakeSettings.json", cmake_settings_file)

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

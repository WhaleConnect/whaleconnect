# Network Socket Terminal Contributing Guidelines

Thank you for contributing to Network Socket Terminal. Please review these contribution guidelines and be mindful of the [code of conduct](https://github.com/NSTerminal/.github/blob/main/CODE_OF_CONDUCT.md) in your contributions.

## Opening Issues

We encourage using GitHub Issues as a centralized place for community engagement. Feel free to submit bug reports, feature requests, or questions, but please follow these steps before doing so:

- Ensure your issue has not already been submitted.
  - You can search by [issue label](https://github.com/NSTerminal/terminal/labels) to effectively find related information.
  - Be sure to include closed issues in your search in case your issue has already been resolved.
- Ensure you have filled out the correct issue template.
- Provide a clear description of your issue in the title.

### Bug Reports

- If you have found a security concern, **please do not submit it in GitHub Issues.** Instead, refer to the project's [security policy](https://github.com/NSTerminal/.github/blob/main/SECURITY.md).
- Make sure your environment is officially supported ([System Requirements](readme.md#minimum-hardware-requirements)).
  - Currently supported OS+architecture combinations are: Windows+x64, Linux+x64, macOS+ARM64.
- Check if the bug still exists on the main branch.
- Provide a clear, reproducible example with sufficient detail. This includes all steps necessary to reproduce the bug and any environment/settings configuration that might be related to the behavior.
- Include relevant screenshots, video captures, or data output to explain the bug.

### Feature Requests

- Clearly explain the proposed feature and how it can be useful.
- Include user interface designs or other mockups if you think they will help in your explanation.
- If you are willing to, feel free to write your own code, open a pull request, and mention it in your issue. We always welcome external contributions.

### Questions

- Clearly explain what you want to accomplish using the software and if it is suitable for your desired task.
  - Be mindful of the [XY Problem](https://xyproblem.info/) and make sure you describe your *original* goals and expectations. This will help us answer your question more easily.
  - Include anything you have already tried and any errors or failures you have encountered.

## Opening Pull Requests

We accept code contributions through GitHub Pull Requests. Please follow these guidelines in your contribution:

- Create a separate branch for your pull request so it can be modified later, if necessary.
- If you are adding a new feature, include a screen recording or screenshot to quickly demonstrate it.
- Follow the project's coding style:
  - Variables and functions are in `camelCase`.
  - Classes, structs, namespaces, and type-aliases are in `PascalCase`.
  - This project uses modern C++ features and idioms: C++-style casts, RAII, coroutines, etc.
  - The code should compile cleanly with no warnings (other than those explicitly silenced in `xmake.lua`).
- Ensure your patch passes the [test cases](testing.md).

Network Socket Terminal uses the following tools for formatting:

- **EditorConfig:** Basic file format standards (UTF-8, indentation character, trailing newline, etc.)
- **Clang Format:** Formatting for C++ code.
- **SwiftFormat:** Formatting for Swift code.

GitHub Actions is set up to check the formatting of C++ files using Clang Format.

By submitting a code contribution to Network Socket Terminal, you agree to have your contribution become part of the repository and be distributed under the project's [license](../COPYING).

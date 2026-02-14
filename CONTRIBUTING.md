# Contributing to M5Stack BLE Scanner

First off, thank you for considering contributing to this project! 

## How Can I Contribute?

### Reporting Bugs

Before creating bug reports, please check existing issues. When you create a bug report, include as many details as possible:

- **Use a clear and descriptive title**
- **Describe the exact steps to reproduce the problem**
- **Provide specific examples**
- **Include serial monitor output** if relevant
- **Describe the behavior you observed** and what you expected
- **Include screenshots** if applicable
- **Specify your hardware**: M5Stack Atom S3R version, power supply, etc.
- **Specify your environment**: Arduino IDE version, library versions, OS

### Suggesting Enhancements

Enhancement suggestions are tracked as GitHub issues. When creating an enhancement suggestion:

- **Use a clear and descriptive title**
- **Provide a detailed description** of the suggested enhancement
- **Explain why this enhancement would be useful**
- **List some examples** of how it would be used
- **Include mockups or examples** if applicable

### Pull Requests

1. **Fork the repository** and create your branch from `main`
2. **Make your changes** following the code style guidelines below
3. **Test your changes** thoroughly
4. **Update documentation** if needed
5. **Write clear commit messages**
6. **Submit a pull request**

#### Code Style Guidelines

- Use **2 spaces** for indentation (Arduino style)
- Comment complex logic
- Use descriptive variable names
- Follow existing naming conventions:
  - `camelCase` for variables and functions
  - `UPPERCASE` for constants and defines
  - `PascalCase` for class names
- Keep functions focused and small when possible
- Add error handling for new features

#### Example Good Commit Message

```
Add support for Eddystone-EID beacons

- Parse EID frame format
- Add ephemeral identifier extraction
- Update documentation with EID examples

Closes #42
```

## Development Setup

1. **Clone your fork:**
   ```bash
   git clone https://github.com/maciej-wierzbowski/m5stack-ble-scanner.git
   cd m5stack-ble-scanner
   ```

2. **Copy config:**
   ```bash
   cp config.h.example config.h
   # Edit config.h with your settings
   ```

3. **Install dependencies** via Arduino IDE Library Manager:
   - M5Unified
   - ArduinoJson (v7+)
   - NimBLE-Arduino

4. **Test your changes:**
   - Compile and upload to hardware
   - Monitor serial output
   - Verify all features work
   - Check for memory issues

## Testing

Please test your contributions on actual hardware. Include test results in your PR:

- Compile status (any warnings?)
- Upload status
- Runtime behavior
- Memory usage (check serial output)
- Any edge cases tested

## Documentation

- Update README.md if you change functionality
- Add code comments for complex logic
- Update config.h.example if you add new configuration options
- Consider updating CODE_IMPROVEMENTS.md if you fix listed issues

## Areas Needing Help

We'd love contributions in these areas:

- **Testing:** More testing on different ESP32-S3 variants
- **Documentation:** Screenshots, diagrams, tutorials
- **Server implementations:** Node.js, Go, Rust examples
- **Data visualization:** Web dashboards, analytics
- **Code improvements:** See CODE_IMPROVEMENTS.md
- **Beacon support:** More beacon types (Ruuvi, etc.)
- **Battery optimization:** Power-saving features
- **Security:** Authentication, encryption

## Questions?

Feel free to open an issue with the `question` label, or start a discussion in GitHub Discussions.

## Code of Conduct

### Our Pledge

We are committed to providing a welcoming and inspiring community for all.

### Our Standards

Examples of behavior that contributes to a positive environment:

- Using welcoming and inclusive language
- Being respectful of differing viewpoints
- Gracefully accepting constructive criticism
- Focusing on what is best for the community
- Showing empathy towards other community members

Examples of unacceptable behavior:

- Trolling, insulting/derogatory comments, personal or political attacks
- Public or private harassment
- Publishing others' private information without permission
- Other conduct which could reasonably be considered inappropriate

### Enforcement

Project maintainers are responsible for clarifying standards of acceptable behavior and will take appropriate and fair corrective action in response to any instances of unacceptable behavior.

## License

By contributing, you agree that your contributions will be licensed under the MIT License.

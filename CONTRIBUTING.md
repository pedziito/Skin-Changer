# Contributing to CS2 Skin Changer

Thank you for your interest in contributing to this educational project!

## Important Notes

This project is for **educational purposes only**. All contributions should maintain this focus.

## Acceptable Contributions

We welcome contributions in the following areas:

### Code Quality
- Bug fixes
- Code refactoring
- Performance improvements
- Memory leak fixes
- Better error handling

### Documentation
- Improved README
- Better code comments
- Tutorial improvements
- Troubleshooting guides
- Architecture documentation

### Features
- Better offset management
- Improved pattern scanning
- Enhanced GUI design
- Configuration improvements
- Cross-platform build support (for compilation, not runtime)

### Security
- Security vulnerability fixes
- Better process validation
- Safer memory operations
- Input validation

## Unacceptable Contributions

Please **DO NOT** submit:

- Features designed to bypass anti-cheat
- Network packet manipulation
- Server-side modifications
- Features encouraging competitive/online use
- Obfuscation techniques
- Malicious code

## Development Setup

### Requirements
- Windows 10/11 (for testing)
- Visual Studio 2019+
- CMake 3.15+
- Git

### Building
See [BUILD.md](BUILD.md) for detailed build instructions.

### Code Style

- Use consistent indentation (4 spaces)
- Follow existing naming conventions:
  - Classes: `PascalCase`
  - Functions: `PascalCase`
  - Variables: `camelCase`
  - Private members: `m_prefix`
- Add comments for complex logic
- Keep functions focused and small
- Use RAII for resource management

### Example
```cpp
class MyClass {
public:
    MyClass();
    void DoSomething();
    
private:
    void InternalHelper();
    int m_memberVariable;
};
```

## Submitting Changes

### Pull Request Process

1. **Fork the repository**

2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow code style
   - Add comments
   - Test thoroughly

4. **Commit with clear messages**
   ```bash
   git commit -m "Add feature: description of what you did"
   ```

5. **Push to your fork**
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Open a Pull Request**
   - Describe what you changed
   - Explain why it's beneficial
   - Reference any issues it fixes

### Commit Message Guidelines

- Use present tense ("Add feature" not "Added feature")
- Use imperative mood ("Move cursor to..." not "Moves cursor to...")
- Limit first line to 72 characters
- Reference issues: "Fixes #123"

Examples:
```
Add pattern scanning fallback mechanism
Fix memory leak in ProcessManager
Improve error messages in GUI
Update README with new build instructions
```

## Code Review Process

All submissions require review. We review:
- Code quality
- Adherence to project goals
- Security implications
- Documentation completeness

## Testing

Before submitting:

1. **Verify it builds**
   ```bash
   cmake --build build --config Release
   ```

2. **Test core functionality**
   - Process attachment
   - Memory operations
   - GUI interaction

3. **Check for memory leaks**
   - Use Visual Studio analyzer
   - Check process memory usage

4. **Verify documentation**
   - Update README if needed
   - Add code comments
   - Update USAGE.md if behavior changes

## Areas That Need Help

Current areas where contributions would be valuable:

### High Priority
- [ ] Better offset update mechanism
- [ ] Improved error messages
- [ ] More comprehensive skin database
- [ ] Better pattern scanning algorithms
- [ ] GUI improvements

### Medium Priority
- [ ] Configuration file validation
- [ ] Better documentation
- [ ] Code refactoring
- [ ] Unit tests (for non-memory components)

### Low Priority
- [ ] Alternative GUI frameworks
- [ ] Localization support
- [ ] Theme customization
- [ ] Logging system

## Questions or Problems?

- Check existing issues first
- Create a new issue for bugs or questions
- Be respectful and constructive
- Remember this is an educational project

## Legal and Ethical Guidelines

By contributing, you agree that:

1. Your contributions are your own work
2. You understand this project is educational only
3. You do not encourage violation of ToS
4. You will not submit malicious code
5. You understand the project's limitations and purpose

## Recognition

Contributors will be recognized in:
- Git commit history
- GitHub contributors page
- Special mentions for significant contributions

## License

By contributing, you agree that your contributions will be licensed under the same terms as the project.

## Final Notes

This project exists to demonstrate:
- Windows API usage
- Memory manipulation techniques
- Process interaction
- Pattern scanning algorithms
- Client-side game modification

All contributions should support these educational goals while being mindful of ethical considerations.

Thank you for contributing responsibly!

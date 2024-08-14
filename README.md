## Custom Shell 


A custom shell created in C that mimics the functionality of a Linux shell.

### Features
- Command Execution: Execute commands just like in a regular shell.
- Built-in Commands: Includes built-in commands such as cd, pwd, exit, etc.
- Input/Output Redirection: Supports redirection of input (<), output (>), and appending (>>).
- Piping: Allows chaining of commands using pipes (|).
- Background Processes: Run commands in the background with &.
- Tab Completion: Provides tab completion for commands and file paths.
- History: Keeps a history of previously executed commands.
- Customization: Customize prompt, colors, and behavior.


## Getting Started
### How to compile 
```make
    make all   # Compile the program and creates a .out file with the name customShell .
    
    make clean  # Delete the .out file. 
```

### How to run 
```bash
    ./customShell
```
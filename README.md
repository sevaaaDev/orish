## Orish (Orion Shell)

this is a minimal shell written in c based on the [posix sh spec](https://pubs.opengroup.org/onlinepubs/9699919799/utilities/V3_chap02.html#tag_18_10)

i dont plan to implement all posix features, so this might not be fully posix compliant. the [roadmap](#Roadmap) list all the features i plan to implement.

### Current Features
- execute list of command (echo hello; echo world)
- interactive mode with builtin cmd (cd, exit)
- scripting mode (execute command from file)

### Usage
```sh
Usage:   orish
         orish [-c COMMANDS]
         orish [FILENAME]
Example:
         orish -c "echo hello"
         orish script.osh
```

### Building
```sh
git clone https://github.com/sevaaaDev/orish.git
cd ./orish
make orish
./orish # you can move this to $PATH or whatever
```

## Roadmap

### Version: bare minimum
- [x] command list (;)
- [x] basic interactive mode
- [x] basic scripting mode

### Version: usable
- [ ] quotes and escape
- [ ] command piping
- [ ] conditional using and (&&) or (||)
- [ ] readline with history

### Version: qol
- [ ] redirection (> >> < <<)
- [ ] globbing
- [ ] readline with keybinding

#### Version: dream
- [ ] job control
- [ ] variable and env var
- [ ] subshell

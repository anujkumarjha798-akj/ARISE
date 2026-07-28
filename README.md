# 🚀 ARISE - A Lightweight, Keyboard-First Terminal Code Editor

<p align="center">

![Version](https://img.shields.io/badge/version-2.0.0-blue?style=for-the-badge)
![Language](https://img.shields.io/badge/C11-orange?style=for-the-badge)
![Platform](https://img.shields.io/badge/Linux-macOS-2ea44f?style=for-the-badge)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)

</p>

<p align="center">
<img src="assets/banner.png" width="900" alt="ARISE Banner">
</p>

<p align="center">

### ⚡ Fast • 🎨 Beautiful • ⌨️ Keyboard First • 🛡️ Safe • 🚀 Lightweight

**ARISE** is a modern terminal-based code editor written in **C11 + ncurses**.  
It is designed for developers who want a **fast**, **beautiful**, and **minimal** editing experience directly inside the terminal.

Perfect for:

🐧 Linux Users • ☁️ SSH Servers • 🐳 Docker • ☸️ Kubernetes • 💻 DevOps Engineers • Raspberry Pi

</p>

---

# ✨ Features

## 🎯 Core Editing

- ✅ Full Terminal UI
- ✅ Keyboard First Workflow
- ✅ Multi-language Syntax Highlighting
- ✅ Smart Auto Indentation
- ✅ Auto Closing Brackets
- ✅ Dynamic Line Numbers
- ✅ Indent Guides
- ✅ Status Bar
- ✅ Current Line Highlighting

---

## 🚀 Performance

- Extremely Fast Startup
- Low Memory Usage
- Works Great over SSH
- Single Binary
- No Heavy Dependencies
- Smooth Rendering

---

## 🎨 Beautiful Dracula Theme

Built-in Dracula inspired theme featuring:

- Syntax Colors
- Current Line Highlight
- Beautiful Status Bar
- Gutter Styling
- Readable Color Palette

---

## 📋 Clipboard Support

Supports

- xclip
- xsel
- wl-clipboard
- pbcopy (macOS)
- OSC52 over SSH
- Internal Clipboard Fallback

---

## ⚡ Powerful Editing

- Smart Auto Indentation
- Multi-line Paste
- Copy / Cut / Paste
- Duplicate Line
- Move Lines
- Toggle Comments
- Go To Line
- Word Autocomplete

---

## 🛡️ Safe Exit

Never lose your work.

When exiting:

```
Save changes before closing?

[Y] Save
[N] Discard
[C] Cancel
```

---

# 📸 Screenshots

## Editor

```
+----------------------------------------------------+
| main.c                                  [+]        |
|----------------------------------------------------|
|  1 #include <stdio.h>                             |
|  2 int main() {                                   |
|  3     printf("Hello World");                     |
|  4 }                                              |
|----------------------------------------------------|
| Ctrl+Q Quit | Ctrl+S Save | Ctrl+C Copy           |
+----------------------------------------------------+
```

(Add real screenshots later)

---

# 🎥 Demo

![Demo](assets/demo.gif)

(Add GIF later)

---

# 🚀 Installation

## Ubuntu / Debian

```bash
sudo apt update
sudo apt install build-essential libncurses-dev xclip xsel
```

---

## Fedora

```bash
sudo dnf install gcc make ncurses-devel xclip xsel
```

---

## Arch Linux

```bash
sudo pacman -S gcc make ncurses xclip xsel
```

---

## Alpine

```bash
sudo apk add gcc make ncurses-dev musl-dev xclip xsel
```

---

## macOS

```bash
brew install ncurses
```

---

# 📥 Clone

```bash
git clone https://github.com/USERNAME/arise.git

cd arise
```

---

# 🔨 Build

```bash
make
```

---

# 📦 Install

```bash
sudo make install
```

---

# ✅ Verify

```bash
which arise
```

Expected

```
/usr/local/bin/arise
```

---

# 🚀 Quick Start

Open empty editor

```bash
arise
```

Open file

```bash
arise main.c
```

Create new file

```bash
arise hello.py
```

Edit Kubernetes YAML

```bash
arise deployment.yml
```

Edit Docker Compose

```bash
arise docker-compose.yml
```

---

# ⌨️ Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| Ctrl+Q | Quit |
| Ctrl+N | New File |
| Ctrl+C | Copy |
| Ctrl+X | Cut |
| Ctrl+V | Paste |
| Ctrl+A | Select All |
| Ctrl+D | Duplicate Line |
| Ctrl+L | Go To Line |
| Ctrl+/ | Toggle Comment |
| Tab | Autocomplete |
| Shift+Tab | Remove Indent |
| Enter | Smart New Line |

---

# 🌈 Supported Languages

- C
- C++
- Python
- JavaScript
- TypeScript
- Java
- Go
- YAML
- JSON
- HTML
- CSS
- Bash
- Dockerfile
- Kubernetes YAML
- Markdown
- SQL
- XML
- TOML
- Rust
- PHP
- Lua
- Ruby
- Terraform

---

# 📂 Project Structure

```
arise/

├── src/
│   ├── main.c
│   ├── editor.c
│   ├── input.c
│   ├── buffer.c
│   ├── file.c
│   ├── clipboard.c
│   ├── syntax.c
│   ├── ui.c
│   ├── actions.c
│   └── arise.h
│
├── Makefile
│
└── README.md
```

---

# 🖥️ Platform Support

✅ Ubuntu

✅ Debian

✅ Fedora

✅ Arch

✅ Alpine

✅ Raspberry Pi

✅ Docker

✅ WSL

✅ macOS

✅ SSH Servers

✅ Cloud VPS

---

# 🛣️ Roadmap

- [x] Syntax Highlighting
- [x] Clipboard Support
- [x] Auto Indentation
- [x] Auto Closing Brackets
- [x] Save Confirmation
- [x] Dracula Theme
- [x] Multi-language Support
- [ ] Split Windows
- [ ] Plugin System
- [ ] LSP Support
- [ ] Git Integration
- [ ] Multiple Tabs
- [ ] File Explorer

---

# 🤝 Contributing

Contributions are welcome!

```bash
Fork the repository

Clone your fork

Create a new branch

git checkout -b feature/my-feature

Commit changes

git commit -m "feat: amazing feature"

Push

git push origin feature/my-feature

Open Pull Request
```

---

# ⭐ Support

If you like this project,

please consider giving it a ⭐ on GitHub.

It helps the project grow and motivates future development.

---

# 👨‍💻 Author

**Anuj Kumar Jha**

GitHub:

https://github.com/anujkumarjha798-akj

---

<p align="center">

## ⭐ Star the Repository

**Made with ❤️ using C11 + ncurses**

If ARISE helped you, don't forget to **Star ⭐ the repository**.

</p>

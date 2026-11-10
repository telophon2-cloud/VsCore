![Logo.png]
# VsCore Operating System v1.2

**VsCore** (formerly VsInux) is a sovereign, independent, and compact 32-bit operating system for the x86 architecture, written completely from scratch ("on bare metal") without using any third-party standard libraries or POSIX layers. The project was developed by the **VladisSystems Team** under the leadership of **Shumatbaev Vladislav**.

The system features a completely custom kernel architecture, a resident file system, an independent window environment, and a built-in C code interpretation engine.

---

## 🚀 Key Features and Capabilities

* **Complete Independence:** The kernel is written in `-ffreestanding` mode with its own hardware drivers for the i8042 keyboard controller and VGA Text Mode video memory.
* **Signature UI:** A deep dark console theme with a bright cyan ASCII logo at startup and a custom `neofetch` that displays protected copyright lines.
* **Shift-Lock Security:** The Shift key is completely disabled and ignored at the hardware level for secure data entry.
* **vSano v2.5 Text Editor:** An improved alternative to the classic nano. It supports full navigation using the keyboard arrow keys, hot-saving and exiting via `Ctrl+S` (or `F2`), and automatic substitution of brackets `{ }`, `( )` and quotes using free unmapped keys.
* **VsCoreFS File System:** A lightweight resident file system with strict string buffer management and crash protection.
* **RunC Interpreter:** Instantly runs and executes C source code files (e.g., `printf` statements) directly from the console prompt.
* **VsDE Desktop Environment:** A unique GUI-like window environment. It features a multi-window layout, keyboard-driven focus switching, and a dynamic **File Explorer** that displays the list of files created within the system.
* **Non-Freezing ATA Installer:** A low-level utility for partitioning virtual drives via ATA PIO ports with an asynchronous status bar and a built-in **hardware auto-restart** feature once the installation is finished.

---

## 🛠️ Built-in Shell Commands

* `help` — Displays the interactive system help guide.
* `neofetch` — Displays the signature VsCore kernel logo.
* `clear` — Clears the screen while preserving the permanent top banner.
* `ls` — Lists files residing on the VsCoreFS partition.
* `vsano [filename]` — Opens an existing file or creates a new one for editing.
* `cat [filename]` — Outputs the text content of a file to the screen.
* `rm [filename]` — Permanently deletes an object from the file system.
* `RunC [filename]` — Compiles and interprets the target C source file.
* `VsDE` — Switches the system into the graphical windowed desktop environment.
* `install` — Deploys the OS structures to the attached storage drive and triggers a hardware reset.
* `about` — Shows credits, organization, and license data.
* `shutdown` — Halts kernel execution and puts the CPU into the `HLT` state.

---

## ⚙️ Building and Running

The compilation is performed on a Linux host system (Ubuntu / KDE Neon) using `nasm`, `gcc-multilib`, and the `qemu-system-i386` emulator.

### One-line build command:

```bash
nasm boot.asm -f bin -o boot.bin && \
nasm kernel_entry.asm -f elf32 -o kernel_entry.o && \
gcc -m32 -ffreestanding -fno-pie -nostdlib -fno-stack-protector -O0 -mgeneral-regs-only -Wno-overflow -c kernel.c -o kernel.o && \
ld -m elf_i386 -T linker.ld -o kernel.bin kernel_entry.o kernel.o && \
qemu-img create -f raw vscore_os.img 10M && \
dd if=boot.bin of=vscore_os.img bs=512 count=1 conv=notrunc status=none && \
dd if=kernel.bin of=vscore_os.img bs=512 seek=1 conv=notrunc status=none && \
qemu-system-i386 -drive format=raw,file=vscore_os.img,if=ide,index=0,media=disk
```

---

## 🔮 Future Roadmap (Планы на будущее)

The VladisSystems Team has big plans for the upcoming versions of VsCore to turn it into a fully capable operating system:

* **BBS Networking & Text Web:** Implement serial COM-port (UART) dial-up emulation to support a retro-networking stack. Develop **NightBrowser** — a custom BBS-board reader capable of browsing legacy text portals like Old-DOS.
* **Dynamic Memory Allocator (`malloc` / `free`):** Move away from completely static arrays toward a real heap manager to scale file sizes and application limits dynamically.
* **Multitasking (Task Switching):** Implement hardware or software context switching to allow background tasks to run concurrently with the shell or VsDE.
* **Expanded C-Interpreter:** Upgrade the `RunC` engine to parse basic variables (`int`, `char*`), control loops (`if`, `for`), and handle simple math operations.
* **VsDE Graphical Desktop Upgrades:** Introduce window dragging via custom keyboard shortcuts and a third application window dedicated exclusively to networking and system metrics.

---

## 📄 License

This project is distributed under the **MIT License**. Developed by VladisSystems Team (c) 2026.

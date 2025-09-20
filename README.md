# Command Timer

A simple Windows timer that runs a command when it's done.

## 🚀 Features

  * **Easy Time Set:** Set the timer with hours, minutes, and seconds.
  * **Time Presets:** Quickly set the timer using one of three customizable preset buttons.
  * **Run Any Command:** Automatically run an app, open a file, or launch a website when the timer finishes.
  * **Simple Controls:** Easily Start, Pause, Resume, and Reset the timer.
  * **Clean Interface:** A simple and easy-to-use design.
  * **Large Display:** The time is large and easy to read.
  * **Ultra-lightweight:** Application size is less than 400KB (statically linked).


<img width="511" height="313" alt="v13" src="https://github.com/user-attachments/assets/7114fe37-2f39-4162-9667-336eb9eb2f5e" />


## 💻 How to Use

1.  Enter the hours, minutes, and seconds, or click a preset button.
2.  In the `Command` box, type what you want to run.
3.  Click **Start**.
4.  Use the **Pause** and **Reset** buttons to control the timer.

When the time is up, your command will run automatically.

## ⚙️ Configuration

You can customize the three preset time buttons by editing the `CommandTimer.ini` file, which is created in the same folder as the application. The times are set in minutes.

**Example `CommandTimer.ini`:**

```ini
[PresetTimes]
Time1=5
Time2=30
Time3=60
```

## ⚙️ Command-Line Arguments

You can also launch the application with arguments to set the timer and command.

  * Supports `-start`, `-h`, `-m`, `-s`, and `-cmd` arguments.
  * The `-cmd` argument must be the last one in the command line.

**Example:**
To set a 30-minute timer that starts immediately and opens Notepad when finished:

```
CommandTimer.exe -start -m 30 -cmd "notepad.exe"
```

## Command Examples

Here are some examples of commands you can use:

**Run an App**

  * `notepad.exe` - Opens Notepad.
  * `calc.exe` - Opens the Calculator.
  * `mspaint.exe` - Opens Microsoft Paint.
  * `explorer.exe C:\` - Opens the C: drive in File Explorer.
  * `rundll32.exe user32.dll,LockWorkStation` - Lock the Windows screen.

**Open a File or Folder** *(Note: Change `YourUser` to your Windows username)*

  * `C:\Users\YourUser\Desktop\document.docx` - Opens a Word document from your desktop.
  * `D:\Music\favorites.mp3` - Plays a music file.
  * `C:\Users\YourUser\Downloads` - Opens your Downloads folder.

**Open a Website**

  * `https://www.google.com`
  * `https://www.youtube.com`

**System Commands**

  * `shutdown /s /t 0` - Shuts down your computer. (e.g., set timer for 1 hour to shut down automatically).
  * `shutdown /r /t 0` - Restarts your computer.
  * `shutdown /l` - Logs you off.

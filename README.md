# Model_worker

This program prevents some applications (e.g. Microsoft Teams) to show "Leave" or "Away" status.

The program detects user input idle time from Windows library function ``GetLastInputInfo()``. If the computer is idled for a while, the program will generate a simulated input via ``mouse_event()`` function. 

# Configure and compile

To compile the code, you need to install [MinGW](https://www.mingw-w64.org/). Then compile the code using:
```cmd
gcc -mwindows model_worker.c -o model_worker.exe
```

If you wish to make the program to run automatically on startup, edit the ``<Path to model_worker.exe>`` in ``model_worker.vbs``. Then copy ``model_worker.vbs`` to the folder ``C:\ProgramData\Microsoft\Windows\Start Menu\Programs\StartUp``. (You can also drag and drop it to the Startup Shortcut in the folder)

# Usage

Double click on ``model_worker.exe`` or using the command line:
```cmd
.\model_worker.exe <target_detect_interval>
```
to set the detect interval in seconds of the application that monitors your idle status.

The program runs in an auto mode by default after start. In this mode, it will run on working hours on weekdays, and otherwise pause.

Auto mode can be exit by uncheck the ``Auto leave`` button or click on  ``Start`` or ``Pause`` button.
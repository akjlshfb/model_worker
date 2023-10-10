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

Simply run ``model_worker.exe``. This program avoids MS Teams from showing "Away" status.

MS Team's detect interval of the user input is said to be [5 minutes](https://learn.microsoft.com/en-us/microsoftteams/troubleshoot/teams-im-presence/presence-not-show-actual-status). If you wish to change the detect interval (in seconds) to prevent other application from showing "away", use command line:
```cmd
.\model_worker.exe APP_DETECT_INTERVAL_IN_SECONDS
```
Then the interval to generate fake input will be calculated base on the application's detect interval, ensuring idle status won't be detected.

This program runs in an auto mode by default after start. In this mode, it will only run on working hours on weekdays. (by default: 0830-1200, 1330-1800 of Mon-Fri)

Auto mode can be cancelled by uncheck the ``Auto leave`` button or click on  ``Start`` or ``Pause`` button.

# Config file

To set your own working hours, create a ``config.txt`` file in the same folder as the exe file. The config file should contain the following content:

The first line should contain multiple integers seperated by a single space character. Each integer represents a workday of a week. Monday is considered the first day of a [week](https://en.wikipedia.org/wiki/ISO_8601).

For the following lines, each line should contain two integers, seperated by a space character, representing the start and end of each period of the working hours in 24-hour  format.

For example, the default setting assumes working hours is 8:30 AM to 12:00 PM and 1:30 PM to 6:00 PM from Mondy to Friday each week. The equivalent config file should be:
```
1 2 3 4 5
0830 1200
1330 1800
```
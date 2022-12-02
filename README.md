# Audesse_Programming-MS
This repository contains code for Audesse Programming

***
Welcome to the Audesse_Programming-MS wiki!

If FlexCase is not detected, the RPi module might not have rebooted.

Force reboot the RPi module in FlexCase:

    1. Power up the FlexCase.
    2. Force the Pi to boot by quickly removing and reapplying power to the board (plug/unplug the connector and target 0.25s of a power outage).
    3. Add delay to the MCU model as described under the section 'MCU-MPU Interface (Required to Prevent Damage) in the link.

The newest version of the raspberry pi OS does not build Simulink code properly, hence the rollback to a previous version.

***

Change OS of the RPi module:

    1. Download the OS image from the link.
    2. With the power disconnected, remove the case, only placing the exposed board on insulating surfaces. If this is your first time doing this, you can see some additional tips at this link.
    3. Connect a micro-USB data cable from your PC to the FlexCase micro-USB port.
    4. Power up the FlexCase, download, and run Rpiboot to make the device appear as a boot partition. Ignore all prompts from your PC to format or interact with the drive.
    5. Download, install and run Raspberry Pi Imager.

        If rpiboot was successful, you should be able to select the Rpi MSD for "Choose Storage".
        For "Choose OS", scroll to the bottom of the menu, select "Use custom" and select the zip file you downloaded in step 1.
        Click "Write" to flash the OS. This will take several minutes.

    6. Once completed, you can power off the FlexCase and remove the micro-USB to restore normal function.

        The host name and password are FlexCase01 and audesse_temp

    7. Place the case back on. You will hear a click sound after a good fit.
***
Change default SPI settings:

    Welcome to the Audesse_Programming-MS wiki!

If FlexCase is not detected, the RPi module might not have rebooted.

Force reboot the RPi module in FlexCase:

    Power up the FlexCase.
    Force the Pi to boot by quickly removing and reapplying power to the board (plug/unplug the connector and target 0.25s of a power outage).
    Add delay to the MCU model as described under the section 'MCU-MPU Interface (Required to Prevent Damage) in the link.

The newest version of the raspberry pi OS does not build Simulink code properly, hence the rollback to a previous version.

Change OS of the RPi module:

    Download the OS image from the link.
    With the power disconnected, remove the case, only placing the exposed board on insulating surfaces. If this is your first time doing this, you can see some additional tips at this link.
    Connect a micro-USB data cable from your PC to the FlexCase micro-USB port.
    Power up the FlexCase, download, and run Rpiboot to make the device appear as a boot partition. Ignore all prompts from your PC to format or interact with the drive.
    Download, install and run Raspberry Pi Imager.

        If rpiboot was successful, you should be able to select the Rpi MSD for "Choose Storage".
        For "Choose OS", scroll to the bottom of the menu, select "Use custom" and select the zip file you downloaded in step 1.
        Click "Write" to flash the OS. This will take several minutes.

    Once completed, you can power off the FlexCase and remove the micro-USB to restore normal function.

        The host name and password are FlexCase01 and audesse_temp

    Place the case back on. You will hear a click sound after a good fit.

Change default SPI settings:

    1. Change the default SPI setting from SPI0 to SPI1 by running the below MATLAB script in the command window.

```Matlab
cd C:\ProgramData\MATLAB\;
possibleDir = dir('**/MW_SPI.c');
for i = 1:length(possibleDir)
    if contains(possibleDir(i).folder, 'rasp') == 1
       SPIDir = possibleDir(i).folder;
    end
end
cd (SPIDir);
fid = fopen('MW_SPI.c','r');
f=fread(fid,'*char')';
fclose(fid);
notificationChange = count(f,"/dev/spidev0.");
f = regexprep(f,'/dev/spidev0.','/dev/spidev1.');
fid  = fopen('MW_SPI.c','w');
fprintf(fid,'%s',f);
fclose(fid);
disp("Successfully made " + notificationChange + " changes to SPI Config.")
```
***
Disable default python SPI on MCU:

    1. Check if any SPI script is running.

```
ps -ax | grep python
```

    2. Disable SPI communication if it is running.
```
sudo systemctl disable spiComm
sudo systemctl stop spiComm
```

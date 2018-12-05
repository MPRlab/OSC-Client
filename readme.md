# OSC-Client

This repository contains the starting code for any STM32 Nucleo F767ZI based instruments and allows them to connect to the MSM Max patch using OSC over UDP.

## OSC Controller

For the latest version of the MSM patch visit:
https://github.com/MPRlab/MPR-Lab-Robot-Interface

This has been tested with version 12.6.2 (coming soon).

## Compiling and Developing

 - Ubuntu 16.04

   - Download the GNU ARM Embedded Toolchain
   
     (Used version 7-2018-q2-update linux 64-bit)
     
     [Download Link](https://developer.arm.com/open-source/gnu-toolchain/gnu-rm/downloads)
   
   - Extract the files into /opt
     ```
     cd /opt
     sudo tar -xf ~/Downloads/gcc-arm-none-eabi-7-2018-q2-update-linux.tar.bz2 
     ```

   - Make symlinks
     ```
     sudo ln -s /opt/gcc-arm-none-eabi-7-2018-q2-update/bin/arm-none-eabi-cpp /usr/bin/arm-none-eabi-cpp
     sudo ln -s /opt/gcc-arm-none-eabi-7-2018-q2-update/bin/arm-none-eabi-g++ /usr/bin/arm-none-eabi-g++
     sudo ln -s /opt/gcc-arm-none-eabi-7-2018-q2-update/bin/arm-none-eabi-gcc /usr/bin/arm-none-eabi-gcc
     ```
   - Clone this repository to any location
     ```
     git clone https://github.com/MPRlab/OSC-Client.git
     cd OSC-Client/
     ```
   - Edit the main.cpp file
   
     Required changes:
     
     - Change line 87 to use your instrument name
       
     - Replace lines 67-68 with the code to play your instrument
   
   - Compile the code
     ```
     make
     ```
     
   - Program the Nucleo
     
     Copy the ```BUILD/osc.bin``` file to the Nucleo using the file explorer or  ```mv```

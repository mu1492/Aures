# Aures

**Advanced acoustics platform**

The project is designed to work with the following hardware:
- Analog Devices **[EVAL-MICCANVASZ](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-miccanvasz.html)** microphone array
- Analog Devices **[EVAL-ADAU1467Z](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-adau146x.html)** evaluation board with ADAU1467 SigmaDSP processor
- Nvidia Jetson **AGX Orin** *or* Nvidia Jetson **Orin NX** *or* Nvidia Jetson **Orin Nano**


**Prerequisites:**
- TDM16 relies on a properly configured I2S bus between EVAL-ADAU1467Z and the Nvidia Jetson computer, which must handle a BCLK of 8.192 MHz.
- The onboard ADAU1467 is programmed and flashed with SigmaStudio (`adau1467_tdm16.dspproj` file is provided). This operation is required only once, therefore the [USB programming interface](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/eval-adusb2ebz.html "USB programming interface") can be disconnected during runtime.
- The pinmux related to the I2S bus on the 40-pins header is enabled on Nvidia Jetson.
- The device tree and Linux kernel are rebuilt for configuring TDM16 via I2S on Nvidia Jetson (`tegra234.dtsi` file is provided).
- The ALSA development package is installed.

		sudo apt install libasound2-dev

- [VOSK](https://alphacephei.com/vosk/ " VOSK") ASR has been built on target with CUDA flags enabled.  At least one [language model](https://alphacephei.com/vosk/models "language model") needs to be extracted and copied to `$HOME/vosk_models/` . Refer to the `Aures/AsrModel` class for adding or removing specific languages.
- If the `USE_ROS` flag is enabled in `Aures/CMakeLists.txt`, then the development ROS2 Humble packages need to be preinstalled.  In this case, **Aures** will publish a ROS2 message coming from the first configured speaker, regardless the selected language and/or the decoded text.
- The **AuresSubscriber** application is always dependent on ROS2, and can be built and used independently from **Aures**, on the same computer, or on two different computers from the same subnetwork.
- After cloning this repository, copy the following three files to the `Aures` folder:
  - `vosk_api.h`
  - `libvosk.so`
  - `libasound.so`

Build steps for **Aures** (first line is only for ROS2) :

	source /opt/ros/humble/setup.bash  
	cd Aures
	mkdir build
	cd build
	cmake ..
	make

, respectively for **AuresSubscriber** :

	source /opt/ros/humble/setup.bash
	cd AuresSubscriber
	mkdir build
	cd build
	cmake ..
	make

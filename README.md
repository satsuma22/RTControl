# RTControl

## Table of Contents
- [About the Project](#about-the-project)
- [Requirements](#requirements)
- [Dependencies](#dependencies)
- [Building](#building)
- [Usage](#usage)
- [Configuration](#configuration)

## About the Project
RTControl is command line tool that let's you self-host 3D applications on Windows. It uses WebRTC and hardware-accelarated video encoding to deliver low-latency streaming.

## Requirements
At the moment, the RTControl is only compatible with systems that are running Windows 10 (and above) and have a Nvidia GPU. The client-side application is browser-based (and hence platform independent) and be can run on most modern browsers. 

## Dependencies
RTControl depends on [libdatachannel](https://github.com/paullouisageneau/libdatachannel/tree/master) for WebRTC support and on [Nvidia Video Codec SDK](https://developer.nvidia.com/video-codec-sdk) for hardware-accelerated encoding.
* [Nvidia Video Codec SDK](https://developer.nvidia.com/nvidia-video-codec-sdk): can be downloaded [here](https://developer.nvidia.com/nvidia-video-codec-sdk/download) (requires sign-in/up).
* [libdatachannel](https://github.com/paullouisageneau/libdatachannel/tree/master) is available as a submodule. All of [libdatachannel's](https://github.com/paullouisageneau/libdatachannel/tree/master) dependencies (except for [OpenSSL](https://www.openssl.org/)) are also available as submodlues. [OpenSSL](https://www.openssl.org/) binaries for windows can be dowloaded [here](https://kb.firedaemon.com/support/solutions/articles/4000121705-openssl-3-1-3-0-and-1-1-1-binary-distributions-for-microsoft-windows).

Additionally, you will need to install [node.js](https://nodejs.org/en) for the signalling server.

## Building
After dowloading both the ```Nvidia Video Codec SDK``` and the ```OpenSSL``` binaries, extract them to some location on your computer. Next create and set the environment variables ```NVIDIAENCODE_ROOT_DIR``` and ```OPENSSL_ROOT_DIR```, which point to the root directories of ```Nvidia Video Codec SDK``` and ```OpenSSL``` respectively.

Then clone the repo and all the submodules with the command:
```bash
git clone --recurse https://github.com/satsuma22/RTControl.git
```

Next, create a build folder in the root directory of ```RTControl``` and run cmake:
```bash
mkdir bulid
cd build
cmake ..
```

This will generate the project files for Visual Studios. Open the project and select ```Debug``` or ```Release``` mode and hit build.

## Usage
```RTControl``` will only encode video if the application is running on an Nvidia GPU. If you are running this application on a laptop with a discrete Nvidia GPU, the application might still default to running on your integrated GPU. You can select which GPU the application will run on by changing settings in the ```Nvidia Control Panel```. On desktops with Nvidia GPU, this shouldn't be a problem.

To run ```RTControl```, you first need to start the signalling server. This can be done with the command:
```bash
node web/SignalingServer/SignalingServer.js
```
Then navigate to the folder containing the executable and launch ```RTControl``` either in default mode by running the command:
```bash
RTControl.exe
```
or in capture initialized mode by running the command:
```bash
RTControl.exe "Application Window Title"
```

In default mode, ```RTControl``` doesn't capture any application immediately. Rather, the user of the client-side can choose to launch (and capture) an application from a list of application (specified in ```config.json```). In capture initialized mode, ```RTControl``` will start capturing the application whose window title was passed on as the second command line argument.

To connect to the ```Signaling Server``` from the client-side browser, go to:
* ```localhost:8000``` if the client and the server are on the same PC,
* ```<LAN_IP_ADDRESS>:8000``` if the client and the server are in the same Local Area Network
* ```<WAN_IP_ADDRESS>:8000``` if the client and the server are in different networks

When you land on the home page of the application, use the same ```IP_ADDRESS:PORT_NUMBER``` from the previous step to connect to the ```RTControl``` server.

## Configuration
Settings for ```RTControl``` can be configured via the ```config.json``` file. If this file is missing, running the application will create a ```config.json``` file with default values. Settings that can be configured are:
* ```Encoder Settings```
  * ```Bitrate Average```: average bitrate of the encoded video in bps.
  * ```Bitrate Max```: maximum bitrate of the encoded video in bps.
  * ```GOP Length```: Length of Group-of-Pictures. Essentially determines how often a key frame is sent.
  * ```I-Frames Only```:(0/1) Whether or not to use only key frames. Useful when dealing with packey loss.
  * ```OutputDelay```: Determines how long after receiving a raw frame, an encoded frame gets generated. Measured in number of frames. 
  *  ```Preset```: (1-7) Determines the trade-off between encoding speed and compression efficiency.
  *  ```QP```: (0-51) Determines the trade-off between picture quality and compression efficiency.
  *  ```Rate Control Mode```: Selects the rate control mode, for example CBR (constant-bit-rate), VBR (variable-bit-rate), etc.
  * ```Target Quality```: (0-51) Sets the desired ```QP``` for ```Rate Control Mode = Target Quality```.
  * ```fps```: Frames-per-second for the encoder. Should be equal to you monitors refresh rate.
* ```Global Settings```
  * ```CaptureCursor```: (0/1) Whether or not to capture cursor.
  * ```CaptureMode```: (Monitor/Window) ```Window``` mode will only capture the application you're trying to stream. Any overlapping window will not be captured. However, this mode will not capture any child windows either (such as drop down windows). ```Monitor``` mode captures the entire monitor and then crops out the application window. This mode captures child windows as well (as long as they are overlapping with the parent window).
 * ```List```: Is a list of applications that the user can launch from the client-side, when the server is launched in default mode. The key needs to be an exact match of the window title of the application when launched and the value needs to be the command (including path, if required) to launch the application.
* ```Streamer Settings```
  * ```fps```: Frames-per-second for the streamer. Always set this to be higher than ```Encoder:fps``` for lowest possible latency.

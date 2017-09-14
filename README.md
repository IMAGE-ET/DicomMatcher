# DicomReader #

## Install prerequisites ##

```
sudo add-apt-repository --yes ppa:xqms/opencv-nonfree
sudo apt-get update
sudo apt-get install build-essential cmake libdcmtk-dev libopencv-dev libopencv-nonfree-dev qtbase5-dev
```

## Build ##

```
git clone https://github.com/agabor/DicomMatcher.git
cd DicomMatcher
mkdir build
cd build
cmake -G Unix\ Makefiles ..
make
```
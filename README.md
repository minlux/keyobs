# keyobs
Brother Scanner Key Observer.


## Setup

```
git clone ...
cd keyobs
git submodule init
git submodule update
```


## Build

```
mkdir build
cd build
cmake ..
make
```


## Usage
Copy `config.json` next to the executable. Adapt ip address of your scanner. Adapt/add command you want to be executed when you select it from menu
- FILE
- IMAGE
- OCR
- EMAIL

Then just start the executable and press scanner key on your device. *keyobs* will executed the configured command as a system call.
```
./keyobs
```

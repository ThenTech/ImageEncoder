#!/bin/bash

sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update
sudo apt install g++-7 -y

## Also change symbolic links to newer version
# sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 \
#                          --slave /usr/bin/g++ g++ /usr/bin/g++-7 
# sudo update-alternatives --config gcc
# gcc --version
# g++ --version
g++-7 --version

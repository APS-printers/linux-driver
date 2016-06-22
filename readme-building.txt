In order to build the linux driver, you need the following packages:
- cups
- libusb; libusb 1.0-0 was used when writing this
- libqrencode

In addition to that, at least these packages need to be installed so that
th driver builds successfully:
libcups2-dev
libcupsppdc1-dev
libcupsfilters-dev
libcupsimage2-dev

That should be it. When you have all these packages installed,
simply run 'make && make install'

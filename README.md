# Unrootless-KEXT
A kext that can be used to disable Rootless in OS X El Capitan. You need to sign it OR use an exploit to make OS X load it.
See [https://youtu.be/dq0-0WVGyq4](https://youtu.be/dq0-0WVGyq4)
This kext can be loaded using [this](https://github.com/LinusHenze/anyKextLoader) exploit. (If you're running OS X 10.11 - 10.11.3, was fixed in OS X 10.11.4)

#Building
Just open the Project in Xcode and select Product->Build.

#Using
Copy the kext to a directory of your choice and make sure that the kext has the right permissions (sudo chown -R root:wheel <path to kext>)
As stated above, you need to sign this kext with a valid certificate or you need an exploit ([like this](https://github.com/LinusHenze/anyKextLoader)) to make OS X load it.

#License
Most files already contain the license they are distributed under (because I shamelessly copied them). If theres no license in a file, please see LICENSE.txt

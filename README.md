# Unrootless-KEXT
A kext that can be used to disable Rootless in OS X El Capitan/macOS Sierra. You need to sign it OR use an exploit to make OS X load it.
See [https://youtu.be/dq0-0WVGyq4](https://youtu.be/dq0-0WVGyq4)
This kext can be loaded using [this](https://github.com/LinusHenze/anyKextLoader) exploit. (If you're running OS X 10.11 - 10.11.3, was fixed in OS X 10.11.4)

# Building
Just open the Project in Xcode and select Product->Build.

# Using
Copy the kext to a directory of your choice and make sure that the kext has the right permissions (sudo chown -R root:wheel <path to kext>)
As stated above, you need to sign this kext with a valid certificate or you need an exploit ([like this](https://github.com/LinusHenze/anyKextLoader)) to make OS X load it.

# Options
This kext will register debug.rootless.disabled and debug.rootless.csrConfig in sysctl. 
To disable rootless, enter "sysctl debug.rootless.disabled=1", to enable enter "sysctl debug.rootless.disabled=0" and to use your own config enter "sysctl debug.rootless.disabled=2". 
If you choose to use your own config, debug.rootless.csrConfig will become visible and you can enter your own config there (see csr.h for valid configuration values). 
If you would like to use the enable/disable options from csrutil without rebooting to Recovery OS, enter "sysctl debug.rootless.disabled=2 && sysctl debug.rootless.csrConfig=0xE7".

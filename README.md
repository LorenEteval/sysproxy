# sysproxy

[![Build sysproxy binding](https://github.com/LorenEteval/sysproxy/actions/workflows/wheels.yml/badge.svg?branch=main)](https://github.com/LorenEteval/sysproxy/actions/workflows/wheels.yml)

Python bindings for shadowsocks sysproxy utility. This is a Windows-only package.

## Install

```
pip install sysproxy
```

## API

```pycon
>>> import sysproxy
>>> sysproxy.off() # Turn proxy settings off.
True
>>> sysproxy.pac('pac_url') # Turn proxy settings on with PAC.
True
>>> sysproxy.set('127.0.0.1:10809', '127.*;10.*;172.16.*') # Turn proxy settings on with server and bypass.
True
>>> sysproxy.daemon_off() # Turn proxy daemon off.
True
>>> sysproxy.daemon_on_() # Turn proxy daemon on. You should launch this function in a Python thread.
```

## sysproxy daemon

When sysproxy daemon turned on, it executes a window-less WINAPI event loop that captures `WM_QUERYENDSESSION` message,
which is sent when Windows is about to shutdown. When the message arrives, the daemon calls `off()`, which turns proxy
settings off.

As mentioned above, `sysproxy.daemon_on_()` will block current Python execution, so you should launch it in a Python
thread.

## Tested Platform

sysproxy works on all reasonable Windows platform with all Python version(Python 3).

Below are tested build in [github actions](https://github.com/LorenEteval/sysproxy/actions).

| Platform     | Python 3.6-Python 3.11 |
|--------------|:----------------------:|
| windows-2019 |   :heavy_check_mark:   |
| windows-2022 |   :heavy_check_mark:   |

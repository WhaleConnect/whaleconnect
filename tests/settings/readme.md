# Test Settings Directory

This directory should contain the settings file for WhaleConnect's tests, which is excluded from version control.

## File Format

Create a file called `settings.ini` with the following structure:

```ini
[ip]
v4 = [IPv4 address of your server]
v6 = [IPv6 address of your server]

; Ports to use for TCP and UDP testing
tcpPort = 3000
udpPort = 3001

[bluetooth]
mac = [MAC address of your server]

rfcommPort = 1
l2capPSM = 12345
```

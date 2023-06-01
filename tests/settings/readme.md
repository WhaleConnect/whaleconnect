# Test Settings Directory

This directory should contain the settings file for NST's tests, which is excluded from version control.

## File Format

Create a file called `settings.json` with the following structure:

```json
{
    "ip": {
        "v4": "[IPv4 address of your server]",
        "v6": "[IPv6 address of your server]",

        // Ports to use for TCP and UDP testing
        "tcpPort": 3000,
        "udpPort": 3001
    }
}
```

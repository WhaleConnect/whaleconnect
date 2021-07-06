# Network Socket Terminal Source

## The Error List Generation Files

This directory contains a JSON file and a Python file.\
The JSON file contains all the error identifiers and string descriptions.\
The Python file reads this JSON and translates it into C++ code. This is outputted to `errorlist.cpp`.

## Format of the JSON File

There are three sections of the JSON data: "crossplatform", "windows", and "unix". These associate error identifiers to descriptions. They contain entries specific to either Windows or Unix (or both), their names are self-explanatory.\
Each entry has the form `"errName": "descString"` which associates the error `errName` to its description `descString`. For example:

```json
"EINTR": "An operation was interrupted by a signal."
```

associates the error `EINTR` to its description "An operation was interrupted by a signal."

\
There can also be Unix errors with Winsock equivalents that don't follow the standard naming convention, which is "WSA" prepended to the name [i.e. `ENOMEM` (Unix) => `WSAENOMEM` (Winsock)].\
For example, the Unix error `ENODATA` is equal to the Winsock error `WSANO_DATA`. This type of unconventional name is called a "remapped name", or RN. *(This is specific to these generator scripts.)*\
To add an RN to an error entry, use the syntax below:

```json
// General form (note no spaces):
"errName:remappedName": "descString"

// Example (`ENODATA` gets remapped to `WSANO_DATA` on Windows):
"ENODATA:WSANO_DATA": "[desc here]"
```

If the Unix error does not have an RN, like `ENOMEM`, it is not added and the entry is left as is.

## Generation

In the "crossplatform" section, which contains errors that exist on both Windows and Unix, Unix-style error names are used (no "WSA" prefix). The Python generator script will insert the correct `#undef`+`#define` directives to make sure a Windows compiler understands these names.\
This is where the RNs come in - if the script sees an error name with one, it will define it to be the same as the one specified.\
Using the example of `ENODATA`, the output is:

```cpp
#undef ENODATA
#define ENODATA WSANO_DATA
```

If the error name does not have an RN, the script assumes the standard naming convention. For example, with `ENOMEM`:

```cpp
#undef ENOMEM
#define ENOMEM WSAENOMEM
```

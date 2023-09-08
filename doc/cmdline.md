# Kernel Command line
In addition to a static built-in kernel command line, stubby supports appending runtime arguments to the kernel command line.

## Runtime command line insertion
The runtime command line options functionality is described below.  It makes use of a marker (`STUBBY_RT_CLI1`) in the builtin kernel command line to indicate where runtime parameters are placed.

 * If there is no builtin command line, use runtime. By not providing a builtin command line, the author is intently desiring runtime parameters.
 * If builtin command line is set to an empty string ("") or does not contain a marker (STUBBY_RT_CLI1), and runtime value is not an empty string, then reject boot
 * If builtin command line is set and contains the string STUBBY_RT_CLI1 replace the marker with the runtime arguments, using an empty string if none are given.
 * stubby gives no special consideration to the value of builtin or runtime at this stage. For example, '--' is treated the same as 'console=X' or any other string.
 * If the string STUBBY_RT is found in the builtin value, but not as part of STUBBY_RT_CLI1, then fail. This is to protect against accidental signing or potentially a different future version with different behavior.
 * STUBBY_RT_CLI1 must be a complete token; if STUBBY_RT_CLI10 or STUBBY_RT_CLI1console=ttyS1 are found in the builtin cmdline, boot will be rejected.
 * STUBBY_RT_CLI1 cannot occur more than once in the builtin; if multiple instances occur boot will be rejected
 * If STUBBY_RT is found in runtime arguments, boot will be rejected.

The table below gives examples and behavior.

 * 'allowed' indicates that the command line will be compared against the supplied allowed-list.  If false, execution will stop with error immediately.
 * 'result cmdline' indicates the command line that will be passed to validation.
 * <empty> indicates empty string "" (no other value)
 * <any> indicates any value including empty string.

| builtin                              | runtime                | allowed? | result cmdline                   |
| ------------------------------------ | --------------------   | -------  | -------------------------------- |
| console=ttyS0                        | \<empty>               | Y        | console=ttyS0                    |
| empty                                | console=ttyS0          | Y        | console=ttyS0                    |
| console=ttyS0                        | \<any>                 | N        | :x:                              |
| console=ttyS0 STUBBY_RT_CLI1 -\- 3   | console=tty1 STUBBY_RT | N        | :x:                              |
| console=ttyS0 STUBBY_RT_CLI1 -\- 3   | console=tty1           | Y        | console=ttyS0 console=tty1 -\- 3 |
| STUBBY_RT_CLI1 console=tty1 acpi=off | acpi=on console=tty1   | Y        | acpi=on console=tty1 acpi=off    |
| STUBBY_RT_CLI1 console=tty1 acpi=off | acpi=on console=tty1   | Y        | acpi=on console=tty1 acpi=off    |
| STUBBY_RT_CLI1console=ttyS0          | \<any>                 | N        | :x:                              |
| console=STUBBY_RT_CLI1,115200        | <any>                  | N        | :x:                              |
| STUBBY_RT_CLI1 console=ttyS0 STUBBY_RT_CLI1 foo=bar | \<any>  | N        | :x:                              |


## Runtime allowed-list support
The `allowed` list is an array of strings.
Each entry in the array is either:
1. An exact value that is allowed (such as `verbose`).
2. A prefix value using the '^' (such as `^console=`).

The resultant command line after substitution is split into tokens on ' '.
Each token is then checked against the allowed list.

Exact values must match a complete token in the result command line.
The value 'verbose' will only match the command line token 'verbose'
not 'verbosity' or 'noverbose'.

Prefix values match only at the beginning of a token. `^console=t` will
match 'console=ttyS0' and 'console=tty0',
but not 'console=serial' or 'vgaconsole=target'.

## Updating allowed list
The allowed list is currently statically defined in kcmdline.c in the 'allowed'
constant.

Updating it requires patching that list.

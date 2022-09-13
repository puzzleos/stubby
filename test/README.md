# OVMF Boot VM test

Running test consists of
 * collecting the inputs to test
 * running test harness

## Collecting Inputs
Getting inputs in order to run test suite

    $ mkdir inputs
    $ cd inputs

    # As of 2022-09 this only works on Ubuntu focal due to
    # https://bugs.launchpad.net/bugs/1986692
    $ sudo ../tools/collect-firmwares .

    $ ../tools/get-krd .

### Detailed info.
Inputs to the test harness are:

 * firmwares - this includes OVMF vars and OVMF code files as well as signing keys.
   These can be created by running with 'test/collect-firmwares' on ubuntu.  The
   collect-firmwares program installs the required packages and collects the required
   files.

       sudo ./tools/collect-firmwares firmwares

   Files used from the firmwares are:

    *  firmwares/ovmf-insecure-code.fd
    *  firmwares/ovmf-insecure-vars.fd
    *  firmwares/ovmf-secure-vars.fd
    *  firmwares/ovmf-secure-code.fd
    *  firmwares/signing.key - encrypted signing key.
    *  firmwares/signing.pem - signing certificate. executables signed by this certificate
       are expected to run in secureboot mode inside vm using ovmf-secure files.
    *  firmwares/signing-unlocked.key - an unlocked (no password version) of signingkey
    *  firmwares/signing.password - contains plaintext password to unlock the signing.key
    *  firmwares/firmware-info.yaml - provides information on where files came from.

 * kernel and initramfs.  These can be downloaded with 'get-krd'.  The kernel and initramfs
   are smashed into a stubby executable and executed from EFI.

    * kernel
    * initramfs - the initramfs is expected to print the kernel command line and power off system.


## Running test
Typical running of test would be:

    $ ./test/harness run \
        --inputs=./inputs --results=./results \
        --sbat=sbat.csv --stubby=stubby.efi \
        test/tests.yaml

That will execute the tests described in tests.yaml using the provided
inputs, sbat, stubby and write results to results/.


## test 'mode'
Based on the 'shim' value in test data, the harness will execute either:

  * `shim.efi kernel.efi [options]` or
  * `kernel.efi [options]`

The tests can execute that "first loader" program in one of 2 modes.

  * **uefi-shell**: This uses a startup.nsh style script to execute
    either shim.efi or kernel.efi. The efi shell (at least from ovmf)
    executes programs differently than nvram.

  * nvram: This uses a startup.nsh script to control the boot order
    via the 'bcmd' command in the efi shell.  The desired boot parameter
    are added to an nvram entry and then a reboot is done.  This path
    is more similar to what you would get on an installed system.

## test.yaml structure
See the comment at the top of tests.yaml for more information.

Notes:
 * jammy ovmf files with snakeoil are broken
   https://bugs.launchpad.net/ubuntu/+source/edk2/+bug/1986692
 * each run/<testdir> has a 'boot' script that can be run and will run the
   test interactively.  that's nice for getting to sehll and seeing things.
 * in the future it'd be nice if the boot script could potentially re-generate the
   esp.img if you made changes.
 * qemu is executed with 'snapshot' on for each of the modifiable items
   (efi image and nvram).  This means that if you kept a working directory
   around to use 'boot', the files will be in their original state.

 * there are really 3 "modes" of boot.  "first loader" indicates either shim.efi
   (which then loads kernel.efi) or kernel.efi directly:
   * first loader specified in nvram entry (bcfg or efibootmgr)
   * first loader executed from startup.nsh script
   * first loader named bootx64.efi finds grubx64.efi .. no current way for
     cmdline arguments this way.  This is how a removable media would operate.
     We'd talked about potentially having a simplistic menu or something to
     read a file and pick one from a list.

 * In 'uefi-shell' boot mode:
   * startup.nsh is selected for execution by the bootloader (special well known name)
   * startup.nsh runs 'launch.nsh' - this is because a .nsh script will
     effectively run with "set -e" unless it is called by another nsh
     script.  This is crazy but true.  So instead of just having
     startup.nsh launch our efi directly, we have it launch launch.nsh

     after executing launch.nsh, startup.nsh will shutdown.

   * launch.nsh will either execute launches the "first loader"

# podio

[![linux](https://github.com/AIDASoft/podio/actions/workflows/test.yml/badge.svg)](https://github.com/AIDASoft/podio/actions/workflows/test.yml)
[![ubuntu](https://github.com/AIDASoft/podio/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/AIDASoft/podio/actions/workflows/ubuntu.yml)
[![coverity](https://scan.coverity.com/projects/22634/badge.svg)](https://scan.coverity.com/projects/aidasoft-podio)

## Documentation

 - [Introduction](./doc/doc.md)
 - [Design](./doc/design.md)
 - [Data Model Syntax](./doc/datamodel_syntax.md)
 - [Examples](./doc/examples.md)
 - [Advanced Topics](./doc/advanced_topics.md)
 - [Contributing](./doc/contributing.md)

<!-- Browse the API documentation created with Doxygen at -->

<!-- [http://fccsw.web.cern.ch/fccsw/podio/index.html](http://fccsw.web.cern.ch/fccsw/podio/index.html). -->

## Prerequisites

If you are on lxplus, all the necessary software is preinstalled if you
use a recent LCG or FCC stack release.

On Mac OS or Ubuntu, you need to install the following software.

### ROOT 6.08.06

Install ROOT 6.08.06 (or later) and set up your ROOT environment:

    source <root_path>/bin/thisroot.sh

### Catch2 v3 (optional)

By default Podio requires that a static [Catch2 v3](https://github.com/catchorg/Catch2/tree/devel) library is available and the unittests will be built against that.
Using the `-DUSE_EXTERNAL_CATCH2=OFF` to configure CMake will instead fetch an appropriate version of the library and build it on the fly.

### Python > 3.6

Check your Python version by doing:

    python --version

or

    python3 --version

depending on your used system.

#### python packages

Podio requires the `yaml` and `jinja2` python modules.
Check that the `yaml` and `jinja2` python modules are available

    python
    >>> import yaml
    >>> import jinja2

If the import goes fine (no message), you're all set. If not, the necessary modules need to be installed. This is most easily done via (first install pip if you don't have it yet)

    pip install -r requirements.txt

In order for the `yaml` module to be working it might also be necessary to install the C++ yaml library. On Mac OS, The easiest way to do that is to use homebrew (install homebrew if you don't have it yet):

    brew install libyaml

Check that you can now import the `yaml` and `jinja2` modules in python.

## Preparing the environment

Full use of PODIO requires you to set the `PODIO` environment variable
and modify `LD_LIBRARY_PATH` and `PYTHONPATH`. Some convenience scripts
are provided:

    # Set PODIO install area to `./install` and setup environment accordingly
    source ./init.sh

or

    # Setup environment based on current value of `PODIO`
    source ./env.sh

Or you can setup the environment entirely under your control: see `init.sh`
and `env.sh`.

## Compiling

If you are using the easy setup from `init.sh` then create separate build
and install areas, and trigger the build:

    mkdir build
    mkdir install
    cd build
    cmake -DCMAKE_INSTALL_PREFIX=../install ..
    make -j 4 install

To see a list of options, do this in the build-directory:

    cmake -LH ..

## Running

The examples are for creating a file "example.root",

    tests/write

and reading it again in C++,

    tests/read

It is also possible to read the file again using the
[`tests/read.py`](tests/read.py) script. Make sure to have run `init.sh`
and `env.sh` first. Additionally you have to make `ROOT` aware of
`libTestDataModel.so` to be able to read in the data types defined there. There
are two ways to do that. First it is possible to add the directory to the
`LD_LIBRARY_PATH`

    export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:$(pwd)/tests

Secondly it is also possible to explicitly load the library via `ROOT` in the
script by adding the following lines *after* the imports from `__future__`

```python
import os
BUILD_PATH = os.getcwd()

import ROOT
ROOT.gSystem.Load(os.path.join(BUILD_PATH, 'tests/libTestDataModel.so'))
```

Either of the two versions allows you to now read the `example.root` file again
using

     python ../tests/read.py

## Installing using SPACK

A recipe for building podio is included with the [spack package manager](https://github.com/spack/spack/blob/develop/var/spack/repos/builtin/packages/podio/package.py), so podio can also installed with:

```
spack install podio
```

Note that if you do not have any previous installations or registered system packages, this will compile ROOT and all its dependencies, which may be time-consuming.

## Modifying the data model

Podio features an example event data model, fully described in the yaml file
[tests/datalayout.yaml](tests/datalayout.yaml).
The C++ in [tests/datamodel/](tests/datamodel/) has been fully generated by a code generation script, [python/podio_class_generator.py](python/podio_class_generator.py).

To run the code generation script, do

    python ../python/podio_class_generator.py ../tests/datalayout.yaml ../Tmp data ROOT

The generation script has the following additional options:

- `--clangformat` (`-c`): Apply clang-format after file creation (uses [option `-style=file`](https://clang.llvm.org/docs/ClangFormatStyleOptions.html) with llvm as backup style), needs clang-format in `$PATH`.
- `--quiet` (`-q`): Suppress all print out to STDOUT
- `--dryrun` (`-d`): Only run the generation logic and validate yaml, do not write files to disk

## Running tests
After compilation you can run rudimentary tests with

    make test

## Running workflows
To run workflows manually (for example, when working on your own fork) go to
`Actions` then click on the workflow that you want to run (for example
`edm4hep`). Then if the workflow has the `workflow_dispatch` trigger you will be
able to run it by clicking `Run workflow` and selecting on which branch it will
run.

## Advanced build topics

It is possible to instrument the complete podio build with sanitizers using the
`USE_SANITIZER` cmake option. Currently `Address`, `Memory[WithOrigin]`,
`Undefined` and `Thread` are available. Given the sanitizers limitations they
are more or less mutually exclusive, with the exception of `Address;Undefined`.
Currently some of the tests will fail with sanitizers enabled
([issue](https://github.com/AIDASoft/podio/issues/250)). In order to allow for a
smoother development experience with sanitizers enabled, these failing tests are
labelled (e.g. `[ASAN-FAIL]` or `[THREAD-FAIL]`) and will not be run by default
if the corresponding sanitizer is enabled. It is possible to force all tests to
be run via the `FORCE_RUN_ALL_TESTS` cmake option.

## Model visualization

There is a tool to generate a diagram of the relationships between the elements
in a model. To generate a diagram run `python python/tools/podio-vis model.yaml`
and use `--help` for checking the possible options. In particular there is the
option `--graph-conf` that can be used to pass a configuration file defining
groups that will be clustered together in the diagram, like it is shown in. The
syntax is
```
group label:
  - datatype1
  - datatype2
another group label:
  - datatype3
  - datatype4
```

Additionally, it is possible to remove from the diagram any
data type (let's call it `removed_datatype`) by adding to this configuration file:
```
Filter:
  - removed_datatype
```

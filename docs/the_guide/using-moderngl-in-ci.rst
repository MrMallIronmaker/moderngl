Using Moderngl in CI
====================

Windows CI Configuration
------------------------

ModernGL can't be run directly on Windows CI without the use of `Mesa`_. To get ModernGL running
you should first install Mesa from the `MSYS2 project`_ and adding it to the ``PATH``.

Steps
_____

1. Usually `MSYS2 project`_ should be installed by default by your CI provider in ``C:\msys64``. You 
   can refer `the documentation <https://www.msys2.org/docs/ci/>`_ on how to get it installed and make 
   sure to update it.

2. Then login through bash and enter ``pacman -S --noconfirm mingw-w64-x86_64-mesa``.
    
   .. code-block:: pwsh
      
       C:\msys64\usr\bin\bash -lc "pacman -S --noconfirm mingw-w64-x86_64-mesa"
   
   This will install Mesa binary, which moderngl would be using.
    
3. Then add ``C:\msys64\mingw64\bin`` to ``PATH``, keeping it at top.
    
   .. code-block:: pwsh
   
       $env:PATH = "C:\msys64\mingw64\bin;$env:PATH"

    .. warning::
    
        Make sure to delete ``C:\msys64\mingw64\bin\python.exe`` if it exists because the python provided
        by them would then be added to Global and some unexpected things may happen.
     
4. Finally use below script before running moderngl, possibly in ``__init__.py`` of ``tests``, to make sure
   that the ``C:\msys64\mingw64\bin\opengl32.dll`` it loaded correctly. The below script is in the assumption
   that when running in CI the environment variable `CI` is set.
   
   .. code-block:: py
   
        import os
        if os.getenv("CI") and os.name == "nt":
            import ctypes
            # Change the statement to the location of MSYS2 installation.
            location = r"C:\msys64\mingw64\bin"
            # add the in front of PATH. Usually usefull for python<3.8
            os.environ["PATH"] = location + os.pathsep + os.getenv("PATH")
            # Load the `dll` first so that it is saved in memory
            ctypes.CDLL(f"{location}\\opengl32.dll")
            if hasattr(os, "add_dll_directory"):
                # this is so that moderngl loads the correct file
                # only applicable for python>3.8
                os.add_dll_directory(location)

5. Then you can run moderngl as you want to.

.. _Mesa: https://mesa3d.org/
.. _MSYS2 project: https://www.msys2.org/

Example Configuration
_____________________

A example configuration for Github Actions:

.. code-block:: yml

    name: Hello World
    on: [push, pull_request]

    jobs:
      build:
        runs-on: windows-latest
        steps:
          - uses: actions/checkout@v2
          - name: Set up Python
            uses: actions/setup-python@v2
            with:
              python-version: 3.9
          - uses: msys2/setup-msys2@v2
            with:
              msystem: MINGW64
              release: false
              install: mingw-w64-x86_64-mesa
          - name: Test using ModernGL
            shell: pwsh
            run: |
              Remove-Item C:\msys64\mingw64\bin\python.exe -Force
              python -m pip install -r requirements.txt
              python -m pytest
              
Linux
-----

For running ModernGL on Linux CI, you would need to configure ``xvfb`` so that it starts a Window in the background.
After that, you should be able to use ModernGL directly.

Steps
_____

1. Install ``xvfb`` from Package Manager.

   .. code-block:: bash
        
        sudo apt-get -y install xvfb

2. The run the below command, to start Xvfb from background.

   .. code-block:: bash
    
        sudo /usr/bin/Xvfb $DISPLAY -screen 0 1280x1024x24 &

3. You can run ModernGL now.

Example Configuration
_____________________

A example configuration for Github Actions:

.. code-block:: yml

    name: Hello World
    on: [push, pull_request]

    jobs:
      build:
        runs-on: ubuntu-latest
        steps:
          - uses: actions/checkout@v2
          - name: Set up Python
            uses: actions/setup-python@v2
            with:
              python-version: 3.9
          - name: Prepare
            run: |
                sudo apt-get -y install xvfb
                sudo /usr/bin/Xvfb $DISPLAY -screen 0 1280x1024x24 &            
          - name: Test using ModernGL
            run: |
              python -m pip install -r requirements.txt
              python -m pytest

macOS
-----

You won't need any specialy configuration to run on macOS.


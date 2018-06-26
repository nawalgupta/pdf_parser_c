This program extract pdf file, save data to json file to use later. It use poppler as PDF parser.

**Following steps of setup instructions is for reference, adjust yourself with _your Linux distro_ and folder structure.**


To checkout:
```commandline
git clone --recursive https://github.com/robinson0812/pdf_parser_c pdf_reader
```

Due to bug [pdftotext only outputs first page content with -bbox-layout option](https://bugs.freedesktop.org/show_bug.cgi?id=93344), after checkout poppler submodule, apply patch to fix:


```commandline
cd poppler
git apply ../0001-Remove-ActualText-class-and-fix-reference-count.patch
```

Then build and verify poppler with command:
```commandline
cd ..
mkdir poppler-build
cd poppler-build
cmake ../pdf_reader/poppler -DCMAKE_BUILD_TYPE=Release -DTESTDATADIR=../pdf_reader/poppler-test
make
make test
sudo make install
```

Copy poppler config header to source folder
```commandline
cp -vf ../poppler-build/config.h poppler-header
cp -vf ../poppler-build/poppler-config.h poppler-header
```

Build project
```commandline
cd ..
mkdir -p pdf_reader-build
cd pdf_reader-build
cmake ../pdf_reader -DCMAKE_BUILD_TYPE=Release
make
sudo cp -v pdf_reader /usr/bin
```

Run with 
```commandline
LD_LIBRARY_PATH=/usr/local/lib64 pdf_reader -x 25 -y 35 -W 515 -H 752 file.pdf
```

Or in Ubuntu (look at poppler install output for more detail)
```commandline
LD_LIBRARY_PATH=/usr/local/lib pdf_reader -x 25 -y 35 -W 515 -H 752 file.pdf
```

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

Build project with poppler lib at /usr/local/lib64, run with 
```commandline
LD_LIBRARY_PATH=/usr/local/lib64 ./pdf_reader -x 25 -y 35 -W 515 -H 752 file.pdf
```

This program extract pdf file, save data to json file to use later. It use poppler as PDF parser.

**Following steps of setup instructions is for reference, adjust yourself with _your Linux distro_ and folder structure to not mess up the source code directory.**

[PDF Implementation Reference](https://www.adobe.com/content/dam/acom/en/devnet/pdf/pdfs/pdf_reference_archives/PDFReference.pdf)

Step 1:
- checkout pdf_reader source code
  

Step 2:
- install poppler-data and poppler at [freedesktop git](https://anongit.freedesktop.org/git/poppler/)
  - checkout poppler-data
  - install poppler-data
  - checkout poppler
  - Due to bug [pdftotext only outputs first page content with -bbox-layout option](https://bugs.freedesktop.org/show_bug.cgi?id=93344), apply patch to fix: `git apply poppler.patch`, poppler.patch is in pdf_reader checkout folder
  - run cmake using following configuration: `cmake poppler_checkout_directory -DCMAKE_BUILD_TYPE=Release -DENABLE_XPDF_HEADERS=ON`
  - (optional) install packages: `sudo apt install libfreetype6-dev libfontconfig1-dev libnss3-dev libjpeg-dev libopenjp2-7-dev` or config to disable all of it. You can use cmake-gui and disable all additional features easily
  - make test and make install

Step 3:
- checkout, config, make and install nlohmann/json

Step 4: 
- config and make pdf_reader

Done!

Run with 
```commandline
LD_LIBRARY_PATH=/usr/local/lib64 pdf_reader file.pdf
```

Or in Ubuntu (look at poppler install output for more detail)
```commandline
LD_LIBRARY_PATH=/usr/local/lib pdf_reader file.pdf
```

pipeline {
  agent {
    docker {
      args '-v /data:/data'
      image 'ubuntu'
    }

  }
  stages {
    stage('Packages Preparation') {
      steps {
        sh '''apt-get update
DEBIAN_FRONTEND=noninteractive apt-get -yq install git'''
        sh '''mkdir -p $HOME/source/poppler
cd "$HOME/source/poppler"
git clone https://anongit.freedesktop.org/git/poppler/poppler-data.git
git clone https://anongit.freedesktop.org/git/poppler/test
git clone https://anongit.freedesktop.org/git/poppler/poppler.git
cd poppler
git apply $WORKSPACE/poppler.patch'''
        sh '''apt-get update
DEBIAN_FRONTEND=noninteractive apt-get -yq install cmake build-essential libfreetype6-dev libfontconfig1-dev'''
        sh '''mkdir -p $HOME/source/poppler-build
cd "$HOME/source/poppler-build"
mkdir poppler-data
cd poppler-data
cmake ../../poppler/poppler-data
make
make install
cd ..
mkdir poppler
cd poppler
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_VERBOSE_MAKEFILE=TRUE -DENABLE_CPP=FALSE -DENABLE_DCTDECODER=none -DENABLE_GLIB=FALSE -DENABLE_GOBJECT_INTROSPECTION=FALSE -DENABLE_GTK_DOC=FALSE -DENABLE_LIBCURL=FALSE -DENABLE_LIBOPENJPEG=none -DENABLE_QT5=FALSE -DENABLE_SPLASH=FALSE -DENABLE_UTILS=FALSE -DENABLE_XPDF_HEADERS=TRUE -DENABLE_ZLIB=FALSE -DENABLE_ZLIB_UNCOMPRESS=FALSE -DWITH_Cairo=FALSE -DWITH_JPEG=FALSE -DWITH_NSS3=FALSE -DWITH_PNG=FALSE -DWITH_TIFF=FALSE ../../poppler/poppler'''
      }
    }
  }
}
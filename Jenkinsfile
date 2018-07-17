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
DEBIAN_FRONTEND=noninteractive apt-get -yq install cmake build-essential'''
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
cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ../../poppler/poppler
cmake -LA | awk \'{if(f)print} /-- Cache values/{f=1}\''''
      }
    }
  }
}
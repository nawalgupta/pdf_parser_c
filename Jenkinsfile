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
        sh 'mkdir -p $HOME/source'
        sh '''cd "$HOME/source"
mkdir poppler
cd poppler
git clone \'https://anongit.freedesktop.org/git/poppler/poppler-data.git
git clone \'https://anongit.freedesktop.org/git/poppler/test\'
git clone \'https://anongit.freedesktop.org/git/poppler/poppler.git
cd poppler
git apply $WORKSPACE/poppler.patch'''
      }
    }
  }
}
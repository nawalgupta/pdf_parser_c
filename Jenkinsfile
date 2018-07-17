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
        dir(path: "$HOME/source") {
          git 'https://anongit.freedesktop.org/git/poppler/poppler-data.git'
          git 'https://anongit.freedesktop.org/git/poppler/test'
          git 'https://anongit.freedesktop.org/git/poppler/poppler.git'
          sh '''env
ls -la
ls -la $WORKSPACE'''
        }

      }
    }
  }
}
pipeline {
  agent {
    docker {
      image 'ubuntu'
      args '-v /data:/data'
    }

  }
  stages {
    stage('Packages Preparation') {
      steps {
        sh '''apt update
DEBIAN_FRONTEND=noninteractive apt-get -yq install git'''
      }
    }
  }
}
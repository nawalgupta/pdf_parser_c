pipeline {
  agent {
    docker {
      args '-v /data:/data'
      image 'ubuntu:16.04'
    }

  }
  stages {
    stage('Packages Preparation') {
      steps {
        sh 'pwd'
      }
    }
  }
}

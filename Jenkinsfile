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
        ws(dir: 'source') {
          dir(path: 'poppler') {
            sh 'mkdir poppler'
            dir(path: 'poppler') {
              sh 'pwd'
            }

          }

        }

      }
    }
  }
}
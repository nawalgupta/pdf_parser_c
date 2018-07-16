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
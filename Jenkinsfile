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
        node(label: 'docker') {
          sh 'apt -y install git'
          ws(dir: 'source') {
            git(url: 'https://github.com/robinson0812/pdf_parser_c.git', branch: 'document_tree')
          }

        }

      }
    }
  }
}
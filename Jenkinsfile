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
        git(url: 'https://github.com/robinson0812/pdf_parser_c.git', branch: 'document_tree')
      }
    }
  }
}
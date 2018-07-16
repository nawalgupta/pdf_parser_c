pipeline {
  agent {
    docker {
      image 'ubuntu'
      args '-v /data:/data'
    }

  }
  stages {
    stage('Prapare environment') {
      steps {
        git(url: 'https://github.com/robinson0812/pdf_parser_c.git', branch: 'document_tree')
        git(url: 'https://anongit.freedesktop.org/git/poppler/poppler-data.git', branch: 'master')
        git(url: 'https://anongit.freedesktop.org/git/poppler/test', branch: 'master')
        git(url: 'https://anongit.freedesktop.org/git/poppler/poppler.git', branch: 'master')
        sh '''apt -y install git
cd poppler
git apply ../pdf_parser_c/poppler.patch'''
      }
    }
  }
}
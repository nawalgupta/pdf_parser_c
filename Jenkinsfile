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
        ws(dir: 'source') {
          git(url: 'https://github.com/robinson0812/pdf_parser_c.git', branch: 'document_tree')
          git 'https://anongit.freedesktop.org/git/poppler/poppler-data.git'
          git 'https://anongit.freedesktop.org/git/poppler/test'
          git 'https://anongit.freedesktop.org/git/poppler/poppler.git'
          sh '''cd poppler
git apply ../pdf_parser_c/poppler.patch'''
        }

      }
    }
  }
}
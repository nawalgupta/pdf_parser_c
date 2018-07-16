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
apt-get install git'''
        sh 'mkdir -p /home/source'
        dir(path: '/home/source') {
          git 'https://anongit.freedesktop.org/git/poppler/poppler-data.git'
          git 'https://anongit.freedesktop.org/git/poppler/test'
          git 'https://anongit.freedesktop.org/git/poppler/poppler.git'
          sh '''echo ${WORKSPACE}
cd poppler
git apply ${WORKSPACE}/poppler.patch'''
        }

      }
    }
  }
}
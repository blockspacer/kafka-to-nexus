@Library('ecdc-pipeline')
import ecdcpipeline.ContainerBuildNode
import ecdcpipeline.PipelineBuilder

<<<<<<< HEAD
container_build_nodes = [
  'centos7-release': new ContainerBuildNode('essdmscdm/centos7-build-node:4.2.0', '/usr/bin/scl enable devtoolset-6 -- /bin/bash -e')
=======
project = "kafka-to-nexus"


// 'no_graylog' builds code with plain spdlog conan package instead of ess-dmsc spdlog-graylog.
// It fails to build in case graylog functionality was used without prior checking if graylog was available.
clangformat_os = "debian9"
test_and_coverage_os = "centos7"
release_os = "centos7-release"
no_graylog = "centos7-no_graylog"

container_build_nodes = [
  'centos7': new ContainerBuildNode('essdmscdm/centos7-build-node:4.0.0', '/usr/bin/scl enable devtoolset-6 -- /bin/bash -e'),
  'centos7-release': new ContainerBuildNode('essdmscdm/centos7-build-node:4.0.0', '/usr/bin/scl enable devtoolset-6 -- /bin/bash -e'),
  'centos7-no_graylog': new ContainerBuildNode('essdmscdm/centos7-build-node:4.0.0', '/usr/bin/scl enable devtoolset-6 -- /bin/bash -e'),
  'debian9': new ContainerBuildNode('essdmscdm/debian9-build-node:2.6.0', 'bash -e'),
  'ubuntu1804': new ContainerBuildNode('essdmscdm/ubuntu18.04-build-node:1.4.0', 'bash -e')
>>>>>>> master
]

// Set number of old builds to keep.
properties([[
  $class: 'BuildDiscarderProperty',
  strategy: [
    $class: 'LogRotator',
    artifactDaysToKeepStr: '',
    artifactNumToKeepStr: '3',
    daysToKeepStr: '',
    numToKeepStr: ''
  ]
]]);


pipeline_builder = new PipelineBuilder(this, container_build_nodes)
pipeline_builder.activateEmailFailureNotifications()

builders = pipeline_builder.createBuilders { container ->
  pipeline_builder.stage("${container.key}: Checkout") {
    dir(pipeline_builder.project) {
      scm_vars = checkout scm
    }
    container.copyTo(pipeline_builder.project, pipeline_builder.project)
  }  // stage

  pipeline_builder.stage("${container.key}: Dependencies") {
    def conan_remote = "ess-dmsc-local"
    if (container.key == no_graylog) {
      container.sh """
        mkdir build
          cd build
          conan remote add \
            --insert 0 \
            ${conan_remote} ${local_conan_server}
          conan install --build=outdated ../${pipeline_builder.project}/conan/conanfile_no_graylog.txt
        """
    } else {
    container.sh """
      mkdir build
      cd build
      conan remote add \
        --insert 0 \
        ${conan_remote} ${local_conan_server}
      conan install --build=outdated ../${pipeline_builder.project}/conan/conanfile.txt
    """
    }
  }  // stage

  pipeline_builder.stage("${container.key}: Configure") {
<<<<<<< HEAD
=======
    if (container.key != release_os && container.key != no_graylog) {
      def coverage_on
      if (container.key == test_and_coverage_os) {
        coverage_on = '-DCOV=1'
      } else {
        coverage_on = ''
      }

      container.sh """
        cd build
        . ./activate_run.sh
        cmake ${coverage_on} ../${pipeline_builder.project}
      """
    } else {
>>>>>>> master
      container.sh """
        cd build
        . ./activate_run.sh
        cmake \
          -DCMAKE_BUILD_TYPE=Release \
          -DCONAN=MANUAL \
          -DCMAKE_SKIP_RPATH=FALSE \
          -DCMAKE_INSTALL_RPATH='\\\\\\\$ORIGIN/../lib' \
          -DCMAKE_BUILD_WITH_INSTALL_RPATH=TRUE \
          ../${pipeline_builder.project}
      """
  }  // stage

  pipeline_builder.stage("${container.key}: Build") {
    container.sh """
      cd build
      . ./activate_run.sh
      make -j4 all UnitTests VERBOSE=1
    """
  }  // stage

  pipeline_builder.stage("${container.key}: Test") {
    container.sh """
      cd build
      . ./activate_run.sh
      ./bin/UnitTests
    """
  }  // stage

<<<<<<< HEAD
  pipeline_builder.stage("${container.key}: Archiving") {
    def archive_output = "${pipeline_builder.project}-${container.key}.tar.gz"
    container.sh """
      cd build
      rm -rf ${pipeline_builder.project}; mkdir ${pipeline_builder.project}
      mkdir ${pipeline_builder.project}/bin
      cp ./bin/{kafka-to-nexus,send-command} ${pipeline_builder.project}/bin/
      cp -r ./lib ${pipeline_builder.project}/
      cp -r ./licenses ${pipeline_builder.project}/
      tar czf ${archive_output} ${pipeline_builder.project}

      # Create file with build information
      touch BUILD_INFO
      echo 'Repository: ${pipeline_builder.project}/${env.BRANCH_NAME}' >> BUILD_INFO
      echo 'Commit: ${scm_vars.GIT_COMMIT}' >> BUILD_INFO
      echo 'Jenkins build: ${env.BUILD_NUMBER}' >> BUILD_INFO
    """
=======
  if (container.key == clangformat_os) {
    pipeline_builder.stage("${container.key}: Formatting") {
      if (!env.CHANGE_ID) {
        // Ignore non-PRs
        return
      }
      try {
        container.sh """
          clang-format -version
          cd ${project}
          find . \\\\( -name '*.cpp' -or -name '*.cxx' -or -name '*.h' -or -name '*.hpp' \\\\) \\
          -exec clang-format -i {} +
          git config user.email 'dm-jenkins-integration@esss.se'
          git config user.name 'cow-bot'
          git status -s
          git add -u
          git commit -m 'GO FORMAT YOURSELF'
        """
        withCredentials([
          usernamePassword(
          credentialsId: 'cow-bot-username',
          usernameVariable: 'USERNAME',
          passwordVariable: 'PASSWORD'
          )
        ]) {
          container.sh """
            cd ${project}
            git push https://${USERNAME}:${PASSWORD}@github.com/ess-dmsc/kafka-to-nexus.git HEAD:${CHANGE_BRANCH}
          """
        } // withCredentials
      } catch (e) {
       // Okay to fail as there could be no badly formatted files to commit
      } finally {
        // Clean up
      }
    }  // stage

    pipeline_builder.stage("${container.key}: Cppcheck") {
      def test_output = "cppcheck.txt"
      container.sh """
        cd ${pipeline_builder.project}
        cppcheck --enable=all --inconclusive --template="{file},{line},{severity},{id},{message}" src/ 2> ${test_output}
      """
      container.copyFrom("${pipeline_builder.project}/${test_output}", '.')
      step([
        $class: 'WarningsPublisher',
        parserConfigurations: [[
          parserName: 'Cppcheck Parser',
          pattern: 'cppcheck.txt'
        ]]
      ])
    }  // stage
  }  // if

  if (container.key == release_os) {
    pipeline_builder.stage("${container.key}: Formatting") {
      def archive_output = "${pipeline_builder.project}-${container.key}.tar.gz"
      container.sh """
        cd build
        rm -rf ${pipeline_builder.project}; mkdir ${pipeline_builder.project}
        mkdir ${pipeline_builder.project}/bin
        cp ./bin/{kafka-to-nexus,send-command} ${pipeline_builder.project}/bin/
        cp -r ./lib ${pipeline_builder.project}/
        cp -r ./licenses ${pipeline_builder.project}/
        tar czf ${archive_output} ${pipeline_builder.project}

        # Create file with build information
        touch BUILD_INFO
        echo 'Repository: ${pipeline_builder.project}/${env.BRANCH_NAME}' >> BUILD_INFO
        echo 'Commit: ${scm_vars.GIT_COMMIT}' >> BUILD_INFO
        echo 'Jenkins build: ${env.BUILD_NUMBER}' >> BUILD_INFO
      """
>>>>>>> master

    container.copyFrom("build/${archive_output}", '.')
    container.copyFrom('build/BUILD_INFO', '.')
    archiveArtifacts "${archive_output},BUILD_INFO"
  }  // stage
}  // createBuilders

node {
  scm_vars = checkout scm

  try {
    parallel builders
  } catch (e) {
    pipeline_builder.handleFailureMessages()
    throw e
  }

  // Delete workspace when build is done
  cleanWs()
}

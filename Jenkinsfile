@Library('ecdc-pipeline')
import ecdcpipeline.ContainerBuildNode
import ecdcpipeline.PipelineBuilder

container_build_nodes = [
  'centos7-release': new ContainerBuildNode('essdmscdm/centos7-build-node:3.4.0', '/usr/bin/scl enable rh-python35 devtoolset-6 -- /bin/bash -e')
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
    container.sh """
      mkdir build
      cd build
      conan remote add \
        --insert 0 \
        ${conan_remote} ${local_conan_server}
      conan install --build=outdated ../${pipeline_builder.project}/conan
    """
  }  // stage

  pipeline_builder.stage("${container.key}: Configure") {
      container.sh """
        cd build
        . ./activate_run.sh
        cmake \
          -DCMAKE_BUILD_TYPE=Release \
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
      make all UnitTests VERBOSE=1
    """
  }  // stage

  pipeline_builder.stage("${container.key}: Test") {
    container.sh """
      cd build
      . ./activate_run.sh
      ./bin/UnitTests
    """
  }  // stage

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

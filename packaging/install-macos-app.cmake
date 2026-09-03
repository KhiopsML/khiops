if(KHIOPS_MACOS_APP)
  # KHIOPS_MACOS_APP is enabled by the macOS packaging script. The companion KHIOPS_MACOS_JRE_DIR cache variable
  # identifies the JRE copied into each app.
  if(NOT IS_MACOS)
    message(FATAL_ERROR "KHIOPS_MACOS_APP is only supported on macOS")
  endif()

  # These paths contain generated files before the install component stages them in the two final application bundles.
  set(MACOS_APP_TMP_DIR "${TMP_DIR}/macos-app")
  set(MACOS_APP_BIN_DIR "${MACOS_APP_TMP_DIR}/bin")
  set(MACOS_APP_KHIOPS_DIR "${MACOS_APP_TMP_DIR}/Khiops.app")
  set(MACOS_APP_COCLUSTERING_DIR "${MACOS_APP_TMP_DIR}/Khiops Coclustering.app")
  set(MACOS_APP_LICENSES_DIR "${MACOS_APP_TMP_DIR}/LICENSES")
  file(MAKE_DIRECTORY "${MACOS_APP_BIN_DIR}" "${MACOS_APP_KHIOPS_DIR}/Contents/MacOS"
       "${MACOS_APP_COCLUSTERING_DIR}/Contents/MacOS" "${MACOS_APP_LICENSES_DIR}")

  configure_file(${PROJECT_SOURCE_DIR}/packaging/macos/java_settings.sh ${MACOS_APP_TMP_DIR}/java_settings.sh COPYONLY)
  file(READ ${MACOS_APP_TMP_DIR}/java_settings.sh KHIOPS_JAVA_SETTINGS)

  # Values consumed by the shared khiops_env.in and khiops.in templates. The app uses bundle-relative paths, enables
  # runtime MPI selection, and does not use environment modules or an external process-number helper.
  set(USE_ENVIRONMENT_MODULE "")
  set(ADDITIONAL_ENV_VAR "")
  set(ADDITIONAL_ENV_VAR_DISPLAY "")
  set(GUI_SUPPORTED "true")
  set(KHIOPS_DRIVERS_PATH "$(get_script_dir)")
  set(IS_MACOS_APP "true")
  set(MODL_PATH "$(get_script_dir)")
  get_target_property(MODL_NAME MODL OUTPUT_NAME)
  get_target_property(MODL_COCLUSTERING_NAME MODL_Coclustering OUTPUT_NAME)
  set(MPI_EXTRA_FLAG "")
  set(SET_PROC_NUMBER "")
  set(MPIEXEC_NUMPROC_FLAG "-n")
  set(KHIOPS_MPI_QUIET "")
  set(LD_LIBRARY_PATH "DYLD_LIBRARY_PATH")

  # KHIOPS_BINARY_PATH and TOOL_EXT select the executable and suffix for each generated launcher: _kh for Khiops and
  # _khc for Khiops Coclustering.
  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops_env/khiops_env.in ${MACOS_APP_BIN_DIR}/khiops_env
                 @ONLY NEWLINE_STYLE UNIX)
  set(KHIOPS_BINARY_PATH "$KHIOPS_PATH")
  set(TOOL_EXT "_kh")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops.in ${MACOS_APP_BIN_DIR}/khiops @ONLY
                 NEWLINE_STYLE UNIX)
  set(KHIOPS_BINARY_PATH "$KHIOPS_COCLUSTERING_PATH")
  set(TOOL_EXT "_khc")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/linux/common/khiops.in ${MACOS_APP_BIN_DIR}/khiops_coclustering @ONLY
                 NEWLINE_STYLE UNIX)

  # These variables provide the bundle metadata used by Info.plist.in and the Finder launcher template for each
  # application.
  set(MACOS_APP_BUNDLE_NAME "Khiops")
  set(MACOS_APP_BUNDLE_IDENTIFIER "org.khiops.Khiops")
  set(MACOS_APP_BUNDLE_EXECUTABLE "khiops")
  set(MACOS_APP_BUNDLE_ICON_FILE "khiops.icns")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/macos/Info.plist.in ${MACOS_APP_KHIOPS_DIR}/Contents/Info.plist @ONLY
                 NEWLINE_STYLE UNIX)
  set(MACOS_APP_LAUNCHER_NAME "khiops")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/macos/app-launcher.in ${MACOS_APP_KHIOPS_DIR}/Contents/MacOS/khiops
                 @ONLY NEWLINE_STYLE UNIX)

  set(MACOS_APP_BUNDLE_NAME "Khiops Coclustering")
  set(MACOS_APP_BUNDLE_IDENTIFIER "org.khiops.KhiopsCoclustering")
  set(MACOS_APP_BUNDLE_EXECUTABLE "khiops_coclustering")
  set(MACOS_APP_BUNDLE_ICON_FILE "khiops_coclustering.icns")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/macos/Info.plist.in ${MACOS_APP_COCLUSTERING_DIR}/Contents/Info.plist
                 @ONLY NEWLINE_STYLE UNIX)
  set(MACOS_APP_LAUNCHER_NAME "khiops_coclustering")
  configure_file(${PROJECT_SOURCE_DIR}/packaging/macos/app-launcher.in
                 ${MACOS_APP_COCLUSTERING_DIR}/Contents/MacOS/khiops_coclustering @ONLY NEWLINE_STYLE UNIX)

  configure_file(${PROJECT_SOURCE_DIR}/packaging/macos/LICENSES-README.txt ${MACOS_APP_LICENSES_DIR}/README.txt
                 COPYONLY)

  # The supplied JRE must contain both its legal notices and the JVM library loaded by the generated Java settings
  # fragment.
  if(NOT DEFINED KHIOPS_MACOS_JRE_DIR OR NOT IS_DIRECTORY "${KHIOPS_MACOS_JRE_DIR}")
    message(FATAL_ERROR "KHIOPS_MACOS_JRE_DIR must point to the JRE embedded in the macOS app bundle")
  endif()
  if(NOT IS_DIRECTORY "${KHIOPS_MACOS_JRE_DIR}/legal" OR NOT EXISTS "${KHIOPS_MACOS_JRE_DIR}/lib/server/libjvm.dylib")
    message(FATAL_ERROR "KHIOPS_MACOS_JRE_DIR is missing legal/ or lib/server/libjvm.dylib")
  endif()
  if(NOT MPI AND NOT BUILD_JARS)
    message(FATAL_ERROR "BUILD_JARS must be enabled for the serial macOS app build")
  endif()

  # All bundle files share one component. MACOS_APP_BUNDLE_DIRS is the list of bundle roots used by both the serial and
  # MPI configuration installs.
  set(MACOS_APP_INSTALL_COMPONENT KHIOPS_MACOS_APP)
  set(MACOS_APP_BUNDLE_DIRS "Khiops.app;Khiops Coclustering.app")
  foreach(MACOS_APP_BUNDLE_DIR IN LISTS MACOS_APP_BUNDLE_DIRS)
    install(TARGETS MODL MODL_Coclustering RUNTIME DESTINATION "${MACOS_APP_BUNDLE_DIR}/Contents/Resources/bin"
                                                   COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
  endforeach()

  install(
    FILES ${PROJECT_SOURCE_DIR}/packaging/common/images/khiops.icns
    DESTINATION "Khiops.app/Contents/Resources"
    COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
  install(
    FILES ${PROJECT_SOURCE_DIR}/packaging/common/images/khiops_coclustering.icns
    DESTINATION "Khiops Coclustering.app/Contents/Resources"
    COMPONENT ${MACOS_APP_INSTALL_COMPONENT})

  if(NOT MPI)
    foreach(MACOS_APP_BUNDLE_DIR IN LISTS MACOS_APP_BUNDLE_DIRS)
      install(
        DIRECTORY "${KHIOPS_MACOS_JRE_DIR}/"
        DESTINATION "${MACOS_APP_BUNDLE_DIR}/Contents/Resources/jre"
        COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
      install(
        FILES ${PROJECT_SOURCE_DIR}/LICENSE
        DESTINATION "${MACOS_APP_BUNDLE_DIR}/Contents/Resources/doc/licenses"
        COMPONENT ${MACOS_APP_INSTALL_COMPONENT}
        RENAME Khiops-LICENSE.txt)
      install_jar(
        khiops_jar
        DESTINATION "${MACOS_APP_BUNDLE_DIR}/Contents/Resources/jars"
        COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
      install_jar(
        norm_jar
        DESTINATION "${MACOS_APP_BUNDLE_DIR}/Contents/Resources/jars"
        COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
    endforeach()
  else()
    foreach(MACOS_APP_BUNDLE_DIR IN LISTS MACOS_APP_BUNDLE_DIRS)
      install(
        PROGRAMS ${MACOS_APP_BIN_DIR}/khiops ${MACOS_APP_BIN_DIR}/khiops_coclustering ${MACOS_APP_BIN_DIR}/khiops_env
        DESTINATION "${MACOS_APP_BUNDLE_DIR}/Contents/Resources/bin"
        COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
    endforeach()

    install(
      PROGRAMS ${MACOS_APP_KHIOPS_DIR}/Contents/MacOS/khiops
      DESTINATION "Khiops.app/Contents/MacOS"
      COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
    install(
      PROGRAMS ${MACOS_APP_COCLUSTERING_DIR}/Contents/MacOS/khiops_coclustering
      DESTINATION "Khiops Coclustering.app/Contents/MacOS"
      COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
    install(
      FILES ${MACOS_APP_KHIOPS_DIR}/Contents/Info.plist
      DESTINATION "Khiops.app/Contents"
      COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
    install(
      FILES ${MACOS_APP_COCLUSTERING_DIR}/Contents/Info.plist
      DESTINATION "Khiops Coclustering.app/Contents"
      COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
    install(
      FILES ${MACOS_APP_TMP_DIR}/LICENSES/README.txt
      DESTINATION LICENSES
      COMPONENT ${MACOS_APP_INSTALL_COMPONENT})
    install(
      FILES ${PROJECT_SOURCE_DIR}/LICENSE
      DESTINATION LICENSES
      COMPONENT ${MACOS_APP_INSTALL_COMPONENT}
      RENAME Khiops-LICENSE.txt)
  endif()
endif()

# Launch genere binary
# cmake-format: off
# This function interface is almost the same as the genere tool:
#
# Genere <ClassName> <ClassLabel> <AttributeFileName> <LogFile> [options]
#   <ClassName>: base name for generated classes
#   <ClassLabel>: label for generated classes
#   <AttributeFileName>: name of the file (.dd) containing the attribute specifications
#   <LogFile>: name of the log file, it must be included in the source files of the binary/library
#
# Options:
#   -nomodel no generation of class <ClassName>
#   -noarrayview no generation of class <ClassName>ArrayView
#   -noview no generation of classes <ClassName>View and <ClassName>ArrayView
#   -nousersection no generation of user sections
#   -specificmodel <SpecificModelClassName> name of a specific model class, to use instead of ClassName
#   -super <SuperClassName> name of the parent class
#
# Do nothing if GENERATE_VIEWS is set to OFF
#
# Example to use genere binary in CMakeLists.txt:
#
#   include(genere-views)
#   add_executable(basetest Foo.cpp Foo.dd.log)
#   add_view_generation(basetest FOO "Foo Example" Foo.dd Foo.dd.log -noarrayview)
#
# cmake-format: on
function(add_view_generation TargetName ClassName ClassLabel AttributeFileName LogFile)
  if(GENERATE_VIEWS)
    # Create the target name
    set(ViewTargetName "${TargetName}_${ClassName}_View")

    # Change the target name if it already exists (may happen when calling with the same ClassName)
    if(TARGET ${ViewTargetName})
      set(ViewTargetName "${ViewTargetName}_Bis")
    endif()

    # Show status  message
    message(STATUS "Setting up VIEW_GENERATION ${ViewTargetName}")

    # Add a custom target fro the view and make the base target to depend on it
    add_custom_target(
      ${ViewTargetName}
      COMMAND
        ${Python_EXECUTABLE} ${CMAKE_SOURCE_DIR}/packaging/genere-script.py ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/genere
        -outputdir ${CMAKE_CURRENT_SOURCE_DIR} ${ARGN} ${ClassName} ${ClassLabel} ${AttributeFileName} ${LogFile}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
      SOURCES ${AttributeFileName})
    add_dependencies(${ViewTargetName} genere)
    add_dependencies(${TargetName} ${ViewTargetName})
  endif()
endfunction()

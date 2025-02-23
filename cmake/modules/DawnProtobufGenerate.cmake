##===------------------------------------------------------------------------------*- CMake -*-===##
##                          _                      
##                         | |                     
##                       __| | __ ___      ___ ___  
##                      / _` |/ _` \ \ /\ / / '_  | 
##                     | (_| | (_| |\ V  V /| | | |
##                      \__,_|\__,_| \_/\_/ |_| |_| - Compiler Toolchain
##
##
##  This file is distributed under the MIT License (MIT). 
##  See LICENSE.txt for details.
##
##===------------------------------------------------------------------------------------------===##

include(yodaIncludeGuard)
yoda_include_guard()

include(CMakeParseArguments)

#.rst:
# dawn_protobuf_generate
# ----------------------
#
# Run the protobuf compiler to generate sources from the proto files. This function excepts the 
# Protobuf compiler to be imported as a target (``protobuf::protoc``).
#
# .. code-block:: cmake
#
#   dawn_protobuf_generate(OUT_FILES PROTOS LANGUAGE)
# 
# ``OUT_FILES``
#   On output this variable contains a List of paths which contain the location of the header and 
#   source files.
# ``OUT_INCLUDE_DIRS``
#   On output this variable contains a list of include directories which need to be added to compile 
#   the generated sources (C++ only).
# ``PROTOS``
#   List of proto files to compile.
# ``LANGUAGE``
#   Language to compile to [default: cpp]. 
#
function(dawn_protobuf_generate)
  set(one_value_args OUT_FILES OUT_INCLUDE_DIRS LANGUAGE)
  set(multi_value_args PROTOS)
  cmake_parse_arguments(ARG "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

  if(NOT("${ARG_UNPARSED_ARGUMENTS}" STREQUAL ""))
    message(FATAL_ERROR "dawn_protobuf_generate: invalid argument ${ARG_UNPARSED_ARGUMENTS}")
  endif()

  if(NOT ARG_PROTOS)
    message(FATAL_ERROR "dawn_protobuf_generate: called without any source files")
    return()
  endif()

  if(NOT ARG_OUT_FILES)
    message(FATAL_ERROR 
            "dawn_protobuf_generate: called without specifying the output variable (OUT_FILES)")
    return()
  endif()

  if(NOT ARG_LANGUAGE)
    set(ARG_LANGUAGE cpp)
  endif()


  if("${ARG_LANGUAGE}" STREQUAL "cpp")
    set(extensions .pb.h .pb.cc)
  elseif("${ARG_LANGUAGE}" STREQUAL "python")
    set(extensions _pb2.py)
  else()
    message(FATAL_ERROR "dawn_protobuf_generate: unknown Language ${ARG_LANGUAGE}")
    return()
  endif()

  # Create an include path for each file specified
  foreach(proto ${ARG_PROTOS})
    get_filename_component(abs_file ${proto} ABSOLUTE)
    get_filename_component(abs_path ${proto} PATH)
    list(FIND include_path ${abs_path} existing)
    if("${existing}" EQUAL "-1")
      list(APPEND include_path "-I${abs_path}")
    endif()
  endforeach()

  # Generate a script to invoke protoc (this is needed to set the LD_LIBRARY_PATH as google 
  # doesn't know about RPATH support in CMake ...)
  get_property(libprotoc_loc TARGET protobuf::libprotoc PROPERTY LOCATION)
  get_filename_component(libprotoc_dir ${libprotoc_loc} PATH)
  get_property(protoc_path TARGET protobuf::protoc PROPERTY LOCATION)

  set(protobuf_script ${CMAKE_CURRENT_BINARY_DIR}/run_protobuf.sh)
  file(WRITE "${protobuf_script}" "#!/usr/bin/env bash\n")
  file(APPEND "${protobuf_script}" "export LD_LIBRARY_PATH=\"${libprotoc_dir}\":$LD_LIBRARY_PATH\n")
  file(APPEND "${protobuf_script}" "${protoc_path} $*\n")
  set(command "${BASH_EXECUTABLE}")

  set(output_files)
  set(output_include_dirs)

  foreach(proto ${ARG_PROTOS})
    get_filename_component(abs_file ${proto} ABSOLUTE)
    get_filename_component(basename ${proto} NAME_WE)

    unset(generated_srcs)
    foreach(ext ${extensions})
      list(APPEND generated_srcs "${CMAKE_CURRENT_BINARY_DIR}/${basename}${ext}")
    endforeach()

    add_custom_command(
      OUTPUT ${generated_srcs}
      COMMAND ${command} 
      ARGS ${protobuf_script} --${ARG_LANGUAGE}_out "${CMAKE_CURRENT_BINARY_DIR}" 
           ${include_path} "${abs_file}"
      COMMENT "Running ${ARG_LANGUAGE} protocol buffer compiler on ${proto}"
      DEPENDS ${abs_file}
      VERBATIM 
    )

    list(APPEND output_files ${generated_srcs})

    foreach(src ${generated_srcs})
      get_filename_component(abs_file ${src} ABSOLUTE)
      get_filename_component(abs_path ${src} PATH)
      list(FIND output_include_dirs ${abs_path} existing)
      if(${existing} EQUAL -1)
        list(APPEND output_include_dirs ${abs_path})
      endif()
    endforeach()
  endforeach()

  set("${ARG_OUT_FILES}" ${output_files} PARENT_SCOPE)
  if(ARG_OUT_INCLUDE_DIRS)
    set("${ARG_OUT_INCLUDE_DIRS}" ${output_include_dirs} PARENT_SCOPE)
  endif()
endfunction()

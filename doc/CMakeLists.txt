cmake_minimum_required(VERSION 3.9)
project(libcsp_docs)

set(SPHINX_EXECUTABLE sphinx-build)
set(SPHINX_SOURCE ${CMAKE_CURRENT_SOURCE_DIR})
set(SAMPLES_ORIG_DIR ${SPHINX_SOURCE}/../samples/posix)
set(SAMPLES_DEST_DIR ${SPHINX_SOURCE}/samples/posix)
set(SPHINX_BUILD ${CMAKE_CURRENT_BINARY_DIR}/html)
set(SPHINX_INDEX_FILE ${SPHINX_BUILD}/index.html)

file(MAKE_DIRECTORY ${SAMPLES_DEST_DIR})

file(GLOB SAMPLE_READMES "${SAMPLES_ORIG_DIR}/*/README.md")
foreach(readme ${SAMPLE_READMES})
    get_filename_component(dir ${readme} DIRECTORY)
    get_filename_component(name ${dir} NAME)
    set(dest "${SAMPLES_DEST_DIR}/${name}.md")
    message(STATUS "Copying ${readme} to ${dest}")
    configure_file(${readme} ${dest} COPYONLY)
endforeach()

file(GLOB DOCS_FILES
    "${SPHINX_SOURCE}/*.md"
    "${SAMPLES_DEST_DIR}/*.md"
    "${SPHINX_SOURCE}/api/*.rst"
    "${SPHINX_SOURCE}/api/arch/*.rst"
    "${SPHINX_SOURCE}/api/crypto/*.rst"
    "${SPHINX_SOURCE}/api/drivers/*.rst"
    "${SPHINX_SOURCE}/api/interfaces/*.rst")

add_custom_target(docs ALL DEPENDS ${SPHINX_INDEX_FILE})
add_custom_command(OUTPUT ${SPHINX_INDEX_FILE}
    COMMAND
        ${SPHINX_EXECUTABLE} -E -b html ${SPHINX_SOURCE} ${SPHINX_BUILD} ${DOCS_FILES}
    MAIN_DEPENDENCY conf.py
    COMMENT "Building DOCS for lib CSP")

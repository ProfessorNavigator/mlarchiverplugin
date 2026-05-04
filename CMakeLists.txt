cmake_minimum_required(VERSION 3.16)

project(MLArchiverPlugin LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(-Wall)

find_package(Qt6 REQUIRED COMPONENTS Core LinguistTools)

find_package(MLPlugin REQUIRED)

find_package(LibUDB REQUIRED)

find_package(Threads REQUIRED)

add_library(MLArchiverPlugin SHARED)

add_subdirectory(include)
add_subdirectory(src)

target_include_directories(MLArchiverPlugin
  PRIVATE
    include
)

target_link_libraries(MLArchiverPlugin
  PRIVATE
    MLPlugin::MLPlugin
    MLBookProc::MLBookProc
    LibUDB::udb
    Threads::Threads
)

qt_collect_translation_source_targets(i18n_targets DIRECTORY src)

qt_add_translations(
    TARGETS MLArchiverPlugin
    SOURCE_TARGETS ${i18n_targets}
    TS_FILES translations/MLArchiverPlugin_ru_RU.ts
)

include(GNUInstallDirs)

install(TARGETS MLArchiverPlugin EXPORT
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}
)



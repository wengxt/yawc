cmake_minimum_required(VERSION 2.6)

project(yawc)

find_package(KDE4 REQUIRED)
find_package(KActivities REQUIRED)
find_package(Gettext REQUIRED)
include(KDE4Defaults)

add_subdirectory(src)
add_subdirectory(po)
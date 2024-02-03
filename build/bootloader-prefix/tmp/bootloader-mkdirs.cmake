# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Espressif/esp-idf/components/bootloader/subproject"
  "E:/Code/ESP32_Espressif_IDF/NewLMIC/lmic_nmea/build/bootloader"
  "E:/Code/ESP32_Espressif_IDF/NewLMIC/lmic_nmea/build/bootloader-prefix"
  "E:/Code/ESP32_Espressif_IDF/NewLMIC/lmic_nmea/build/bootloader-prefix/tmp"
  "E:/Code/ESP32_Espressif_IDF/NewLMIC/lmic_nmea/build/bootloader-prefix/src/bootloader-stamp"
  "E:/Code/ESP32_Espressif_IDF/NewLMIC/lmic_nmea/build/bootloader-prefix/src"
  "E:/Code/ESP32_Espressif_IDF/NewLMIC/lmic_nmea/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "E:/Code/ESP32_Espressif_IDF/NewLMIC/lmic_nmea/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "E:/Code/ESP32_Espressif_IDF/NewLMIC/lmic_nmea/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()

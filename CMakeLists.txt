cmake_minimum_required(VERSION 3.11)

project(kubepsp)

# Main executable
add_executable(${PROJECT_NAME} main.cpp)

# Link required PSP libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    pspgu
    pspgum
    pspge
    pspctrl
    pspdebug
    pspdisplay
    pspnet
    pspnet_apctl
    pspnet_inet
    m
)

# Optional: Include path to PSP headers if needed manually
# target_include_directories(${PROJECT_NAME} PRIVATE ${PSPDEV}/psp/include)

# Generate EBOOT.PBP with metadata
create_pbp_file(
    TARGET ${PROJECT_NAME}
    ICON_PATH NULL
    BACKGROUND_PATH NULL
    PREVIEW_PATH NULL
    TITLE "Kube Dashboard"
    VERSION 01.00
    CATEGORY MG)

cmake_minimum_required(VERSION 3.16)
project(k8s_dashboard_psp C CXX)

# Correct way to define a PSP app
psp_executable(k8s_dashboard
  main.cpp
)

target_compile_features(k8s_dashboard PRIVATE cxx_std_23)

target_link_libraries(k8s_dashboard PRIVATE
  pspkernel
  pspdebug
  pspdisplay
  pspctrl
  pspnet
  pspnet_apctl
  pspnet_inet
  pspnet_adhoc
  pspge
)

create_pbp_file(
  TARGET k8s_dashboard
  TITLE "K8s Dashboard PSP"
)

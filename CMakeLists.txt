cmake_minimum_required(VERSION 3.16)
project(qingming_affine_shear LANGUAGES CXX)

add_executable(exact_synth src/exact_synth.cpp)
add_executable(exact_circuit src/exact_circuit.cpp)

target_compile_features(exact_synth PRIVATE cxx_std_17)
target_compile_features(exact_circuit PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(exact_synth PRIVATE /O2)
    target_compile_options(exact_circuit PRIVATE /O2)
else()
    target_compile_options(exact_synth PRIVATE -O3)
    target_compile_options(exact_circuit PRIVATE -O3)
endif()

enable_testing()
add_test(NAME exact_synth_self_test COMMAND exact_synth --self-test)
add_test(NAME exact_circuit_self_test COMMAND exact_circuit --self-test)
add_test(NAME ck_case_study COMMAND exact_synth --case-study)

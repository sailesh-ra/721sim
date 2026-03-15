# CMakeLists.txt for the Memory Dependency Predictor (MDP) module of uarchsim

# If lib/libuarchsim-mdp.a exists, we can use it directly. Otherwise, we need to build the MDP library from source.
set(MDP_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lib/libuarchsim-mdp.a")
if(EXISTS ${MDP_LIB_PATH})
        message(STATUS "Will use precompiled MDP library: ${MDP_LIB_PATH}")

        add_library(uarchsim-mdp STATIC IMPORTED GLOBAL)
        set_target_properties(uarchsim-mdp PROPERTIES
                IMPORTED_LOCATION ${MDP_LIB_PATH}
                INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/include"
        )
else()
        message(STATUS "Will build MDP library from source")

        file(GLOB uarchsim_mdp_srcs ${CMAKE_CURRENT_SOURCE_DIR}/srcs/*)
        file(GLOB uarchsim_mdp_hdrs ${CMAKE_CURRENT_SOURCE_DIR}/include/*)

        add_library(uarchsim-mdp STATIC ${uarchsim_mdp_srcs} ${uarchsim_mdp_hdrs})
        target_include_directories(uarchsim-mdp PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/srcs)
        target_include_directories(uarchsim-mdp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
        target_compile_options(uarchsim-mdp PRIVATE -Wall -fPIC)
endif()

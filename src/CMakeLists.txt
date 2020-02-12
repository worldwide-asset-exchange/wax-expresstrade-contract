project(Barter)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( Barter Barter Barter.cpp )
target_include_directories( Barter PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( Barter ${CMAKE_SOURCE_DIR}/../ricardian )
project(AffiliateBank)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( AffiliateBank AffiliateBank AffiliateBank.cpp )
target_include_directories( AffiliateBank PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( AffiliateBank ${CMAKE_SOURCE_DIR}/../ricardian )
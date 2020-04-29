project(wax_express_trade)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

add_contract( wax_express_trade wax_express_trade wax_express_trade.cpp )
target_include_directories( wax_express_trade PUBLIC ${CMAKE_SOURCE_DIR}/../include )
target_ricardian_directory( wax_express_trade ${CMAKE_SOURCE_DIR}/../ricardian )